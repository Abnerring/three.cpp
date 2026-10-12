import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync, spawnSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

if (process.argv.length !== 4) throw new Error('Usage: node compare-primitive-geometries.mjs <three.js> <probe.exe>');
const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const names = ['Box', 'Plane', 'Circle', 'Ring', 'Sphere', 'Cylinder', 'Cone', 'Torus', 'TorusKnot', 'Lathe', 'Capsule'].map(s => s + 'Geometry');
const classes = Object.fromEntries(await Promise.all(names.map(async name => [name,
    (await import(pathToFileURL(path.join(upstream, 'src/geometries', name + '.js'))))[name]])));
const number = n => Number.isNaN(n) ? 'NaN' : n === Infinity ? 'Infinity' : n === -Infinity ? '-Infinity' : Object.is(n, -0) ? '-0' : n;
const encode = value => typeof value === 'number' ? number(value) : Array.isArray(value) ? value.map(encode) :
    value && typeof value === 'object' ? Object.fromEntries(Object.entries(value).map(([k,v])=>[k,encode(v)])) : value;
const serialized = geometry => { const data = geometry.toJSON(); delete data.uuid; return encode(data); };
const state = geometry => ({ type: geometry.type, keys: Object.keys(geometry.attributes),
    attributes: Object.fromEntries(Object.entries(geometry.attributes).map(([k,a])=>[k,encode(a.toJSON())])),
    index: encode(geometry.index.toJSON()), groups: geometry.groups.map(g=>[g.start,g.count,g.materialIndex]), json: serialized(geometry) });
const fixtures = [];
const add = (kind,...args) => fixtures.push({ kind:kind+'Geometry', args });
for (const name of names) fixtures.push({kind:name,args:[]});
const pi=Math.PI;
for (const args of [[2,3,4,2.8,1.5,3.9],[-1,0,2,0,1,0],[1,2,3,NaN,2,1],[1,2,3,1,NaN,1],
    [1,2,3,2,1,NaN],[1,2,3,-Infinity,2,1],[1,2,3,1,-Infinity,2],[1,2,3,-1,-1,2]]) add('Box',...args);
for (const args of [[2,3,2.8,3.5],[0,-1,0,0],[2,3,NaN,1],[2,3,1,NaN],[1,2,-Infinity,1],
    [1,2,1,-Infinity],[1,2,Infinity,-1],[1,2,-1,-1]]) add('Plane',...args);
