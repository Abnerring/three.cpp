import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url),'utf8'));
const revision=spawnSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'});assert.equal(revision.status,0);assert.equal(revision.stdout.trim(),lock.commit);
const {WebGLAttributes}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLAttributes.js')));
const constructors=[Int8Array,Uint8Array,Int16Array,Uint16Array,Int32Array,Uint32Array,Float32Array,Float64Array,Uint8ClampedArray],typedKinds=new WeakMap();
const nativeWeakHas=WeakMap.prototype.has,nativeWeakGet=WeakMap.prototype.get,nativeWeakSet=WeakMap.prototype.set;
function encode(value){
    if(value===undefined)return 'undefined';if(typeof value==='number'){if(Number.isNaN(value))return 'NaN';if(!Number.isFinite(value))return value<0?'-Infinity':'Infinity';if(Object.is(value,-0))return '-0';return value;}
    if(value===null||typeof value==='boolean'||typeof value==='string')return value;if(typeof value==='function')return 'function';
    if(nativeWeakHas.call(typedKinds,value))return {typed:nativeWeakGet.call(typedKinds,value),values:Array.from(value,encode)};
    if(Array.isArray(value))return Array.from(value,encode);
    const result=Object.create(null);for(const key of Object.keys(value)){const d=Object.getOwnPropertyDescriptor(value,key);result[key]=Object.hasOwn(d,'value')?encode(d.value):'accessor';}return result;
}
function invoke(fn){try{return encode(fn());}catch(error){return error.name==='Error'?{error:'Error',message:error.message}:{error:error.name};}}
function typed(kind,length=8){const array=new constructors[kind](Array.from({length},(_,i)=>i+.25));nativeWeakSet.call(typedKinds,array,kind);return array;}
const range=(start,count)=>({start,count});
const scalars=()=>[undefined,null,false,true,0,-0,-1,1,2,10,'1','2','10','',NaN,Infinity,'😀','\uE000'];
class Fixture{
    constructor(kind=6,behavior=0){
        const state=this.state={log:[],behavior,nextBuffer:0,busy:false},attribute=this.attribute={},gl=this.gl={};state.gl=gl;state.attribute=attribute;
        for(const name of ['createBuffer','bindBuffer','bufferData','bufferSubData','deleteBuffer'])gl[name]=function(...args){
            state.log.push([name,this===gl,...args.map(encode)]);
            if(name==='createBuffer'){const n=++state.nextBuffer;return state.behavior===12?null:n;}
            if(name==='bufferData'&&state.behavior===11)attribute.array=typed(3);
            if(name==='bufferSubData'&&state.behavior===7)throw RangeError('subdata');
            if(name==='bufferSubData'&&state.behavior===15)attribute.updateRanges=[range(6,1)];
            if(name==='deleteBuffer'&&state.behavior===10&&!state.busy){state.busy=true;attribute.version=Number(attribute.version)+1;state.manager.update(attribute,99);state.busy=false;}
        };
        ['FLOAT','HALF_FLOAT','UNSIGNED_SHORT','SHORT','UNSIGNED_INT','INT','BYTE','UNSIGNED_BYTE'].forEach((name,i)=>gl[name]=100+i);
        const values=typed(kind);Object.assign(attribute,{array:values,usage:35044,version:0,updateRanges:[]});state.original=values;
        attribute.onUploadCallback=function(){state.log.push(['onUpload',this===attribute]);const behavior=state.behavior;
            if(behavior===1)attribute.version=Number(attribute.version)+1;if(behavior===2)attribute.array=typed(3);
            if(behavior===3)Object.setPrototypeOf(state.original,Uint16Array.prototype);if(behavior===4)throw RangeError('upload');
            if(behavior===5)state.manager.remove(attribute);
            if((behavior===6||behavior===14)&&!state.busy){state.busy=true;if(behavior===14)attribute.version=Number(attribute.version)+1;state.manager.update(attribute,77);state.busy=false;}
            if(behavior===16)attribute.isFloat16BufferAttribute=true;
        };
        attribute.clearUpdateRanges=function(){state.log.push(['clear',this===attribute]);if(state.behavior===8)throw RangeError('clear');this.updateRanges.length=0;if(state.behavior===9)this.version=Number(this.version)+1;};
        this.manager=state.manager=new WebGLAttributes(gl);
    }
    update(){return this.manager.update(this.attribute,34962);}cache(){return encode(this.manager.get(this.attribute));}trace(){return encode(this.state.log);}
}
function typedCase(kind,flag){const f=new Fixture(kind);if(flag)f.attribute.isFloat16BufferAttribute=flag===1?true:'half';const result=invoke(()=>f.update());return {kind:'typed',type:kind,flag,result,cache:f.cache(),trace:f.trace()};}
function versionCase(first,second,raw){const f=new Fixture(),values=scalars();f.attribute.version=values[first];if(raw)Object.assign(f.attribute,{isGLBufferAttribute:true,buffer:55,type:'kind',elementSize:'4'});
    const before=invoke(()=>f.update()),cache=f.manager.get(f.attribute);f.attribute.version=values[second];if(raw)f.attribute.buffer=56;const after=invoke(()=>f.update());
    return {kind:'version',first,second,raw,before,after,same:cache===f.manager.get(f.attribute),cache:f.cache(),trace:f.trace()};}
