import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync,spawnSync} from 'node:child_process';
import {strict as assert} from 'node:assert';

const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
const commit=execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim();
assert.equal(commit,lock.commit);
const {default:earcut,deviation,flatten}=await import(pathToFileURL(path.join(upstream,'src/extras/lib/earcut.js')).href);
const token=x=>Object.is(x,-0)?'-0':String(x),lines=[],expected=[];
const normalize=v=>{if(typeof v==='number'){if(Number.isNaN(v))return'NaN';if(v===Infinity)return'Infinity';if(v===-Infinity)return'-Infinity';if(Object.is(v,-0))return'-0';return v}if(Array.isArray(v))return v.map(normalize);if(v&&typeof v==='object')return Object.fromEntries(Object.entries(v).map(([k,e])=>[k,normalize(e)]));return v};
const attempt=fn=>{try{return normalize(fn())}catch(e){return{error:e.name}}};
function test(data,holes=[],dim=2){lines.push(['T',dim,data.length,...data,holes.length,...holes].map(token).join(' '));expected.push(attempt(()=>{const triangles=earcut(data,holes,dim);return{triangles,deviation:deviation(data,holes,dim,triangles)}}))}
function polygon(rings,dim=2){const data=[],holes=[];for(let i=0;i<rings.length;++i){if(i)holes.push(data.length/dim);for(const point of rings[i])data.push(...point)}test(data,holes,dim)}
function flat(rings){lines.push(['F',rings.length,...rings.flatMap(ring=>[ring.length,...ring.flatMap(p=>[p.length,...p])])].map(token).join(' '));expected.push(attempt(()=>flatten(rings)))}
const square=[[0,0],[10,0],[10,10],[0,10]],hole=[[2,2],[2,8],[8,8],[8,2]];
for(const outer of [square,[...square].reverse(),[...square,square[0]],[[0,0],[5,0],[10,0],[10,10],[0,10]],[[0,0],[10,10],[0,10],[10,0]],[[0,0],[4,0],[4,4],[2,1],[0,4]],[[0,0],[0,0],[0,0]],[[0,0],[1,1],[2,2],[3,3]],[[0,0]],[],[[0,0],[1,1]]]){
polygon([outer]);polygon([outer,hole]);
}
for(const holes of [[hole],[hole,[[-3,1],[-2,1],[-2,2]]],[[[2,2]]],[[[0,0]]],[[[0,0],[2,1],[1,2]]],[[[2,2],[4,2],[3,4]],[[2,2],[3,0],[4,2]]],[[[2,2],[2,4],[4,4],[4,2]],[[2,4],[2,6],[4,6],[4,4]]],[[[2,2],[2,2],[2,2]]]])polygon([square,...holes]);
for(const invalid of [[],[4],[5],[4,4],[4,3],[0],[1],[2]])test(square.flat(),invalid);
for(const data of [[],[0],[0,0],[0,0,1],[0,0,1,1,2],[NaN,0,1,0,1,1,0,1],[0,NaN,1,0,1,1,0,1],[Infinity,0,1,0,1,1,0,1],[0,0,1,0,Infinity,1,0,1],[-0,-0,1,-0,1,1,-0,1]])for(const dim of [1,2,3,4])test(data,[],dim);
const ring=(count,radius=10,cx=0,cy=0,phase=0)=>Array.from({length:count},(_,i)=>[cx+radius*Math.cos(phase+i*2*Math.PI/count),cy+radius*Math.sin(phase+i*2*Math.PI/count)]);
for(const count of [3,4,5,20,79,80,81,82,120,257]){
const outer=ring(count);polygon([outer]);polygon([[...outer].reverse()]);polygon([outer,ring(9,2)]);polygon([outer,ring(12,1,-4,0),ring(14,1,4,0)]);
polygon([outer.map(([x,y],i)=>[x,y,i])],3);
for(const scale of [1e-100,1e100])polygon([outer.map(([x,y])=>[x*scale,y*scale])]);
}
for(const x of [NaN,Infinity,-Infinity]){const p=ring(100);p[2][0]=x;polygon([p]);const q=ring(100);q[0][1]=x;polygon([q])}
// Large hole queues exercise stable ordering, including shared leftmost points.
for(const count of [2,3,8,32,64,100]){const holes=[];for(let i=count-1;i>=0;--i)holes.push(ring(4,.15,-8+16*(i%10)/10,-8+16*Math.floor(i/10)/10));polygon([ring(100,20),...holes])}
// Previously failing nontransitive comparison: multiple NaN holes interleaved
// with descending finite x coordinates exercise V8's initial run detection.
test([0,0,100,0,100,100,0,100,NaN,34,NaN,34,NaN,35,70,85,71,85,70,86,
    NaN,34,NaN,34,NaN,35,57,81,58,81,57,82,26,3,27,3,26,4,
    56,21,57,21,56,22,63,60,64,60,63,61,15,22,16,22,15,23],
    [4,7,10,13,16,19,22,25]);
let seed=0x11738a9d;const random=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296};
for(let k=0;k<100;k++){
    const holes=[];
    for(let i=0;i<3+k%70;i++){
        const x=i%7===1?NaN:1+Math.floor(random()*95),y=1+Math.floor(random()*95);
        holes.push([[x,y],[x+1,y],[x,y+1]]);
    }
    polygon([[[0,0],[100,0],[100,100],[0,100]],...holes]);
}
for(let k=0;k<300;++k){const count=3+k%110,points=[];for(let i=0;i<count;++i){const radius=3+random()*7,angle=i*2*Math.PI/count;points.push([Math.cos(angle)*radius,Math.sin(angle)*radius])}if(k%3===0)points.reverse();if(k%7===0)points.splice(2,0,[...points[2]]);if(k%5===0)polygon([points,ring(3+k%8,1)]);else polygon([points]);}
// Self-intersections and repeated vertices activate local curing and polygon splitting.
for(let k=0;k<160;++k){const count=4+k%25,points=[];for(let i=0;i<count;++i)points.push([Math.floor(random()*15),Math.floor(random()*15)]);polygon([points])}
for(const rings of [[],[[]],[[[0,0],[1,0],[0,1]]],[square,hole],[[[0,0,1],[1,0,2],[0,1,3]],[[.2,.2,4]]],[square,[],hole],[[[]],[[]]]])flat(rings);
const run=spawnSync(binary,[],{input:lines.join('\n')+'\n',encoding:'utf8',maxBuffer:32*1024*1024});
if(run.error)throw run.error;if(run.status!==0)throw new Error(`Probe exited ${run.status}: ${run.stderr}`);
const actual=JSON.parse(run.stdout);assert.equal(actual.length,expected.length);let indices=0;const failures=[];
for(let i=0;i<expected.length;++i){const a=actual[i],b=expected[i];indices+=b.triangles?.length??0;try{assert.deepEqual(a.triangles,b.triangles);if(typeof a.deviation==='number'&&typeof b.deviation==='number')assert(Math.abs(a.deviation-b.deviation)<=1e-12*Math.max(1,Math.abs(a.deviation),Math.abs(b.deviation)));else assert.deepEqual(a.deviation,b.deviation);if(!('triangles'in b))assert.deepEqual(a,b)}catch{failures.push({i,input:lines[i],cpp:a,js:b})}}
if(failures.length){console.error(JSON.stringify(failures.slice(0,8),null,2));throw new Error(`${failures.length} earcut mismatches`)}
console.log(`Earcut parity: ${expected.length} cases and ${indices} exact triangle indices passed against ${commit}.`);
