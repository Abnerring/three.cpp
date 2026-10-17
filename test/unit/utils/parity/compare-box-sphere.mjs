// Differential check for the native src/math/Box3.js and src/math/Sphere.js ports.
//
// Box operations read a min and a max, sphere operations a centre and a radius, and what follows. The
// cases lean on what a hand port drifts on: an empty box (min above max), a degenerate sphere, points on
// the surface, and the clamped point on each side.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-box-sphere.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Box3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Box3.js')).href);
const {Sphere}=await import(pathToFileURL(path.join(sourceDir,'src/math/Sphere.js')).href);
const {Vector3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector3.js')).href);
const boxA=[-1,-1,-1, 1,1,1];
const boxB=[0,0,0, 2,2,2];
const boxFar=[10,10,10, 11,11,11];
const boxEmpty=[1,1,1, -1,-1,-1];
const sphereA=[0,0,0, 1];
const sphereFar=[10,0,0, 1];
const sphereZero=[0,0,0, 0];
const scenarios=[
    ['box.center',...boxA],['box.center',...boxEmpty],['box.size',...boxA],['box.size',...boxEmpty],
    ['box.contains',...boxA, 0,0,0],['box.contains',...boxA, 1,1,1],['box.contains',...boxA, 2,0,0],
    ['box.clamp',...boxA, 5,0,0],['box.clamp',...boxA, -5,-5,-5],
    ['box.distance',...boxA, 5,0,0],['box.distance',...boxA, 0,0,0],
    ['box.intersects',...boxA, ...boxB],['box.intersects',...boxA, ...boxFar],
    ['box.expand',...boxA, 3,0,0],
    ['box.intersect',...boxA, ...boxB],['box.intersect',...boxA, ...boxFar],
    ['box.union',...boxA, ...boxFar],
    ['box.json',...boxA],
    ['box.fromJSON',...boxA],
    ['sphere.contains',...sphereA, 0,0,0],['sphere.contains',...sphereA, 1,0,0],['sphere.contains',...sphereA, 2,0,0],
    ['sphere.distance',...sphereA, 2,0,0],['sphere.distance',...sphereA, 0,0,0],
    ['sphere.clamp',...sphereA, 2,0,0],['sphere.clamp',...sphereZero, 1,0,0],
    ['sphere.intersectsSphere',...sphereA, ...sphereFar],['sphere.intersectsSphere',...sphereA, 1.5,0,0,1],
    ['sphere.intersectsBox',...sphereA, ...boxA],['sphere.intersectsBox',...sphereA, ...boxFar],
    ['sphere.union',...sphereA, ...sphereFar],
    ['sphere.json',...sphereA],
    ['sphere.fromJSON',...sphereA],
    ['sphere.boundingBox',...sphereA],['sphere.boundingBox',...sphereZero]
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
const boxOf=args=>new Box3(point(args.slice(0,3)),point(args.slice(3,6)));
const sphereOf=args=>new Sphere(point(args.slice(0,3)),args[3]);
const encodePoint=v=>({x:encode(v.x),y:encode(v.y),z:encode(v.z)});
const encodeBox=b=>({min:encodePoint(b.min),max:encodePoint(b.max)});
const call=(op,args)=>{
    switch(op){
        case 'box.center': return encodePoint(boxOf(args).getCenter(new Vector3()));
        case 'box.size': return encodePoint(boxOf(args).getSize(new Vector3()));
        case 'box.contains': return {value:boxOf(args).containsPoint(point(args.slice(6)))};
        case 'box.clamp': return encodePoint(boxOf(args).clampPoint(point(args.slice(6)),new Vector3()));
        case 'box.distance': return {value:encode(boxOf(args).distanceToPoint(point(args.slice(6))))};
        case 'box.intersects': return {value:boxOf(args).intersectsBox(boxOf(args.slice(6)))};
        case 'box.expand': return encodeBox(boxOf(args).expandByPoint(point(args.slice(6))));
        case 'box.intersect': return encodeBox(boxOf(args).intersect(boxOf(args.slice(6))));
        case 'sphere.contains': return {value:sphereOf(args).containsPoint(point(args.slice(4)))};
        case 'sphere.distance': return {value:encode(sphereOf(args).distanceToPoint(point(args.slice(4))))};
        case 'sphere.clamp': return encodePoint(sphereOf(args).clampPoint(point(args.slice(4)),new Vector3()));
        case 'sphere.intersectsSphere': return {value:sphereOf(args).intersectsSphere(sphereOf(args.slice(4)))};
        case 'sphere.intersectsBox': return {value:sphereOf(args).intersectsBox(boxOf(args.slice(4)))};
        case 'box.union': return encodeBox(boxOf(args).union(boxOf(args.slice(6))));
        case 'box.json': { const j=boxOf(args).toJSON(); return {min:j.min.map(encode),max:j.max.map(encode)}; }
        case 'box.fromJSON': return encodeBox(new Box3().fromJSON(boxOf(args).toJSON()));
        case 'sphere.union': return {radius:encode(sphereOf(args).union(sphereOf(args.slice(4))).radius)};
        case 'sphere.json': { const j=sphereOf(args).toJSON(); return {center:j.center.map(encode),radius:encode(j.radius)}; }
        case 'sphere.fromJSON': return {radius:encode(new Sphere().fromJSON(sphereOf(args).toJSON()).radius)};
        case 'sphere.boundingBox': return encodeBox(sphereOf(args).getBoundingBox(new Box3()));
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
console.log(`Box3/Sphere parity: ${scenarios.length} scenarios`);