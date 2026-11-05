// Differential check for the native SceneUtils port.
//
// The pinned module imports the bare "three" specifier and a sibling utils module by relative path. A data URL
// cannot resolve the relative import, so the rewritten source is written to a temporary file inside the pinned
// utils directory, imported from there and removed again in a finally block.
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

const utilsDir = path.join(upstream, 'examples/jsm/utils');
const tempPath = path.join(utilsDir, '__parity_scene_utils.mjs');
const sourcePath = path.join(utilsDir, 'SceneUtils.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
fs.writeFileSync(tempPath, rewritten);

let helpers;
try {
  helpers = await import(pathToFileURL(tempPath).href + '?v=' + Date.now());
} finally {
  fs.rmSync(tempPath, { force: true });
}
const { createMeshesFromInstancedMesh, createMultiMaterialObject, createMeshesFromMultiMaterialMesh, reduceVertices, sortInstancedMesh } = helpers;

const geometry = new THREE.BoxGeometry();
const material = new THREE.MeshBasicMaterial();
const instanced = new THREE.InstancedMesh(geometry, material, 3);
const matrix = new THREE.Matrix4();
matrix.makeTranslation(1, 2, 3);
instanced.setMatrixAt(0, matrix);
matrix.makeTranslation(4, 5, 6);
instanced.setMatrixAt(1, matrix);
matrix.makeTranslation(7, 8, 9);
instanced.setMatrixAt(2, matrix);
instanced.name = 'instanced';
const group = createMeshesFromInstancedMesh(instanced);

const mesh = new THREE.Mesh(new THREE.BoxGeometry(), new THREE.MeshBasicMaterial());
const scene = new THREE.Scene();
scene.add(mesh);
const vertexCount = reduceVertices(scene, (value) => value + 1, 0);
const sumY = reduceVertices(scene, (value, vertex) => value + vertex.y, 0);
const minX = reduceVertices(scene, (value, vertex) => (value < vertex.x ? value : vertex.x), 1e9);

const multi = createMultiMaterialObject(geometry, [ new THREE.MeshBasicMaterial(), new THREE.MeshBasicMaterial() ]);

const payloads = new THREE.InstancedMesh(geometry, material, 3);
const payloadMatrix = new THREE.Matrix4();
payloadMatrix.makeTranslation(3, 0, 0);
payloads.setMatrixAt(0, payloadMatrix);
payloadMatrix.makeTranslation(1, 0, 0);
payloads.setMatrixAt(1, payloadMatrix);
payloadMatrix.makeTranslation(2, 0, 0);
payloads.setMatrixAt(2, payloadMatrix);
payloads.setColorAt(0, new THREE.Color(1, 0, 0));
payloads.setColorAt(1, new THREE.Color(0, 1, 0));
payloads.setColorAt(2, new THREE.Color(0, 0, 1));
payloads.geometry.setAttribute('instanceValue',
  new THREE.InstancedBufferAttribute(new Float32Array([ 30, 10, 20 ]), 1));
sortInstancedMesh(payloads, (a, b) => {
  const left = new THREE.Matrix4();
  const right = new THREE.Matrix4();
  payloads.getMatrixAt(a, left);
  payloads.getMatrixAt(b, right);
  return left.elements[12] - right.elements[12];
});
const payloadColours = [];
const payloadValues = [];
const payloadPositions = [];
const payloadColour = new THREE.Color();
const payloadAttribute = payloads.geometry.getAttribute('instanceValue');
const payloadCurrent = new THREE.Matrix4();
for (let i = 0; i < payloads.count; i += 1) {
  payloads.getColorAt(i, payloadColour);
  payloadColours.push(payloadColour.r, payloadColour.g, payloadColour.b);
  payloadValues.push(payloadAttribute.getX(i));
  payloads.getMatrixAt(i, payloadCurrent);
  payloadPositions.push(payloadCurrent.elements[12]);
}

const splitGeometry = new THREE.BufferGeometry();
splitGeometry.setAttribute('position', new THREE.BufferAttribute(
  new Float32Array([ 0, 0, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 3, 0, 0, 2, 1, 0 ]), 3));
splitGeometry.addGroup(0, 3, 0);
splitGeometry.addGroup(3, 3, 1);
const splitMaterialA = new THREE.MeshBasicMaterial();
const splitMaterialB = new THREE.MeshBasicMaterial();
const splitMesh = new THREE.Mesh(splitGeometry, [ splitMaterialA, splitMaterialB ]);
const split = createMeshesFromMultiMaterialMesh(splitMesh);
const multiMaterialChildCount = split.children.length;
const multiMaterialPositions = [];
const multiMaterialMaterials = [];
for (const child of split.children) {
  const position = child.geometry.getAttribute('position');
  for (let i = 0; i < position.count; i += 1) multiMaterialPositions.push(position.getX(i));
  multiMaterialMaterials.push(child.material === splitMaterialA ? 0 : 1);
}

const expected = {
  multiMaterialChildCount,
  multiMaterialPositions,
  multiMaterialMaterials,
  payloadColours,
  payloadValues,
  payloadPositions,
  groupChildCount: group.children.length,
  groupName: group.name,
  firstPosition: [ group.children[0].position.x, group.children[0].position.y, group.children[0].position.z ],
  multiMaterialCount: multi.children.length,
  vertexCount,
  sumY,
  minX
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
for (const key of Object.keys(expected)) {
  const e = expected[key];
  const a = actual[key];
  if (typeof e === 'number') {
    assert.ok(Math.abs(a - e) < 1e-12, `${key}: ${a} vs ${e}`);
  } else if (Array.isArray(e)) {
    assert.equal(a.length, e.length, `${key} length`);
    for (let i = 0; i < e.length; i += 1) assert.ok(Math.abs(a[i] - e[i]) < 1e-12, `${key}[${i}]`);
  } else {
    assert.deepEqual(a, e, key);
  }
}
console.log('SceneUtils parity passed');
