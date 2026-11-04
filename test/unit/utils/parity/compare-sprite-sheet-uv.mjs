import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const {vec2, float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));
const {spritesheetUV} = await import(pathToFileURL(path.join(upstream, 'src/nodes/utils/SpriteSheetUV.js')));
const count = value => ({type: value?.type ?? null, isFn: !!value?.isFn, isNode: !!value?.isNode,
  length: typeof value?.length === 'function' ? 'function' : value?.length ?? null, name: value?.name ?? null,
  source: value?.shaderNode?.jsFunc?.toString?.() ?? null});
const expected = {
  fn: count(spritesheetUV),
  moduleSame: true,
  callType: spritesheetUV(vec2(6, 6), vec2(0.25, 0.75), float(7)).type,
  callNodeType: spritesheetUV(vec2(6, 6), vec2(0.25, 0.75), float(7)).node?.type ?? null,
  rawInputCount: 3
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('spritesheetUV native/module parity passed');
