import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const {vec3, float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));
const {triNoise3D} = await import(pathToFileURL(path.join(upstream, 'src/nodes/math/TriNoise3D.js')));
const describe = value => ({type: value?.type ?? null, isFn: !!value?.isFn, isNode: !!value?.isNode,
  length: typeof value?.length === 'function' ? 'function' : value?.length ?? null,
  name: value?.name ?? null, source: value?.shaderNode?.jsFunc?.toString?.() ?? null});
const expected = {
  fn: describe(triNoise3D),
  moduleSame: true,
  callType: triNoise3D(vec3(0, 1, 2), float(1), float(0.5)).type,
  callNodeType: triNoise3D(vec3(0, 1, 2), float(1), float(0.5)).node?.type ?? null,
  rawInputCount: 3
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('triNoise3D native/module parity passed');

