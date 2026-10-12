import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
import { execFileSync } from 'node:child_process';

const upstream = path.resolve(process.argv[2]);
const executable = path.resolve(process.argv[3]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
const commit = execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim();
if (commit !== lock.commit) throw new Error(`Expected ${lock.commit}, got ${commit}`);
const source = (name) => import(pathToFileURL(path.join(upstream, `src/${name}.js`)));
const {Curve} = await source('extras/core/Curve');
const {Vector2} = await source('math/Vector2');
const {Vector3} = await source('math/Vector3');
const controls = c => ['v0', 'v1', 'v2', 'v3'].filter(k => k in c).map(k => c[k]);
const points = values => values.map(value => value.toArray());
function makeCurve(C, V, count) {
    return new C(...[new V(-10, 0, 1), new V(-5, 15, -2), new V(20, 15, 4), new V(10, 0, 8)].slice(0, count));
}
function scenario(C, V, count) {
    const curve = makeCurve(C, V, count), result = {};
    result.defaults = new C().toJSON();
    result.json = curve.toJSON();
    result.points = points(curve.getPoints(4));
    result.spaced = points(curve.getSpacedPoints(4));
    result.length = curve.getLength();
    result.lengths = [...curve.getLengths(4)];
    result.pointAt = curve.getPointAt(.3).toArray();
    result.tangent = curve.getTangent(.25).toArray();
    result.tangentAt = curve.getTangentAt(.75).toArray();
    const target = new V();
    result.targets = [curve.getPoint(.3,target) === target, curve.getPointAt(.3,target) === target,
        curve.getTangent(.3,target) === target, curve.getTangentAt(.3,target) === target];
    const copied = curve.clone();
    result.clone = {type:copied.type, independent:controls(copied).every((v,i) => v !== controls(curve)[i]),
        cacheNull:copied.cacheArcLengths === null, json:copied.toJSON()};
    const alias = makeCurve(C,V,count), aliasTarget = controls(alias)[0];
    result.aliasedTarget = alias.getPoint(.25,aliasTarget).toArray();
    result.aliasedControl = aliasTarget.toArray();
    result.pointsZero = points(curve.getPoints(0));
    result.pointsNegative = points(curve.getPoints(-1));
    result.pointsFractional = points(curve.getPoints(2.5));
    result.tangentOutside = points([curve.getTangent(-.25),curve.getTangent(1.25)]);
    result.pointOutside = points([curve.getPoint(-.25),curve.getPoint(1.25)]);
    curve.arcLengthDivisions = 4;
    const original = curve.getLengths();
    result.cacheSame = curve.getLengths() === original;
    controls(curve).at(-1).setScalar(30);
    result.cacheStale = [...curve.getLengths()];
    curve.needsUpdate = true;
    const updated = curve.getLengths();
    result.cacheUpdated = {newIdentity:updated !== original, needsUpdate:curve.needsUpdate,
        lengths:[...updated], oldLengths:[...original]};
    curve.updateArcLengths();
    result.updateArcLengths = {newIdentity:curve.cacheArcLengths !== updated,
        needsUpdate:curve.needsUpdate, lengths:[...curve.getLengths()]};
    result.mappings = [-.25,0,.3,.5,1,1.25,NaN,Infinity,-Infinity].map(u => curve.getUtoTmapping(u));
    result.mappings.push(...[0,-0,NaN,1,-1,Infinity].map(d => curve.getUtoTmapping(.3,d)));
    const destination = makeCurve(C,V,count);
    destination.arcLengthDivisions = 4;
    controls(destination).at(-1).setScalar(50);
    const destinationCache = destination.getLengths(), destinationControls = controls(destination);
    result.copyIdentity = destination.copy(curve) === destination;
    result.copyControlIdentity = controls(destination).every((v,i) => v === destinationControls[i]);
    result.copyCacheIdentity = destination.getLengths() === destinationCache;
    result.copyJson = destination.toJSON();
    const restored = makeCurve(C,V,count), restoredControls = controls(restored);
    result.fromJSONIdentity = restored.fromJSON(curve.toJSON()) === restored;
    result.fromJSONControls = controls(restored).every((v,i) => v === restoredControls[i]);
    result.restored = restored.toJSON();
    const zero = new C();
    result.zeroMapping = zero.getUtoTmapping(.3);
    result.zeroTangent = zero.getTangent(.3).toArray();
    curve.arcLengthDivisions = 0;
    result.zeroLengths = [...curve.getLengths()];
    result.zeroDivisionMapping = curve.getUtoTmapping(.3);
    if (V === Vector3) {
        const frameCurve = makeCurve(C,V,count);
        for (const closed of [false,true]) {
            const frames = frameCurve.computeFrenetFrames(4,closed);
            result[closed ? 'framesClosed' : 'framesOpen'] = {
                tangents:points(frames.tangents),normals:points(frames.normals),binormals:points(frames.binormals)};
        }
    }
    return result;
}
const expected = {};
for (const [name,V,count] of [['LineCurve',Vector2,2],['LineCurve3',Vector3,2],
    ['CubicBezierCurve',Vector2,4],['CubicBezierCurve3',Vector3,4],
    ['QuadraticBezierCurve',Vector2,3],['QuadraticBezierCurve3',Vector3,3]]) {
    const {[name]:C} = await source(`extras/curves/${name}`);
    expected[name] = scenario(C,V,count);
}
class NonlinearCurve extends Curve {
    constructor() { super(); this.calls = 0; }
    getPoint(t,target = new Vector2()) { ++this.calls; return target.set(t*t,0); }
}
const nonlinear = new NonlinearCurve();
nonlinear.arcLengthDivisions = 4;
const cache = nonlinear.getLengths();
expected.base = {json:new Curve().toJSON(),lengths:[...cache],calls:nonlinear.calls,
    mapping:nonlinear.getUtoTmapping(.3),pointAt:nonlinear.getPointAt(.3).toArray(),tangent:nonlinear.getTangent(.5).toArray()};
function normalized(value) {
    if (typeof value === 'number') {
        if (Number.isNaN(value)) return 'NaN';
        if (value === Infinity) return 'Infinity';
        if (value === -Infinity) return '-Infinity';
        if (Object.is(value,-0)) return '-0';
        return value;
    }
    if (Array.isArray(value)) return value.map(normalized);
    if (value && typeof value === 'object') return Object.fromEntries(Object.entries(value).map(([k,v]) => [k,normalized(v)]));
    return value;
}
const actual = JSON.parse(execFileSync(executable, [], {encoding:'utf8'}));
let comparisons = 0;
function compare(a,b,location) {
    if (typeof a === 'number' && typeof b === 'number') {
        ++comparisons;
        if (Math.abs(a-b) <= 5e-12*Math.max(1,Math.abs(a),Math.abs(b))) return;
        throw new Error(`${location}: ${a} != ${b}`);
    }
    if (Array.isArray(a) && Array.isArray(b)) {
        if(a.length !== b.length) throw new Error(`${location}: array length mismatch`);
        a.forEach((value,i) => compare(value,b[i],`${location}[${i}]`)); return;
    }
    if(a && b && typeof a === 'object' && typeof b === 'object') {
        const keys = Object.keys(a);
        if(keys.length !== Object.keys(b).length || keys.some(k => !(k in b))) throw new Error(`${location}: keys mismatch`);
        keys.forEach(k => compare(a[k],b[k],`${location}.${k}`)); return;
    }
    ++comparisons;
    if(a !== b) throw new Error(`${location}: ${String(a)} != ${String(b)}`);
}
compare(normalized(expected),actual,'curves');
console.log(`Curve parity: 6 concrete classes, ${comparisons} values/identity checks passed (${commit}).`);
