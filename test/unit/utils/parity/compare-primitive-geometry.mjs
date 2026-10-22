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
        const limit = Math.min(attribute.count, 24);
        const values = [];
        for (let i = 0; i < limit * itemSize; i += 1) values.push(array[i]);
        out[name] = {count: attribute.count, itemSize, bits: bitArray(values)};
    }
    return out;
};

const describeIndex = (geometry) => {
    if (!geometry.index) return null;
    const array = geometry.index.array;
    return {count: geometry.index.count, first: Array.from(array.slice(0, 32)).map(Number)};
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
    const samples = Array.from({length: 17}, (_, k) => k / 16);
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
    ['line3 -1 -1 -1 1 1 1', () => new LineCurve3(new Vector3(-1, -1, -1), new Vector3(1, 1, 1))],
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
        points: Array.from({length: 17}, (_, k) => { const p = curve.getPoint(k / 16); return [bitOf(p.x), bitOf(p.y), bitOf(p.z)]; }),
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
    ['ellipse 0 0 1 1 0 6.283185307179586 1 0', () => new EllipseCurve(0, 0, 1, 1, 0, Math.PI * 2, true, 0)],
    ['arc 0 0 1 0 3.141592653589793 0', () => new ArcCurve(0, 0, 1, 0, Math.PI, false)],
    ['arc 1 2 2 0 4.71238898038469 1', () => new ArcCurve(1, 2, 2, 0, Math.PI * 1.5, true)],
    ['arc 0 0 2 0 6.283185307179586 0', () => new ArcCurve(0, 0, 2, 0, Math.PI * 2, false)],
    ['line2 0 0 3 4', () => new LineCurve(new Vector2(0, 0), new Vector2(3, 4))],
    ['line2 -1 -1 1 1', () => new LineCurve(new Vector2(-1, -1), new Vector2(1, 1))],
    ['quad2 0 0 1 2 3 0', () => new QuadraticBezierCurve(new Vector2(0, 0), new Vector2(1, 2), new Vector2(3, 0))],
    ['quad2 0 0 2 2 4 0', () => new QuadraticBezierCurve(new Vector2(0, 0), new Vector2(2, 2), new Vector2(4, 0))],
    ['cubic2 0 0 1 2 3 -2 4 0', () => new CubicBezierCurve(new Vector2(0, 0), new Vector2(1, 2), new Vector2(3, -2), new Vector2(4, 0))],
    ['cubic2 0 0 2 3 3 -3 5 0', () => new CubicBezierCurve(new Vector2(0, 0), new Vector2(2, 3), new Vector2(3, -3), new Vector2(5, 0))],
    ['spline2 3 0 0 1 2 2 0', () => new SplineCurve([new Vector2(0, 0), new Vector2(1, 2), new Vector2(2, 0)])],
    ['spline2 3 0 0 0 1 1 0', () => new SplineCurve([new Vector2(0, 0), new Vector2(0, 1), new Vector2(1, 0)])],
    ['spline2 5 0 0 1 3 2 -1 3 0 4 2', () => new SplineCurve([new Vector2(0, 0), new Vector2(1, 3), new Vector2(2, -1), new Vector2(3, 0), new Vector2(4, 2)])],
    ['spline2 4 0 0 1 1 2 0 3 1', () => new SplineCurve([new Vector2(0, 0), new Vector2(1, 1), new Vector2(2, 0), new Vector2(3, 1)])],
];

