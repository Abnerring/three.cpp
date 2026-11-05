// Differential check for the native MorphAnimMesh port.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to
// the pinned build. The mixer wiring, the time scales and the unknown label error are compared as data, and the
// error message prefix is compared as a boolean because the exception types differ between the two languages.
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
const sourcePath = path.join(upstream, 'examples/jsm/misc/MorphAnimMesh.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { MorphAnimMesh } = await import(moduleUrl);

const makeClip = (name, duration, trackCount) => {
  const tracks = [];
  for (let i = 0; i < trackCount; i += 1) {
    tracks.push(new THREE.NumberKeyframeTrack('.position[x]', [ 0, duration ], [ 0, 1 ]));
  }
  return new THREE.AnimationClip(name, duration, tracks);
};

const mesh = new MorphAnimMesh(new THREE.BoxGeometry(), new THREE.MeshBasicMaterial());
const initialActiveNull = mesh.activeAction === null;
const hasRoot = mesh.mixer.getRoot() !== undefined;

mesh.setDirectionBackward();
const backwardScale = mesh.mixer.timeScale;
mesh.setDirectionForward();
const forwardScale = mesh.mixer.timeScale;

mesh.animations.push(makeClip('walk', 2.0, 2));
mesh.playAnimation('walk', 30.0);
const playing = mesh.activeAction !== null && mesh.activeAction !== undefined;
const actionTimeScale = mesh.activeAction.timeScale;
const running = mesh.activeAction.isRunning();

let updateThrew = false;
try {
  mesh.updateAnimation(0.1);
} catch {
  updateThrew = true;
}

let missingThrew = false;
let missingMessage = '';
try {
  mesh.playAnimation('missing', 30.0);
} catch (error) {
  missingThrew = true;
  missingMessage = String(error && error.message ? error.message : error);
}

const expected = {
  type: mesh.type,
  initialActiveNull,
  hasRoot,
  backwardScale,
  forwardScale,
  playing,
  actionTimeScale,
  running,
  updateThrew,
  missingThrew,
  missingMessageHasPrefix: missingMessage.startsWith('THREE.MorphAnimMesh: animations[missing]')
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('MorphAnimMesh parity passed');
