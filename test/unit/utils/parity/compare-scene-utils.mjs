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
const { createMeshesFromInstancedMesh, createMultiMaterialObject, reduceVertices } = helpers;

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

const expected = {
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
