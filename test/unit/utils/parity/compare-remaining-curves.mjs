import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync,spawnSync} from 'node:child_process';

const upstream=path.resolve(process.argv[2]), executable=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
const commit=execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim();
if(commit!==lock.commit) throw new Error(`Expected ${lock.commit}, got ${commit}`);
const source = name => import(pathToFileURL(path.join(upstream,`src/${name}.js`)).href);
const {Vector2}=await source('math/Vector2'),{Vector3}=await source('math/Vector3');
const {EllipseCurve}=await source('extras/curves/EllipseCurve'),{ArcCurve}=await source('extras/curves/ArcCurve');
const {SplineCurve}=await source('extras/curves/SplineCurve'),{CatmullRomCurve3}=await source('extras/curves/CatmullRomCurve3');
const {CatmullRom,QuadraticBezier,CubicBezier}=await source('extras/core/Interpolations');
const token = x=>Object.is(x,-0)?'-0':String(x), lines=[], queries=[];
const attempt=fn=>{try{return fn()}catch(e){return {error:e.name}}};
const evaluate=(curve,t,alias=-1)=>{const target=alias<0?(curve instanceof CatmullRomCurve3?new Vector3():new Vector2()):curve.points[alias];const result=curve.getPoint(t,target);return {point:result.toArray(),sameTarget:result===target}};
function emit(kind,args,evaluate){lines.push(kind+' '+args.map(token).join(' '));queries.push(attempt(evaluate))}
const controlTokens=points=>points===null?[-1]:[points.length,...points.flatMap(p=>p===null?[0]:[1,...p])];
function spline(points,t,alias=-1){emit('S',[...controlTokens(points),t,alias],()=>evaluate(new SplineCurve(points===null?null:points.map(p=>p===null?null:new Vector2(...p))),t,alias))}
function catmull(points,closed,kind,tension,t,alias=-1){emit('C',[...controlTokens(points),+closed,kind,tension,t,alias],()=>evaluate(new CatmullRomCurve3(points===null?null:points.map(p=>p===null?null:new Vector3(...p)),closed,kind,tension),t,alias))}
const p3=[[-10,0,1],[-5,15,-2],[20,15,4],[10,0,8]],p2=p3.map(p=>p.slice(0,2));
// The first invalid type observes initially zero module polynomial coefficients.
catmull(p3,false,'invalid',.5,.25);
for(const kind of ['centripetal','chordal','catmullrom','invalid']) for(const closed of [false,true])
for(const t of [-2,-.25,-0,0,.1,.25,.5,.9,1,1.25,2,NaN,Infinity,-Infinity]) catmull(p3,closed,kind,.7,t);
for(const points of [null,[],[[1,2,3]],[[1,2,3],[4,5,6]],[[1,2,3],[1,2,3],[1,2,3]],[[0,0,0],[1e-12,0,0],[1,2,3]],[[0,0,0],null,[4,5,6]],[[NaN,1,2],[3,4,5],[6,7,8]],[[Infinity,0,0],[1,2,3],[4,5,6]]])
for(const kind of ['centripetal','chordal','catmullrom','invalid']) for(const closed of [false,true])
for(const t of [0,.5,1,NaN]) catmull(points,closed,kind,.5,t);
for(const tension of [-2,-0,0,.5,2,NaN,Infinity]) for(const t of [0,.3,1]) catmull(p3,false,'catmullrom',tension,t);
for(const t of [-1,-.1,-0,0,.25,.5,1,1.1,1.5,2,NaN,Infinity,-Infinity]) spline(p2,t);
for(const points of [null,[],[[1,2]],[[1,2],[3,4]],[[1,2],[1,2],[1,2]],[[0,0],null,[4,5]],[[NaN,1],[3,4],[6,7]],[[Infinity,0],[1,2],[4,5]]])
for(const t of [0,.5,1,NaN]) spline(points,t);
for(let alias=0;alias<4;++alias){spline(p2,.3,alias);catmull(p3,false,'centripetal',.5,.3,alias)}
const pi=Math.PI;
for(const [start,end] of [[0,2*pi],[0,0],[1,1+Number.EPSILON/2],[0,Number.EPSILON],[2*pi,0],[-7*pi,9*pi],[0,-4*pi],[NaN,1],[1,NaN]])
for(const clockwise of [false,true]) for(const rotation of [0,.7,NaN]) for(const t of [-.2,0,.3,1,1.2,NaN]){
const args=[2,-3,5,2,start,end,+clockwise,rotation,t];emit('E',args,()=>evaluate(new EllipseCurve(2,-3,5,2,start,end,clockwise,rotation),t))}
for(const [x,y,rx,ry] of [[0,0,0,0],[-0,-0,-0,-0],[Infinity,1,2,3],[2,NaN,3,4],[1,2,Infinity,-Infinity],[1e200,-1e200,1e200,1e200]])
for(const t of [0,.25,.5,1,Infinity])emit('E',[x,y,rx,ry,0,2*pi,0,0,t],()=>evaluate(new EllipseCurve(x,y,rx,ry),t));
for(const clockwise of [false,true])for(const t of [0,.2,.5,1,NaN])emit('A',[2,-3,5,-.4,7.2,+clockwise,t],()=>evaluate(new ArcCurve(2,-3,5,-.4,7.2,clockwise),t));
for(const t of [-1,-0,0,.3,1,2,NaN,Infinity])for(const p of [[-3,4,7,-2],[1e200,-1e200,1e-200,-1e-200],[NaN,1,2,3],[0,0,0,Infinity]])
emit('I',[t,...p],()=>[CatmullRom(t,...p),QuadraticBezier(t,...p.slice(0,3)),CubicBezier(t,...p)]);
let seed=0x11738a9d;const random=()=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296};
for(let i=0;i<240;++i){const points=Array.from({length:2+i%7},()=>[random()*40-20,random()*40-20,random()*40-20]);const t=random();catmull(points,i%2===0,['centripetal','chordal','catmullrom'][i%3],random()*2-1,t);spline(points.map(p=>p.slice(0,2)),t)}

