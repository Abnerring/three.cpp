// Differential check for the native GroundedSkybox port.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to the
// pinned build. The whole deformed position buffer is compared, which is the contract of this addon, together with
// the material wiring and the geometry parameters.
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
const sourcePath = path.join(upstream, 'examples/jsm/objects/GroundedSkybox.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { GroundedSkybox } = await import(moduleUrl);

const texture = new THREE.Texture();
const skybox = new GroundedSkybox(texture, 15, 100, 4);
const geometry = skybox.geometry;
const position = geometry.getAttribute('position');
const positions = Array.from(position.array);
const minimumY = Math.min(...positions.filter((_, index) => index % 3 === 1));

const expected = {
  isMesh: skybox.isMesh === true,
  isSphereGeometry: geometry.type === 'SphereGeometry',
  radius: geometry.parameters.radius,
  widthSegments: geometry.parameters.widthSegments,
  heightSegments: geometry.parameters.heightSegments,
  vertexCount: position.count,
  minimumY,
  materialIsBasic: skybox.material.type === 'MeshBasicMaterial',
  mapIsSameTexture: skybox.material.map === texture,
  depthWrite: skybox.material.depthWrite,
  positions
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));

for (const key of [ 'isMesh', 'isSphereGeometry', 'radius', 'widthSegments', 'heightSegments',
  'vertexCount', 'materialIsBasic', 'mapIsSameTexture', 'depthWrite' ]) {
  assert.deepEqual(actual[key], expected[key], key);
}
assert.ok(Math.abs(actual.minimumY - expected.minimumY) < 1e-12, 'minimumY');
assert.equal(actual.positions.length, expected.positions.length, 'position length');
for (let i = 0; i < expected.positions.length; i += 1) {
  assert.ok(Math.abs(actual.positions[i] - expected.positions[i]) < 1e-12,
    `position[${i}] ${actual.positions[i]} vs ${expected.positions[i]}`);
}
console.log('GroundedSkybox parity passed');
