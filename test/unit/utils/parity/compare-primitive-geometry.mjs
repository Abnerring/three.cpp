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