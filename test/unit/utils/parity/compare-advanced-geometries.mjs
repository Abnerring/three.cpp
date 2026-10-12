import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
if (process.argv.length !== 4) throw new Error('Usage: node compare-advanced-geometries.mjs <three.js> <probe.exe>');
const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const T = await import(pathToFileURL(path.join(upstream, 'src/Three.Core.js')));
const serialized = value => JSON.parse(JSON.stringify(value));
function snapshot(g) {
    const result = {type: g.type, attributes: Object.fromEntries(Object.entries(g.attributes).map(([name, a]) => [name, a.toJSON()])),
        index: g.index ? g.index.toJSON() : null, groups: g.groups.map(v => [v.start, v.count, v.materialIndex]), json: serialized(g.toJSON())};
    if (g instanceof T.TubeGeometry) result.frames = [g.tangents, g.normals, g.binormals].map(array => array.map(v => v.toArray()));
    return result;
}
function polyhedra(Geometry) {
    const result = [[1,0],[2.5,1],[0,2],[-2,0],[1,-1],[1,-.5],[1,NaN]].map(([r,d]) => snapshot(new Geometry(r,d)));
    const geometry = new Geometry(1.3, 2);
    result.push(snapshot(geometry.clone()), snapshot(Geometry.fromJSON(geometry.toJSON())));
    geometry.translate(1, -2, 3); result.push(snapshot(geometry));
    return result;
}
function makeShape(hole, offset = 0) {
    const shape = new T.Shape(); shape.uuid = offset === 0 ? 'shape0' : 'shape1';
    shape.moveTo(offset, 0).lineTo(offset + 8, 0).lineTo(offset + 8, 8).lineTo(offset + 3, 8)
        .quadraticCurveTo(offset - 1, 4, offset, 0);
    if (hole) shape.holes.push(new T.Path().moveTo(offset + 2, 2).lineTo(offset + 2, 4)
        .lineTo(offset + 4, 4).lineTo(offset + 4, 2).lineTo(offset + 2, 2));
    return shape;
}
function shapes() {
    const result = [], defaultGeometry = new T.ShapeGeometry(); defaultGeometry.parameters.shapes.uuid = 'default';
    result.push(snapshot(defaultGeometry));
    const a = makeShape(true), b = makeShape(false, 20), list = [a,b];
    const single = new T.ShapeGeometry(a, 4), array = new T.ShapeGeometry(list, 4), empty = new T.ShapeGeometry([], 4);
    result.push(snapshot(single), snapshot(array), snapshot(empty), snapshot(array.clone()),
        snapshot(T.ShapeGeometry.fromJSON(single.toJSON(), {[a.uuid]: a})));
    const clone = array.clone(); list.push(a);
    result.push({arrayIdentity: clone.parameters.shapes === list, singleIdentity: single.parameters.shapes === a,
        changedParameters: serialized(array.toJSON()), unchangedVertexCount: array.attributes.position.count});
    single.translate(1, 2, 3); result.push(snapshot(single)); return result;
}
function tubes() {
    const result = [snapshot(new T.TubeGeometry())];
    const line = new T.LineCurve3(new T.Vector3(1,-2,3), new T.Vector3(4,5,-1));
    const curved = new T.CatmullRomCurve3([[0,0,0],[2,3,1],[-1,4,3],[-2,0,-1]].map(v => new T.Vector3(...v)), true);
    for (const closed of [false,true]) result.push(snapshot(new T.TubeGeometry(line,3,.7,5,closed)),
        snapshot(new T.TubeGeometry(curved,7,-.4,4,closed)));
    for (const radial of [0,2.5,-1,NaN]) result.push(snapshot(new T.TubeGeometry(line,2,1,radial)));
    result.push(snapshot(new T.TubeGeometry(line,0,1,3)));
    const original = new T.TubeGeometry(line,3,.5,4), clone = original.clone();
    result.push(snapshot(clone), [clone.parameters.path === line, clone.tangents.length, clone.attributes.position !== original.attributes.position],
        snapshot(T.TubeGeometry.fromJSON(original.toJSON())));
    original.scale(2,1,.5); result.push(snapshot(original));
    const framesJSON = frames => [frames.tangents, frames.normals, frames.binormals].map(a => a.map(v => v.toArray()));
    for (const segments of [.5,1.5,2.5]) {
        result.push(snapshot(new T.TubeGeometry(curved,segments,1,-1)),
            snapshot(new T.TubeGeometry(curved,segments,1,NaN)), framesJSON(curved.computeFrenetFrames(segments)));
    }
    for (const segments of [-1,NaN,-Infinity,.5,2.5]) for (const closed of [false,true]) {
        try { new T.TubeGeometry(line,segments,1,0,closed); result.push('success'); }
        catch (error) { result.push(error.constructor.name); }
        try { result.push(framesJSON(curved.computeFrenetFrames(segments,closed))); }
        catch (error) { result.push(error.constructor.name); }
    }
    return result;
}
function edges() {
    const result = [snapshot(new T.EdgesGeometry()), snapshot(new T.WireframeGeometry())], box = new T.BoxGeometry();
    for (const threshold of [0,1,90,91,180,-20,NaN]) result.push(snapshot(new T.EdgesGeometry(box,threshold)));
    result.push(snapshot(new T.WireframeGeometry(box)), snapshot(new T.EdgesGeometry(box.toNonIndexed())),
        snapshot(new T.WireframeGeometry(box.toNonIndexed())));
    box.clearGroups(); box.addGroup(6,6,0); box.addGroup(0,3,1); box.addGroup(0,3,2); box.setDrawRange(0,0);
    result.push(snapshot(new T.WireframeGeometry(box)), snapshot(new T.EdgesGeometry(box)));
    for (const values of [[0,0,0, 1,0,0, 0,1,0, 0,1,0, 1,0,0, 1,1,0],
        [0,0,0, .000049,0,0, 0,1,0], [0,0,0, .00005,0,0, 0,1,0],
        [0,0,0, 1,0,0, 2,0,0], [0,0,0, 1,0,0, NaN,1,0], [0,0,0, 1,0,0], []]) {
        const geometry = new T.BufferGeometry().setAttribute('position', new T.BufferAttribute(new Float64Array(values),3));
        result.push(snapshot(new T.EdgesGeometry(geometry)), snapshot(new T.WireframeGeometry(geometry)));
    }
    const original = new T.EdgesGeometry(box), copied = original.clone();
    result.push([copied.parameters.geometry === box, copied.attributes.position !== original.attributes.position], snapshot(copied));
    original.translate(1,2,3); result.push(snapshot(original)); return result;
}
function normalize(value) {
    if (typeof value === 'number') return Number.isNaN(value) ? 'NaN' : value === Infinity ? 'Infinity' : value === -Infinity ? '-Infinity' : Object.is(value,-0) ? '-0' : value;
    if (Array.isArray(value)) return value.map(normalize);
    if (value && typeof value === 'object') return Object.fromEntries(Object.entries(value).map(([key,v]) => [key, key === 'uuid' ? 'uuid' : normalize(v)]));
    return value;
}
const vertices = [1,0,0, 0,1,0, 0,0,1], indices = [0,1,2], poly = new T.PolyhedronGeometry(vertices,indices,1.7,1), clone = poly.clone();
const expected = normalize({tetrahedron:polyhedra(T.TetrahedronGeometry), octahedron:polyhedra(T.OctahedronGeometry),
    icosahedron:polyhedra(T.IcosahedronGeometry), dodecahedron:polyhedra(T.DodecahedronGeometry),
    polyhedron:[snapshot(new T.PolyhedronGeometry()), snapshot(poly), snapshot(T.PolyhedronGeometry.fromJSON(poly.toJSON())),
        [clone.parameters.vertices === vertices, clone.parameters.indices === indices, clone.attributes.position !== poly.attributes.position]],
    shapes:shapes(), tubes:tubes(), edges:edges(),
    earlySerialization: [T.PolyhedronGeometry, T.TetrahedronGeometry, T.OctahedronGeometry,
        T.IcosahedronGeometry, T.DodecahedronGeometry, T.ShapeGeometry, T.TubeGeometry,
        T.EdgesGeometry, T.WireframeGeometry].map(Geometry => {
            const geometry = new Geometry();
            if (geometry instanceof T.ShapeGeometry) geometry.parameters.shapes.uuid = 'default';
            geometry.setAttribute('position', null);
            geometry.morphAttributes.position = [null];
            return serialized(geometry.toJSON());
        })});
const actual = JSON.parse(execFileSync(path.resolve(process.argv[3]), [], {encoding:'utf8',maxBuffer:32*1024*1024}));
let comparisons = 0;
function compare(a,b,location='$') {
    ++comparisons;
    if (typeof a === 'number' && typeof b === 'number') {
        assert.ok(Math.abs(a-b) <= 2e-12 * Math.max(1,Math.abs(a),Math.abs(b)), `${location}: ${a} != ${b}`);
    } else if (b && typeof b === 'object') {
        assert.ok(a && typeof a === 'object', `${location}: type`);
        assert.deepEqual(Object.keys(a), Object.keys(b), `${location}: keys`);
        for (const key of Object.keys(b)) compare(a[key], b[key], `${location}.${key}`);
    } else assert.ok(Object.is(a,b), `${location}: ${JSON.stringify(a)} != ${JSON.stringify(b)}`);
}
compare(actual,expected);
console.log(`Advanced geometry parity: ${comparisons} values/identities; fixed upstream ${lock.commit}`);
