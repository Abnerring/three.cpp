import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: LineDashedNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/LineDashedNodeMaterial.js')).href);

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

const material = new LineDashedNodeMaterial();
let variantsBuilt = true;
let variantFailure;
try {
  material.setupVariants({});
} catch (error) {
  variantsBuilt = false;
  variantFailure = String(error && error.message ? error.message : error);
}

const expected = {
  name: LineDashedNodeMaterial.name,
  length: LineDashedNodeMaterial.length,
  staticType: LineDashedNodeMaterial.type,
  type: material.type,
  flag: material.isLineDashedNodeMaterial,
  isNodeMaterial: material.isNodeMaterial,
  dashOffset: material.dashOffset,
  dashSize: material.dashSize,
  gapSize: material.gapSize,
  offsetNodeIsNull: material.offsetNode === null,
  dashScaleNodeIsNull: material.dashScaleNode === null,
  dashSizeNodeIsNull: material.dashSizeNode === null,
  gapSizeNodeIsNull: material.gapSizeNode === null,
  snapshot: snapshot(material),
  setupVariantsLength: material.setupVariants.length,
  variantsBuilt,
  source: LineDashedNodeMaterial.toString()
};
if (!variantsBuilt) expected.variantFailure = variantFailure;

const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('LineDashedNodeMaterial parity passed');
