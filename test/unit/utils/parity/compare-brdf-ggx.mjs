import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const {default: fn} = await import(pathToFileURL(path.join(upstream, 'src/nodes/functions/BSDF/BRDF_GGX.js')));
const {vec3, float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));
const result = fn({
    lightDirection: vec3(1, 0, 0),
    f0: vec3(.04, .05, .06),
    f90: float(1),
    roughness: float(.5),
    f: vec3(.2, .3, .4)
});
const expected = {
    name: fn.name,
    isFn: fn.isFn,
    source: fn.shaderNode.jsFunc.toString(),
    type: result.type,
    nodeType: result.node.type
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('BRDF_GGX native/module parity passed');
