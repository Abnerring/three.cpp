// Differential check for the native DebugEnvironment port.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to the
// pinned build. The scene structure, the material and light parameters and the de-duplicated dispose events are
// compared as data.
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
const sourcePath = path.join(upstream, 'examples/jsm/environments/DebugEnvironment.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { DebugEnvironment } = await import(moduleUrl);

const vec3 = (vector) => [ vector.x, vector.y, vector.z ];
const environment = new DebugEnvironment();
const room = environment.children[0];
const mainLight = environment.children[1];
const first = environment.children[2];
const second = environment.children[3];
const third = environment.children[4];

let geometryEvents = 0;
let materialEvents = 0;
room.geometry.addEventListener('dispose', () => { geometryEvents += 1; });
for (const child of environment.children) {
  if (child.isMesh && child.material) {
    child.material.addEventListener('dispose', () => { materialEvents += 1; });
  }
}
environment.dispose();
environment.dispose();

const expected = {
  childrenCount: environment.children.length,
  roomIsMesh: room.isMesh === true,
  roomHasUv: room.geometry.getAttribute('uv') !== undefined,
  roomMetalness: room.material.metalness,
  roomSide: room.material.side,
  roomScale: vec3(room.scale),
  mainLightIsPointLight: mainLight.isPointLight === true,
  mainLightIntensity: mainLight.intensity,
  mainLightDistance: mainLight.distance,
  mainLightDecay: mainLight.decay,
  firstPosition: vec3(first.position),
  firstScale: vec3(first.scale),
  firstColor: first.material.color.getHex(),
  firstEmissive: first.material.emissive.getHex(),
  firstEmissiveIntensity: first.material.emissiveIntensity,
  secondPosition: vec3(second.position),
  secondScale: vec3(second.scale),
  secondColor: second.material.color.getHex(),
  thirdPosition: vec3(third.position),
  thirdScale: vec3(third.scale),
  thirdColor: third.material.color.getHex(),
  sharedGeometry: room.geometry === first.geometry,
  geometryDisposeEvents: geometryEvents,
  materialDisposeEvents: materialEvents
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('DebugEnvironment parity passed');
