// Differential for the primitive geometries: the same parameters through the port probe and through the
// pinned upstream module, compared field for field. Attribute values travel as float32 bit patterns so the
// comparison is exact.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const sourceDir = process.argv[2];
const probePath = process.argv[3];

const {BoxGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/BoxGeometry.js')).href);
const {ShapeGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/ShapeGeometry.js')).href);
const {Shape} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/core/Shape.js')).href);
const {EdgesGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/EdgesGeometry.js')).href);
const {ExtrudeGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/ExtrudeGeometry.js')).href);
const {TubeGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/TubeGeometry.js')).href);
const {CatmullRomCurve3} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/curves/CatmullRomCurve3.js')).href);
const {Vector3} = await import(pathToFileURL(path.join(sourceDir, 'src/math/Vector3.js')).href);
const {WireframeGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/WireframeGeometry.js')).href);
const {PlaneGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/PlaneGeometry.js')).href);
const {CircleGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/CircleGeometry.js')).href);
const {RingGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/RingGeometry.js')).href);
const {ConeGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/ConeGeometry.js')).href);
const {CylinderGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/CylinderGeometry.js')).href);
const {SphereGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/SphereGeometry.js')).href);
const {TorusGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/TorusGeometry.js')).href);
const {TorusKnotGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/TorusKnotGeometry.js')).href);
const {DodecahedronGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/DodecahedronGeometry.js')).href);
const {IcosahedronGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/IcosahedronGeometry.js')).href);
const {OctahedronGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/OctahedronGeometry.js')).href);
const {TetrahedronGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/TetrahedronGeometry.js')).href);
const {LatheGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/LatheGeometry.js')).href);
const {Vector2} = await import(pathToFileURL(path.join(sourceDir, 'src/math/Vector2.js')).href);

const bitArray = (values) => Array.from(new Uint32Array(new Float32Array(values).buffer));
const bitOf = (value) => new Uint32Array(new Float32Array([value]).buffer)[0];

const describeAttributes = (geometry) => {
    const out = {};
    for (const [name, itemSize] of [['position', 3], ['normal', 3], ['uv', 2]]) {
        const attribute = geometry.attributes[name];
        if (!attribute) {
            out[name] = null;
            continue;
        }
        const array = attribute.array;
        const limit = Math.min(attribute.count, 6);
        const values = [];
        for (let i = 0; i < limit * itemSize; i += 1) values.push(array[i]);
        out[name] = {count: attribute.count, itemSize, bits: bitArray(values)};
    }
    return out;
};

const describeIndex = (geometry) => {
    if (!geometry.index) return null;
    const array = geometry.index.array;
    return {count: geometry.index.count, first: Array.from(array.slice(0, 8)).map(Number)};
};

const scenarios = [
    ['box 2 3 4 1 1 1', () => new BoxGeometry(2, 3, 4, 1, 1, 1)],
    ['box 1 1 1 2 2 2', () => new BoxGeometry(1, 1, 1, 2, 2, 2)],
    ['box 0.5 7 3 3 1 2', () => new BoxGeometry(0.5, 7, 3, 3, 1, 2)],
    ['plane 2 4 1 1', () => new PlaneGeometry(2, 4, 1, 1)],
    ['plane 2 4 2 3', () => new PlaneGeometry(2, 4, 2, 3)],
    ['plane 1.5 0.25 4 1', () => new PlaneGeometry(1.5, 0.25, 4, 1)],
    ['circle 1 8 0 6.283185307179586', () => new CircleGeometry(1, 8, 0, Math.PI * 2)],
    ['circle 2 16 0.5 3', () => new CircleGeometry(2, 16, 0.5, 3)],
    ['ring 0.5 1 8 1 0 6.283185307179586', () => new RingGeometry(0.5, 1, 8, 1, 0, Math.PI * 2)],
    ['ring 1 3 12 2 0 3', () => new RingGeometry(1, 3, 12, 2, 0, 3)],
    ['cone 1 2 8 1 0 0 6.283185307179586', () => new ConeGeometry(1, 2, 8, 1, false, 0, Math.PI * 2)],
    ['cone 0.75 3 6 2 1 0.25 4', () => new ConeGeometry(0.75, 3, 6, 2, true, 0.25, 4)],
    ['cylinder 1 1 2 8 1 0 0 6.283185307179586', () => new CylinderGeometry(1, 1, 2, 8, 1, false, 0, Math.PI * 2)],
    ['cylinder 0.5 1.5 3 12 3 1 0 4', () => new CylinderGeometry(0.5, 1.5, 3, 12, 3, true, 0, 4)],
    ['sphere 1 8 6 0 6.283185307179586 0 3.141592653589793', () => new SphereGeometry(1, 8, 6, 0, Math.PI * 2, 0, Math.PI)],
    ['sphere 2 12 8 0.5 3 0.25 2', () => new SphereGeometry(2, 12, 8, 0.5, 3, 0.25, 2)],
    ['torus 1 0.4 8 16 6.283185307179586', () => new TorusGeometry(1, 0.4, 8, 16, Math.PI * 2)],
    ['torus 2 0.5 6 24 3', () => new TorusGeometry(2, 0.5, 6, 24, 3)],
    ['torusknot 1 0.4 16 8 2 3', () => new TorusKnotGeometry(1, 0.4, 16, 8, 2, 3)],
    ['torusknot 2 0.3 20 6 3 4', () => new TorusKnotGeometry(2, 0.3, 20, 6, 3, 4)],
    ['polyhedron dodecahedron 1 0', () => new DodecahedronGeometry(1, 0)],
    ['polyhedron icosahedron 1 0', () => new IcosahedronGeometry(1, 0)],
    ['polyhedron icosahedron 1.5 1', () => new IcosahedronGeometry(1.5, 1)],
    ['polyhedron octahedron 1 1', () => new OctahedronGeometry(1, 1)],
    ['polyhedron tetrahedron 2 2', () => new TetrahedronGeometry(2, 2)],
    ['lathe 6 0 6.283185307179586 3 0 0 1 0 0 1', () => new LatheGeometry([new Vector2(0, 0), new Vector2(1, 0), new Vector2(0, 1)], 6, 0, Math.PI * 2)],
    ['lathe 8 0.5 3 4 0 1 1 1 1 0 0 0', () => new LatheGeometry([new Vector2(0, 1), new Vector2(1, 1), new Vector2(1, 0), new Vector2(0, 0)], 8, 0.5, 3)],
    ['shape 4 4 0 0 2 0 2 2 0 2', () => { const shape = new Shape(); shape.moveTo(0, 0); shape.lineTo(2, 0); shape.lineTo(2, 2); shape.lineTo(0, 2); return new ShapeGeometry(shape, 4); }],
    ['shape 12 3 0 0 1 0 0 1', () => { const shape = new Shape(); shape.moveTo(0, 0); shape.lineTo(1, 0); shape.lineTo(0, 1); return new ShapeGeometry(shape, 12); }],
    ['edges 2 2 2 1', () => new EdgesGeometry(new BoxGeometry(2, 2, 2, 1, 1, 1), 1)],
    ['edges 2 2 2 30', () => new EdgesGeometry(new BoxGeometry(2, 2, 2, 1, 1, 1), 30)],
    ['wireframe 2 2 2', () => new WireframeGeometry(new BoxGeometry(2, 2, 2, 1, 1, 1))],
    ['wireframe 1 3 5', () => new WireframeGeometry(new BoxGeometry(1, 3, 5, 1, 1, 1))],
    ['extrude 2 0 1 12 4 0 0 2 0 2 2 0 2', () => { const s = new Shape(); s.moveTo(0, 0); s.lineTo(2, 0); s.lineTo(2, 2); s.lineTo(0, 2); return new ExtrudeGeometry(s, {depth: 2, bevelEnabled: false, steps: 1, curveSegments: 12}); }],
    ['extrude 1.5 1 3 12 3 0 0 1 0 0 1', () => { const s = new Shape(); s.moveTo(0, 0); s.lineTo(1, 0); s.lineTo(0, 1); return new ExtrudeGeometry(s, {depth: 1.5, bevelEnabled: true, steps: 3, curveSegments: 12}); }],
    ['extrude 3 0 2 6 4 0 0 2 0 2 2 0 2', () => { const s = new Shape(); s.moveTo(0, 0); s.lineTo(2, 0); s.lineTo(2, 2); s.lineTo(0, 2); return new ExtrudeGeometry(s, {depth: 3, bevelEnabled: false, steps: 2, curveSegments: 6}); }],
    ['tube 8 0.2 6 0 3 0 0 0 1 0 0 1 0 1', () => new TubeGeometry(new CatmullRomCurve3([new Vector3(0, 0, 0), new Vector3(1, 0, 0), new Vector3(1, 0, 1)], false), 8, 0.2, 6, false)],
    ['tube 12 0.5 8 1 4 0 0 0 1 1 1 2 0 1 2 2 2', () => new TubeGeometry(new CatmullRomCurve3([new Vector3(0, 0, 0), new Vector3(1, 1, 1), new Vector3(2, 0, 1), new Vector3(2, 2, 2)], true), 12, 0.5, 8, true)],
    ['tube 16 0.3 5 0 3 0 0 0 0 1 0 1 1 0', () => new TubeGeometry(new CatmullRomCurve3([new Vector3(0, 0, 0), new Vector3(0, 1, 0), new Vector3(1, 1, 0)], false), 16, 0.3, 5, false)],
];

const raw = execFileSync(probePath, [], {input: scenarios.map(([line]) => line).join('\n'), encoding: 'utf8', maxBuffer: 64 * 1024 * 1024});
const lines = raw.trim().split('\n').map((line) => JSON.parse(line));
assert.equal(lines.length, scenarios.length);

for (let i = 0; i < scenarios.length; i += 1) {
    const [line, build] = scenarios[i];
    const geometry = build();
    const expected = {
        kind: line.split(' ')[0],
        ...describeAttributes(geometry),
        index: describeIndex(geometry),
        groups: geometry.groups.length,
    };
    assert.deepEqual(lines[i], expected, `scenario ${line}`);
}

console.log(`primitive geometry parity: ${scenarios.length} scenarios`);

// The arc length functions behind the tube framing, checked directly: cached segment lengths, the parameter
// mapping, the arc length point, and the parameter and arc length tangents.
const curveScenarios = [
    ['curve 0 3 0 0 0 1 0 0 1 0 1', () => new CatmullRomCurve3([new Vector3(0, 0, 0), new Vector3(1, 0, 0), new Vector3(1, 0, 1)], false)],
    ['curve 1 4 0 0 0 1 1 0 2 0 1 2 2 2', () => new CatmullRomCurve3([new Vector3(0, 0, 0), new Vector3(1, 1, 0), new Vector3(2, 0, 1), new Vector3(2, 2, 2)], true)],
];

const curveRaw = execFileSync(probePath, [], {input: curveScenarios.map(([line]) => line).join('\n'), encoding: 'utf8', maxBuffer: 64 * 1024 * 1024});
const curveLines = curveRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < curveScenarios.length; i += 1) {
    const [line, build] = curveScenarios[i];
    const curve = build();
    const samples = [0, 0.25, 0.5, 0.75, 1];
    const expected = {
        kind: 'curve',
        points: samples.map((t) => { const p = curve.getPoint(t); return [bitOf(p.x), bitOf(p.y), bitOf(p.z)]; }),
        tangents: samples.map((t) => { const g = curve.getTangent(t); return [bitOf(g.x), bitOf(g.y), bitOf(g.z)]; }),
        lengths: Array.from(curve.getLengths()).map(bitOf),
        utoT: samples.map((t) => bitOf(curve.getUtoTmapping(t))),
        pointAt: samples.map((t) => { const p = curve.getPointAt(t); return [bitOf(p.x), bitOf(p.y), bitOf(p.z)]; }),
    };
    assert.deepEqual(curveLines[i], expected, `scenario ${line}`);
}
console.log(`curve parity: ${curveScenarios.length} scenarios`);
// The plain 3D curves: the parameter point at five samples, again as bits.
const {LineCurve3} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/curves/LineCurve3.js')).href);
const {QuadraticBezierCurve3} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/curves/QuadraticBezierCurve3.js')).href);
const {CubicBezierCurve3} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/curves/CubicBezierCurve3.js')).href);

const simpleCurveScenarios = [
    ['line3 0 0 0 1 2 3', () => new LineCurve3(new Vector3(0, 0, 0), new Vector3(1, 2, 3))],
    ['quad3 0 0 0 1 2 0 3 0 1', () => new QuadraticBezierCurve3(new Vector3(0, 0, 0), new Vector3(1, 2, 0), new Vector3(3, 0, 1))],
    ['cubic3 0 0 0 1 2 0 3 -2 0 4 0 1', () => new CubicBezierCurve3(new Vector3(0, 0, 0), new Vector3(1, 2, 0), new Vector3(3, -2, 0), new Vector3(4, 0, 1))],
];

const simpleRaw = execFileSync(probePath, [], {input: simpleCurveScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const simpleLines = simpleRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < simpleCurveScenarios.length; i += 1) {
    const [line, build] = simpleCurveScenarios[i];
    const curve = build();
    const expected = {
        kind: line.split(' ')[0],
        points: [0, 0.25, 0.5, 0.75, 1].map((t) => { const p = curve.getPoint(t); return [bitOf(p.x), bitOf(p.y), bitOf(p.z)]; }),
    };
    assert.deepEqual(simpleLines[i], expected, `scenario ${line}`);
}
console.log(`simple curve parity: ${simpleCurveScenarios.length} scenarios`);
// The 2D curves: the parameter point at five samples, as two bits each.
const {EllipseCurve} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/curves/EllipseCurve.js')).href);
const {ArcCurve} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/curves/ArcCurve.js')).href);
const {LineCurve} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/curves/LineCurve.js')).href);
const {QuadraticBezierCurve} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/curves/QuadraticBezierCurve.js')).href);
const {CubicBezierCurve} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/curves/CubicBezierCurve.js')).href);
const {SplineCurve} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/curves/SplineCurve.js')).href);

const flatCurveScenarios = [
    ['ellipse 0 0 2 1 0 6.283185307179586 0 0', () => new EllipseCurve(0, 0, 2, 1, 0, Math.PI * 2, false, 0)],
    ['ellipse 1 1 3 2 0.5 4 1 0.25', () => new EllipseCurve(1, 1, 3, 2, 0.5, 4, true, 0.25)],
    ['arc 0 0 1 0 3.141592653589793 0', () => new ArcCurve(0, 0, 1, 0, Math.PI, false)],
    ['arc 1 2 2 0 4.71238898038469 1', () => new ArcCurve(1, 2, 2, 0, Math.PI * 1.5, true)],
    ['line2 0 0 3 4', () => new LineCurve(new Vector2(0, 0), new Vector2(3, 4))],
    ['quad2 0 0 1 2 3 0', () => new QuadraticBezierCurve(new Vector2(0, 0), new Vector2(1, 2), new Vector2(3, 0))],
    ['cubic2 0 0 1 2 3 -2 4 0', () => new CubicBezierCurve(new Vector2(0, 0), new Vector2(1, 2), new Vector2(3, -2), new Vector2(4, 0))],
    ['spline2 3 0 0 1 2 2 0', () => new SplineCurve([new Vector2(0, 0), new Vector2(1, 2), new Vector2(2, 0)])],
    ['spline2 5 0 0 1 3 2 -1 3 0 4 2', () => new SplineCurve([new Vector2(0, 0), new Vector2(1, 3), new Vector2(2, -1), new Vector2(3, 0), new Vector2(4, 2)])],
];

const flatRaw = execFileSync(probePath, [], {input: flatCurveScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const flatLines = flatRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < flatCurveScenarios.length; i += 1) {
    const [line, build] = flatCurveScenarios[i];
    const curve = build();
    const expected = {
        kind: line.split(' ')[0],
        points: [0, 0.25, 0.5, 0.75, 1].map((t) => { const p = curve.getPoint(t); return [bitOf(p.x), bitOf(p.y)]; }),
    };
    assert.deepEqual(flatLines[i], expected, `scenario ${line}`);
}
console.log(`flat curve parity: ${flatCurveScenarios.length} scenarios`);
// The interpolants behind the keyframe tracks: the result at five parameters, as bits.
const {LinearInterpolant} = await import(pathToFileURL(path.join(sourceDir, 'src/math/interpolants/LinearInterpolant.js')).href);
const {DiscreteInterpolant} = await import(pathToFileURL(path.join(sourceDir, 'src/math/interpolants/DiscreteInterpolant.js')).href);
const {CubicInterpolant} = await import(pathToFileURL(path.join(sourceDir, 'src/math/interpolants/CubicInterpolant.js')).href);
const {QuaternionLinearInterpolant} = await import(pathToFileURL(path.join(sourceDir, 'src/math/interpolants/QuaternionLinearInterpolant.js')).href);

const interpolantScenarios = [
    ['interp linear 1 3 0 1 2 3 0 10 20', () => new LinearInterpolant([0, 1, 2], [0, 10, 20], 1)],
    ['interp linear 2 4 0 1 2 3 8 0 0 1 1 2 2 3 3', () => new LinearInterpolant([0, 1, 2, 3], [0, 0, 1, 1, 2, 2, 3, 3], 2)],
    ['interp discrete 2 3 0 1 2 6 0 0 1 1 2 2', () => new DiscreteInterpolant([0, 1, 2], [0, 0, 1, 1, 2, 2], 2)],
    ['interp cubic 1 2 0 1 6 0 10 1 20 0 1', () => new CubicInterpolant([0, 1], [0, 10, 1, 20, 0, 1], 1)],
    ['interp cubic 2 2 0 1 12 0 0 1 1 2 0 0 3 1 4 0 0', () => new CubicInterpolant([0, 1], [0, 0, 1, 1, 2, 0, 0, 3, 1, 4, 0, 0], 2)],
    ['interp quaternionLinear 4 2 0 1 8 0 0 0 1 0 0 0.7071067811865476 0.7071067811865476', () => new QuaternionLinearInterpolant([0, 1], [0, 0, 0, 1, 0, 0, 0.7071067811865476, 0.7071067811865476], 4)],
    ['interp quaternionLinear 4 3 0 1 2 12 0 0 0 1 0 0 0 1 0.7071 0 0.7071 0', () => new QuaternionLinearInterpolant([0, 1, 2], [0, 0, 0, 1, 0, 0, 0, 1, 0.7071, 0, 0.7071, 0], 4)],
];

const interpolantRaw = execFileSync(probePath, [], {input: interpolantScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const interpolantLines = interpolantRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < interpolantScenarios.length; i += 1) {
    const [line, build] = interpolantScenarios[i];
    const interpolant = build();
    const expected = {
        kind: 'interp',
        which: line.split(' ')[1],
        results: [0, 0.25, 0.5, 0.75, 1].map((t) => Array.from(interpolant.evaluate(t)).map(bitOf)),
    };
    assert.deepEqual(interpolantLines[i], expected, `scenario ${line}`);
}
console.log(`interpolant parity: ${interpolantScenarios.length} scenarios`);
// The keyframe tracks: the value size the track reports and the interpolated result at five parameters.
const {NumberKeyframeTrack} = await import(pathToFileURL(path.join(sourceDir, 'src/animation/tracks/NumberKeyframeTrack.js')).href);
const {ColorKeyframeTrack} = await import(pathToFileURL(path.join(sourceDir, 'src/animation/tracks/ColorKeyframeTrack.js')).href);
const {VectorKeyframeTrack} = await import(pathToFileURL(path.join(sourceDir, 'src/animation/tracks/VectorKeyframeTrack.js')).href);
const {QuaternionKeyframeTrack} = await import(pathToFileURL(path.join(sourceDir, 'src/animation/tracks/QuaternionKeyframeTrack.js')).href);
const {BooleanKeyframeTrack} = await import(pathToFileURL(path.join(sourceDir, 'src/animation/tracks/BooleanKeyframeTrack.js')).href);
const {StringKeyframeTrack} = await import(pathToFileURL(path.join(sourceDir, 'src/animation/tracks/StringKeyframeTrack.js')).href);

const trackScenarios = [
    ['track number 3 0 1 2 3 0 10 20', () => new NumberKeyframeTrack('.v', [0, 1, 2], [0, 10, 20])],
    ['track color 3 0 1 2 9 1 0 0 0 1 0 0 0 1', () => new ColorKeyframeTrack('.c', [0, 1, 2], [1, 0, 0, 0, 1, 0, 0, 0, 1])],
    ['track vector 3 0 1 2 6 0 0 0 1 1 1', () => new VectorKeyframeTrack('.p', [0, 1, 2], [0, 0, 0, 1, 1, 1])],
    ['track quaternion 2 0 1 8 0 0 0 1 0 0 0.7071 0.7071', () => new QuaternionKeyframeTrack('.q', [0, 1], [0, 0, 0, 1, 0, 0, 0.7071, 0.7071])],
    ['track boolean 3 0 1 2 3 1 0 1', () => new BooleanKeyframeTrack('.b', [0, 1, 2], [1, 0, 1])],
];

const trackRaw = execFileSync(probePath, [], {input: trackScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const trackLines = trackRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < trackScenarios.length; i += 1) {
    const [line, build] = trackScenarios[i];
    const track = build();
    const interpolant = track.createInterpolant();
    const results = [0, 0.25, 0.5, 0.75, 1].map((t) => {
        interpolant.evaluate(t);
        return Array.from(interpolant.resultBuffer).map(bitOf);
    });
    const expected = {kind: 'track', which: line.split(' ')[1], valueSize: track.getValueSize(), results};
    assert.deepEqual(trackLines[i], expected, `scenario ${line}`);
}
console.log(`track parity: ${trackScenarios.length} scenarios`);
// The string track carries words rather than numbers, so it is compared separately.
const textScenarios = [
    ['text 3 0 1 2 a b c', () => new StringKeyframeTrack('.s', [0, 1, 2], ['a', 'b', 'c'])],
    ['text 4 0 1 2 3 idle walk run jump', () => new StringKeyframeTrack('.s', [0, 1, 2, 3], ['idle', 'walk', 'run', 'jump'])],
];
const textRaw = execFileSync(probePath, [], {input: textScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const textLines = textRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < textScenarios.length; i += 1) {
    const [line, build] = textScenarios[i];
    const track = build();
    const interpolant = track.createInterpolant();
    const results = [0, 0.25, 0.5, 0.75, 1].map((t) => { interpolant.evaluate(t); return interpolant.resultBuffer[0]; });
    const expected = {kind: 'text', valueSize: track.getValueSize(), results};
    assert.deepEqual(textLines[i], expected, `scenario ${line}`);
}
console.log(`text parity: ${textScenarios.length} scenarios`);
// The lights: their type, colour and intensity, plus the scalars each kind adds.
const {AmbientLight} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/AmbientLight.js')).href);
const {DirectionalLight} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/DirectionalLight.js')).href);
const {PointLight} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/PointLight.js')).href);
const {SpotLight} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/SpotLight.js')).href);
const {HemisphereLight} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/HemisphereLight.js')).href);
const {RectAreaLight} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/RectAreaLight.js')).href);
const {LightProbe} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/LightProbe.js')).href);
const {Color} = await import(pathToFileURL(path.join(sourceDir, 'src/math/Color.js')).href);

const lightScenarios = [
    ['light ambient 1 0.5 0.25 3', () => new AmbientLight(new Color(1, 0.5, 0.25), 3)],
    ['light directional 0.5 0.25 1 2.5', () => new DirectionalLight(new Color(0.5, 0.25, 1), 2.5)],
    ['light point 1 1 1 2 10 1', () => new PointLight(new Color(1, 1, 1), 2, 10, 1)],
    ['light spot 1 0.5 0 3 10 0.5 0.25 1.5', () => new SpotLight(new Color(1, 0.5, 0), 3, 10, 0.5, 0.25, 1.5)],
    ['light hemisphere 1 1 1 0.2 0.2 0.4 1.5', () => new HemisphereLight(new Color(1, 1, 1), new Color(0.2, 0.2, 0.4), 1.5)],
    ['light rectarea 1 1 1 5 4 3', () => new RectAreaLight(new Color(1, 1, 1), 5, 4, 3)],
    ['light probe 2', () => new LightProbe(undefined, 2)],
];

const lightRaw = execFileSync(probePath, [], {input: lightScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const lightLines = lightRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < lightScenarios.length; i += 1) {
    const [line, build] = lightScenarios[i];
    const light = build();
    const expected = {kind: 'light', which: line.split(' ')[1], type: light.type,
        color: [bitOf(light.color.r), bitOf(light.color.g), bitOf(light.color.b)],
        intensity: bitOf(light.intensity)};
    if ('distance' in light) { expected.distance = bitOf(light.distance); expected.decay = bitOf(light.decay); }
    if ('angle' in light) { expected.angle = bitOf(light.angle); expected.penumbra = bitOf(light.penumbra); }
    if ('width' in light) { expected.width = bitOf(light.width); expected.height = bitOf(light.height); }
    assert.deepEqual(lightLines[i], expected, `scenario ${line}`);
}
console.log(`light parity: ${lightScenarios.length} scenarios`);
// The materials, compared on the defaults they all share.
const materialModules = {
    linebasic: ['LineBasicMaterial', 'src/materials/LineBasicMaterial.js'],
    linedashed: ['LineDashedMaterial', 'src/materials/LineDashedMaterial.js'],
    points: ['PointsMaterial', 'src/materials/PointsMaterial.js'],
    sprite: ['SpriteMaterial', 'src/materials/SpriteMaterial.js'],
    shadow: ['ShadowMaterial', 'src/materials/ShadowMaterial.js'],
    depth: ['MeshDepthMaterial', 'src/materials/MeshDepthMaterial.js'],
    distance: ['MeshDistanceMaterial', 'src/materials/MeshDistanceMaterial.js'],
    normal: ['MeshNormalMaterial', 'src/materials/MeshNormalMaterial.js'],
    lambert: ['MeshLambertMaterial', 'src/materials/MeshLambertMaterial.js'],
    phong: ['MeshPhongMaterial', 'src/materials/MeshPhongMaterial.js'],
    toon: ['MeshToonMaterial', 'src/materials/MeshToonMaterial.js'],
    matcap: ['MeshMatcapMaterial', 'src/materials/MeshMatcapMaterial.js'],
    physical: ['MeshPhysicalMaterial', 'src/materials/MeshPhysicalMaterial.js'],
};
const materialClasses = {};
for (const [which, [name, modulePath]] of Object.entries(materialModules)) {
    const module = await import(pathToFileURL(path.join(sourceDir, modulePath)).href);
    materialClasses[which] = module[name];
}
const materialScenarios = Object.keys(materialModules).map((which) => [`mat ${which}`, which]);

const materialRaw = execFileSync(probePath, [], {input: materialScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const materialLines = materialRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < materialScenarios.length; i += 1) {
    const [line, which] = materialScenarios[i];
    const material = new materialClasses[which]();
    const expected = {kind: 'mat', which, type: material.type, fields: {
        opacity: bitOf(material.opacity),
        transparent: material.transparent,
        visible: material.visible,
        side: material.side,
        blending: material.blending,
        depthTest: material.depthTest,
        depthWrite: material.depthWrite,
        alphaTest: bitOf(material.alphaTest),
        premultipliedAlpha: material.premultipliedAlpha,
        dithering: material.dithering,
        toneMapped: material.toneMapped,
        vertexColors: material.vertexColors,
    }};
    assert.deepEqual(materialLines[i], expected, `scenario ${line}`);
}
console.log(`material parity: ${materialScenarios.length} scenarios`);
// The line based helpers: the helper describes itself, so its own geometry is compared.
const {AxesHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/AxesHelper.js')).href);
const {GridHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/GridHelper.js')).href);

const helperScenarios = [
    ['helper axes 1', () => new AxesHelper(1)],
    ['helper axes 2.5', () => new AxesHelper(2.5)],
    ['helper grid 10 10', () => new GridHelper(10, 10)],
    ['helper grid 6 3', () => new GridHelper(6, 3)],
];
const helperRaw = execFileSync(probePath, [], {input: helperScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const helperLines = helperRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < helperScenarios.length; i += 1) {
    const [line, build] = helperScenarios[i];
    const helper = build();
    const position = helper.geometry.attributes.position;
    const limit = Math.min(position.count * 3, 6);
    const points = [];
    for (let k = 0; k < limit; k++) points.push(bitOf(position.array[k]));
    const expected = {kind: 'helper', which: line.split(' ')[1], type: helper.type,
        selfCount: position.count, selfPoints: points, children: []};
    assert.deepEqual(helperLines[i], expected, `scenario ${line}`);
}
console.log(`helper parity: ${helperScenarios.length} scenarios`);
// The helpers that build child objects instead of being a line themselves.
const {ArrowHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/ArrowHelper.js')).href);
const {PointLightHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/PointLightHelper.js')).href);
const {DirectionalLightHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/DirectionalLightHelper.js')).href);
const {PointLight: PointLightForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/PointLight.js')).href);
const {DirectionalLight: DirectionalLightForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/DirectionalLight.js')).href);

const childHelperScenarios = [
    ['helper pointlight 1', () => new PointLightHelper(new PointLightForHelper(), 1)],
    ['helper directional 1', () => new DirectionalLightHelper(new DirectionalLightForHelper(), 1)],
    ['helper arrow 0 0 1 0 0 0 2', () => new ArrowHelper(new Vector3(0, 0, 1), new Vector3(0, 0, 0), 2)],
];

// The probe reports a child as its type, plus the vertex count and first points when the child is a line.
const describeChild = (child) => {
    const described = {type: child.type};
    if (child.isLine && child.geometry) {
        const position = child.geometry.attributes.position;
        described.count = position.count;
        const limit = Math.min(position.count * 3, 6);
        const points = [];
        for (let k = 0; k < limit; k++) points.push(bitOf(position.array[k]));
        described.points = points;
    }
    return described;
};
const describeHelper = (helper) => {
    const described = {kind: 'helper', which: null, type: helper.type};
    if (helper.isLine && helper.geometry) {
        const position = helper.geometry.attributes.position;
        described.selfCount = position.count;
        const limit = Math.min(position.count * 3, 6);
        const points = [];
        for (let k = 0; k < limit; k++) points.push(bitOf(position.array[k]));
        described.selfPoints = points;
    }
    described.children = helper.children.map(describeChild);
    return described;
};

const childRaw = execFileSync(probePath, [], {input: childHelperScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const childLines = childRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < childHelperScenarios.length; i += 1) {
    const [line, build] = childHelperScenarios[i];
    const expected = describeHelper(build());
    expected.which = line.split(' ')[1];
    assert.deepEqual(childLines[i], expected, `scenario ${line}`);
}
console.log(`child helper parity: ${childHelperScenarios.length} scenarios`);
// The scene graph defaults and the two render targets.
const {PerspectiveCamera} = await import(pathToFileURL(path.join(sourceDir, 'src/cameras/PerspectiveCamera.js')).href);
const {OrthographicCamera} = await import(pathToFileURL(path.join(sourceDir, 'src/cameras/OrthographicCamera.js')).href);
const {Scene} = await import(pathToFileURL(path.join(sourceDir, 'src/scenes/Scene.js')).href);
const {Group} = await import(pathToFileURL(path.join(sourceDir, 'src/objects/Group.js')).href);
const {Mesh} = await import(pathToFileURL(path.join(sourceDir, 'src/objects/Mesh.js')).href);
const {Points} = await import(pathToFileURL(path.join(sourceDir, 'src/objects/Points.js')).href);
const {Sprite} = await import(pathToFileURL(path.join(sourceDir, 'src/objects/Sprite.js')).href);
const {LOD} = await import(pathToFileURL(path.join(sourceDir, 'src/objects/LOD.js')).href);
const {RenderTarget} = await import(pathToFileURL(path.join(sourceDir, 'src/core/RenderTarget.js')).href);
const {RenderTarget3D} = await import(pathToFileURL(path.join(sourceDir, 'src/core/RenderTarget3D.js')).href);

const nodeScenarios = [
    ['node perspective 50 1 0.1 2000', () => new PerspectiveCamera(50, 1, 0.1, 2000)],
    ['node perspective 35 1.5 0.5 100', () => new PerspectiveCamera(35, 1.5, 0.5, 100)],
    ['node ortho -2 2 1 -1 0.1 50', () => new OrthographicCamera(-2, 2, 1, -1, 0.1, 50)],
    ['node scene', () => new Scene()],
    ['node group', () => new Group()],
    ['node mesh', () => new Mesh()],
    ['node points', () => new Points()],
    ['node sprite', () => new Sprite()],
    ['node lod', () => new LOD()],
    ['node rendertarget 16 8', () => new RenderTarget(16, 8)],
    ['node rendertarget3d 4 4 2', () => new RenderTarget3D(4, 4, 2)],
];

const nodeRaw = execFileSync(probePath, [], {input: nodeScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const nodeLines = nodeRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < nodeScenarios.length; i += 1) {
    const [line, build] = nodeScenarios[i];
    const node = build();
    const which = line.split(' ')[1];
    let expected;
    if (which.startsWith('rendertarget')) {
        expected = {kind: 'node', which, width: bitOf(node.width), height: bitOf(node.height)};
        if (which === 'rendertarget3d') expected.depth = bitOf(node.depth);
    } else {
        expected = {kind: 'node', which, type: node.type, fields: {
            visible: node.visible,
            castShadow: node.castShadow,
            receiveShadow: node.receiveShadow,
            frustumCulled: node.frustumCulled,
            renderOrder: bitOf(node.renderOrder),
            matrixAutoUpdate: node.matrixAutoUpdate,
            matrixWorldAutoUpdate: node.matrixWorldAutoUpdate,
            layersMask: node.layers.mask,
        }};
        if (node.isPerspectiveCamera) {
            expected.fields.fov = bitOf(node.fov);
            expected.fields.aspect = bitOf(node.aspect);
            expected.fields.near = bitOf(node.near);
            expected.fields.far = bitOf(node.far);
        }
        if (node.isOrthographicCamera) {
            for (const key of ['left', 'right', 'top', 'bottom', 'near', 'far']) expected.fields[key] = bitOf(node[key]);
        }
    }
    assert.deepEqual(nodeLines[i], expected, `scenario ${line}`);
}
console.log(`node parity: ${nodeScenarios.length} scenarios`);
// The texture defaults and the two data backed textures.
const {Texture} = await import(pathToFileURL(path.join(sourceDir, 'src/textures/Texture.js')).href);
const {DataTexture} = await import(pathToFileURL(path.join(sourceDir, 'src/textures/DataTexture.js')).href);
const {DepthTexture} = await import(pathToFileURL(path.join(sourceDir, 'src/textures/DepthTexture.js')).href);

const textureScenarios = [
    ['texture base', () => new Texture()],
    ['texture data 4 2', () => new DataTexture(new Uint8Array(4 * 2 * 4), 4, 2)],
    ['texture depth 4 2', () => new DepthTexture(4, 2)],
];

const textureRaw = execFileSync(probePath, [], {input: textureScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const textureLines = textureRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < textureScenarios.length; i += 1) {
    const [line, build] = textureScenarios[i];
    const texture = build();
    const expected = {kind: 'texture', which: line.split(' ')[1], fields: {
        mapping: bitOf(texture.mapping),
        wrapS: bitOf(texture.wrapS),
        wrapT: bitOf(texture.wrapT),
        magFilter: bitOf(texture.magFilter),
        minFilter: bitOf(texture.minFilter),
        format: bitOf(texture.format),
        type: bitOf(texture.type),
        anisotropy: bitOf(texture.anisotropy),
        flipY: texture.flipY,
        generateMipmaps: texture.generateMipmaps,
        premultiplyAlpha: texture.premultiplyAlpha,
        unpackAlignment: bitOf(texture.unpackAlignment),
        colorSpace: texture.colorSpace,
    }};
    assert.deepEqual(textureLines[i], expected, `scenario ${line}`);
}
console.log(`texture parity: ${textureScenarios.length} scenarios`);
// The fog forms and the spherical harmonics coefficients.
const {Fog} = await import(pathToFileURL(path.join(sourceDir, 'src/scenes/Fog.js')).href);
const {FogExp2} = await import(pathToFileURL(path.join(sourceDir, 'src/scenes/FogExp2.js')).href);
const {SphericalHarmonics3} = await import(pathToFileURL(path.join(sourceDir, 'src/math/SphericalHarmonics3.js')).href);

const fogScenarios = [
    ['fog linear 0.5 0.25 1 1 100', () => new Fog(new Color(0.5, 0.25, 1), 1, 100)],
    ['fog linear 1 1 1 2 50', () => new Fog(new Color(1, 1, 1), 2, 50)],
    ['fog exp2 0.5 0.25 1 0.02', () => new FogExp2(new Color(0.5, 0.25, 1), 0.02)],
    ['fog exp2 0 0 0 1.5', () => new FogExp2(new Color(0, 0, 0), 1.5)],
];
const fogRaw = execFileSync(probePath, [], {input: fogScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const fogLines = fogRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < fogScenarios.length; i += 1) {
    const [line, build] = fogScenarios[i];
    const fog = build();
    const which = line.split(' ')[1];
    const expected = {kind: 'fog', which, name: fog.name,
        color: [bitOf(fog.color.r), bitOf(fog.color.g), bitOf(fog.color.b)]};
    if (which === 'linear') { expected.near = bitOf(fog.near); expected.far = bitOf(fog.far); }
    else expected.density = bitOf(fog.density);
    assert.deepEqual(fogLines[i], expected, `scenario ${line}`);
}
console.log(`fog parity: ${fogScenarios.length} scenarios`);

// The spherical harmonics coefficients, compared as the nine triples they hold.

const shScenarios = [
    ['sh 9 0.1 0.2 0.3 1 2 3 4 5 6', [0.1, 0.2, 0.3, 1, 2, 3, 4, 5, 6]],
    ['sh 27 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27', Array.from({length: 27}, (_, i) => i + 1)],
    ['sh 0', []],
];
const shRaw = execFileSync(probePath, [], {input: shScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const shLines = shRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < shScenarios.length; i += 1) {
    const [line, values] = shScenarios[i];
    const sh = new SphericalHarmonics3();
    // The count in the line is the number of scalar values, three per coefficient.
    for (let k = 0; k < values.length / 3 && k < 9; k++) sh.coefficients[k].set(values[k * 3], values[k * 3 + 1], values[k * 3 + 2]);
    const expected = {kind: 'sh', coefficients: sh.coefficients.map((c) => [bitOf(c.x), bitOf(c.y), bitOf(c.z)])};
    assert.deepEqual(shLines[i], expected, `scenario ${line}`);
}
console.log(`spherical harmonics parity: ${shScenarios.length} scenarios`);

// The instanced mesh: its count and the buffer it allocates for the instance matrices.
const {InstancedMesh} = await import(pathToFileURL(path.join(sourceDir, 'src/objects/InstancedMesh.js')).href);
const {BoxGeometry: BoxForInstancing} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/BoxGeometry.js')).href);
const {MeshBasicMaterial: BasicForInstancing} = await import(pathToFileURL(path.join(sourceDir, 'src/materials/MeshBasicMaterial.js')).href);

const instanceScenarios = [
    ['instance 1', 1],
    ['instance 3', 3],
    ['instance 8', 8],
];
const instanceRaw = execFileSync(probePath, [], {input: instanceScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const instanceLines = instanceRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < instanceScenarios.length; i += 1) {
    const [line, count] = instanceScenarios[i];
    const mesh = new InstancedMesh(new BoxForInstancing(1, 1, 1, 1, 1, 1), new BasicForInstancing(), count);
    const matrix = mesh.instanceMatrix;
    const first = [];
    for (let k = 0; k < 8; k++) first.push(bitOf(matrix.array[k]));
    const expected = {kind: 'instance', type: mesh.type, count: bitOf(mesh.count),
        matrixCount: matrix.count, matrixItemSize: matrix.itemSize, matrixFirst: first,
        hasColor: mesh.instanceColor !== null, frustumCulled: mesh.frustumCulled};
    assert.deepEqual(instanceLines[i], expected, `scenario ${line}`);
}
console.log(`instanced mesh parity: ${instanceScenarios.length} scenarios`);