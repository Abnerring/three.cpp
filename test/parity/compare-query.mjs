// Executes the pinned upstream implementation for every C++ probe input.
import { spawnSync } from 'node:child_process';
import { resolve } from 'node:path';
import { pathToFileURL } from 'node:url';

const [binary, upstream] = process.argv.slice(2);
if (!binary || !upstream) throw new Error('Usage: node compare-query.mjs <probe> <three.js path>');
const pin = '11738a9d55e502fb484cefffcea3a66dc44e9fe9';
const revision = spawnSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' });
if (revision.status !== 0 || revision.stdout.trim() !== pin) throw new Error(`Expected upstream commit ${pin}`);
const load = async (file, name) => (await import(pathToFileURL(resolve(upstream, 'src', file)).href))[name];
const [Vector2, Vector3, Vector4, Plane, Line3, Triangle, Ray, Sphere, Box3, Frustum, Matrix3, Matrix4,
       BufferAttribute, Float32BufferAttribute, BoxGeometry, Mesh] = await Promise.all([
    load('math/Vector2.js', 'Vector2'), load('math/Vector3.js', 'Vector3'), load('math/Vector4.js', 'Vector4'),
    load('math/Plane.js', 'Plane'), load('math/Line3.js', 'Line3'), load('math/Triangle.js', 'Triangle'),
    load('math/Ray.js', 'Ray'), load('math/Sphere.js', 'Sphere'), load('math/Box3.js', 'Box3'),
    load('math/Frustum.js', 'Frustum'), load('math/Matrix3.js', 'Matrix3'), load('math/Matrix4.js', 'Matrix4'),
    load('core/BufferAttribute.js', 'BufferAttribute'), load('core/BufferAttribute.js', 'Float32BufferAttribute'),
    load('geometries/BoxGeometry.js', 'BoxGeometry'), load('objects/Mesh.js', 'Mesh')
]);
const decode = value => {
    if (Array.isArray(value)) return value.map(decode);
    if (value && typeof value === 'object') return Object.fromEntries(Object.entries(value).map(([k, v]) => [k, decode(v)]));
    if (value === 'NaN') return NaN;
    if (value === 'Infinity') return Infinity;
    if (value === '-Infinity') return -Infinity;
    if (value === '-0') return -0;
    return value;
};
const encode = value => {
    if (typeof value === 'number') {
        if (Number.isNaN(value)) return 'NaN';
        if (!Number.isFinite(value)) return value > 0 ? 'Infinity' : '-Infinity';
        if (Object.is(value, -0)) return '-0';
        return value;
    }
    if (Array.isArray(value)) return value.map(encode);
    if (value && typeof value === 'object') return Object.fromEntries(Object.entries(value).map(([k, v]) => [k, encode(v)]));
    return value;
};
const v = a => new Vector3(...a);
const p = a => new Plane(v(a.normal), a.constant);
const l = a => new Line3(v(a[0]), v(a[1]));
const t = a => new Triangle(...a.map(v));
const r = a => new Ray(v(a[0]), v(a[1]));
const b = a => new Box3(v(a[0]), v(a[1]));
const s = a => new Sphere(v(a[0]), a[1]);
const m = a => new Matrix4().fromArray(a);
const vl = a => [a.start.toArray(), a.end.toArray()];
const vt = a => [a.a.toArray(), a.b.toArray(), a.c.toArray()];
const vr = a => [a.origin.toArray(), a.direction.toArray()];
const nullable = (hit, target) => ({ hit: hit !== null, target: target.toArray() });
const points = [[0, 0, -.3], [0, 0, -20], [0, 0, -.299], [0, 0, -20.001],
                [0, 0, -2], [3, 4, -2], [-2, -1, -2], [NaN, 0, 0]].map(v);
const spheres = [[[0, 0, 0], 1], [[2, 0, 0], 0], [[0, 0, 0], -1], [[NaN, 0, 0], 1], [[0, 0, 0], Infinity]].map(s);
const boxes = [[[-1, -1, -1], [1, 1, 1]], [[2, 0, 0], [2, 0, 0]],
               [[Infinity, Infinity, Infinity], [-Infinity, -Infinity, -Infinity]],
               [[NaN, -1, -1], [2, 2, 2]], [[-Infinity, -Infinity, -Infinity], [Infinity, Infinity, Infinity]]].map(b);