function make(C){if(C===EllipseCurve)return new C(2,-3,5,2,-.4,7.2,true,.7);if(C===ArcCurve)return new C(2,-3,5,-.4,7.2,true);return new C(p3.map(p=>C===SplineCurve?new Vector2(...p):new Vector3(...p)))}
function scenario(C){const curve=make(C),result={defaults:new C().toJSON(),json:curve.toJSON()};
result.points=curve.getPoints(4).map(p=>p.toArray());result.spaced=curve.getSpacedPoints(4).map(p=>p.toArray());
result.length=curve.getLength();result.lengths=[...curve.getLengths(4)];result.pointAt=curve.getPointAt(.3).toArray();
result.tangent=curve.getTangent(.25).toArray();result.tangentAt=curve.getTangentAt(.75).toArray();
const target=C===CatmullRomCurve3?new Vector3():new Vector2();result.targets=[curve.getPoint(.3,target)===target,curve.getPointAt(.3,target)===target,curve.getTangent(.3,target)===target,curve.getTangentAt(.3,target)===target];
const clone=curve.clone();result.clone={type:clone.type,cacheNull:clone.cacheArcLengths===null,json:clone.toJSON()};
const destination=make(C);destination.arcLengthDivisions=5;const cache=destination.getLengths();result.copyIdentity=destination.copy(curve)===destination;result.copyCache=destination.cacheArcLengths===cache;result.copyJson=destination.toJSON();result.fromIdentity=destination.fromJSON(curve.toJSON())===destination;result.restored=destination.toJSON();
if('points'in curve){result.independent=clone.points!==curve.points&&clone.points.every((p,i)=>p!==curve.points[i]);const alias=make(C),shared=alias.points,second=new C(shared);shared[0].setScalar(7);shared.push(C===CatmullRomCurve3?new Vector3():new Vector2());result.sharedArray=alias.points===second.points;result.sharedMutation=second.toJSON();const pointAlias=shared[1];result.pointAlias=alias.getPoint(.3,pointAlias).toArray();result.aliasStored=second.points[1].toArray();const self=make(C),previous=self.points;self.copy(self);result.selfCopy=self.toJSON();result.selfOldLength=previous.length;const repeated=C===CatmullRomCurve3?new Vector3():new Vector2(),unique=new C([repeated,repeated]).clone();result.duplicateCloneIndependent=unique.points[0]!==unique.points[1]}
if(C===ArcCurve){curve.yRadius=8;curve.aRotation=.9;result.arcInheritedFields=curve.clone().toJSON()}
return result}
const expected={queries};for(const C of [EllipseCurve,ArcCurve,SplineCurve,CatmullRomCurve3])expected[C.name]=scenario(C);
const normalize=value=>{if(typeof value==='number'){if(Number.isNaN(value))return'NaN';if(value===Infinity)return'Infinity';if(value===-Infinity)return'-Infinity';if(Object.is(value,-0))return'-0';return value}if(Array.isArray(value))return value.map(normalize);if(value&&typeof value==='object')return Object.fromEntries(Object.entries(value).map(([k,v])=>[k,normalize(v)]));return value};
const run=spawnSync(executable,[],{input:lines.join('\n')+'\n',encoding:'utf8',maxBuffer:32*1024*1024});
if(run.error)throw run.error;if(run.status!==0)throw new Error(`Probe exited ${run.status}: ${run.stderr}`);
const actual=JSON.parse(run.stdout),reference=normalize(expected);let comparisons=0;const failures=[];
function compare(a,b,where){++comparisons;if(typeof a==='number'&&typeof b==='number'){if(Math.abs(a-b)>1e-11*Math.max(1,Math.abs(a),Math.abs(b)))failures.push({where,cpp:a,js:b});return}if(Array.isArray(a)&&Array.isArray(b)){if(a.length!==b.length){failures.push({where,cpp:a.length,js:b.length});return}a.forEach((v,i)=>compare(v,b[i],`${where}[${i}]`));return}if(a&&b&&typeof a==='object'&&typeof b==='object'){const ak=Object.keys(a),bk=Object.keys(b);if(JSON.stringify(ak)!==JSON.stringify(bk)){failures.push({where,cpp:ak,js:bk});return}for(const k of ak)compare(a[k],b[k],`${where}.${k}`);return}if(a!==b)failures.push({where,cpp:a,js:b})}
compare(actual,reference,'remainingCurves');if(failures.length){console.error(JSON.stringify(failures.slice(0,30),null,2));throw new Error(`${failures.length} remaining curve mismatches`)}
console.log(`Remaining curve parity: ${queries.length} queries, ${comparisons} comparisons passed against ${commit}.`);
