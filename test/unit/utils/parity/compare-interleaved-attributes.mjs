import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node --expose-gc compare-interleaved-attributes.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]),lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const T=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const constructors=[Int8Array,Uint8Array,Int16Array,Uint16Array,Int32Array,Uint32Array,Float32Array,Float64Array,Uint8ClampedArray];
const number=value=>value===undefined?'#undefined':typeof value!=='number'?value:Number.isNaN(value)?'#NaN':value===Infinity?'#Infinity':value===-Infinity?'#-Infinity':Object.is(value,-0)?'#-0':value;
const values=array=>Array.from(array,number),vector=value=>value.toArray().map(number);
const attempt=callback=>{try{return callback();}catch(error){return error.name;}};
function make(kind){const array=new constructors[kind](24);for(let i=0;i<24;++i)array[i]=(i-8)*3.25;array.buffer._uuid='storage';return array;}
function attributeState(attribute){
    const result={keys:Object.keys(attribute)};for(const key of ['isBufferAttribute','isInterleavedBufferAttribute','name','count','itemSize','offset','normalized','needsUpdate'])result[key]=number(attribute[key]);
    result.components=[0,1,2,.5,-.5].map(index=>['getX','getY','getZ','getW'].map(name=>attempt(()=>number(attribute[name](index)))));
    result.values=values(attribute.array);return result;
}
function basicCase(kind,normalized){
    const array=make(kind),data=new T.InterleavedBuffer(array,6);data.uuid='buffer';const attr=new T.InterleavedBufferAttribute(data,3,1,normalized);attr.name='position';
    const result={initial:attributeState(attr),identity:[attr.data===data,attr.array===array]};
    const writes=[['setX',[.5,.3]],['setY',[1,-.7]],['setZ',[0,.25]],['setW',[2,1.5]],['setComponent',[1,1,.125]],['setXY',[0,.1,.2]],['setXYZ',[1,-.2,.3,.4]],['setXYZW',[2,.5,.6,.7,.8]]];
    result.writes=writes.map(([name,args])=>attempt(()=>attr[name](...args)===attr));result.after=attributeState(attr);
    for(const value of [true,1,false,true])attr.needsUpdate=value;result.version=data.version;result.deinterleavedJSON=attr.toJSON();
    const flat=attr.clone();result.flat=[flat.isBufferAttribute,number(flat.isInterleavedBufferAttribute),values(flat.array),flat.name,flat.normalized,flat.count];
    const meta={};result.json=attr.toJSON(meta);result.meta=meta;const cloneMeta={},copied=attr.clone(cloneMeta);result.clone=attributeState(copied);
    result.cloneIdentity=[copied.data!==data,copied.array!==array,copied.data.version===0];return result;
}
function transformCase(kind,operation){
    const data=new T.InterleavedBuffer(make(kind),6),attr=new T.InterleavedBufferAttribute(data,3,1,kind!==6&&kind!==7),matrix=new T.Matrix4().makeRotationY(.35);
    matrix.scale(new T.Vector3(1.2,.8,1.1));matrix.setPosition(2,-3,4);
    if(operation===0)attr.applyMatrix4(matrix);else if(operation===1)attr.applyNormalMatrix(new T.Matrix3().getNormalMatrix(matrix));else attr.transformDirection(matrix);
    return {data:values(data.array),version:data.version};
}
function bounds(geometry){geometry.computeBoundingBox();geometry.computeBoundingSphere();return {box:[vector(geometry.boundingBox.min),vector(geometry.boundingBox.max)],sphere:[vector(geometry.boundingSphere.center),number(geometry.boundingSphere.radius)]};}
function geometryCase(indexed,relative){
    const array=new Float32Array([-1,-1,0,0,0,1,0,0,1,-1,0,0,0,1,1,0,0,1,0,0,0,1,.5,1]);array.buffer._uuid='storage';
    const data=new T.InterleavedBuffer(array,8);data.uuid='buffer';const position=new T.InterleavedBufferAttribute(data,3,0),normal=new T.InterleavedBufferAttribute(data,3,3),uv=new T.InterleavedBufferAttribute(data,2,6);
    const geometry=new T.BufferGeometry();geometry.uuid='geometry';geometry.setAttribute('position',position).setAttribute('normal',normal).setAttribute('uv',uv);if(indexed)geometry.setIndex([2,0,1]);
    const morph=new T.InterleavedBuffer(new Float32Array([.1,.2,.3,9,.4,.5,.6,9,.7,.8,.9,9]),4);morph.uuid='morph';morph.array.buffer._uuid='morph-storage';geometry.morphAttributes.position=[new T.InterleavedBufferAttribute(morph,3,0)];geometry.morphTargetsRelative=relative;
    const result={before:bounds(geometry),json:geometry.toJSON()},clone=geometry.clone(),cp=clone.getAttribute('position'),cn=clone.getAttribute('normal');
    result.cloneIdentity=[cp.data===cn.data,cp.data!==data,cp.array!==array,cp!==position];cp.setX(0,42);result.cloneData=[cp.getX(0),position.getX(0),cn.array[0]];
    const matrix=new T.Matrix4().makeRotationZ(.2);matrix.setPosition(1,2,3);geometry.applyMatrix4(matrix);result.after=bounds(geometry);result.data=values(array);result.version=data.version;
    const expanded=geometry.toNonIndexed();result.expandedSame=expanded===geometry;if(indexed){result.expanded=expanded.getAttribute('position').toJSON();result.expandedUV=expanded.getAttribute('uv').toJSON();}
    geometry.computeVertexNormals();result.normals=values(array);result.normalVersion=data.version;
    const mesh=new T.Mesh(geometry);mesh.updateMatrixWorld(true);const hits=[];mesh.raycast(new T.Raycaster(new T.Vector3(1,2,10),new T.Vector3(0,0,-1)),hits);
    result.hits=hits.map(hit=>({distance:hit.distance,point:vector(hit.point),...(hit.uv?{uv:hit.uv.toArray()}: {})}));return result;
}
const getter=(target,name,log,value)=>Object.defineProperty(target,name,{configurable:true,enumerable:true,get(){log.push(name);return value;}});
function orderCase(){
    const result={};for(const name of ['getX','getY','getComponent','setX','setY','setComponent','setXY','setXYZ']){
        const log=[],data={},array=make(1);getter(data,'array',log,array);getter(data,'stride',log,6);const attr=new T.InterleavedBufferAttribute(data,3,1,true);
        getter(attr,'data',log,data);getter(attr,'offset',log,1);getter(attr,'normalized',log,true);
        const value=attempt(()=>{const value=attr[name](1,2,.3,.4);return value===attr?'self':number(value);});result[name]=[value,log,values(array)];}
    for(const operation of ['clone','toJSON']){
        const log=[],data=new T.InterleavedBuffer(make(6),6);data.uuid='buffer';const value=new T.InterleavedBufferAttribute(data,3,1),meta={},cache={};
        getter(meta,'interleavedBuffers',log,cache);getter(value,'data',log,data);getter(value,'itemSize',log,3);getter(value,'offset',log,1);getter(value,'normalized',log,false);value[operation](meta);result[operation]=log;}
    return result;
}
function edgeCase(){
    const result={},data=new T.InterleavedBuffer(make(6),6),attr=new T.InterleavedBufferAttribute(data,3,'1');
    result.stringOffset=[number(attr.getX(0)),number(attr.getY(0)),number(attr.getComponent(0,'2'))];attr.setXY(0,7,8);result.stringWrites=values(data.array);
    attr.offset=1;data.count=1.5;result.changedCount=attr.count;data.array=new Float32Array([1,2,3,4]);result.changedArray=values(attr.array);result.readonly=attempt(()=>{attr.count=9;return 'ok';});
    const absent=new T.InterleavedBufferAttribute();result.missingCount=attempt(()=>absent.count);result.missingArray=attempt(()=>absent.array);result.nullMeta=attempt(()=>attr.clone(null));
    const regular=new T.InterleavedBufferAttribute(new T.InterleavedBuffer(make(6),6),3,1);regular.array.constructor={};regular.normalized=true;result.customCtorNormalization=attempt(()=>regular.getX(0));
    const object={offset:9},raw=new T.InterleavedBufferAttribute(new T.InterleavedBuffer(make(6),6),3,1,object);result.normalizedIdentity=raw.clone().normalized===object;
    raw.itemSize='3';const stringClone=raw.clone();result.rawItemSize=[stringClone.itemSize,stringClone.toJSON().itemSize,stringClone.count];return result;
}
function transformOrderCase(operation,reentrant){
    const log=[],state={coercions:0,entered:false},count={valueOf(){const n=++state.coercions;log.push(`count:${n}`);return n<=2?2:0;}};
    const data=new T.InterleavedBuffer(new Float32Array([1,2,3,4,5,6]),3);data.count=count;
    const attribute=new T.InterleavedBufferAttribute(data,3,0),nested=new T.InterleavedBufferAttribute(new T.InterleavedBuffer(new Float32Array([11,12,13]),3),3,0);
    const matrix={},elements=operation===1?[1,0,0,0,1,0,0,0,1]:[1,0,0,0,0,1,0,0,0,0,1,0,10,20,30,1];getter(matrix,'elements',log,elements);
    const name=['applyMatrix4','applyNormalMatrix','transformDirection'][operation];
    for(const component of ['getX','getY','getZ']){const original=attribute[component];attribute[component]=function(index){log.push(`${component}:${index}`);if(component==='getY'&&reentrant&&!state.entered){state.entered=true;nested[name](matrix);}return original.call(this,index);};}
    const setter=attribute.setXYZ;attribute.setXYZ=function(...args){log.push(['set',...args]);return setter.apply(this,args);};attribute[name](matrix);return [log,values(data.array),values(nested.array)];
}
function gcCase(){return {retained:true,identity:true,released:true};}
const expected={};
for(let kind=0;kind<9;++kind)for(const normalized of [false,true])expected[`basic:${kind}:${+normalized}`]=basicCase(kind,normalized);
for(const kind of [0,1,2,6,7])for(let op=0;op<3;++op)expected[`transform:${kind}:${op}`]=transformCase(kind,op);
for(const indexed of [false,true])for(const relative of [false,true])expected[`geometry:${+indexed}:${+relative}`]=geometryCase(indexed,relative);
for(let operation=0;operation<3;++operation)for(const reentrant of [false,true])expected[`transformOrder:${operation}:${+reentrant}`]=transformOrderCase(operation,reentrant);
Object.assign(expected,{order:orderCase(),edges:edgeCase(),gc:gcCase()});
const actual=JSON.parse(execFileSync(path.resolve(process.argv[3]),{encoding:'utf8',maxBuffer:32*1024*1024}));
let comparisons=0;
function compare(actual,expected,where='root'){
    ++comparisons;if(expected&&typeof expected==='object'){assert.deepEqual(Object.keys(actual),Object.keys(expected),`${where}: key order`);for(const key of Object.keys(expected))compare(actual[key],expected[key],`${where}.${key}`);}
    else if(typeof expected==='number'&&typeof actual==='number')assert.ok(Math.abs(expected-actual)<=1e-11*Math.max(1,Math.abs(expected)),`${where}: ${actual} !== ${expected}`);
    else assert.deepEqual(actual,expected,where);
}
compare(actual,JSON.parse(JSON.stringify(expected)));console.log(`Interleaved attribute parity: ${Object.keys(expected).length} scenarios / ${comparisons} comparisons`);
