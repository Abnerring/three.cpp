// Differential check for the native MorphBlendMesh port.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to the
// pinned build. The animation records, the derived frame ranges and the morph target influences after two update
// steps are compared as data. Infinite durations serialise as null on both sides and are additionally compared
// through an explicit boolean.
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
const sourcePath = path.join(upstream, 'examples/jsm/misc/MorphBlendMesh.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { MorphBlendMesh } = await import(moduleUrl);

const makeMorphGeometry = () => {
  const geometry = new THREE.BoxGeometry();
  const count = geometry.attributes.position.count * 3;
  geometry.morphAttributes.position = [ 0, 1, 2 ].map(
    () => new THREE.BufferAttribute(new Float32Array(count), 3)
  );
  return geometry;
};
const makeMorphMesh = () => new MorphBlendMesh(makeMorphGeometry(), new THREE.MeshBasicMaterial());
const animationValue = (animation) => ({
  start: animation.start,
  end: animation.end,
  length: animation.length,
  fps: animation.fps,
  duration: animation.duration,
  lastFrame: animation.lastFrame,
  currentFrame: animation.currentFrame,
  active: animation.active,
  time: animation.time,
  direction: animation.direction,
  weight: animation.weight,
  directionBackwards: animation.directionBackwards,
  mirroredLoop: animation.mirroredLoop
});

const mesh = makeMorphMesh();
const defaultAnimation = mesh.animationsMap.__default;
const defaultDurationIsNegativeInfinity =
  defaultAnimation.duration === -Infinity;

mesh.createAnimation('walk', 0, 2, 2);
mesh.setAnimationFPS('walk', 4);
const durationAfterFps = mesh.getAnimationDuration('walk');
mesh.setAnimationDuration('walk', 1);
mesh.setAnimationWeight('walk', 0.5);
mesh.setAnimationTime('walk', 0.25);
mesh.setAnimationDirectionBackward('walk');
const backwardDirection = mesh.animationsMap.walk.direction;
const backwardFlag = mesh.animationsMap.walk.directionBackwards;
mesh.setAnimationDirectionForward('walk');

mesh.morphTargetDictionary = { run_01: 0, run_02: 1, walk_01: 2, walk_02: 3 };
mesh.autoCreateAnimations(30);

const updateMesh = makeMorphMesh();
// The constructor already produced three zero influences.
updateMesh.createAnimation('walk', 0, 2, 2);
updateMesh.playAnimation('walk');
updateMesh.update(0.1);
const influencesAfterFirst = Array.from(updateMesh.morphTargetInfluences);
updateMesh.update(0.5);
const influencesAfterSecond = Array.from(updateMesh.morphTargetInfluences);

const mirroredMesh = makeMorphMesh();
// The constructor already produced three zero influences.
mirroredMesh.createAnimation('walk', 0, 2, 2);
mirroredMesh.animationsMap.walk.mirroredLoop = true;
mirroredMesh.playAnimation('walk');
mirroredMesh.setAnimationTime('walk', 0.9);
mirroredMesh.update(0.2);

const expected = {
  animationCount: Object.keys(mesh.animationsMap).length,
  listCount: mesh.animationsList.length,
  defaultAnimation: animationValue(defaultAnimation),
  defaultDurationIsNegativeInfinity,
  runAnimation: animationValue(mesh.animationsMap.run),
  walkAnimation: animationValue(mesh.animationsMap.walk),
  firstAnimation: mesh.firstAnimation,
  durationAfterFps,
  backwardDirection,
  backwardFlag,
  influencesAfterFirst,
  influencesAfterSecond,
  mirroredAnimation: animationValue(mirroredMesh.animationsMap.walk)
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('MorphBlendMesh parity passed');
