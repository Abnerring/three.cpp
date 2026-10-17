// Differential check for the native src/math/Spherical.js, Cylindrical.js and Box2.js ports.
//
// The two coordinate systems are driven through their conversions, including a point at the pole where the
// azimuth is not determined and a zero radius where makeSafe has to lift it. The box is driven through its
// queries, including a box whose min sits above its max and a point on the edge.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-spherical-cylindrical-box2.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Spherical}=await import(pathToFileURL(path.join(sourceDir,'src/math/Spherical.js')).href);
const {Cylindrical}=await import(pathToFileURL(path.join(sourceDir,'src/math/Cylindrical.js')).href);
const {Box2}=await import(pathToFileURL(path.join(sourceDir,'src/math/Box2.js')).href);
const {Vector2}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector2.js')).href);
const {Vector3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector3.js')).href);
const scenarios=[
    ['sph.setFromVector3',1,0,0],
    ['sph.setFromVector3',0,0,1],
    ['sph.setFromVector3',0,1,0],
    ['sph.setFromVector3',0,0,0],
    ['sph.makeSafe',0.1,0,0.5],
    ['sph.makeSafe',1,2,-1],
    ['cyl.setFromVector3',1,0,0],
    ['cyl.setFromVector3',0,0,0],
    ['cyl.setFromVector3',3,4,5],
    ['box2.getCenter',0,0, 2,2],
    ['box2.getCenter',1,1, 0,0],
    ['box2.getSize',0,0, 2,2],
    ['box2.containsPoint',0,0, 2,2, 1,1],
    ['box2.containsPoint',0,0, 2,2, 2,2],
    ['box2.containsPoint',0,0, 2,2, 3,3],
    ['box2.clampPoint',0,0, 2,2, 5,0],
    ['box2.distanceToPoint',0,0, 2,2, 5,3],
    ['box2.distanceToPoint',0,0, 2,2, 1,1],
    ['box2.intersectsBox',0,0, 2,2, 1,1, 3,3],
    ['box2.intersectsBox',0,0, 2,2, 5,5, 6,6],
    ['box2.expandByPoint',0,0, 2,2, 5,3]
];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const lines=scenarios.map(parts=>parts.join(' '));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
const call=(op,args)=>{
    if(op.startsWith('sph.')){
        const s=new Spherical(args[0],args[1],args[2]);
        if(op==='sph.setFromVector3')return new Spherical().setFromVector3(new Vector3(args[0],args[1],args[2]));
        s.makeSafe(); return s;
    }
    if(op.startsWith('cyl.'))return new Cylindrical().setFromVector3(new Vector3(args[0],args[1],args[2]));
    const p=(offset)=>new Vector2(args[offset],args[offset+1]);
    const box=new Box2(p(0),p(2));
    switch(op){
        case 'box2.getCenter': return box.getCenter(new Vector2());
        case 'box2.getSize': return box.getSize(new Vector2());
        case 'box2.containsPoint': return box.containsPoint(p(4));
        case 'box2.clampPoint': return box.clampPoint(p(4),new Vector2());
        case 'box2.distanceToPoint': return box.distanceToPoint(p(4));
        case 'box2.intersectsBox': return box.intersectsBox(new Box2(p(4),p(6)));
        case 'box2.expandByPoint': return box.expandByPoint(p(4)).max;
        default: throw new Error('unknown op '+op);
    }
};
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    const value=call(op,args);
    let expected;
    if(typeof value==='number')expected={value:encode(value)};
    else if(typeof value==='boolean')expected={value:value};
    else if(op.startsWith('sph.'))expected={radius:encode(value.radius),phi:encode(value.phi),theta:encode(value.theta)};
    else if(op.startsWith('cyl.'))expected={radius:encode(value.radius),theta:encode(value.theta),y:encode(value.y)};
    else expected={x:encode(value.x),y:encode(value.y)};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,args.join(' '));
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,`${op} #${index}`);
}
console.log(`Spherical/Cylindrical/Box2 parity: ${scenarios.length} scenarios`);