import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const {getDistanceAttenuation} = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/LightUtils.js')));
const {float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLCore.js')));
const result = getDistanceAttenuation({
  lightDistance: float(2),
  cutoffDistance: float(10),
  decayExponent: float(2)
});
const expected = {
  name: getDistanceAttenuation.name,
  isFn: getDistanceAttenuation.isFn,
  isNode: getDistanceAttenuation.isNode,
  source: getDistanceAttenuation.shaderNode.jsFunc.toString(),
  layout: getDistanceAttenuation.shaderNode.layout,
  moduleSame: true,
  type: result.type,
  nodeType: result.node.type
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('LightUtils native/module parity passed');
