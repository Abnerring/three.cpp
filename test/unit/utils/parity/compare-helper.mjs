import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
import { execFileSync } from 'node:child_process';
const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
if (execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding:'utf8'}).trim() !== lock.commit)
    throw new Error('Upstream checkout does not match upstream.json');
const T = await import(pathToFileURL(path.join(upstream, 'src/Three.Core.js')));
const {Matrix2} = await import(pathToFileURL(path.join(upstream, 'src/math/Matrix2.js')));
const number = value => Number.isNaN(value) ? 'NaN' : value === Infinity ? 'Infinity' : value === -Infinity ? '-Infinity' : Object.is(value, -0) ? '-0' : value;
const vec = value => value.toArray().map(number);
const spherical = value => [value.radius,value.phi,value.theta].map(number);
const cylindrical = value => [value.radius,value.theta,value.y].map(number);
const matrix = value => value.elements.map(number);
const box = value => {
    const point = new T.Vector2(4,-2);
    return {min:vec(value.min),max:vec(value.max),empty:value.isEmpty(),center:vec(value.getCenter(new T.Vector2())),
        size:vec(value.getSize(new T.Vector2())),parameter:vec(value.getParameter(point,new T.Vector2())),
        clamped:vec(value.clampPoint(point,new T.Vector2())),distance:number(value.distanceToPoint(point)),
        contains:value.containsPoint(point),containsSelf:value.containsBox(value),intersectsSelf:value.intersectsBox(value)};
};
const expected = {sphericalDefault:spherical(new T.Spherical()),cylindricalDefault:cylindrical(new T.Cylindrical())};
expected.coordinates = [[0,0,0],[2,-3,4],[-0,0,-0],[0,1,0],[0,-1,0],[1e200,-1e200,1e200],[1e-200,-1e-200,1e-200],[NaN,2,3],[Infinity,1,0]].map(values => {
    const point=new T.Vector3(...values),s=new T.Spherical().setFromVector3(point),c=new T.Cylindrical().setFromVector3(point);
    return {spherical:spherical(s),cylindrical:cylindrical(c),sphericalVector:vec(new T.Vector3().setFromSpherical(s)),cylindricalVector:vec(new T.Vector3().setFromCylindrical(c))};
});
expected.safe = [-1,0,0.0000001,0.3,Math.PI,Math.PI+1,Infinity,-Infinity,NaN].map(phi => {
    const original=new T.Spherical(3,phi,-0),copy=original.clone().makeSafe();
    return {original:spherical(original),safe:spherical(copy)};
});
expected.sphericalCopy=spherical(new T.Spherical().copy(new T.Spherical().set(2,3,4)));
expected.cylindricalCopy=cylindrical(new T.Cylindrical().copy(new T.Cylindrical().set(5,6,7).clone()));
const m=new Matrix2(); expected.matrices=[matrix(m),matrix(m.set(1,2,3,4)),matrix(m.fromArray([-1,5,6,7,8],1)),matrix(m.identity()),matrix(new Matrix2(-0,Infinity,NaN,-Infinity))];
const b=new T.Box2();expected.boxes=[box(b)];
b.setFromPoints([new T.Vector2(-3,2),new T.Vector2(4,-1),new T.Vector2(1,5)]);expected.boxes.push(box(b));
b.setFromCenterAndSize(new T.Vector2(2,-3),new T.Vector2(4,6));expected.boxes.push(box(b));
const copy=b.clone().expandByPoint(new T.Vector2(-7,9)).expandByVector(new T.Vector2(2,3)).expandByScalar(0.25);
expected.boxes.push(box(copy),box(b));
copy.intersect(new T.Box2(new T.Vector2(-1,-1),new T.Vector2(1,1)));expected.boxes.push(box(copy));
copy.intersect(new T.Box2(new T.Vector2(5,5),new T.Vector2(6,6)));expected.boxes.push(box(copy));
copy.union(b).translate(new T.Vector2(-2,3));expected.boxes.push(box(copy));
b.set(new T.Vector2(1,2),new T.Vector2(1,2));expected.boxes.push(box(b));
b.setFromPoints([new T.Vector2(NaN,1),new T.Vector2(0,NaN)]);expected.boxes.push(box(b));
b.setFromPoints([]);expected.boxes.push(box(b));
const p=new T.Box2(new T.Vector2(0,0),new T.Vector2(2,2));
expected.boxRelations=[p.equals(p.clone()),p.containsBox(new T.Box2(new T.Vector2(1,1),new T.Vector2(2,2))),
    p.containsBox(new T.Box2(new T.Vector2(-1,0),new T.Vector2(1,1))),p.intersectsBox(new T.Box2(new T.Vector2(2,2),new T.Vector2(3,3))),
    p.intersectsBox(new T.Box2(new T.Vector2(2.000001,2),new T.Vector2(3,3))),p.copy(new T.Box2()).isEmpty()];
const clipped=new T.MeshBasicMaterial(),clipPlane=new T.Plane(new T.Vector3(0,1,0),-2);
clipped.clippingPlanes=[clipPlane];const clippedCopy=clipped.clone();clipPlane.constant=-3;
expected.clippingPlanes={source:clipped.toJSON().clippingPlanes,clone:clippedCopy.toJSON().clippingPlanes,independent:clippedCopy.clippingPlanes[0]!==clipPlane};
const actual=JSON.parse(execFileSync(process.argv[3],[],{encoding:'utf8'}));
let count=0;
function compare(actual,expected,where) {
    if (typeof actual!==typeof expected || actual===null || expected===null) {
        if(actual!==expected) throw new Error(`${where}: ${actual} != ${expected}`);
    } else if(typeof actual==='number') {
        if(Math.abs(actual-expected)>2e-12*Math.max(1,Math.abs(expected))) throw new Error(`${where}: ${actual} != ${expected}`);
    } else if(typeof actual==='object') {
        if(Array.isArray(actual)!==Array.isArray(expected) || JSON.stringify(Object.keys(actual).sort())!==JSON.stringify(Object.keys(expected).sort()))
            throw new Error(`${where}: structure differs`);
        for(const key of Object.keys(expected)) compare(actual[key],expected[key],`${where}.${key}`);
        return;
    } else if(actual!==expected) throw new Error(`${where}: ${actual} != ${expected}`);
    ++count;
}
compare(actual,expected,'root');
console.log(`three.js r${T.REVISION} helper parity passed: ${count} values.`);
