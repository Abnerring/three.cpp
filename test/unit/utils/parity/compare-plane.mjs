// Differential check for the native src/math/Plane.js port.
//
// A plane reads a normal and a constant, then what the operation needs. The cases lean on the sign
// convention - distance is positive on the side the normal points to - and on the normalisation that
// applyMatrix4 has to redo after transforming the normal, so an unnormalised plane and a matrix with a
// non-uniform scale are both in the list.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-plane.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Plane}=await import(pathToFileURL(path.join(sourceDir,'src/math/Plane.js')).href);
const {Vector3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector3.js')).href);
const {Matrix4}=await import(pathToFileURL(path.join(sourceDir,'src/math/Matrix4.js')).href);
const {Box3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Box3.js')).href);
const {Sphere}=await import(pathToFileURL(path.join(sourceDir,'src/math/Sphere.js')).href);
const unitPlane=[0,1,0,0];
const offsetPlane=[0,1,0,-2];
const unnormalised=[0,3,0,-6];
const tilted=[1,1,0,0];
const identity=[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1];
const translate=[1,0,0,0, 0,1,0,0, 0,0,1,0, 1,2,3,1];
const scale=[2,0,0,0, 0,2,0,0, 0,0,2,0, 0,0,0,1];
const scenarios=[
    ['plane.normalize',...unitPlane],['plane.normalize',...unnormalised],
    ['plane.distance',...unitPlane, 0,0,0],['plane.distance',...unitPlane, 0,5,0],
    ['plane.distance',...offsetPlane, 0,0,0],['plane.distance',...tilted, 1,1,0],
    ['plane.project',...unitPlane, 0,5,0],['plane.project',...offsetPlane, 0,0,0],
    ['plane.negate',...offsetPlane],['plane.negate',...unnormalised],
    ['plane.translate',...offsetPlane, 0,1,0],
    ['plane.setFromNormalAndCoplanar', 0,2,0, 0,3,0],
    ['plane.applyMatrix4',...unitPlane, ...identity],
    ['plane.applyMatrix4',...unitPlane, ...translate],
    ['plane.applyMatrix4',...tilted, ...scale],
    ['plane.intersectsBox',...unitPlane, -1,-1,-1, 1,1,1],
    ['plane.intersectsBox',...offsetPlane, -1,-1,-1, 1,1,1],
    ['plane.intersectsSphere',...unitPlane, 0,0,0, 1],
    ['plane.intersectsSphere',...offsetPlane, 0,0,0, 1]
];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const num=value=>Number.isNaN(value)?'NaN':value;
const lines=scenarios.map(([op,...args])=>`${op} ${args.map(num).join(' ')}`);
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
const point=args=>new Vector3(args[0],args[1],args[2]);
const planeOf=args=>new Plane(point(args.slice(0,3)),args[3]);
const matrixOf=args=>{
    const m=new Matrix4();
    m.set(...args.slice(4,20));
    return m;
};
const encodePoint=v=>({x:encode(v.x),y:encode(v.y),z:encode(v.z)});
const encodePlane=p=>({normal:encodePoint(p.normal),constant:encode(p.constant)});
const call=(op,args)=>{
    switch(op){
        case 'plane.normalize': return encodePlane(planeOf(args).normalize());
        case 'plane.distance': return {value:encode(planeOf(args).distanceToPoint(point(args.slice(4))))};
        case 'plane.project': return encodePoint(planeOf(args).projectPoint(point(args.slice(4)),new Vector3()));
        case 'plane.negate': return encodePlane(planeOf(args).negate());
        case 'plane.translate': return encodePlane(planeOf(args).translate(point(args.slice(4))));
        case 'plane.setFromNormalAndCoplanar': return encodePlane(new Plane().setFromNormalAndCoplanarPoint(point(args.slice(0)),point(args.slice(3))));
        case 'plane.applyMatrix4': return encodePlane(planeOf(args).applyMatrix4(matrixOf(args)));
        case 'plane.intersectsBox': return {value:planeOf(args).intersectsBox(new Box3(point(args.slice(4,7)),point(args.slice(7,10))))};
        case 'plane.intersectsSphere': return {value:planeOf(args).intersectsSphere(new Sphere(point(args.slice(4,7)),args[7]))};
        default: throw new Error('unknown op '+op);
    }
};
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    const expected=call(op,args);
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,args.join(' '));
        console.log('  probe :',JSON.stringify(actual).slice(0,240));
        console.log('  oracle:',JSON.stringify(expected).slice(0,240));
    }
    assert.deepEqual(actual,expected,`${op} #${index}`);
}
console.log(`Plane parity: ${scenarios.length} scenarios`);