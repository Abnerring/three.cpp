import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const [upstream,probe,oraclePath]=process.argv.slice(2),lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url),'utf8'));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=rel=>import(pathToFileURL(path.join(upstream,'src',rel)).href);
const [{ShapeUtils},{Earcut},{Vector2}]=await Promise.all([load('extras/ShapeUtils.js'),load('extras/Earcut.js'),load('math/Vector2.js')]);
const outcome=(fn,reason)=>{try{return fn();}catch(e){return e instanceof TypeError?'TypeError':e===reason?'reason':'other';}};
const getter=(o,k,get)=>Object.defineProperty(o,k,{get,configurable:true});
function metadata(){const methods={};for(const name of ['area','isClockWise','triangulateShape']){const d=Object.getOwnPropertyDescriptor(ShapeUtils,name),f=d.value;methods[name]={name:f.name,length:f.length,source:f.toString(),flags:[d.writable,d.enumerable,d.configurable]};}return {name:ShapeUtils.name,length:ShapeUtils.length,source:ShapeUtils.toString(),instancePrototype:Object.getPrototypeOf(new ShapeUtils())===ShapeUtils.prototype,call:outcome(()=>ShapeUtils()),methods};}
function makePoints(rows,alias){const out=[];for(const row of rows){const p=new Vector2(...row);out.push(alias&&out.length>0&&p.equals(out[0])?out[0]:p);}return out;}
const coordinates=points=>points.map(p=>[p.x,p.y]);
function numeric(request){const contour=makePoints(request.contour,request.alias),holes=request.holes.map(rows=>request.shared?contour:makePoints(rows,request.alias)),original=contour;return {area:ShapeUtils.area(contour),clockwise:ShapeUtils.isClockWise(contour),faces:ShapeUtils.triangulateShape(contour,holes),sameContour:contour===original,contour:coordinates(contour),holes:holes.map(coordinates)};}
function areaOrder(variant){const trace=[],reason={},contour={};getter(contour,'length',()=>{trace.push('length');if(variant==='lengthObject')return {valueOf(){trace.push('length:valueOf');return 3;}};return 3;});for(let i=0;i<3;i++){const point={};for(const axis of ['x','y'])getter(point,axis,()=>{const name=`${i}${axis}`;trace.push(`get:${name}`);if(variant==='throw'&&name==='0y')throw reason;if(variant==='reenter'&&name==='0y'){trace.push('reenter');ShapeUtils.area([]);}return {valueOf(){trace.push(`value:${name}`);return axis==='x'?i:i*i;}};});contour[i]=point;}return {result:outcome(()=>ShapeUtils.area(contour),reason),trace};}
function dynamicArea(variant){const old=Object.getOwnPropertyDescriptor(ShapeUtils,'area'),trace=[],reason={},argument={};getter(ShapeUtils,'area',function(){trace.push(['get:area',this===ShapeUtils]);return function(value){trace.push(['call:area',this===ShapeUtils,value===argument]);return {valueOf(){trace.push('valueOf');if(variant==='throw')throw reason;return -1;}};};});try{return {result:outcome(()=>ShapeUtils.isClockWise.call({},argument),reason),trace};}finally{Object.defineProperty(ShapeUtils,'area',old);}}
function triangulateOrder(variant){const old=Object.getOwnPropertyDescriptor(Earcut,'triangulate'),trace=[],reason={},coordinate={valueOf(){throw 'unexpected coordinate conversion';}};const makePoint=name=>{const p={};getter(p,'x',()=>{trace.push(`x:${name}`);return coordinate;});getter(p,'y',()=>{trace.push(`y:${name}`);return 1;});return p;};const a=makePoint('a'),b=makePoint('b'),last=makePoint('last'),contour=[a,b,last],holes=[];
 getter(last,'equals',function(){trace.push(['get:equals',this===last]);return function(value){trace.push(['equals',this===last,value===a]);if(variant==='equalsThrow')throw reason;return variant==='keep'?false:'truthy';};});
 const originalPop=contour.pop;contour.pop=function(){trace.push(['pop',this===contour]);return originalPop.call(this);};holes.forEach=function(callback){trace.push(['forEach',this===holes,callback.name,callback.length]);if(variant==='forEachThrow')throw reason;};
 getter(Earcut,'triangulate',function(){trace.push(['get:earcut',this===Earcut]);return function(vertices,indices){trace.push(['earcut',this===Earcut,vertices[0]===coordinate,vertices.length,indices.length]);if(variant==='earcutThrow')throw reason;const triangles=[0,1,2,3],originalSlice=triangles.slice;triangles.slice=function(i,end){trace.push(['slice',i,end]);return originalSlice.call(this,i,end);};return triangles;};});
 try{return {result:outcome(()=>ShapeUtils.triangulateShape(contour,holes),reason),length:contour.length,trace};}finally{Object.defineProperty(Earcut,'triangulate',old);}
}
function run({mode,variant='',...request}){if(mode==='metadata')return metadata();if(mode==='numeric')return numeric(request);if(mode==='areaOrder')return areaOrder(variant);if(mode==='dynamicArea')return dynamicArea(variant);if(mode==='triangulateOrder')return triangulateOrder(variant);if(mode==='nullish')return {undefined:outcome(()=>ShapeUtils.area()),null:outcome(()=>ShapeUtils.area(null)),empty:ShapeUtils.area({}),missingHoles:outcome(()=>ShapeUtils.triangulateShape([]))};throw Error(`unknown ${mode}`);}
const requests=[{mode:'metadata'},{mode:'nullish'}];
for(const variant of ['', 'lengthObject','throw','reenter'])requests.push({mode:'areaOrder',variant});
for(const variant of ['', 'throw'])requests.push({mode:'dynamicArea',variant});
for(const variant of ['', 'keep','equalsThrow','forEachThrow','earcutThrow'])requests.push({mode:'triangulateOrder',variant});
const square=[[0,0],[10,0],[10,10],[0,10]],hole=[[2,2],[2,4],[4,4],[4,2]];
for(const contour of [[],[[1,1]],[[0,0],[1,0]],square,[...square].reverse(),[[0,0],[4,0],[4,4],[2,1],[0,4]],[[0,0],[1,0],[2,0],[3,0]]])requests.push({mode:'numeric',contour,holes:[]});
for(const alias of [false,true])for(const holes of [[],[hole],[hole,[[6,6],[6,8],[8,8],[8,6]]]])requests.push({mode:'numeric',contour:[...square,square[0]],holes:holes.map(h=>[...h,h[0]]),alias});
requests.push({mode:'numeric',contour:[...square,square[0]],holes:[[]],shared:true});
const expected=requests.map(run);if(oraclePath)fs.writeFileSync(oraclePath,JSON.stringify({requests,expected},null,2));if(probe==='--source-only'){console.log(`ShapeUtils source oracle passed: ${requests.length} scenarios`);process.exit(0);}
const stdout=execFileSync(path.resolve(probe),{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:16*1024*1024}),actual=JSON.parse(stdout.trim().split('\n').at(-1));
for(let i=0;i<requests.length;i++)assert.deepEqual(actual[i],JSON.parse(JSON.stringify(expected[i])),JSON.stringify(requests[i]));assert.equal(actual.length,expected.length);console.log(`ShapeUtils parity passed: ${requests.length} scenarios`);
