import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import test from 'node:test';
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
import { extractApi } from './api-inventory.mjs';
import { AUDIT_SCOPES, auditCheckout, walk, writeAudit } from './port-audit-support.mjs';

const root = fileURLToPath(new URL('../', import.meta.url));
const upstream = path.resolve(process.argv[2] ?? process.env.THREEJS_SOURCE_DIR ?? '../three.js');
const require = createRequire(path.join(upstream, 'package.json'));
const { parse } = require('acorn');

test('AST distinguishes declarations from strings and preserves exported aliases and parameters', () => {
    const inventory = extractApi(`
        import Default, { Thing as Alias } from './thing.js';
        const text = 'export class Fake { method() {} }';
        export function sum(a = 1, ...rest) { return a; }
        export const value = { method(x = {}) {}, get size() { return 1; }, flag: true };
        export { sum as renamed };
        export { Other as Remote } from './other.js';
        export * from './all.js';
        export * as namespace from './namespace.js';
    `, parse);
    assert.equal(inventory.classes.length, 0);
    assert.deepEqual(inventory.functions.find(f => f.name === 'sum').parameters, ['a = 1', '...rest']);
    assert.deepEqual(inventory.functions.find(f => f.name === 'sum').exportedAs, ['sum', 'renamed']);
    assert.equal(inventory.exports.find(e => e.name === 'Remote').source, './other.js');
    assert.equal(inventory.exports.find(e => e.name === 'namespace').kind, 'namespace-reexport');
    assert.deepEqual(inventory.imports[0].bindings, [{ local: 'Default', imported: 'default' }, { local: 'Alias', imported: 'Thing' }]);
    assert.deepEqual(inventory.objects.find(o => o.name === 'value').members.map(m => [m.name, m.kind]), [['method', 'method'], ['size', 'get'], ['flag', 'property']]);
    assert.equal(inventory.extraction.apiCoverageComplete, false);
});

test('class members retain private/computed/static semantics and lexical this boundaries', () => {
    const inventory = extractApi(`
        export class Example extends Base {
            #secret = 7;
            field = 2;
            constructor(value = 3) {
                this.count = value;
                Object.defineProperty(this, 'id', { value: 1 });
                Object.defineProperties(this, { label: { value: 'example' } });
                const arrow = () => { this.arrow = true; };
                function nested() { this.unrelated = true; }
                this.run = function(v) { this.updated = v; };
                Object.assign(this, { assigned: true });
            }
            static reset() { this.total = 0; }
            get value() { return this.count; }
            set value(v) { this.count = v; }
            [Symbol.iterator]() {}
            [computed]() {}
            /** @private */
            internal() {}
        }
        Example.DEFAULT = 4;
        Example.prototype.extra = function(a = {}) {};
        export { Example as Alias };
    `, parse);
    const klass = inventory.classes[0];
    assert.deepEqual(klass.exportedAs, ['Example', 'Alias']);
    assert.equal(klass.superclass, 'Base');
    const properties = new Map(klass.properties.map(p => [p.name, p]));
    for (const name of ['#secret', 'field', 'count', 'id', 'label', 'arrow', 'run', 'updated', 'assigned', 'total', 'DEFAULT']) assert(properties.has(name), name);
    assert.equal(properties.has('unrelated'), false);
    assert.equal(properties.get('#secret').visibility, 'private');
    assert.equal(properties.get('total').static, true);
    assert.equal(properties.get('DEFAULT').static, true);
    assert.equal(klass.methods.find(m => m.name === 'internal').visibility, 'private');
    assert.equal(klass.methods.find(m => m.name === '[Symbol.iterator]').generator, false);
    assert(klass.methods.some(m => m.name === 'run' && m.kind === 'instance-assignment'));
    assert(klass.methods.some(m => m.name === 'extra' && m.kind === 'external-assignment'));
    assert(klass.methods.some(m => m.name === 'value' && m.kind === 'get'));
    assert(klass.methods.some(m => m.name === 'value' && m.kind === 'set'));
    assert(inventory.reviewRequired.some(item => item.reason === 'computed-class-member'));
    assert.equal(inventory.extraction.status, 'partial-static');
});

