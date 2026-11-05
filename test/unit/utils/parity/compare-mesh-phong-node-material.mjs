import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: MeshPhongNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/MeshPhongNodeMaterial.js')).href);

// The same snapshot the probe takes.
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

const material = new MeshPhongNodeMaterial();
const model = material.setupLightingModel({});
const secondModel = material.setupLightingModel({});
const environment = material.setupEnvironment({ material: {} });
let variantsBuilt = true;
try {
  material.setupVariants({ material: {} });
} catch {
  variantsBuilt = false;
}

const expected = {
  name: MeshPhongNodeMaterial.name,
  length: MeshPhongNodeMaterial.length,
  staticType: MeshPhongNodeMaterial.type,
  type: material.type,
  flag: material.isMeshPhongNodeMaterial,
  isNodeMaterial: material.isNodeMaterial,
  lights: material.lights,
  shininessNodeIsNull: material.shininessNode === null,
  specularNodeIsNull: material.specularNode === null,
  shininess: material.shininess,
  snapshot: snapshot(material),
  modelSpecular: model.specular,
  modelDirectLength: model.direct.length,
  modelIndirectLength: model.indirect.length,
  modelsDistinct: secondModel !== model,
  environmentIsNull: environment === null,
  variantsBuilt,
  source: MeshPhongNodeMaterial.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('MeshPhongNodeMaterial parity passed');