for (const args of [[2,3.5,.2,3.7],[0,0,0,2*pi],[-2,5,-.2,-3],[1,NaN,0,1],[NaN,3,0,1],[1,-Infinity,0,2*pi]]) add('Circle',...args);
for (const args of [[.3,2,3.5,1.5,.2,4],[0,0,4,1,0,2*pi],[2,-1,4,2,1,-3],[1,2,NaN,1,0,1],[1,2,3,NaN,0,1]]) add('Ring',...args);
for (const args of [[2,5.8,3.8,.3,4,.4,2],[-2,4,3,0,2*pi,0,pi],[0,3,2,0,2*pi,0,pi],[1,NaN,2,0,2*pi,0,pi],[1,3,NaN,0,2*pi,0,pi],[1,4,3,0,2*pi,2,3]]) add('Sphere',...args);
for (const args of [[1,2,3,4.8,2.8,0,.2,4],[0,2,3,4,2,0,0,2*pi],[2,0,3,4,2,0,0,2*pi],[0,0,0,3,1,0,0,2*pi],[1,1,1,0,0,0,0,2*pi],[1,1,1,-1,-1,0,0,2*pi],[1,1,1,NaN,2,0,0,2*pi],[1,1,1,4,NaN,0,0,2*pi],[1,-2,-3,5,2,1,-1,-3]]) add('Cylinder',...args);
for (const args of [[2,3,4.8,2.8,0,.2,4],[-2,-3,4,2,0,0,2*pi],[0,0,0,0,1,0,1],[1,1,NaN,1,0,0,2*pi]]) add('Cone',...args);
for (const args of [[2,.7,4.8,6.8,3,.2,4],[1,0,3,4,2*pi,0,2*pi],[-1,-.4,3,4,-2,2,-3],[1,.4,0,0,2*pi,0,2*pi],[1,.4,-1,2,2*pi,0,2*pi],[1,.4,NaN,3,2*pi,0,2*pi]]) add('Torus',...args);
for (const args of [[2,.7,9.8,4.8,3,5],[1,0,3,2,2,3],[1,.4,0,0,2,3],[1,.4,-1,2,2,3],[1,.4,3,2,0,3],[1,.4,3,2,2.5,-1.5],[1,.4,NaN,2,2,3]]) add('TorusKnot',...args);
for (const args of [[4.8,.3,4,1,0,3,4],[3,0,2*pi],[3,0,2*pi,1,1],[0,0,2*pi,1,0,2,1],[3,.2,-3,1,0,2,2,0,4],[4,0,20,-1,-2,1,0,1,3,0,4],[NaN,0,2*pi,0,0,1,1]]) add('Lathe',...args);
for (const args of [[2,3,2.8,5.8,2.8],[1,-2,0,2,-1],[0,0,1,3,1],[-1,2,2,3,1],[1,NaN,1,3,1],[1,1,NaN,3,1],[1,1,1,NaN,1]]) add('Capsule',...args);
let seed=0x132ac521;
const rand=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/2**32;};
for(let i=0;i<36;i++) {
    const r=rand()*4-1, h=rand()*4-1, seg=3+rand()*5, a=rand()*6-2, b=rand()*7-1;
    switch(i%9) {
    case 0:add('Circle',r,seg,a,b);break;
    case 1:add('Ring',r,h,seg,1+rand()*2,a,b);break;
    case 2:add('Sphere',r,seg,2+rand()*3,a,b,a,b);break;
    case 3:add('Cylinder',r,h,rand()*3,seg,1+rand()*2,i%2,a,b);break;
    case 4:add('Cone',r,h,seg,1+rand()*2,i%2,a,b);break;
    case 5:add('Torus',r,h,seg,seg+2,a,a,b);break;
    case 6:add('TorusKnot',r,h,seg+4,seg,2,3);break;
    case 7:add('Lathe',seg,a,b,r,0,h,2,r/2,3);break;
    case 8:add('Capsule',r,h,1+rand()*3,seg,1+rand()*2);break;
    }
}
// Full arrays across the primitive-restart boundary verify Uint32 selection too.
add('Circle',1,65535,0,2*pi);
const construct = ({kind,args}) => {
    const G=classes[kind];
    if(!args.length) return new G();
    if(kind==='LatheGeometry') {
        const points=[];
        for(let i=3;i+1<args.length;i+=2) points.push({x:args[i],y:args[i+1]});
        return new G(points,...args.slice(0,3));
    }
    const values=[...args];
    if(kind==='CylinderGeometry') values[5]=Boolean(values[5]);
    if(kind==='ConeGeometry') values[4]=Boolean(values[4]);
    return new G(...values);
};
const trace = fixture => {
    try {
        const G=classes[fixture.kind], geometry=construct(fixture), original=state(geometry);
        const clone=geometry.clone(), copied=new G();
        const copyReturn=copied.copy(geometry)===copied;
        const restored=G.fromJSON(geometry.toJSON()), defaults=G.fromJSON({});
        const result={geometry:original,cloneEqual:JSON.stringify(state(clone))===JSON.stringify(original),
            copyEqual:JSON.stringify(state(copied))===JSON.stringify(original),copyReturn,
            cloneUuid:clone.uuid!==geometry.uuid,cloneIndex:clone.index!==geometry.index,
            cloneAttribute:clone.attributes.position!==geometry.attributes.position,
            roundTripEqual:JSON.stringify(state(restored))===JSON.stringify(original),defaults:serialized(defaults)};
        if(fixture.kind==='LatheGeometry') result.pointsShared=geometry.parameters.points===clone.parameters.points;
        geometry.translate(.25,-.5,1);
        result.transformed=serialized(geometry);
        result.transformedCloneEqual=JSON.stringify(serialized(geometry.clone()))===JSON.stringify(serialized(geometry));
        return result;
    } catch(error) {return {error:error.name};}
};
const expected=fixtures.map(trace);
const input=fixtures.map(({kind,args})=>kind+' '+args.map(n=>String(number(n))).join(' ')).join('\n')+'\n';
const result=spawnSync(process.argv[3],[],{input,encoding:'utf8',maxBuffer:128*1024*1024});
if(result.error) throw result.error;
assert.equal(result.status,0,result.stderr);
const actual=result.stdout.trim().split(/\r?\n/).map(line=>JSON.parse(line));
assert.equal(actual.length,expected.length);
let comparisons=0;
const compare=(a,b,where)=>{
    ++comparisons;
    if(typeof a==='number' && typeof b==='number') {
        const isFloatArray=/\.(?:position|normal|uv)\.array\[/.test(where);
        if(isFloatArray) assert.ok(Math.abs(a-b)<=2e-6*Math.max(1,Math.abs(a),Math.abs(b)),`${where}: ${a} != ${b}`);
        else assert.equal(a,b,where);
    } else if(Array.isArray(a)&&Array.isArray(b)) {
        assert.equal(a.length,b.length,where+'.length');
        for(let i=0;i<a.length;i++)compare(a[i],b[i],`${where}[${i}]`);
    } else if(a&&b&&typeof a==='object'&&typeof b==='object') {
        assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),where+'.keys');
        for(const key of Object.keys(a))compare(a[key],b[key],where+'.'+key);
    } else assert.equal(a,b,where);
};
for(let i=0;i<expected.length;i++) compare(actual[i],expected[i],`${i}:${fixtures[i].kind}`);
console.log(`Primitive geometry parity: ${fixtures.length} cases / ${comparisons} comparisons passed against ${lock.commit}`);