function rangeCase(scenario){const f=new Fixture();f.update();f.state.log.length=0;let ranges=[];
    switch(scenario){case 0:break;case 1:ranges=[range(6,1),range(1,2),range(3,2),range(7,1)];break;case 2:ranges=[range(1,1),range(3,1)];break;
    case 3:ranges=[range(0,1),range(3,1),range(6,1)];break;case 4:ranges=[range('0','1'),range('3','2'),range('6','1')];break;
    case 5:ranges=[range(-0,-0),range(0,0)];break;case 6:ranges=[range(NaN,2),range(1,2)];break;case 7:ranges=[range(2,-1),range(0,1),range(5,2)];break;
    case 8:ranges=[range(.5,.25),range(1.75,.5)];break;case 9:ranges=[range(1,2),undefined,range(0,1)];break;
    case 10:ranges=new Array(4);ranges[0]=range(3,1);ranges[3]=range(0,1);break;case 11:ranges=[range(5,1),range(0,1)];Object.preventExtensions(ranges);break;
    case 12:ranges=[range(0,1),range(0,2),range(0,3)];break;default:for(let i=0;i<40;i++)ranges.push(range((i*17+scenario)%71,i%3));}
    f.attribute.updateRanges=ranges;f.attribute.version=1;const result=invoke(()=>f.update());return {kind:'ranges',scenario,result,ranges:encode(ranges),cache:f.cache(),trace:f.trace()};}
function behaviorCase(behavior,update){const f=new Fixture(behavior===16?3:6,update?0:behavior);if(update){f.update();f.state.log.length=0;f.state.behavior=behavior;f.attribute.version=1;f.attribute.updateRanges=[range(0,1),range(5,1)];}
    const result=invoke(()=>f.update()),cached=f.cache();let removed='not-run';if(behavior===10)removed=invoke(()=>f.manager.remove(f.attribute));
    return {kind:'behavior',behavior,update,result,cached,removed,cache:f.cache(),version:encode(f.attribute.version),ranges:encode(f.attribute.updateRanges),trace:f.trace()};}
function interleavedCase(scenario){const f=new Fixture(),a={isInterleavedBufferAttribute:true,data:f.attribute},b={isInterleavedBufferAttribute:'yes',data:f.attribute};
    const result=invoke(()=>f.manager.update(a,34962)),first=f.manager.get(a),same=first===f.manager.get(b);if(scenario===1){f.attribute.version=1;f.manager.update(b,34962);}
    if(scenario===2){const nested={isInterleavedBufferAttribute:true,data:a};return {kind:'interleaved',scenario,result,same,nested:invoke(()=>f.manager.update(nested,34962)),trace:f.trace()};}
    const removed=invoke(()=>f.manager.remove(b));return {kind:'interleaved',scenario,result,same,removed,cache:f.cache(),trace:f.trace()};}
function resizeCase(scenario){const f=new Fixture();f.update();f.state.log.length=0;f.attribute.version=1;if(scenario===0)f.attribute.array=typed(6,9);if(scenario===1)f.attribute.array=typed(3,16);
    if(scenario>=2)Object.defineProperty(f.state.original,'byteLength',{value:scenario===2?'32':32,writable:true,configurable:true});const result=invoke(()=>f.update());return {kind:'resize',scenario,result,cache:f.cache(),trace:f.trace()};}
