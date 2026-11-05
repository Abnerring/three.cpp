import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: VolumeNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/VolumeNodeMaterial.js')).href);

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

const material = new VolumeNodeMaterial();
const model = material.setupLightingModel({});
const secondModel = material.setupLightingModel({});
const configured = new VolumeNodeMaterial({ steps: 50, transparent: false });

const expected = {
  name: VolumeNodeMaterial.name,
  length: VolumeNodeMaterial.length,
  staticType: VolumeNodeMaterial.type,
  type: material.type,
  flag: material.isVolumeNodeMaterial,
  isNodeMaterial: material.isNodeMaterial,
  steps: material.steps,
  offsetNodeIsNull: material.offsetNode === null,
  scatteringNodeIsNull: material.scatteringNode === null,
  lights: material.lights,
  transparent: material.transparent,
  side: material.side,
  depthTest: material.depthTest,
  depthWrite: material.depthWrite,
  snapshot: snapshot(material),
  setupLightingModelLength: material.setupLightingModel.length,
  modelScatteringLightLength: model.scatteringLight.length,
  modelDirectLength: model.direct.length,
  modelFinishLength: model.finish.length,
  modelsDistinct: secondModel !== model,
  configuredSteps: configured.steps,
  configuredTransparent: configured.transparent,
  source: VolumeNodeMaterial.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('VolumeNodeMaterial parity passed');
