import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: MeshBasicNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/MeshBasicNodeMaterial.js')).href);
const { normalViewGeometry } = await import(pathToFileURL(path.join(upstream, 'src/nodes/accessors/Normal.js')).href);
const { diffuseColor } = await import(pathToFileURL(path.join(upstream, 'src/nodes/core/PropertyNode.js')).href);

const snapshot = (material) => {
  const names = Object.getOwnPropertyNames(material).filter((name) => name !== 'uuid').sort();
  const kinds = {};
  const scalars = {};
  for (const name of names) {
    const value = material[name];
    let kind = 'object';
    if (value === null) kind = 'null';
    else if (typeof value === 'number') kind = 'number';
    else if (typeof value === 'boolean') kind = 'boolean';
    else if (typeof value === 'string') kind = 'string';
    else if (typeof value === 'function') kind = 'function';
    else if (value === undefined) kind = 'undefined';
    kinds[name] = kind;
    if (name === 'id') continue;
    if (kind === 'number' || kind === 'boolean' || kind === 'string') scalars[name] = value;
  }
  return { names, kinds, scalars };
};

const material = new MeshBasicNodeMaterial();
const normal = material.setupNormal();
const outgoing = material.setupOutgoingLight();
const model = material.setupLightingModel();
const secondModel = material.setupLightingModel();
const plainBuilder = { material: { lightMap: null } };
const noLightMap = material.setupLightMap(plainBuilder);
const environment = material.setupEnvironment(plainBuilder);
const lightMap = material.setupLightMap({ material: { lightMap: true } });

const expected = {
  name: MeshBasicNodeMaterial.name,
  length: MeshBasicNodeMaterial.length,
  staticType: MeshBasicNodeMaterial.type,
  type: material.type,
  flag: material.isMeshBasicNodeMaterial,
  isNodeMaterial: material.isNodeMaterial,
  lights: material.lights,
  fog: material.fog,
  snapshot: snapshot(material),
  normalIsNode: normal.isNode === true,
  normalDiffers: normal !== normalViewGeometry,
  outgoingIsNode: outgoing.isNode === true,
  outgoingIsDiffuseRgb: outgoing === diffuseColor.rgb,
  noLightMapIsNull: noLightMap === null,
  lightMapIsNode: lightMap.isNode === true,
  environmentIsNull: environment === null,
  modelIndirectLength: model.indirect.length,
  modelStartLength: model.start.length,
  modelsDistinct: secondModel !== model,
  setupNormalLength: material.setupNormal.length,
  setupEnvironmentLength: material.setupEnvironment.length,
  setupLightMapLength: material.setupLightMap.length,
  setupOutgoingLightLength: material.setupOutgoingLight.length,
  setupLightingModelLength: material.setupLightingModel.length,
  source: MeshBasicNodeMaterial.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('MeshBasicNodeMaterial parity passed');