function watch(object,key,state,label){let value=object[key];Object.defineProperty(object,key,{enumerable:true,configurable:true,get(){state.log.push(['get',label,this===object]);return value;},set(next){state.log.push(['set',label,this===object,encode(next)]);value=next;}});}
function orderCase(scenario){const f=new Fixture(3);if(scenario){f.update();f.attribute.version=1;f.attribute.updateRanges=[range(5,1),range(0,1),range(1,1)];}
    if(scenario===2){const cached=f.manager.get(f.attribute);for(const name of ['version','size','buffer'])watch(cached,name,f.state,`cached.${name}`);}
    for(const name of ['isInterleavedBufferAttribute','isGLBufferAttribute','array','usage','version','onUploadCallback','updateRanges','clearUpdateRanges','isFloat16BufferAttribute'])watch(f.attribute,name,f.state,`attribute.${name}`);
    for(const name of ['createBuffer','bindBuffer','bufferData','bufferSubData','deleteBuffer','UNSIGNED_SHORT','HALF_FLOAT'])watch(f.gl,name,f.state,`gl.${name}`);
    for(const name of ['byteLength','BYTES_PER_ELEMENT'])watch(f.state.original,name,f.state,`array.${name}`);
    const ranges=f.attribute.updateRanges;for(let i=0;i<ranges.length;i++)for(const name of ['start','count'])watch(ranges[i],name,f.state,`range${i}.${name}`);
    f.state.log.length=0;const result=invoke(()=>f.update()),cached=f.cache();return {kind:'order',scenario,result,cache:cached,trace:f.trace()};}
function weakMethodCase(scenario){const f=new Fixture(),saved=new Map();try{
    for(const name of ['get','set','delete']){saved.set(name,Object.getOwnPropertyDescriptor(WeakMap.prototype,name));const original=WeakMap.prototype[name];
        const replacement=function(...args){f.state.log.push(['weak-call',name,this instanceof WeakMap]);if((scenario===1&&name==='get')||(scenario===3&&name==='set')||(scenario===5&&name==='delete'))throw RangeError('weak-call');
            if(scenario===7&&name==='get')return null;if(scenario===8&&name==='get')return false;return original.apply(this,args);};
        Object.defineProperty(WeakMap.prototype,name,[0,2,4,6].includes(scenario)?{configurable:true,get(){f.state.log.push(['weak-get',name,this instanceof WeakMap]);
            if((scenario===2&&name==='set')||(scenario===4&&name==='get'))throw RangeError('weak-get');if(scenario===6&&name==='set')return 17;return replacement;
        }}:{configurable:true,writable:true,value:replacement});}
    const result=invoke(()=>f.update()),removed=invoke(()=>f.manager.remove(f.attribute));return {kind:'weak-method',scenario,result,removed,trace:f.trace()};
    }finally{for(const[name,d]of saved)Object.defineProperty(WeakMap.prototype,name,d);}}
function pollutionCase(scenario){const f=new Fixture(),saved=new Map();try{
    for(const name of ['get','remove','update','buffer','type','bytesPerElement','version','size']){saved.set(name,Object.getOwnPropertyDescriptor(Object.prototype,name));
        Object.defineProperty(Object.prototype,name,scenario?{__proto__:null,configurable:true,value:77,writable:false}:{__proto__:null,configurable:true,get(){f.state.log.push(['pollution-get',name]);return 77;},set(){f.state.log.push(['pollution-set',name]);}});}
    f.manager=new WebGLAttributes(f.gl);f.state.manager=f.manager;const result=invoke(()=>f.update()),cached=f.cache(),descriptors=[];
    for(const name of Object.keys(f.manager)){const d=Object.getOwnPropertyDescriptor(f.manager,name);descriptors.push([name,!!d.writable,!!d.enumerable,!!d.configurable]);}
    return {kind:'pollution',scenario,result,cache:cached,descriptors,trace:f.trace()};
    }finally{for(const[name,d]of saved)if(d)Object.defineProperty(Object.prototype,name,d);else delete Object.prototype[name];}}
function convertValue(state,label,result,throws=false){return {[Symbol.toPrimitive](hint){state.log.push(['convert',label,hint]);if(throws)throw RangeError('convert');return result;}};}
function coercionCase(scenario){const f=new Fixture();if(scenario<=3){f.attribute.version=convertValue(f.state,'old',scenario===3?'2':0,scenario===1);f.update();f.state.log.length=0;f.attribute.version=convertValue(f.state,'new',scenario===3?'10':1,scenario===2);}
    else if(scenario<=9){f.update();f.state.log.length=0;f.attribute.version=1;
        const first=range(convertValue(f.state,'a.start',scenario===9?'0':0,scenario===4),convertValue(f.state,'a.count',scenario===9?'1':1,scenario===6));
        const second=range(convertValue(f.state,'b.start',scenario===9?'2':2,scenario===5),convertValue(f.state,'b.count',scenario===9?'1':1,scenario===7));f.attribute.updateRanges=[first,second];
        if(scenario===8)Object.defineProperty(f.state.original,'BYTES_PER_ELEMENT',{value:convertValue(f.state,'bytes',4),configurable:true});
    }else{const unsupported=convertValue(f.state,'format','unsupported',scenario===11);unsupported.byteLength=32;f.attribute.array=unsupported;}
    const result=invoke(()=>f.update());return {kind:'coercion',scenario,result,cache:f.cache(),trace:f.trace()};}
