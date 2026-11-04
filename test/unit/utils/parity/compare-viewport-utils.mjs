import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const root = path.join(upstream, 'src');
const {viewportSafeUV} = await import(pathToFileURL(path.join(root, 'nodes/utils/ViewportUtils.js')));
const {vec2} = await import(pathToFileURL(path.join(root, 'nodes/tsl/TSLBase.js')));
const describe = value => ({
  type: value?.type ?? null,
  isFn: !!value?.isFn,
  isNode: !!value?.isNode,
  length: typeof value?.length === 'function' ? 'function' : value?.length ?? null,
  source: value?.shaderNode?.jsFunc?.toString?.() ?? null
});
const call = viewportSafeUV(vec2(0.25, 0.75));
const expected = {
  ...describe(viewportSafeUV),
  moduleSame: true,
  callType: call.type,
  callNodeType: call.node?.type ?? null,
  rawInputCount: call.node?.rawInputs?.length ?? null
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('viewport utils native/module parity passed');