const flatRaw = execFileSync(probePath, [], {input: flatCurveScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const flatLines = flatRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < flatCurveScenarios.length; i += 1) {
    const [line, build] = flatCurveScenarios[i];
    const curve = build();
    const expected = {
        kind: line.split(' ')[0],
        points: Array.from({length: 17}, (_, k) => { const p = curve.getPoint(k / 16); return [bitOf(p.x), bitOf(p.y)]; }),
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
    ['interp linear 1 2 0 1 2 0 5', () => new LinearInterpolant([0, 1], [0, 5], 1)],
    ['interp linear 2 4 0 1 2 3 8 0 0 1 1 2 2 3 3', () => new LinearInterpolant([0, 1, 2, 3], [0, 0, 1, 1, 2, 2, 3, 3], 2)],
    ['interp linear 3 2 0 1 6 0 0 0 1 1 1', () => new LinearInterpolant([0, 1], [0, 0, 0, 1, 1, 1], 3)],
    ['interp linear 4 2 0 1 8 0 1 2 3 4 5 6 7', () => new LinearInterpolant([0, 1], [0, 1, 2, 3, 4, 5, 6, 7], 4)],
    ['interp linear 4 3 0 1 2 12 0 1 2 3 4 5 6 7 8 9 10 11', () => new LinearInterpolant([0, 1, 2], [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11], 4)],
    ['interp linear 2 3 0 1 2 6 0 0 1 1 2 2', () => new LinearInterpolant([0, 1, 2], [0, 0, 1, 1, 2, 2], 2)],
    ['interp discrete 2 3 0 1 2 6 0 0 1 1 2 2', () => new DiscreteInterpolant([0, 1, 2], [0, 0, 1, 1, 2, 2], 2)],
    ['interp discrete 1 2 0 1 2 5 7', () => new DiscreteInterpolant([0, 1], [5, 7], 1)],
    ['interp discrete 4 2 0 1 8 0 1 2 3 4 5 6 7', () => new DiscreteInterpolant([0, 1], [0, 1, 2, 3, 4, 5, 6, 7], 4)],
    ['interp discrete 4 3 0 1 2 12 0 1 2 3 4 5 6 7 8 9 10 11', () => new DiscreteInterpolant([0, 1, 2], [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11], 4)],
    ['interp discrete 2 2 0 1 4 3 9 4 2', () => new DiscreteInterpolant([0, 1], [3, 9, 4, 2], 2)],
    ['interp cubic 1 2 0 1 6 0 10 1 20 0 1', () => new CubicInterpolant([0, 1], [0, 10, 1, 20, 0, 1], 1)],
    ['interp cubic 2 2 0 1 12 0 0 1 1 2 0 0 3 1 4 0 0', () => new CubicInterpolant([0, 1], [0, 0, 1, 1, 2, 0, 0, 3, 1, 4, 0, 0], 2)],
    ['interp cubic 1 3 0 1 2 9 0 10 1 20 0 1 30 2 0', () => new CubicInterpolant([0, 1, 2], [0, 10, 1, 20, 0, 1, 30, 2, 0], 1)],
    ['interp cubic 2 3 0 1 2 18 0 0 1 1 1 0 2 2 0 0 3 3 1 0 4 4 0 0', () => new CubicInterpolant([0, 1, 2], [0, 0, 1, 1, 1, 0, 2, 2, 0, 0, 3, 3, 1, 0, 4, 4, 0, 0], 2)],
    ['interp cubic 3 2 0 1 18 0 0 0 1 1 1 2 2 2 3 3 3 4 4 4 5 5 5', () => new CubicInterpolant([0, 1], [0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5], 3)],
    ['interp quaternionLinear 4 2 0 1 8 0 0 0 1 0 0 0.7071067811865476 0.7071067811865476', () => new QuaternionLinearInterpolant([0, 1], [0, 0, 0, 1, 0, 0, 0.7071067811865476, 0.7071067811865476], 4)],
    ['interp quaternionLinear 4 2 0 1 8 0 0 0 1 0 0 0 1', () => new QuaternionLinearInterpolant([0, 1], [0, 0, 0, 1, 0, 0, 0, 1], 4)],
    ['interp quaternionLinear 4 3 0 1 2 12 0 0 0 1 0 0 0.7071067811865476 0.7071067811865476 0.7071067811865476 0 0 0.7071067811865476', () => new QuaternionLinearInterpolant([0, 1, 2], [0, 0, 0, 1, 0, 0, 0.7071067811865476, 0.7071067811865476, 0.7071067811865476, 0, 0, 0.7071067811865476], 4)],
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
        results: Array.from({length: 17}, (_, k) => k / 16).map((t) => Array.from(interpolant.evaluate(t)).map(bitOf)),
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
    ['track number 2 0 1 2 0 5', () => new NumberKeyframeTrack('.v', [0, 1], [0, 5])],
    ['track color 3 0 1 2 9 1 0 0 0 1 0 0 0 1', () => new ColorKeyframeTrack('.c', [0, 1, 2], [1, 0, 0, 0, 1, 0, 0, 0, 1])],
    ['track color 2 0 1 6 1 0 0 0 0 1', () => new ColorKeyframeTrack('.c', [0, 1], [1, 0, 0, 0, 0, 1])],
    ['track vector 3 0 1 2 6 0 0 0 1 1 1', () => new VectorKeyframeTrack('.p', [0, 1, 2], [0, 0, 0, 1, 1, 1])],
    ['track vector 2 0 1 6 1 2 3 4 5 6', () => new VectorKeyframeTrack('.p', [0, 1], [1, 2, 3, 4, 5, 6])],
    ['track quaternion 2 0 1 8 0 0 0 1 0 0 0.7071 0.7071', () => new QuaternionKeyframeTrack('.q', [0, 1], [0, 0, 0, 1, 0, 0, 0.7071, 0.7071])],
    ['track quaternion 3 0 1 2 12 0 0 0 1 0 0 0.7071 0.7071 0.7071 0 0.7071 0', () => new QuaternionKeyframeTrack('.q', [0, 1, 2], [0, 0, 0, 1, 0, 0, 0.7071, 0.7071, 0.7071, 0, 0.7071, 0])],
    ['track boolean 3 0 1 2 3 1 0 1', () => new BooleanKeyframeTrack('.b', [0, 1, 2], [1, 0, 1])],
    ['track boolean 2 0 1 2 0 0', () => new BooleanKeyframeTrack('.b', [0, 1], [0, 0])],
];

const trackRaw = execFileSync(probePath, [], {input: trackScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const trackLines = trackRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < trackScenarios.length; i += 1) {
    const [line, build] = trackScenarios[i];
    const track = build();
    const interpolant = track.createInterpolant();
    const results = Array.from({length: 17}, (_, k) => k / 16).map((t) => {
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
    ['text 2 0 1 a b', () => new StringKeyframeTrack('.s', [0, 1], ['a', 'b'])],
    ['text 3 0 1 2 alpha beta gamma', () => new StringKeyframeTrack('.s', [0, 1, 2], ['alpha', 'beta', 'gamma'])],
];
const textRaw = execFileSync(probePath, [], {input: textScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const textLines = textRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < textScenarios.length; i += 1) {
    const [line, build] = textScenarios[i];
    const track = build();
    const interpolant = track.createInterpolant();
    const results = Array.from({length: 17}, (_, k) => k / 16).map((t) => { interpolant.evaluate(t); return interpolant.resultBuffer[0]; });
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
    ['light ambient 0 0 0 0', () => new AmbientLight(new Color(0, 0, 0), 0)],
    ['light directional 0.5 0.25 1 2.5', () => new DirectionalLight(new Color(0.5, 0.25, 1), 2.5)],
    ['light directional 0 0 0 0.5', () => new DirectionalLight(new Color(0, 0, 0), 0.5)],
    ['light point 1 1 1 2 10 1', () => new PointLight(new Color(1, 1, 1), 2, 10, 1)],
    ['light point 1 0 0 5 0 0', () => new PointLight(new Color(1, 0, 0), 5, 0, 0)],
    ['light point 0 0 1 1 2 3', () => new PointLight(new Color(0, 0, 1), 1, 2, 3)],
    ['light point 1 0.5 0 100 50 4', () => new PointLight(new Color(1, 0.5, 0), 100, 50, 4)],
    ['light spot 1 0.5 0 3 10 0.5 0.25 1.5', () => new SpotLight(new Color(1, 0.5, 0), 3, 10, 0.5, 0.25, 1.5)],
    ['light spot 0 1 0 1 5 0.75 0.5 2', () => new SpotLight(new Color(0, 1, 0), 1, 5, 0.75, 0.5, 2)],
    ['light spot 1 1 1 1 0 0 0 0', () => new SpotLight(new Color(1, 1, 1), 1, 0, 0, 0, 0)],
    ['light spot 0.5 0.5 0.5 2 4 0.25 0.75 1', () => new SpotLight(new Color(0.5, 0.5, 0.5), 2, 4, 0.25, 0.75, 1)],
    ['light spot 1 1 1 10 100 1.5 0 1', () => new SpotLight(new Color(1, 1, 1), 10, 100, 1.5, 0, 1)],
    ['light hemisphere 1 1 1 0.2 0.2 0.4 1.5', () => new HemisphereLight(new Color(1, 1, 1), new Color(0.2, 0.2, 0.4), 1.5)],
    ['light hemisphere 0 0 1 1 0 0 2', () => new HemisphereLight(new Color(0, 0, 1), new Color(1, 0, 0), 2)],
    ['light hemisphere 0.5 0.5 0.5 0.5 0.5 0.5 0', () => new HemisphereLight(new Color(0.5, 0.5, 0.5), new Color(0.5, 0.5, 0.5), 0)],
    ['light hemisphere 1 1 1 0 0 0 3', () => new HemisphereLight(new Color(1, 1, 1), new Color(0, 0, 0), 3)],
    ['light rectarea 1 1 1 5 4 3', () => new RectAreaLight(new Color(1, 1, 1), 5, 4, 3)],
    ['light rectarea 0.5 0.25 1 2 3 4', () => new RectAreaLight(new Color(0.5, 0.25, 1), 2, 3, 4)],
    ['light rectarea 1 1 1 1 0 0', () => new RectAreaLight(new Color(1, 1, 1), 1, 0, 0)],
    ['light rectarea 0 0 0 0 1 1', () => new RectAreaLight(new Color(0, 0, 0), 0, 1, 1)],
    ['light rectarea 1 1 1 50 100 200', () => new RectAreaLight(new Color(1, 1, 1), 50, 100, 200)],
    ['light probe 2', () => new LightProbe(undefined, 2)],
    ['light probe 0', () => new LightProbe(undefined, 0)],
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
    shader: ['ShaderMaterial', 'src/materials/ShaderMaterial.js'],
    rawshader: ['RawShaderMaterial', 'src/materials/RawShaderMaterial.js'],
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
const {Box3Helper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/Box3Helper.js')).href);
const {Box3} = await import(pathToFileURL(path.join(sourceDir, 'src/math/Box3.js')).href);
const {PolarGridHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/PolarGridHelper.js')).href);
const {CameraHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/CameraHelper.js')).href);
const {SkeletonHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/SkeletonHelper.js')).href);
const {BoxHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/BoxHelper.js')).href);
const {Mesh: MeshForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/objects/Mesh.js')).href);
const {BoxGeometry: BoxGeometryForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/BoxGeometry.js')).href);
const {MeshBasicMaterial: BasicForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/materials/MeshBasicMaterial.js')).href);
const {Object3D: Object3DForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/core/Object3D.js')).href);
const {PerspectiveCamera: PerspectiveCameraForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/cameras/PerspectiveCamera.js')).href);
const {PlaneHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/PlaneHelper.js')).href);
const {Plane} = await import(pathToFileURL(path.join(sourceDir, 'src/math/Plane.js')).href);

// A helper child is its type, plus the vertex count and first points when the child is a line.
const describeHelperChildren = (helper) => helper.children.map((child) => {
    const described = {type: child.type};
    if (child.isLine && child.geometry) {
        const position = child.geometry.attributes.position;
        described.count = position.count;
        const values = [];
        for (let k = 0; k < Math.min(position.count * 3, 6); k++) values.push(bitOf(position.array[k]));
        described.points = values;
    }
    return described;
});

const helperScenarios = [
    ['helper axes 1', () => new AxesHelper(1)],
    ['helper axes 2.5', () => new AxesHelper(2.5)],
    ['helper axes 0.5', () => new AxesHelper(0.5)],
    ['helper axes 0.000001', () => new AxesHelper(0.000001)],
    ['helper axes 100', () => new AxesHelper(100)],
    ['helper axes 1000', () => new AxesHelper(1000)],
    ['helper grid 10 10', () => new GridHelper(10, 10)],
    ['helper grid 6 3', () => new GridHelper(6, 3)],
    ['helper grid 2 2', () => new GridHelper(2, 2)],
    ['helper grid 1 1', () => new GridHelper(1, 1)],
    ['helper grid 100 100', () => new GridHelper(100, 100)],
    ['helper grid 10 1', () => new GridHelper(10, 1)],
    ['helper grid 5 5', () => new GridHelper(5, 5)],
    ['helper grid 4 8', () => new GridHelper(4, 8)],
    ['helper grid 7 3', () => new GridHelper(7, 3)],
    ['helper grid 3 7', () => new GridHelper(3, 7)],
    ['helper box3 -1 -1 -1 1 1 1', () => new Box3Helper(new Box3(new Vector3(-1, -1, -1), new Vector3(1, 1, 1)))],
    ['helper box3 0 0 0 0 0 0', () => new Box3Helper(new Box3(new Vector3(0, 0, 0), new Vector3(0, 0, 0)))],
    ['helper box3 0 0 0 10 10 10', () => new Box3Helper(new Box3(new Vector3(0, 0, 0), new Vector3(10, 10, 10)))],
    ['helper planehelper 0 1 0 -1 2', () => new PlaneHelper(new Plane(new Vector3(0, 1, 0), -1), 2)],
    ['helper planehelper 1 0 0 0 0.001', () => new PlaneHelper(new Plane(new Vector3(1, 0, 0), 0), 0.001)],
    ['helper polargrid 5 8 4 16', () => new PolarGridHelper(5, 8, 4, 16)],
    ['helper polargrid 1 3 2 4', () => new PolarGridHelper(1, 3, 2, 4)],
    ['helper polargrid 10 16 8 64', () => new PolarGridHelper(10, 16, 8, 64)],
    ['helper polargrid 2 6 3 12', () => new PolarGridHelper(2, 6, 3, 12)],
    ['helper polargrid 3 5 4 10', () => new PolarGridHelper(3, 5, 4, 10)],
    ['helper polargrid 7 12 6 24', () => new PolarGridHelper(7, 12, 6, 24)],
    ['helper polargrid 20 32 16 128', () => new PolarGridHelper(20, 32, 16, 128)],
    ['helper polargrid 2 4 2 8', () => new PolarGridHelper(2, 4, 2, 8)],
    ['helper camerahelper', () => new CameraHelper(new PerspectiveCameraForHelper())],
    ['helper skeletonhelper', () => new SkeletonHelper(new Object3DForHelper())],
    ['helper boxhelper', () => new BoxHelper(new MeshForHelper(new BoxGeometryForHelper(1, 1, 1, 1, 1, 1), new BasicForHelper()))],
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
        selfCount: position.count, selfPoints: points, children: describeHelperChildren(helper)};
    assert.deepEqual(helperLines[i], expected, `scenario ${line}`);
}
console.log(`helper parity: ${helperScenarios.length} scenarios`);
// The helpers that build child objects instead of being a line themselves.
const {ArrowHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/ArrowHelper.js')).href);
const {PointLightHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/PointLightHelper.js')).href);
const {DirectionalLightHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/DirectionalLightHelper.js')).href);
const {HemisphereLightHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/HemisphereLightHelper.js')).href);
const {SpotLightHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/helpers/SpotLightHelper.js')).href);
const {SpotLight: SpotLightForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/SpotLight.js')).href);
const {HemisphereLight: HemisphereLightForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/HemisphereLight.js')).href);
const {PointLight: PointLightForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/PointLight.js')).href);
const {DirectionalLight: DirectionalLightForHelper} = await import(pathToFileURL(path.join(sourceDir, 'src/lights/DirectionalLight.js')).href);

const childHelperScenarios = [
    ['helper pointlight 1', () => new PointLightHelper(new PointLightForHelper(), 1)],
    ['helper directional 1', () => new DirectionalLightHelper(new DirectionalLightForHelper(), 1)],
    ['helper hemisphere 1', () => new HemisphereLightHelper(new HemisphereLightForHelper(), 1)],
    ['helper hemisphere 2.5', () => new HemisphereLightHelper(new HemisphereLightForHelper(), 2.5)],
    ['helper spotlight', () => new SpotLightHelper(new SpotLightForHelper())],
    ['helper arrow 0 0 1 0 0 0 2', () => new ArrowHelper(new Vector3(0, 0, 1), new Vector3(0, 0, 0), 2)],
    ['helper arrow 1 0 0 1 1 1 5', () => new ArrowHelper(new Vector3(1, 0, 0), new Vector3(1, 1, 1), 5)],
    ['helper arrow 0 1 0 0 0 0 0.5', () => new ArrowHelper(new Vector3(0, 1, 0), new Vector3(0, 0, 0), 0.5)],
    ['helper arrow 0 0 1 0 0 0 0.1', () => new ArrowHelper(new Vector3(0, 0, 1), new Vector3(0, 0, 0), 0.1)],
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
const {ArrayCamera} = await import(pathToFileURL(path.join(sourceDir, 'src/cameras/ArrayCamera.js')).href);
const {CubeCamera} = await import(pathToFileURL(path.join(sourceDir, 'src/cameras/CubeCamera.js')).href);
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
    ['node perspective 90 2 1 10', () => new PerspectiveCamera(90, 2, 1, 10)],
    ['node perspective 1 0.1 0.01 1', () => new PerspectiveCamera(1, 0.1, 0.01, 1)],
    ['node perspective 180 1 0.1 100', () => new PerspectiveCamera(180, 1, 0.1, 100)],
    ['node perspective 75 1.7777777777777777 0.1 1000', () => new PerspectiveCamera(75, 16 / 9, 0.1, 1000)],
    ['node ortho -2 2 1 -1 0.1 50', () => new OrthographicCamera(-2, 2, 1, -1, 0.1, 50)],
    ['node ortho -5 5 5 -5 1 100', () => new OrthographicCamera(-5, 5, 5, -5, 1, 100)],
    ['node ortho 0 0 0 0 0 0', () => new OrthographicCamera(0, 0, 0, 0, 0, 0)],
    ['node ortho 1 1 1 1 1 1', () => new OrthographicCamera(1, 1, 1, 1, 1, 1)],
    ['node ortho -100 100 100 -100 1 1000', () => new OrthographicCamera(-100, 100, 100, -100, 1, 1000)],
    ['node scene', () => new Scene()],
    ['node group', () => new Group()],
    ['node mesh', () => new Mesh()],
    ['node points', () => new Points()],
    ['node sprite', () => new Sprite()],
    ['node lod', () => new LOD()],
    ['node array 2', () => new ArrayCamera([new PerspectiveCamera(), new PerspectiveCamera()])],
    ['node array 4', () => new ArrayCamera([new PerspectiveCamera(), new PerspectiveCamera(), new PerspectiveCamera(), new PerspectiveCamera()])],
    ['node array 1', () => new ArrayCamera([new PerspectiveCamera()])],
    ['node array 3', () => new ArrayCamera([new PerspectiveCamera(), new PerspectiveCamera(), new PerspectiveCamera()])],
    ['node cubecamera 0.5 500', () => new CubeCamera(0.5, 500, {})],
    ['node rendertarget 16 8', () => new RenderTarget(16, 8)],
    ['node rendertarget 1 1', () => new RenderTarget(1, 1)],
    ['node rendertarget 8 32', () => new RenderTarget(8, 32)],
    ['node rendertarget 256 1', () => new RenderTarget(256, 1)],
    ['node rendertarget 1920 1080', () => new RenderTarget(1920, 1080)],
    ['node rendertarget3d 4 4 2', () => new RenderTarget3D(4, 4, 2)],
    ['node rendertarget3d 2 2 1', () => new RenderTarget3D(2, 2, 1)],
    ['node rendertarget3d 8 8 4', () => new RenderTarget3D(8, 8, 4)],
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
        if (node.isArrayCamera) expected.fields.camerasCount = node.cameras.length;
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
const {CubeTexture} = await import(pathToFileURL(path.join(sourceDir, 'src/textures/CubeTexture.js')).href);
const {DataArrayTexture} = await import(pathToFileURL(path.join(sourceDir, 'src/textures/DataArrayTexture.js')).href);
const {Data3DTexture} = await import(pathToFileURL(path.join(sourceDir, 'src/textures/Data3DTexture.js')).href);
const {CompressedTexture} = await import(pathToFileURL(path.join(sourceDir, 'src/textures/CompressedTexture.js')).href);
const {CompressedArrayTexture} = await import(pathToFileURL(path.join(sourceDir, 'src/textures/CompressedArrayTexture.js')).href);

const textureScenarios = [
    ['texture base', () => new Texture()],
    ['texture data 4 2', () => new DataTexture(new Uint8Array(4 * 2 * 4), 4, 2)],
    ['texture data 1 1', () => new DataTexture(new Uint8Array(1 * 1 * 4), 1, 1)],
    ['texture data 8 8', () => new DataTexture(new Uint8Array(8 * 8 * 4), 8, 8)],
    ['texture data 3 5', () => new DataTexture(new Uint8Array(3 * 5 * 4), 3, 5)],
    ['texture data 16 2', () => new DataTexture(new Uint8Array(16 * 2 * 4), 16, 2)],
    ['texture data 64 64', () => new DataTexture(new Uint8Array(64 * 64 * 4), 64, 64)],
    ['texture data 128 1', () => new DataTexture(new Uint8Array(128 * 1 * 4), 128, 1)],
    ['texture data 32 32', () => new DataTexture(new Uint8Array(32 * 32 * 4), 32, 32)],
    ['texture depth 4 2', () => new DepthTexture(4, 2)],
    ['texture depth 1 1', () => new DepthTexture(1, 1)],
    ['texture depth 2 8', () => new DepthTexture(2, 8)],
    ['texture depth 640 480', () => new DepthTexture(640, 480)],
    ['texture array2d 2 2 2', () => new DataArrayTexture(new Uint8Array(2 * 2 * 2 * 4), 2, 2, 2)],
    ['texture array2d 1 1 1', () => new DataArrayTexture(new Uint8Array(4), 1, 1, 1)],
    ['texture array2d 4 4 4', () => new DataArrayTexture(new Uint8Array(4 * 4 * 4 * 4), 4, 4, 4)],
    ['texture data3d 2 2 2', () => new Data3DTexture(new Uint8Array(2 * 2 * 2 * 4), 2, 2, 2)],
    ['texture data3d 3 1 2', () => new Data3DTexture(new Uint8Array(3 * 1 * 2 * 4), 3, 1, 2)],
    ['texture data3d 4 4 4', () => new Data3DTexture(new Uint8Array(4 * 4 * 4 * 4), 4, 4, 4)],
    ['texture compressed', () => new CompressedTexture()],
    ['texture compressedarray', () => new CompressedArrayTexture()],
    ['texture cube 6', () => new CubeTexture([{width: 4, height: 4}, {width: 4, height: 4}, {width: 4, height: 4}, {width: 4, height: 4}, {width: 4, height: 4}, {width: 4, height: 4}])],
    ['texture cube 1', () => new CubeTexture([{width: 4, height: 4}])],
    ['texture cube 2', () => new CubeTexture([{width: 4, height: 4}, {width: 4, height: 4}])],
    ['texture cube 4', () => new CubeTexture([{width: 4, height: 4}, {width: 4, height: 4}, {width: 4, height: 4}, {width: 4, height: 4}])],
    ['texture cube 5', () => new CubeTexture([{width: 4, height: 4}, {width: 4, height: 4}, {width: 4, height: 4}, {width: 4, height: 4}, {width: 4, height: 4}])],
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
    ['fog linear 0 0 0 0 1', () => new Fog(new Color(0, 0, 0), 0, 1)],
    ['fog linear 1 1 1 0 1000', () => new Fog(new Color(1, 1, 1), 0, 1000)],
    ['fog linear 0.5 0.5 0.5 10 100', () => new Fog(new Color(0.5, 0.5, 0.5), 10, 100)],
    ['fog exp2 0.5 0.25 1 0.02', () => new FogExp2(new Color(0.5, 0.25, 1), 0.02)],
    ['fog exp2 0 0 0 1.5', () => new FogExp2(new Color(0, 0, 0), 1.5)],
    ['fog exp2 0.5 0.5 0.5 0', () => new FogExp2(new Color(0.5, 0.5, 0.5), 0)],
    ['fog exp2 1 0 0 1000', () => new FogExp2(new Color(1, 0, 0), 1000)],
    ['fog exp2 0.2 0.4 0.6 5', () => new FogExp2(new Color(0.2, 0.4, 0.6), 5)],
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
    ['sh 9 0 0 0 0 0 0 0 0 0', [0, 0, 0, 0, 0, 0, 0, 0, 0]],
    ['sh 18 1 1 1 2 2 2 3 3 3 4 4 4 5 5 5 6 6 6', [1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6]],
    ['sh 12 1 0 0 0 1 0 0 0 1 2 2 2', [1, 0, 0, 0, 1, 0, 0, 0, 1, 2, 2, 2]],
    ['sh 24 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24', [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24]],
    ['sh 6 -1 -2 -3 -4 -5 -6', [-1, -2, -3, -4, -5, -6]],
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
    ['instance 16', 16],
    ['instance 32', 32],
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
// The path container: the curves it accumulates and the point at five parameters.
const {Path} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/core/Path.js')).href);

const pathScenarios = [
    ['path 4 0 0 2 0 2 2 0 2', [[0, 0], [2, 0], [2, 2], [0, 2]]],
    ['path 2 1 1 4 1', [[1, 1], [4, 1]]],
    ['path 2 0 0 1 1', [[0, 0], [1, 1]]],
    ['path 5 0 0 1 0 1 1 0 1 0 0', [[0, 0], [1, 0], [1, 1], [0, 1], [0, 0]]],
    ['path 6 0 0 1 0 2 1 1 2 -1 1 -2 0', [[0, 0], [1, 0], [2, 1], [1, 2], [-1, 1], [-2, 0]]],
    ['path 7 0 0 1 1 2 0 3 1 4 0 5 1 6 0', [[0, 0], [1, 1], [2, 0], [3, 1], [4, 0], [5, 1], [6, 0]]],
    ['path 3 0 0 1 0 2 0', [[0, 0], [1, 0], [2, 0]]],
    ['path 2 -1 -1 2 2', [[-1, -1], [2, 2]]],
];
const pathRaw = execFileSync(probePath, [], {input: pathScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const pathLines = pathRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < pathScenarios.length; i += 1) {
    const [line, coordinates] = pathScenarios[i];
    const path = new Path();
    coordinates.forEach(([x, y], index) => { if (index === 0) path.moveTo(x, y); else path.lineTo(x, y); });
    const expected = {kind: 'path', curves: path.curves.length,
        points: [0, 0.25, 0.5, 0.75, 1].map((t) => { const p = path.getPoint(t); return [bitOf(p.x), bitOf(p.y)]; })};
    assert.deepEqual(pathLines[i], expected, `scenario ${line}`);
}
console.log(`path parity: ${pathScenarios.length} scenarios`);
// The stereo camera is not a scene graph object, so it is compared on its own fields.
const {StereoCamera} = await import(pathToFileURL(path.join(sourceDir, 'src/cameras/StereoCamera.js')).href);
const stereoScenarios = [
    ['stereo', () => new StereoCamera()],
];
const stereoRaw = execFileSync(probePath, [], {input: stereoScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const stereoLines = stereoRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < stereoScenarios.length; i += 1) {
    const [line, build] = stereoScenarios[i];
    const stereo = build();
    const expected = {kind: 'stereo', type: stereo.type, aspect: bitOf(stereo.aspect), eyeSep: bitOf(stereo.eyeSep),
        hasLeft: stereo.cameraL !== null && stereo.cameraL !== undefined,
        hasRight: stereo.cameraR !== null && stereo.cameraR !== undefined};
    if (stereo.cameraL) { expected.leftFov = bitOf(stereo.cameraL.fov); expected.leftAspect = bitOf(stereo.cameraL.aspect); }
    assert.deepEqual(stereoLines[i], expected, `scenario ${line}`);
}
console.log(`stereo camera parity: ${stereoScenarios.length} scenarios`);
// The raycaster: its ray, its clipping range and the thresholds it starts with.
const {Raycaster} = await import(pathToFileURL(path.join(sourceDir, 'src/core/Raycaster.js')).href);
const rayScenarios = [
    ['ray 0 0 0 0 0 -1 0 100', () => new Raycaster(new Vector3(0, 0, 0), new Vector3(0, 0, -1), 0, 100)],
    ['ray 1 2 3 0 1 0 0.5 10', () => new Raycaster(new Vector3(1, 2, 3), new Vector3(0, 1, 0), 0.5, 10)],
    ['ray 0 0 0 0 0 1 0 0', () => new Raycaster(new Vector3(0, 0, 0), new Vector3(0, 0, 1), 0, 0)],
];
const rayRaw = execFileSync(probePath, [], {input: rayScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const rayLines = rayRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < rayScenarios.length; i += 1) {
    const [line, build] = rayScenarios[i];
    const raycaster = build();
    const expected = {kind: 'ray',
        origin: [bitOf(raycaster.ray.origin.x), bitOf(raycaster.ray.origin.y), bitOf(raycaster.ray.origin.z)],
        direction: [bitOf(raycaster.ray.direction.x), bitOf(raycaster.ray.direction.y), bitOf(raycaster.ray.direction.z)],
        near: bitOf(raycaster.near), far: bitOf(raycaster.far),
        paramKeys: Object.keys(raycaster.params).length,
        lineThreshold: bitOf(raycaster.params.Line.threshold),
        pointsThreshold: bitOf(raycaster.params.Points.threshold)};
    assert.deepEqual(rayLines[i], expected, `scenario ${line}`);
}
console.log(`raycaster parity: ${rayScenarios.length} scenarios`);

// The curve path container: two line curves joined end to end.
const {CurvePath} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/core/CurvePath.js')).href);
const curvePathScenarios = [
    ['curvepath 0 0 1 0 1 1', [[0, 0], [1, 0], [1, 1]]],
    ['curvepath 1 1 3 2 3 5', [[1, 1], [3, 2], [3, 5]]],
    ['curvepath 0 0 1 0 2 0', [[0, 0], [1, 0], [2, 0]]],
    ['curvepath 0 0 0 0 0 0', [[0, 0], [0, 0], [0, 0]]],
    ['curvepath 1 1 1 1 1 1', [[1, 1], [1, 1], [1, 1]]],
    ['curvepath 0 0 0 1 1 1', [[0, 0], [0, 1], [1, 1]]],
    ['curvepath 0 0 1 1 2 2', [[0, 0], [1, 1], [2, 2]]],
    ['curvepath 0 0 2 0 1 1', [[0, 0], [2, 0], [1, 1]]],
    ['curvepath 4 4 5 4 5 5', [[4, 4], [5, 4], [5, 5]]],
    ['curvepath -1 -1 0 0 1 1', [[-1, -1], [0, 0], [1, 1]]],
];
const curvePathRaw = execFileSync(probePath, [], {input: curvePathScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const curvePathLines = curvePathRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < curvePathScenarios.length; i += 1) {
    const [line, coordinates] = curvePathScenarios[i];
    const path = new CurvePath();
    path.add(new LineCurve(new Vector2(coordinates[0][0], coordinates[0][1]), new Vector2(coordinates[1][0], coordinates[1][1])));
    path.add(new LineCurve(new Vector2(coordinates[1][0], coordinates[1][1]), new Vector2(coordinates[2][0], coordinates[2][1])));
    const expected = {kind: 'curvepath', curves: path.curves.length,
        points: [0, 0.25, 0.5, 0.75, 1].map((t) => { const p = path.getPoint(t); return [bitOf(p.x), bitOf(p.y)]; })};
    assert.deepEqual(curvePathLines[i], expected, `scenario ${line}`);
}
console.log(`curve path parity: ${curvePathScenarios.length} scenarios`);
// The shape path: the outline a shape is built from.
const {ShapePath} = await import(pathToFileURL(path.join(sourceDir, 'src/extras/core/ShapePath.js')).href);
const shapePathScenarios = [
    ['shapath 4 0 0 2 0 2 2 0 2', [[0, 0], [2, 0], [2, 2], [0, 2]]],
    ['shapath 3 0 0 1 0 0 1', [[0, 0], [1, 0], [0, 1]]],
    ['shapath 2 0 0 4 3', [[0, 0], [4, 3]]],
    ['shapath 4 0 0 1 0 1 1 0 1', [[0, 0], [1, 0], [1, 1], [0, 1]]],
];
const shapePathRaw = execFileSync(probePath, [], {input: shapePathScenarios.map(([line]) => line).join('\n'), encoding: 'utf8'});
const shapePathLines = shapePathRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < shapePathScenarios.length; i += 1) {
    const [line, coordinates] = shapePathScenarios[i];
    const shapePath = new ShapePath();
    coordinates.forEach(([x, y], index) => { if (index === 0) shapePath.moveTo(x, y); else shapePath.lineTo(x, y); });
    const subPath = shapePath.subPaths[0];
    const expected = {kind: 'shapath', type: shapePath.type, subPaths: shapePath.subPaths.length,
        curves: subPath ? subPath.curves.length : 0,
        points: subPath ? [0, 0.25, 0.5, 0.75, 1].map((t) => { const p = subPath.getPoint(t); return [bitOf(p.x), bitOf(p.y)]; }) : []};
    assert.deepEqual(shapePathLines[i], expected, `scenario ${line}`);
}
console.log(`shape path parity: ${shapePathScenarios.length} scenarios`);
// Cube render targets: type flags, size and the six face cube texture.
const {WebGLCubeRenderTarget} = await import(pathToFileURL(path.join(sourceDir, 'src/renderers/WebGLCubeRenderTarget.js')).href);
const cubeTargetScenarios = [8, 32, 1, 300, 16, 256];
const cubeTargetRaw = execFileSync(probePath, [], {input: cubeTargetScenarios.map((size) => `node cubetarget ${size}`).join('\n'), encoding: 'utf8'});
const cubeTargetLines = cubeTargetRaw.trim().split('\n').map((line) => JSON.parse(line));
for (let i = 0; i < cubeTargetScenarios.length; i += 1) {
    const size = cubeTargetScenarios[i];
    const target = new WebGLCubeRenderTarget(size);
    const faces = target.texture.images;
    const expected = {kind: 'node', which: 'cubetarget',
        isWebGLCubeRenderTarget: target.isWebGLCubeRenderTarget,
        width: bitOf(target.width), height: bitOf(target.height),
        faces: faces.length,
        face0Width: faces.length ? bitOf(faces[0].width) : 0,
        renderTargetTexture: target.texture.isRenderTargetTexture === true,
        mapping: bitOf(target.texture.mapping)};
    assert.deepEqual(cubeTargetLines[i], expected, `cube target ${size}`);
}
console.log(`cube target parity: ${cubeTargetScenarios.length} scenarios`);