function functionMetadata(fn){let constructible=true;try{Reflect.construct(String,[],fn);}catch{constructible=false;}const pd=Object.getOwnPropertyDescriptor(fn,'prototype');return {name:fn.name,length:fn.length,constructible,own:Object.getOwnPropertyNames(fn),prototypeDescriptor:pd?[!!pd.writable,!!pd.enumerable,!!pd.configurable]:null,constructorSelf:!!pd&&fn.prototype.constructor===fn};}
function metadataCase(index){const f=new Fixture(),fn=f.manager[['get','remove','update'][index]],prototype=fn.prototype,constructor=Object.getOwnPropertyDescriptor(prototype,'constructor');return {kind:'metadata',index,function:functionMetadata(fn),constructorDescriptor:[!!constructor.writable,!!constructor.enumerable,!!constructor.configurable],prototypeParent:Object.getPrototypeOf(prototype)===Object.prototype};}
function constructCase(index,mode){const f=new Fixture();if(mode!==0)f.update();f.state.log.length=0;const cached=f.manager.get(f.attribute),fn=f.manager[['get','remove','update'][index]];let target=fn,expectedPrototype=fn.prototype;
    if(mode>=2){target=function Other(){};expectedPrototype=mode===2?{}:Object.prototype;target.prototype=mode===2?expectedPrototype:null;}
    if(mode===4)f.attribute={isInterleavedBufferAttribute:true,data:f.attribute};let result;const outcome=invoke(()=>result=Reflect.construct(fn,[f.attribute,34962],target));
    return {kind:'construct',index,mode,outcome,returnedCached:!!cached&&result===cached,newTargetPrototype:!!result&&Object.getPrototypeOf(result)===expectedPrototype,cache:f.cache(),trace:f.trace()};}
function arrowCase(){const f=new Fixture();f.update();f.attribute.version=1;const ranges=f.attribute.updateRanges; ranges[0]=range(0,1);let callback;ranges.sort=function(fn){callback=fn;return this;};f.update();return {kind:'arrow',function:functionMetadata(callback)};}
const expected=[];
for(let kind=0;kind<9;kind++)for(let flag=0;flag<3;flag++)expected.push(typedCase(kind,flag));
for(const raw of [false,true])for(let first=0;first<scalars().length;first++)for(let second=0;second<scalars().length;second++)expected.push(versionCase(first,second,raw));
for(let i=0;i<17;i++)expected.push(rangeCase(i));for(const update of [false,true])for(let i=0;i<17;i++)expected.push(behaviorCase(i,update));
for(let i=0;i<3;i++)expected.push(interleavedCase(i));for(let i=0;i<4;i++)expected.push(resizeCase(i));
for(let i=0;i<3;i++)expected.push(orderCase(i));for(let i=0;i<9;i++)expected.push(weakMethodCase(i));for(let i=0;i<2;i++)expected.push(pollutionCase(i));
for(let i=0;i<12;i++)expected.push(coercionCase(i));
for(let i=0;i<3;i++)expected.push(metadataCase(i));for(let i=0;i<3;i++)for(let mode=0;mode<5;mode++)expected.push(constructCase(i,mode));expected.push(arrowCase());
const result=spawnSync(process.argv[3],[],{encoding:'utf8',maxBuffer:64*1024*1024});if(result.error)throw result.error;assert.equal(result.status,0,result.stderr);
const actual=result.stdout.trim().split(/\r?\n/).map(line=>JSON.parse(line));assert.equal(actual.length,expected.length);
let comparisons=0;function compare(a,b,where){comparisons++;if(Array.isArray(a)&&Array.isArray(b)){assert.equal(a.length,b.length,where+'.length');for(let i=0;i<a.length;i++)compare(a[i],b[i],`${where}[${i}]`);}
    else if(a&&b&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),where+'.keys');for(const key of Object.keys(b))compare(a[key],b[key],where+'.'+key);}else assert.equal(a,b,where);}
for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`${i}:${expected[i].kind}`);
console.log(`WebGLAttributes parity: ${expected.length} cases / ${comparisons} comparisons passed against ${lock.commit}`);
