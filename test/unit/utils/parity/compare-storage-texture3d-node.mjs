import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const {storageTexture3D} = await import(pathToFileURL(path.join(upstream, 'src/nodes/accessors/StorageTexture3DNode.js')));
const {Texture} = await import(pathToFileURL(path.join(upstream, 'src/textures/Texture.js')));
const {vec3, vec4} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));
const node = storageTexture3D(new Texture(), vec3(1, 2, 3), vec4(1, 0, 0, 1));
const expected = {
  type: node.type,
  isStorageTextureNode: node.isStorageTextureNode,
  isStorageTexture3DNode: node.isStorageTexture3DNode,
  inputType: node.getInputType(),
  access: node.access,
  mipLevel: node.mipLevel,
  updateMatrix: node.updateMatrix,
  sampler: node.getSampler(),
  uvNodeType: node.uvNode.type,
  storeNodeType: node.storeNode.type,
  defaultUVType: node.getDefaultUV().type
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('StorageTexture3DNode native/module parity passed');
