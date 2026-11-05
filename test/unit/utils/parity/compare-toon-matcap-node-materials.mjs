import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: MeshToonNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/MeshToonNodeMaterial.js')).href);
const { default: MeshMatcapNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/MeshMatcapNodeMaterial.js')).href);

// The same snapshot the probe takes: own property names, the kind of every value and every scalar,
// skipping the random uuid and the per process id value.
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

const toon = new MeshToonNodeMaterial();
const matcap = new MeshMatcapNodeMaterial();
const model = toon.setupLightingModel({});
const secondModel = toon.setupLightingModel({});

let matcapVariantsBuilt = true;
try {
  matcap.setupVariants({ material: { matcap: null } });
} catch {
  matcapVariantsBuilt = false;
}

const expected = {
  toonName: MeshToonNodeMaterial.name,
  toonLength: MeshToonNodeMaterial.length,
  toonStaticType: MeshToonNodeMaterial.type,
  toonType: toon.type,
  toonFlag: toon.isMeshToonNodeMaterial,
  toonIsNodeMaterial: toon.isNodeMaterial,
  toonLights: toon.lights,
  toonFog: toon.fog,
  toonGradientMapIsNull: toon.gradientMap === null,
  toonSnapshot: snapshot(toon),
  modelDirectLength: model.direct.length,
  modelIndirectLength: model.indirect.length,
  modelStartLength: model.start.length,
  modelsDistinct: secondModel !== model,
  matcapName: MeshMatcapNodeMaterial.name,
  matcapLength: MeshMatcapNodeMaterial.length,
  matcapStaticType: MeshMatcapNodeMaterial.type,
  matcapType: matcap.type,
  matcapFlag: matcap.isMeshMatcapNodeMaterial,
  matcapIsNodeMaterial: matcap.isNodeMaterial,
  matcapMatcapIsNull: matcap.matcap === null,
  matcapSnapshot: snapshot(matcap),
  matcapVariantsBuilt,
  toonSource: MeshToonNodeMaterial.toString(),
  matcapSource: MeshMatcapNodeMaterial.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('MeshToonNodeMaterial and MeshMatcapNodeMaterial parity passed');
