import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const root = path.join(upstream, 'src');
const {vec3, float} = await import(pathToFileURL(path.join(root, 'nodes/tsl/TSLBase.js')));
const {triplanarTextures, triplanarTexture} = await import(pathToFileURL(path.join(root, 'nodes/utils/TriplanarTextures.js')));
const {texture} = await import(pathToFileURL(path.join(root, 'nodes/accessors/TextureNode.js')));
const {Texture} = await import(pathToFileURL(path.join(root, 'textures/Texture.js')));
const describeFn = value => ({
  type: value?.type ?? null,
  isFn: !!value?.isFn,
  isNode: !!value?.isNode,
  length: typeof value?.length === 'function' ? 'function' : value?.length ?? null,
  name: value?.name ?? null,
  source: value?.shaderNode?.jsFunc?.toString?.() ?? null
});
const textureNode = texture(new Texture());
const args = [textureNode, null, null, float(2), vec3(1, 2, 3), vec3(0, 1, 0)];
const call = triplanarTextures(...args);
const aliasCall = triplanarTexture(...args);
const describeCall = value => ({type: value?.type ?? null, nodeType: value?.node?.type ?? null, rawInputCount: value?.node?.rawInputs?.length ?? null});
const expected = {
  fn: describeFn(triplanarTextures),
  aliasName: triplanarTexture.name,
  aliasLength: triplanarTexture.length,
  moduleSame: true,
  aliasModuleSame: true,
  call: describeCall(call),
  aliasCall: describeCall(aliasCall)
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('triplanarTextures native/module parity passed');
