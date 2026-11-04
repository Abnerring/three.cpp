import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const {default: fn} = await import(pathToFileURL(path.join(upstream, 'src/nodes/functions/BSDF/V_GGX_SmithCorrelated_Anisotropic.js')));
const result = fn({
    alphaT: {isNode: true}, alphaB: {isNode: true}, dotTV: {isNode: true}, dotBV: {isNode: true},
    dotTL: {isNode: true}, dotBL: {isNode: true}, dotNV: {isNode: true}, dotNL: {isNode: true}
});
const expected = {
    name: fn.name,
    isFn: fn.isFn,
    source: fn.shaderNode.jsFunc.toString(),
    layoutType: fn.shaderNode.layout.type,
    type: result.type,
    nodeType: result.node.type
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('V_GGX_SmithCorrelated_Anisotropic native/module parity passed');
