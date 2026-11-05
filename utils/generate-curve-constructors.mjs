// Compile the pinned curve class registry and source metadata for native C++.
import fs from 'node:fs';
import path from 'node:path';
import { createRequire } from 'node:module';
import { execFileSync } from 'node:child_process';
const upstream = path.resolve(process.argv[2]);
const repo = path.resolve(import.meta.dirname, '..');
const lock = JSON.parse(fs.readFileSync(path.join(repo, 'upstream.json')));
if (execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim() !== lock.commit)
    throw new Error('Unexpected upstream commit');
const { parse } = createRequire(path.join(upstream, 'package.json'))('acorn');
const core = ['Curve', 'CurvePath', 'Path', 'Shape'];
const curves = ['EllipseCurve', 'ArcCurve', 'LineCurve', 'LineCurve3', 'QuadraticBezierCurve',
    'QuadraticBezierCurve3', 'CubicBezierCurve', 'CubicBezierCurve3', 'SplineCurve', 'CatmullRomCurve3'];
const exported = ['ArcCurve', 'CatmullRomCurve3', 'CubicBezierCurve', 'CubicBezierCurve3',
    'EllipseCurve', 'LineCurve', 'LineCurve3', 'QuadraticBezierCurve', 'QuadraticBezierCurve3', 'SplineCurve'];
const names = [...core, ...curves];
let metadata = '// Class metadata from the fixed upstream source; never executed as JavaScript.\nnamespace CurveConstructorsSource {\n';
const registry = [];
for (const name of names) {
    const source = fs.readFileSync(path.join(upstream, 'src/extras', core.includes(name) ? 'core' : 'curves', `${name}.js`), 'utf8');
    const ast = parse(source, { sourceType: 'module', ecmaVersion: 'latest' });
    const cls = ast.body.find(node => node.type === 'ClassDeclaration');
    const ctor = cls.body.body.find(node => node.kind === 'constructor');
    const defaultIndex = ctor.value.params.findIndex(node => node.type === 'AssignmentPattern');
    const arity = defaultIndex < 0 ? ctor.value.params.length : defaultIndex;
    metadata += `inline constexpr char ${name}[] = ${JSON.stringify(source.slice(cls.start, cls.end))};\n`;
    registry.push(`    add(${JSON.stringify(name)}, ${cls.superClass ? JSON.stringify(cls.superClass.name) : 'nullptr'}, ${arity}, CurveConstructorsSource::${name});`);
}
metadata += '}\n';
let output = `// Generated native curve constructors and lexical class identities.\n`;
output += names.map(name => `#include "extras/${core.includes(name) ? 'core' : 'curves'}/${name}.h"`).join('\n');
output += `
#include "extras/curves/Curves.h"
#include "extras/core/CurveConstructors.h"
#include "extras/core/CurveConstructorsSources.inc"
#include "extras/core/CurveRuntime.h"
#include "extras/core/CurvePathRuntime.h"
#include "extras/curves/CurvesRuntime.h"
#include "core/JSModuleObject.h"

namespace {
using Args = std::vector<JSValue>;
void own(JSValue object, const char* name, JSValue value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = std::move(value);
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(name, descriptor);
}
JSValue construct(const JSValue& newTarget, const Args& args, const Args& captures) {
    const auto constructor = captures[0].getInternalSlot("constructor");
    return constructor.getInternalSlot("nativeCurveConstructor").construct(args, newTarget);
}
JSValue classModule(const char* name) {
    auto constructor = nativeCurveClassTable().get(name);
    auto module = constructor.getInternalSlot("nativeCurveModule");
    if (module.isUndefined()) {
        module = JSValue::object(nullptr);
        own(module, name, constructor);
        constructor.setInternalSlot("nativeCurveModule", module);
    }
    return module;
}
}
JSValue nativeCurveClassTable() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope guard(initializing, "Curve classes");
    auto table = JSValue::object(nullptr);
    const auto add = [&](const char* name, const char* parent, unsigned arity, const char* source) {
        auto holder = JSValue::object(nullptr);
        auto constructor = JSValue::builtinConstructor(construct, {holder}, name, arity);
        holder.setInternalSlot("constructor", constructor);
        if (parent) {
            constructor.setPrototype(table.get(parent));
            constructor.get("prototype").setPrototype(table.get(parent).get("prototype"));
        }
        constructor.setFunctionSourceText(source);
        own(table, name, constructor);
    };
${registry.join('\n')}
    installNativeCurveNumericMethods(table.get("Curve"), table);
    installNativeCurveSubclassMethods(table);
    installNativeCurvePathMethods(table);
    installNativePathMethods(table);
    installNativeShapeMethods(table);
    table.retainForHeapLifetime();
    weak = JSWeakValue(table);
    return table;
}
JSValue nativeCurvesModule() {
    auto table = nativeCurveClassTable();
    auto module = table.getInternalSlot("nativeCurvesModule");
    if (module.isUndefined()) {
        module = JSValue::object(nullptr);
${exported.map(name => `        own(module, "${name}", table.get("${name}"));`).join('\n')}
        table.setInternalSlot("nativeCurvesModule", module);
    }
    return module;
}
JSValue nativeCurvesBinding(const std::string& name) {
    const auto table = nativeCurveClassTable();
    for (const auto* key : {${exported.map(JSON.stringify).join(', ')}})
        if (name == key) return table.get(name);
    return {};
}
`;
for (const name of names) output += `JSValue native${name}Constructor() { return nativeCurveClassTable().get("${name}"); }
JSValue native${name}Module() { return classModule("${name}"); }
JSValue native${name}Binding(const std::string& name) { return name == "${name}" ? native${name}Constructor() : JSValue{}; }
`;
for (const [relative, content] of [['src/extras/core/CurveConstructorsSources.inc', metadata], ['src/extras/core/CurveConstructors.cpp', output]]) {
    const target = path.join(repo, relative);
    if (!fs.existsSync(target) || fs.readFileSync(target, 'utf8') !== content) fs.writeFileSync(target, content);
}
console.log(`${names.length} canonical curve constructors, ${exported.length} Curves exports`);