test('function constructors, anonymous defaults, generated interfaces and invalid syntax stay explicit', () => {
    const inventory = extractApi(`
        function Legacy(options = {}) { this.enabled = true; this.render = function(scene) { this.last = scene; }; }
        Legacy.prototype.dispose = function() {};
        export { Legacy };
        export const generated = new Proxy({}, {});
        export default function(a = 2) {}
        import('./dynamic.js');
        import(target);
        new URL('./decoder.wasm', import.meta.url);
    `, parse);
    const legacy = inventory.functions.find(f => f.name === 'Legacy');
    assert(legacy.properties.some(p => p.name === 'enabled'));
    assert(legacy.properties.some(p => p.name === 'last'));
    assert(legacy.methods.some(m => m.name === 'render'));
    assert(legacy.methods.some(m => m.name === 'dispose'));
    assert(inventory.functions.some(f => f.name === '<default>' && f.exportedAs[0] === 'default'));
    assert(inventory.imports.some(i => i.kind === 'dynamic-import' && i.source === './dynamic.js'));
    assert.equal(inventory.assets[0].source, './decoder.wasm');
    assert(inventory.reviewRequired.some(item => item.reason === 'runtime-generated-interface'));
    assert(inventory.reviewRequired.some(item => item.reason === 'dynamic-import-target'));
    const invalid = extractApi('export class {', parse);
    assert.equal(invalid.extraction.status, 'parse-error');
    assert.equal(invalid.extraction.apiCoverageComplete, false);
    const script = extractApi('with ({}) {}; module.exports = {};', parse);
    assert.equal(script.extraction.sourceType, 'script');
    assert(script.reviewRequired.some(item => item.reason === 'commonjs-export-interface-not-resolved'));
    assert.equal(script.extraction.apiCoverageComplete, false);
});

test('pinned core and addons are inventoried exactly once with resolvable evidence and no inferred completion', async () => {
    const audit = await auditCheckout({ root, upstream });
    const expected = AUDIT_SCOPES.flatMap(scope => walk(path.join(upstream, scope)).filter(file => file.endsWith('.js')).map(file => path.relative(upstream, file).split(path.sep).join('/')));
    assert.deepEqual(audit.report.modules.map(module => module.source), expected);
    assert.equal(new Set(expected).size, expected.length);
    assert.equal(audit.report.scopeCounts.src.modules, 755);
    assert.equal(audit.report.scopeCounts['examples/jsm'].modules, 496);
    assert.equal(audit.report.modules.filter(module => module.kind === 'third-party-dependency').length, 20);
    assert.equal(audit.api.counts.parseErrors, 0);
    for (let index = 0; index < expected.length; ++index) {
        const module = audit.report.modules[index];
        const inventory = audit.api.modules[index];
        assert.equal(module.verifiedComplete, false);
        assert.equal(inventory.extraction.apiCoverageComplete, false);
        assert.equal(inventory.source, module.source);
        assert.equal(inventory.upstreamSha256, module.upstreamSha256);
        assert.equal(module.verification.apiInventory.pointer, `/modules/${index}`);
        for (const file of [module.header, module.implementation, ...module.verification.cppUnitTests].filter(Boolean)) assert(fs.statSync(path.join(root, file)).isFile(), file);
        for (const file of module.verification.upstreamUnitTests) assert(fs.statSync(path.join(upstream, file)).isFile(), file);
        if (module.kind === 'third-party-dependency') assert.equal(module.status, 'dependency-unverified');
    }
    const vector3 = audit.api.modules.find(module => module.source === 'src/math/Vector3.js');
    assert(vector3.classes.some(klass => klass.name === 'Vector3' && klass.methods.some(method => method.name === 'applyMatrix4') && klass.properties.some(property => property.name === 'x')));
    const renderer = audit.api.modules.find(module => module.source === 'src/renderers/WebGLRenderer.js');
    assert(renderer.classes.some(klass => klass.name === 'WebGLRenderer' && klass.methods.some(method => method.name === 'render' && method.kind === 'instance-assignment')));
    const addons = audit.api.modules.find(module => module.source === 'examples/jsm/Addons.js');
    assert(addons.exports.some(item => item.source === './controls/OrbitControls.js'));
    writeAudit(root, audit, { checkOnly: true });
    assert.throws(() => writeAudit(root, { ...audit, report: { ...audit.report, note: 'deliberately stale test value' } }, { checkOnly: true }), /inventory is stale/);
});

test('auditing another Git checkout fails before accepting a different upstream revision', async () => {
    await assert.rejects(auditCheckout({ root, upstream: root }), /Expected upstream .* found/);
});
