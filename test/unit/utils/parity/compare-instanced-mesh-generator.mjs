// Differential check for the native InstancedMeshGenerator port.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to the pinned
// build. The created mesh state and the instance translations are compared.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const threeUrl = pathToFileURL(path.join(upstream, 'build/three.module.js')).href;
const THREE = await import(threeUrl);
const sourcePath = path.join(upstream, 'examples/jsm/generators/city/InstancedMeshGenerator.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { InstancedMeshGenerator } = await import(moduleUrl);

const generator = new InstancedMeshGenerator(
  { size: 1 },
  () => new THREE.BoxGeometry(),
  () => new THREE.MeshBasicMaterial(),
  'city',
  false
);

const placements = [];
for (let i = 0; i < 2; i += 1) {
  const matrix = new THREE.Matrix4();
  matrix.makeTranslation(i + 1, 2.0, 3.0);
  placements.push(matrix);
}
const mesh = generator.build(placements);

const translations = [];
for (let i = 0; i < placements.length; i += 1) {
  const matrix = new THREE.Matrix4();
  mesh.getMatrixAt(i, matrix);
  translations.push(matrix.elements[12], matrix.elements[13], matrix.elements[14]);
}

// Capacity growth: a third placement no longer fits the capacity of two.
const threePlacements = [ ...placements ];
{
  const extra = new THREE.Matrix4();
  extra.makeTranslation(3.0, 2.0, 3.0);
  threePlacements.push(extra);
}
const grownMesh = generator.build(threePlacements);

// Parameter change: the same placements rebuild because the parameters key changed.
generator.parameters.size = 2;
const rebuiltMesh = generator.build(placements);
const rebuiltTranslations = [];
for (let i = 0; i < placements.length; i += 1) {
  const matrix = new THREE.Matrix4();
  rebuiltMesh.getMatrixAt(i, matrix);
  rebuiltTranslations.push(matrix.elements[12], matrix.elements[13], matrix.elements[14]);
}

const expected = {
  grownCapacity: grownMesh.instanceMatrix.count,
  grownCount: grownMesh.count,
  rebuiltCapacity: rebuiltMesh.instanceMatrix.count,
  rebuiltCount: rebuiltMesh.count,
  rebuiltTranslations,
  capacity: mesh.instanceMatrix.count,
  count: mesh.count,
  visible: mesh.visible,
  name: mesh.name,
  castShadow: mesh.castShadow,
  receiveShadow: mesh.receiveShadow,
  translations,
  geometryIsBox: generator.geometry.type === 'BoxGeometry',
  materialIsBasic: generator.material.type === 'MeshBasicMaterial'
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('InstancedMeshGenerator parity passed');