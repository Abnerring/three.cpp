// Regenerate: node test/math/generate-threejs-fixtures.mjs /path/to/three.js
// Expected upstream: r187dev (package 0.186.0), 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
import { pathToFileURL, fileURLToPath } from 'node:url';
import { resolve, dirname } from 'node:path';
import { writeFileSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
const upstream = resolve(process.argv[2] ?? '../three.js');
const revision = execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim();
if (revision !== '11738a9d55e502fb484cefffcea3a66dc44e9fe9') throw new Error(`Expected pinned upstream revision, got ${revision}`);
const source = (name) => import(pathToFileURL(resolve(upstream, `src/math/${name}.js`)));
const { Vector2 } = await source('Vector2');
const { Vector3 } = await source('Vector3');
const { Vector4 } = await source('Vector4');
const { Matrix3 } = await source('Matrix3');
const { Matrix4 } = await source('Matrix4');
const { Euler } = await source('Euler');
const { Quaternion } = await source('Quaternion');
const { Color } = await source('Color');
const { PerspectiveCamera } = await import(pathToFileURL(resolve(upstream, 'src/cameras/PerspectiveCamera.js')));
const MathUtils = await source('MathUtils');
const data = {};
data.vector2Matrix = new Vector2(1.25, -2.5).applyMatrix3(new Matrix3(2, 3, 5, 7, 11, 13, 17, 19, 23)).toArray();
data.vector2Normal = new Vector2(3, 4).normalize().toArray();
data.vector2Clamp = new Vector2(3, 4).clampLength(2, 3).toArray();
data.vector3Projective = new Vector3(1.25, -2.5, -8).applyMatrix4(new Matrix4().makePerspective(-2, 3, 4, -1, 0.5, 25, 2001)).toArray();
data.vector3Direction = new Vector3(1, 2, 3).transformDirection(new Matrix4().makeScale(2, 3, 4)).toArray();
data.vector3Quaternion = new Vector3(1.25, -2.5, 3.75).applyQuaternion(new Quaternion(0.2, 0.3, -0.4, 0.8)).toArray();
for (const order of ['XYZ', 'XZY', 'YXZ', 'YZX', 'ZXY', 'ZYX']) {
  const e = new Euler(0.3, -0.7, 1.2, order);
  const m = new Matrix4().makeRotationFromEuler(e);
  data[`rotation${order}`] = m.toArray();
  data[`quaternion${order}`] = new Quaternion().setFromEuler(e).toArray();
  data[`euler${order}`] = new Euler(0, 0, 0, order).setFromRotationMatrix(m).toArray().slice(0, 3);
  const locked = new Euler(Math.PI / 2, Math.PI / 2, Math.PI / 2, order);
  data[`gimbal${order}`] = new Euler(0, 0, 0, order).setFromRotationMatrix(new Matrix4().makeRotationFromEuler(locked)).toArray().slice(0, 3);
}
data.axisNonunit = new Quaternion().setFromAxisAngle(new Vector3(2, 0, 0), 0.7).toArray();
data.unitVectors = new Quaternion().setFromUnitVectors(new Vector3(1, 0, 0), new Vector3(0, 1, 0)).toArray();
data.oppositeVectors = new Quaternion().setFromUnitVectors(new Vector3(0, 0, 1), new Vector3(0, 0, -1)).toArray();
const a = new Quaternion().setFromEuler(new Euler(0.2, 0.3, 0.4));
const b = new Quaternion().setFromEuler(new Euler(-0.8, 1.1, -0.7));
const minusB = new Quaternion(-b.x, -b.y, -b.z, -b.w);
data.slerpNegative = a.clone().slerp(minusB, 0.35).toArray();
data.slerpExtrapolate = a.clone().slerp(b, -0.75).toArray();
data.slerpSmall = new Quaternion().slerp(new Quaternion().setFromAxisAngle(new Vector3(0, 1, 0), 0.0002), 0.2).toArray();
const src0 = [90, ...a.toArray(), 91], src1 = [80, 81, ...minusB.toArray()];
const flatSlerp = [70];
Quaternion.slerpFlat(flatSlerp, 1, src0, 1, src1, 2, 0.35);
data.flatSlerp = flatSlerp;
const flatMultiply = [70];
Quaternion.multiplyQuaternionsFlat(flatMultiply, 1, src0, 1, src1, 2);
data.flatMultiply = flatMultiply;
data.quaternionZero = new Quaternion(0, 0, 0, 0).normalize().toArray();
data.quaternionInverse = new Quaternion(1, 2, 3, 4).invert().toArray();
data.axisQuaternion = new Vector4().setAxisAngleFromQuaternion(a).toArray();
data.axisMatrix = new Vector4().setAxisAngleFromRotationMatrix(new Matrix4().makeRotationFromQuaternion(a)).toArray();
data.axisIdentity = new Vector4().setAxisAngleFromRotationMatrix(new Matrix4()).toArray();
data.axisHalfTurn = new Vector4().setAxisAngleFromRotationMatrix(new Matrix4().makeRotationY(Math.PI)).toArray();
for (const coordinate of [2000, 2001]) for (const reverse of [false, true]) {
  data[`perspective${coordinate}${Number(reverse)}`] = new Matrix4().makePerspective(-2, 3, 4, -1, 0.5, 25, coordinate, reverse).toArray();
  data[`orthographic${coordinate}${Number(reverse)}`] = new Matrix4().makeOrthographic(-2, 3, 4, -1, 0.5, 25, coordinate, reverse).toArray();
}
const composed = new Matrix4().compose(new Vector3(5, -2, 7), a, new Vector3(-2, 3, 4));
const p = new Vector3(), q = new Quaternion(), s = new Vector3();
composed.decompose(p, q, s);
data.decompose = [...p.toArray(), ...q.toArray(), ...s.toArray()];
data.normalMatrix = new Matrix3().getNormalMatrix(composed).toArray();
// Evaluation order matters here: a distributive expansion loses cancellation
// differently and overflows/underflows intermediates that this source avoids.
data.determinantCancellation = [new Matrix4(
  80000, -100000, 90000, -70000,
  -100000, 50000, -70000, 40000,
  -70000, 90000, -80000, 80000,
  -20000, -50000, 20000, -29999.99999
).determinant()];
data.determinantOverflow = [new Matrix4(
  1e200, 0, 0, 0, 0, 1e200, 0, 0,
  0, 0, 1e-200, 0, 0, 0, 0, 1e-200
).determinant()];
data.determinantUnderflow = [new Matrix4(
  1e-200, 0, 0, 0, 0, 1e-200, 0, 0,
  0, 0, 1e200, 0, 0, 0, 0, 1e200
).determinant()];
const singular = new Matrix4().makeScale(0, 2, 3).setPosition(5, -2, 7);
singular.decompose(p, q, s);
data.decomposeSingular = [...p.toArray(), ...q.toArray(), ...s.toArray()];
data.extractSingular = new Matrix4().extractRotation(singular).toArray();
for (const [name, value] of Object.entries({hex: '#123abc', short: '#f82', rgb: 'rgb(125, 64, 255)', percent: 'rgb(50.5%, 0%, 100%)', hsl: 'hsl(-120, 60%, 40%)', name: 'rebeccapurple'})) {
  data[`color${name}`] = new Color(value).toArray();
}
data.colorSub = new Color(0.1, 0.2, 0.3).sub(new Color(0.2, 0.1, 0.8)).toArray();
data.colorHsl = new Color().setHSL(2.2, 1.7, 0.4).toArray();
data.colorLerpHsl = new Color().setHSL(0.9, 0.5, 0.6).lerpHSL(new Color().setHSL(0.1, 0.7, 0.2), 0.3).toArray();
data.colorOffsetHsl = new Color().setHSL(0.2, 0.4, 0.6).offsetHSL(0.3, 0.2, -0.1).toArray();
data.seededRandom = [MathUtils.seededRandom(123), ...Array.from({length: 7}, () => MathUtils.seededRandom())];
data.mathUtils = [MathUtils.euclideanModulo(-1.25, 0.75), MathUtils.pingpong(2.75, 1.5), MathUtils.inverseLerp(-2, 5, 0.7), MathUtils.degToRad(37.2), MathUtils.radToDeg(0.4)];
data.powerOfTwo = [2 ** 32 + 1, 2 ** 32, 1.5, 2 ** 53, -2, 0, NaN, Infinity, -Infinity].map(value => Number(MathUtils.isPowerOfTwo(value)));
for (const order of ['XYX', 'XZX', 'YXY', 'YZY', 'ZXZ', 'ZYZ']) {
  const quaternion = new Quaternion();
  MathUtils.setQuaternionFromProperEuler(quaternion, 0.3, -0.7, 1.2, order);
  data[`proper${order}`] = quaternion.toArray();
}
data.minMaxEdges = [Number(Number.isNaN(Math.min(NaN, 1))), Number(Number.isNaN(Math.max(1, NaN))), Number(Object.is(Math.min(0, -0), -0)), Number(Object.is(Math.max(-0, 0), -0))];
const camera = new PerspectiveCamera(55, 1.5, 0.5, 25);
camera.coordinateSystem = 2001;
camera.position.set(5, -2, 7);
camera.rotation.set(0.2, 0.3, 0.4);
camera.updateProjectionMatrix(); camera.updateMatrixWorld(true);
data.cameraProject = new Vector3(1.2, -0.5, -2.3).project(camera).toArray();
data.cameraUnproject = new Vector3(0.3, -0.6, 0.7).unproject(camera).toArray();
data.colorMatrix = new Color(0.1, 0.2, 0.3).applyMatrix3(new Matrix3(2, 3, 5, 7, 11, 13, 17, 19, 23)).toArray();
let ec = 0, qc = 0;
const ce = new Euler()._onChange(() => ++ec), cq = new Quaternion()._onChange(() => ++qc);
ce.x = 0.3; ce.set(0.2, 0.3, 0.4); ce.setFromQuaternion(a, 'YXZ', false); ce.setFromQuaternion(a);
cq.x = 0.3; cq.copy(a); cq.setFromEuler(ce, false); cq.setFromEuler(ce); cq.slerp(b, 0); cq.normalize();
data.callbacks = [ec, qc];
const lines = Object.entries(data).map(([name, values]) => `    { "${name}", { ${values.map(value => Object.is(value, -0) ? '-0.0' : String(value)).join(', ')} } },`);
writeFileSync(resolve(dirname(fileURLToPath(import.meta.url)), 'ThreeJsFixtures.h'), '// Generated by generate-threejs-fixtures.mjs from pinned three.js r187dev (package 0.186.0).\n#pragma once\n#include <map>\n#include <string>\n#include <vector>\nnamespace ThreeJsFixtures {\ninline const std::map<std::string, std::vector<double>> data {\n' + lines.join('\n') + '\n};\n}\n');
console.log(`Generated ${Object.keys(data).length} upstream math fixtures.`);
