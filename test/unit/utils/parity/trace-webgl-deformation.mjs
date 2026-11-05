import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { pathToFileURL } from 'node:url';

const pinned = path.resolve(process.argv[2] ?? 'D:/Temp/surui/threejs-pinned-11738a-20261104');
const output = path.resolve(process.argv[3] ?? `D:/Temp/surui/webgl-deformation-${Date.now()}`);
const load = relative => import(pathToFileURL(path.join(pinned, 'src', relative)).href);
const [{ Scene }, { PlaneGeometry }, { Float32BufferAttribute, Uint16BufferAttribute }, { MeshBasicMaterial },
  { Mesh }, { SkinnedMesh }, { Skeleton }, { Bone }, { InstancedMesh }, { Matrix4 }, { Vector3 },
  { WebGLObjects }, constants] = await Promise.all([
  load('scenes/Scene.js'), load('geometries/PlaneGeometry.js'), load('core/BufferAttribute.js'),
  load('materials/MeshBasicMaterial.js'), load('objects/Mesh.js'), load('objects/SkinnedMesh.js'),
  load('objects/Skeleton.js'), load('objects/Bone.js'), load('objects/InstancedMesh.js'),
  load('math/Matrix4.js'), load('math/Vector3.js'), load('renderers/webgl/WebGLObjects.js'), load('constants.js')
]);
let assertions = 0;
const same = (actual, expected) => { assertions++; assert.strictEqual(actual, expected); };
const close = (actual, expected) => { assertions++; assert.ok(Math.abs(actual - expected) < 1e-6, `${actual} != ${expected}`); };
const records = [];
function shape(first = 1, second = 0) {
  const geometry = new PlaneGeometry(.3, .3), indices = [], weights = [];
  for (let i = 0; i < geometry.attributes.position.count; i++) {
    indices.push(0, 1, 0, 0); weights.push(first, second, 0, 0);
  }
  geometry.setAttribute('skinIndex', new Uint16BufferAttribute(indices, 4));
  geometry.setAttribute('skinWeight', new Float32BufferAttribute(weights, 4));
  return geometry;
}
function skin(geometry = shape()) {
  const mesh = new SkinnedMesh(geometry, new MeshBasicMaterial()), bone = new Bone();
  mesh.add(bone); const skeleton = new Skeleton([bone]); mesh.bind(skeleton);
  const scene = new Scene(); scene.add(mesh); return { mesh, bone, skeleton, scene };
}
{
  const { mesh, bone, skeleton, scene } = skin();
  same(mesh.bindMode, constants.AttachedBindMode); same(skeleton.boneTexture, null);
  bone.position.x = .65; scene.updateMatrixWorld(true); skeleton.update();
  close(skeleton.boneMatrices[12], .65); same(skeleton.computeBoneTexture(), skeleton);
  const texture = skeleton.boneTexture, matrices = skeleton.boneMatrices;
  same(matrices.length, 64); same(texture.image.data, matrices); same(texture.image.width, 4); same(texture.image.height, 4);
  same(texture.format, constants.RGBAFormat); same(texture.type, constants.FloatType); same(texture.minFilter, constants.NearestFilter);
  same(texture.generateMipmaps, false); same(texture.flipY, false);
  const version = texture.version; skeleton.update(); same(texture.version, version + 1); same(skeleton.boneMatrices, matrices);
  const vertex = mesh.getVertexPosition(0, new Vector3()); close(vertex.x, .5); close(vertex.y, .15);
  let disposed = 0; texture.addEventListener('dispose', event => { same(event.target, texture); disposed++; });
  skeleton.dispose(); same(disposed, 1); same(skeleton.boneTexture, null); skeleton.update(); skeleton.computeBoneTexture();
  same(skeleton.boneTexture === texture, false); close(skeleton.boneMatrices[12], .65);
  records.push({ name: 'bone-texture', translation: .65, size: [4, 4], floats: 64,
    pixelCenters: { rest: [32, 32], translated: [53, 32] }, versionPerUpdate: 1, recreatedOnNull: true });
}
{
  const { mesh, bone, scene } = skin(shape(3, 1)), other = new Bone(); mesh.add(other);
  const skeleton = new Skeleton([bone, other]); mesh.bind(skeleton); mesh.normalizeSkinWeights();
  close(mesh.geometry.attributes.skinWeight.getX(0), .75); close(mesh.geometry.attributes.skinWeight.getY(0), .25);
  bone.position.x = .6; other.position.x = -.2; scene.updateMatrixWorld(true); skeleton.update();
  const vertex = mesh.getVertexPosition(0, new Vector3()); close(vertex.x, .25); close(vertex.y, .15);
  const weights = mesh.geometry.attributes.skinWeight; weights.setXYZW(0, 0, 0, 0, 0); mesh.normalizeSkinWeights();
  same(weights.getX(0), 1); same(weights.getY(0), 0);
  records.push({ name: 'weighted-bones', weights: [.75, .25], weightedTranslation: .4, pixelCenter: [45, 32], zeroWeightFallback: [1, 0, 0, 0] });
}
{
  // Execute the actual pinned WebGLObjects WeakMap/frame algorithm. Geometry
  // and attribute services below explicitly record the CPU device boundary.
  const events = [], geometry = shape(), material = new MeshBasicMaterial(), bone = new Bone(), skeleton = new Skeleton([bone]);
  const first = new SkinnedMesh(geometry, material), second = new SkinnedMesh(geometry, material);
  for (const mesh of [first, second]) { mesh.bindMode = constants.DetachedBindMode; mesh.bind(skeleton, new Matrix4()); }
  first.position.x = -.5; second.position.x = .5;
  const scene = new Scene(); scene.add(bone, first, second); scene.updateMatrixWorld(true);
  const info = { render: { frame: 0 } }, geometries = {
    get(object, value) { events.push(`geometry.get:${object === first ? 'first' : 'second'}`); return value; },
    update(value) { same(value, geometry); events.push('geometry.update'); }
  };
  const objects = WebGLObjects({}, geometries, {}, {}, info), originalUpdate = skeleton.update;
  let calls = 0, getterReads = 0;
  const update = function (...args) { same(this, skeleton); same(args.length, 0); calls++; events.push('skeleton.update'); return originalUpdate.apply(this, args); };
  Object.defineProperty(skeleton, 'update', { configurable: true, get() { getterReads++; return update; } });
  same(objects.update(first), geometry); same(objects.update(second), geometry); same(calls, 1); same(getterReads, 1);
  bone.position.y = .5; bone.updateMatrixWorld(true); objects.update(second); same(calls, 1); close(skeleton.boneMatrices[13], 0);
  info.render.frame++; objects.update(first); objects.update(second); same(calls, 2); same(getterReads, 2); close(skeleton.boneMatrices[13], .5);
  const sentinel = new Error('skeleton update sentinel'); info.render.frame++;
  assertions++; assert.throws(() => { skeleton.update = undefined; }, TypeError);
  records.push({ name: 'shared-skeleton-cache', callsPerFrame: 1, updateGetterReadsPerFrame: 1,
    beforeCallbackMatrixY: 0, nextFrameMatrixY: .5, events });
  Object.defineProperty(skeleton, 'update', { configurable: true, value() { calls++; throw sentinel; } });
  for (let i = 0; i < 2; i++) { assertions++; assert.throws(() => objects.update(first), error => error === sentinel); }
  same(calls, 4); Object.defineProperty(skeleton, 'update', { configurable: true, value: update });
  objects.update(first); same(calls, 5); objects.update(second); same(calls, 5);
  records.push({ name: 'throw-does-not-cache-frame', retriesBeforeSuccess: 2, successfulRetryUpdatesSharedObjects: 1 });
}
function morph(relative, skinned = true) {
  const geometry = skinned ? shape() : new PlaneGeometry(.3, .3), values = [], positions = geometry.attributes.position;
  for (let i = 0; i < positions.count; i++) values.push(relative ? .7 : positions.getX(i) + .7, relative ? 0 : positions.getY(i), 0);
  const target = new Float32BufferAttribute(values, 3); target.name = 'shift';
  geometry.morphAttributes.position = [target]; geometry.morphTargetsRelative = relative; return geometry;
}
for (const relative of [false, true]) {
  const { mesh, bone, scene, skeleton } = skin(morph(relative));
  same(mesh.morphTargetDictionary.shift, 0); mesh.morphTargetInfluences[0] = 1; bone.rotation.z = Math.PI / 2;
  scene.updateMatrixWorld(true); skeleton.update(); const vertex = mesh.getVertexPosition(0, new Vector3());
  close(vertex.x, -.15); close(vertex.y, .55);
  records.push({ name: `morph-before-skin-${relative ? 'relative' : 'absolute'}`, firstVertex: vertex.toArray(), pixelCenter: [32, 10] });
}
{
  const geometry = morph(false, false), material = new MeshBasicMaterial(), mesh = new InstancedMesh(geometry, material, 2), source = new Mesh(geometry, material);
  source.morphTargetInfluences[0] = 0; same(mesh.setMorphAt(0, source), mesh); source.morphTargetInfluences[0] = 1; mesh.setMorphAt(1, source);
  const texture = mesh.morphTexture; same(texture.version, 0); same(texture.format, constants.RedFormat); same(texture.type, constants.FloatType);
  assertions++; assert.deepEqual(Array.from(texture.image.data), [1, 0, 0, 1]);
  texture.needsUpdate = true; const version = texture.version;
  source.morphTargetInfluences[0] = 1; mesh.setMorphAt(0, source); source.morphTargetInfluences[0] = 0; mesh.setMorphAt(1, source); same(texture.version, version);
  assertions++; assert.deepEqual(Array.from(texture.image.data), [0, 1, 1, 0]); mesh.getMorphAt(0, source); same(source.morphTargetInfluences[0], 1);
  records.push({ name: 'instance-morph-influences', initialRows: [[1, 0], [0, 1]], updatedRows: [[0, 1], [1, 0]],
    setMorphAtMarksNeedsUpdate: false, firstPixels: [[16, 32], [48, 16]], versionedPixels: [[38, 32], [26, 16]] });
}
const sourceHashes = {};
for (const file of ['objects/SkinnedMesh.js', 'objects/Skeleton.js', 'objects/InstancedMesh.js', 'renderers/webgl/WebGLObjects.js']) {
  sourceHashes[file] = crypto.createHash('sha256').update(fs.readFileSync(path.join(pinned, 'src', file))).digest('hex');
}
const result = { boundary: 'Actual pinned CPU methods and WebGLObjects cache; command services are recorders; no GPU execution', assertions, sourceHashes, records };
fs.mkdirSync(output, { recursive: false }); fs.writeFileSync(path.join(output, 'trace.json'), JSON.stringify(result, null, 2) + '\n');
console.log(JSON.stringify({ output, assertions, scenarios: records.length, boundary: result.boundary }));
