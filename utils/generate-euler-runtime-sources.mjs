// Development-time source metadata only; the native runtime does not execute it.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';

const root = fileURLToPath(new URL('../', import.meta.url));
const args = process.argv.slice(2);
const check = args.includes('--check');
const positional = args.filter(arg => !arg.startsWith('--'));
if (args.filter(arg => arg === '--check').length > 1 ||
    args.some(arg => arg.startsWith('--') && arg !== '--check') || positional.length > 1) {
    throw new Error('Usage: node utils/generate-euler-runtime-sources.mjs [three.js checkout] [--check]');
}
const upstream = path.resolve(positional[0] ?? path.join(root, '../three.js'));
const revision = execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim();
if (revision !== '11738a9d55e502fb484cefffcea3a66dc44e9fe9') throw new Error('Unexpected upstream');
const require = createRequire(path.join(upstream, 'package.json'));
const {parse} = require('acorn');
const arity = parameters => {
    const index = parameters.findIndex(node => node.type === 'AssignmentPattern' || node.type === 'RestElement');
    return index < 0 ? parameters.length : index;
};
const source = fs.readFileSync(path.join(upstream, 'src/math/Euler.js'), 'utf8');
const tree = parse(source, {ecmaVersion: 'latest', sourceType: 'module'});
const cls = tree.body.find(node => node.type === 'ClassDeclaration');
const methods = cls.body.body.map(method => ({
    name: method.computed ? '[Symbol.iterator]' : method.key.name,
    kind: method.kind,
    arity: arity(method.value.params),
    generator: method.value.generator,
    source: source.slice(method.start, method.end)
}));
const euler = [
    '// Pinned source metadata only; production does not execute these strings.',
    'namespace EulerRuntimeSource {',
    `constexpr const char* classSource=${JSON.stringify(source.slice(cls.start, cls.end))};`,
    'struct Entry { const char* name; const char* kind; unsigned arity; bool generator; const char* source; };',
    'constexpr Entry entries[]={',
    ...methods.map(method => `{${JSON.stringify(method.name)},${JSON.stringify(method.kind)},${method.arity},${method.generator},${JSON.stringify(method.source)}},`),
    '};',
    '}'
].join('\n') + '\n';
const matrixSource = fs.readFileSync(path.join(upstream, 'src/math/Matrix4.js'), 'utf8');
const matrixTree = parse(matrixSource, {ecmaVersion: 'latest', sourceType: 'module'});
const matrixClass = matrixTree.body.find(node => node.type === 'ClassDeclaration');
const matrix = ['namespace Matrix4QuaternionSource {'];
for (const name of ['makeRotationFromQuaternion', 'compose']) {
    const method = matrixClass.body.body.find(node => node.key?.name === name);
    if (!method) throw new Error('Missing Matrix4 method: ' + name);
    matrix.push(`constexpr const char* ${name}=${JSON.stringify(matrixSource.slice(method.start, method.end))};`);
}
matrix.push('}');
const outputs = [
    ['src/math/EulerRuntimeSource.inc', euler],
    ['src/math/Matrix4QuaternionSource.inc', matrix.join('\n') + '\n']
];
// Check every output before writing; --check performs no filesystem mutations.
for (const [name, value] of outputs) {
    const target = path.join(root, name);
    if (check && fs.readFileSync(target, 'utf8').replaceAll('\r\n', '\n') !== value) {
        throw new Error('Stale ' + name);
    }
}
if (!check) for (const [name, value] of outputs) fs.writeFileSync(path.join(root, name), value);
console.log(`${check ? 'checked' : 'generated'} Euler ${methods.length} entries and two Matrix4 methods`);