function expected(kind, a) {
    switch (kind) {
    case 'vectorNormalization': {
        const V = a.length === 2 ? Vector2 : a.length === 3 ? Vector3 : Vector4;
        const value = new V(...a);
        return { normalized: value.clone().normalize().toArray(), clamped: value.clone().clampLength(2, 3).toArray(),
            setLength: value.clone().setLength(2).toArray() };
    }
    case 'plane': {
        const plane = p(a.plane), point = v(a.point), line = l(a.line), matrix = m(a.matrix);
        const projected = plane.projectPoint(point, new Vector3());
        const coplanar = plane.coplanarPoint(new Vector3());
        const target = new Vector3(9, 8, 7), extended = target.clone();
        const hit = plane.intersectLine(line, target), extendedHit = plane.intersectLine(line, extended, false);
        const restored = new Plane().fromJSON(plane.toJSON());
        const fromPoints = new Plane().setFromCoplanarPoints(line.start, line.end, point);
        return { distance: plane.distanceToPoint(point), projected: projected.toArray(), coplanar: coplanar.toArray(),
            lineHit: nullable(hit, target), extendedHit: nullable(extendedHit, extended), intersectsLine: plane.intersectsLine(line),
            normalized: plane.clone().normalize().toJSON(), negated: plane.clone().negate().toJSON(),
            translated: plane.clone().translate(point).toJSON(), transformed: plane.clone().applyMatrix4(matrix).toJSON(),
            explicitNormalMatrix: plane.clone().applyMatrix4(matrix, new Matrix3().getNormalMatrix(matrix)).toJSON(),
            restored: restored.toJSON(), cloneEquals: plane.clone().equals(plane), fromPoints: fromPoints.toJSON(),
            sphereDistance: plane.distanceToSphere(s(a.sphere)), boxHit: plane.intersectsBox(b(a.box)),
            sphereHit: plane.intersectsSphere(s(a.sphere)) };
    }
    case 'line': {
        const line = l(a.line), other = l(a.other), point = v(a.point), c1 = new Vector3(), c2 = new Vector3();
        const segmentDistanceSq = line.distanceSqToLine3(other, c1, c2);
        return { center: line.getCenter(new Vector3()).toArray(), delta: line.delta(new Vector3()).toArray(),
            at: line.at(.37, new Vector3()).toArray(), distance: line.distance(), distanceSq: line.distanceSq(),
            parameter: line.closestPointToPointParameter(point, false), clampedParameter: line.closestPointToPointParameter(point, true),
            clamped: line.closestPointToPoint(point, true, new Vector3()).toArray(),
            unclamped: line.closestPointToPoint(point, false, new Vector3()).toArray(),
            segmentDistanceSq, defaultSegmentDistanceSq: line.distanceSqToLine3(other), c1: c1.toArray(), c2: c2.toArray(),
            transformed: vl(line.clone().applyMatrix4(m(a.matrix))), cloneEquals: line.clone().equals(line) };
    }
    case 'triangle': {
        const triangle = t(a.triangle), point = v(a.point), bary = new Vector3(9, 8, 7);
        const baryHit = triangle.getBarycoord(point, bary);
        const interpolation2 = new Vector2(9, 8), interpolation3 = new Vector3(9, 8, 7), interpolation4 = new Vector4(9, 8, 7, 6);
        const i2 = triangle.getInterpolation(point, new Vector2(1, 2), new Vector2(3, -4), new Vector2(5, 6), interpolation2);
        const i3 = triangle.getInterpolation(point, v([1, 2, 3]), v([3, -4, 5]), v([5, 6, 7]), interpolation3);
        const i4 = triangle.getInterpolation(point, new Vector4(1, 2, 3, 4), new Vector4(3, -4, 5, -6), new Vector4(5, 6, 7, 8), interpolation4);
        return { area: triangle.getArea(), normal: triangle.getNormal(new Vector3()).toArray(),
            midpoint: triangle.getMidpoint(new Vector3()).toArray(), plane: triangle.getPlane(new Plane()).toJSON(),
            bary: nullable(baryHit, bary), closest: triangle.closestPointToPoint(point, new Vector3()).toArray(),
            contains: triangle.containsPoint(point), front: triangle.isFrontFacing(v([0, 0, -1])),
            back: triangle.isFrontFacing(v([0, 0, 1])), boxHit: triangle.intersectsBox(b(a.box)),
            interpolation2: nullable(i2, interpolation2), interpolation3: nullable(i3, interpolation3),
            interpolation4: nullable(i4, interpolation4), cloneEquals: triangle.clone().equals(triangle) };
    }
    case 'attribute': {
        const attr = new BufferAttribute(new Uint8Array(a.values), a.itemSize, a.normalized), bary = v(a.bary);
        return Object.fromEntries([[2, Vector2], [3, Vector3], [4, Vector4]].map(([dim, V]) =>
            [`v${dim}`, Triangle.getInterpolatedAttribute(attr, ...a.indices, bary, new V()).toArray()]));
    }
    case 'attribute3Tail': {
        const attr = new Float32BufferAttribute([1, 2, 3, 4, 5, 6, 7, 8, 9], 3), bary = v([.2, .3, .5]);
        return { v3: Triangle.getInterpolatedAttribute(attr, 0, 1, 2, bary, new Vector3()).toArray(),
            v4: Triangle.getInterpolatedAttribute(attr, 0, 1, 2, bary, new Vector4()).toArray(),
            attributeTriangle: vt(new Triangle().setFromAttributeAndIndices(attr, 2, 0, 1)),
            indexedTriangle: vt(new Triangle().setFromPointsAndIndices([[1, 2, 3], [4, 5, 6], [7, 8, 9]].map(v), 2, 0, 1)) };
    }
    case 'raySphere': {
        const ray = r(a.ray), sphere = s(a.sphere), target = v([9, 8, 7]);
        return { intersection: nullable(ray.intersectSphere(sphere, target), target), intersects: ray.intersectsSphere(sphere) };
    }
    case 'rayPlane': {
        const ray = r(a.ray), plane = p(a.plane), target = v([9, 8, 7]);
        return { intersection: nullable(ray.intersectPlane(plane, target), target), intersects: ray.intersectsPlane(plane),
            distance: ray.distanceToPlane(plane) };
    }
    case 'rayBox': {
        const ray = r(a.ray), box = b(a.box), target = v([9, 8, 7]);
        return { intersection: nullable(ray.intersectBox(box, target), target), intersects: ray.intersectsBox(box) };
    }
    case 'rayTriangle': {
        const ray = r(a.ray), triangle = t(a.triangle), target = v([9, 8, 7]);
        return nullable(ray.intersectTriangle(triangle.a, triangle.b, triangle.c, a.cull, target), target);
    }
    case 'rayPoint': {
        const ray = r(a.ray), point = v(a.point);
        return { closest: ray.closestPointToPoint(point, new Vector3()).toArray(), distance: ray.distanceToPoint(point),
            distanceSq: ray.distanceSqToPoint(point) };
    }
    case 'raySegment': {
        const ray = r(a.ray), line = l(a.line), rayPoint = new Vector3(), segmentPoint = new Vector3();
        return { distanceSq: ray.distanceSqToSegment(line.start, line.end, rayPoint, segmentPoint),
            rayPoint: rayPoint.toArray(), segmentPoint: segmentPoint.toArray(),
            defaultDistanceSq: ray.distanceSqToSegment(line.start, line.end) };
    }
    case 'rayMutators': {
        const ray = r(a.ray);
        return { at: ray.at(.37, new Vector3()).toArray(), recast: vr(ray.clone().recast(.37)),
            lookAt: vr(ray.clone().lookAt(v([4, 5, 6]))), transformed: vr(ray.clone().applyMatrix4(m(a.matrix))),
            cloneEquals: ray.clone().equals(ray) };
    }
    case 'frustum': {
        const f = new Frustum().setFromProjectionMatrix(m(a.matrix), a.coordinate, a.reverse);
        const world = new Matrix4().makeTranslation(0, 0, -2), mesh = new Mesh(new BoxGeometry(1, 2, 3));
        mesh.matrixWorld.copy(world);
        const objectHit = f.intersectsObject(mesh);
        const owned = { boundingSphere: null, matrixWorld: world, computes: 0,
            computeBoundingSphere() { this.boundingSphere = new Sphere(new Vector3(), 1); ++this.computes; } };
        const ownedHit = f.intersectsObject(owned), clone = f.clone();
        return { planes: f.planes.map(p => p.toJSON()), contained: points.map(p => f.containsPoint(p)),
            sphereHits: spheres.map(s => f.intersectsSphere(s)), boxHits: boxes.map(b => f.intersectsBox(b)),
            spriteHits: [[.5, .5], [0, 1], [5, 3]].map(center => f.intersectsSprite({ center: new Vector2(...center), matrixWorld: world })),
            objectHit, geometryComputed: mesh.geometry.boundingSphere !== null, ownedHit, ownedComputes: owned.computes,
            cloneEquals: clone.planes.every((p, i) => p.equals(f.planes[i])) };
    }
    case 'frustumEdge': {
        const f = new Frustum().setFromProjectionMatrix(new Matrix4().set(...Array(16).fill(0)));
        const zeroPlanes = f.planes.map(p => p.toJSON());
        let invalidThrows = false; try { f.setFromProjectionMatrix(new Matrix4(), -1); } catch { invalidThrows = true; }
        f.setFromProjectionMatrix(new Matrix4(), -1, true);
        return { zeroPlanes, invalidThrows, invalidReversedContained: f.containsPoint(new Vector3()) };
    }
    case 'aliases': {
        const line = new Line3().set(v([1, 2, 3]), v([5, 6, 7])); line.at(.37, line.start);
        const ray = new Ray().set(v([1, 2, 3]), v([0, 0, -1])); ray.at(.37, ray.direction);
        const plane = new Plane().setComponents(1, 0, 0, -2); plane.projectPoint(v([1, 2, 3]), plane.normal);
        const normalPlane = new Plane().setFromNormalAndCoplanarPoint(v([0, 2, 0]), v([3, 4, 5]));
        const triangle = new Triangle().set(v([2, 3, 4]), v([5, 6, 7]), v([8, 10, 9])); triangle.getNormal(triangle.a);
        const attr = new Float32BufferAttribute([1, 2, 3, 4, 5, 6, 7, 8, 9], 3), bary = v([.2, .3, .5]);
        Triangle.getInterpolatedAttribute(attr, 0, 1, 2, bary, bary);
        const f = new Frustum().set(new Plane(v([1, 0, 0]), 1), new Plane(v([-1, 0, 0]), 1),
            new Plane(v([0, 1, 0]), 1), new Plane(v([0, -1, 0]), 1), new Plane(v([0, 0, 1]), 1), new Plane(v([0, 0, -1]), 1));
        return { lineAtStart: vl(line), rayAtDirection: vr(ray), planeProjectNormal: plane.toJSON(),
            planeFromNormal: normalPlane.toJSON(), triangleNormalA: vt(triangle), attributeBaryTarget: bary.toArray(),
            frustumInside: f.containsPoint(v([1, 1, 1])), frustumOutside: f.containsPoint(v([1.0000001, 0, 0])) };
    }
    default: throw new Error(`Unknown probe ${kind}`);
    }
}
const run = spawnSync(resolve(binary), [], { encoding: 'utf8', maxBuffer: 32 * 1024 * 1024 });
if (run.status !== 0) throw new Error(`C++ probe failed (${run.status}): ${run.stderr}`);
const samples = JSON.parse(run.stdout);
let checked = 0;
const failures = [];
function compare(actual, reference, path) {
    ++checked;
    if (typeof actual === 'number' && typeof reference === 'number') {
        if (Math.abs(actual - reference) <= 2e-12 * Math.max(1, Math.abs(actual), Math.abs(reference))) return;
    } else if (actual === reference) return;
    else if (Array.isArray(actual) && Array.isArray(reference) && actual.length === reference.length) {
        actual.forEach((n, i) => compare(n, reference[i], `${path}[${i}]`)); return;
    } else if (actual && reference && typeof actual === 'object' && typeof reference === 'object') {
        const ak = Object.keys(actual).sort(), rk = Object.keys(reference).sort();
        if (JSON.stringify(ak) === JSON.stringify(rk)) { ak.forEach(k => compare(actual[k], reference[k], `${path}.${k}`)); return; }
    }
    failures.push(`${path}: C++ ${JSON.stringify(actual)} != JS ${JSON.stringify(reference)}`);
}
for (let i = 0; i < samples.length; ++i) {
    const sample = samples[i];
    compare(sample.result, encode(expected(sample.kind, decode(sample.input))), `[${i}] ${sample.kind}`);
}
if (failures.length) {
    console.error(failures.slice(0, 40).join('\n'));
    throw new Error(`${failures.length} mismatches among ${samples.length} samples / ${checked} comparisons`);
}
console.log(`Query parity: ${samples.length} samples, ${checked} comparisons against ${pin}; NaN/Infinity/signed zero exact, finite tolerance 2e-12.`);
