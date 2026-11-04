import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const {default: fn} = await import(pathToFileURL(path.join(upstream, 'src/nodes/functions/BSDF/BRDF_EON.js')));
const {vec3, float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));
const result = fn({
    lightDirection: vec3(0.2, 0.7, 0.6),
    diffuseColor: vec3(0.5, 0.4, 0.3),
    roughness: float(0.35),
    normalView: vec3(0, 0, 1),
    viewDirection: vec3(0.1, 0.2, 0.97)
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
console.log('BRDF_EON native/module parity passed');
