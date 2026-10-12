import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync,spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node compare-uniforms.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {Uniform,UniformsGroup,EventDispatcher}=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const {cloneUniforms,mergeUniforms,cloneUniformsGroups,getUnlitUniformColorSpace,UniformsUtils}=await import(pathToFileURL(path.join(upstream,'src/renderers/shaders/UniformsUtils.js')));
const {ColorManagement}=await import(pathToFileURL(path.join(upstream,'src/math/ColorManagement.js')));
const warnings=[];console.warn=(...args)=>warnings.push(args.join(' '));
const getter=(object,key,get)=>Object.defineProperty(object,key,{get,enumerable:true,configurable:true});
function attempt(f){try{f();return '';}catch(error){return error.name;}}
function data(value){
    if(value===undefined)return '<undefined>';
    if(typeof value==='function')return '<function>';
    if(Array.isArray(value))return {length:value.length,keys:Object.keys(value),slots:Array.from({length:value.length},(_,i)=>Object.hasOwn(value,i)?data(value[i]):'<hole>')};
    if(value&&typeof value==='object'){
        if(Object.hasOwn(value,'value'))return {value:data(value.value)};
        if(Object.hasOwn(value,'marker'))return {marker:data(value.marker)};
        return {keys:Object.keys(value)};
    }
    return value;
}
function cloneable(result,trace,label='clone'){return {marker:result,clone(...args){if(trace!==undefined)trace.push([label,args.length,this.marker]);return this.marker;}};}
function metadata(){
    const uniform=new Uniform(2),a=new UniformsGroup(),b=new UniformsGroup();
    const methods=['add','remove','setName','setUsage','dispose','copy','clone'].map(name=>{const d=Object.getOwnPropertyDescriptor(UniformsGroup.prototype,name),method=UniformsGroup.prototype[name];return [name,method.name,method.length,d.writable,d.enumerable,d.configurable];});
    const id=Object.getOwnPropertyDescriptor(a,'id');
    return {audit:'metadata',uniformKeys:Object.keys(uniform),uniformPrototype:Object.getOwnPropertyNames(Uniform.prototype),groupKeys:Object.keys(a),groupNames:Object.getOwnPropertyNames(a),prototypeNames:Object.getOwnPropertyNames(UniformsGroup.prototype),id:[b.id-a.id,id.writable,id.enumerable,id.configurable],defaults:[a.isUniformsGroup,a.name,a.usage,data(a.uniforms)],methods,inherits:Object.getPrototypeOf(Object.getPrototypeOf(a))===EventDispatcher.prototype,constructorInherits:Object.getPrototypeOf(UniformsGroup)===EventDispatcher,callError:attempt(()=>UniformsGroup())};
}
function uniformCase(mode){
    let value;const trace=[];
    if(mode===1)value=null;if(mode===2)value=5;if(mode===3)value='text';if(mode===4)value=false;
    if(mode===5)value={};if(mode===6)value=[1,2];if(mode>=7)value=cloneable(7,trace);
    const uniform=new Uniform(value),owner=uniform;
    if(mode===8)value.clone=null;
    if(mode===9)getter(value,'clone',()=>{trace.push('first');owner.value=cloneable(20,trace,'replacement');return 1;});
    if(mode===10){let n=0;const next=cloneable(33,trace,'next');getter(owner,'value',()=>{trace.push(n);return n++===0?value:next;});}
    if(mode===11){let n=0;getter(value,'clone',()=>{trace.push(n);return n++===0?1:undefined;});}
    if(mode===12)getter(value,'clone',()=>{owner.value=88;trace.push('absent');return undefined;});
    if(mode===13)value.clone=()=>{trace.push('throw');throw TypeError('clone');};
    if(mode===14)owner.constructor=null;
    if(mode===15)value.clone=function(){return this;};
    let result;const error=attempt(()=>{result=uniform.clone();});
    return {audit:'uniform',mode,error,result:data(result),sameValue:result!==undefined&&Object.is(result.value,value),freshUniform:result!==undefined&&result!==owner,prototype:result!==undefined&&Object.getPrototypeOf(result)===Uniform.prototype,trace:data(trace)};
}
function copyCase(mode){
    const source=new UniformsGroup(),destination=new UniformsGroup();source.setName('source');source.setUsage(27);const trace=[];
    const original=destination.uniforms,replacement=[],src=source.uniforms;
    const callback={clone(){trace.push('clone0');
        if(mode===1)destination.uniforms=replacement;
        if(mode===2)delete src[1];
        if(mode===3)src[2]=new Uniform(42);
        if(mode===4)src.push(new Uniform(55));
        if(mode===5)src.length=1;
        if(mode===6)throw TypeError('clone failure');
        if(mode===7)destination.uniforms=null;
        if(mode===8)source.uniforms=[];
        if(mode===9)src[1]=null;
        return 10;
    }};
    source.add(callback);const nested=new Array(3);nested[0]=new Uniform(20);nested[2]=new Uniform(30);source.add(nested);source.add(new Uniform(40));
    if(mode===10)destination.uniforms=src;
    if(mode===11)nested.map=function(...args){trace.push(['map',args.length,args[0].length,args[0].name]);return args[0].call(null,this[2]);};
    if(mode===12)nested.map=null;
    if(mode===13){const proto=Object.create(Object.getPrototypeOf(nested));proto[1]=new Uniform(25);Object.setPrototypeOf(nested,proto);}
    if(mode===14)getter(original,'push',()=>{trace.push('getPush');destination.uniforms=replacement;return Array.prototype.push;});
    if(mode===15)Object.defineProperty(original,'length',{writable:false});
    if(mode===16)getter(source,'usage',()=>{trace.push('usage');throw TypeError('usage');});
    if(mode===17)nested.constructor=4;
    if(mode===18)getter(nested,'constructor',()=>{trace.push('constructor');nested[1]=new Uniform(25);return undefined;});
    if(mode===19){const length={valueOf(){trace.push('length');return 2;}};source.uniforms={length,0:callback,1:new Uniform(80)};}
    let result;const error=attempt(()=>{result=destination.copy(source);});
    return {audit:'copy',mode,error,chain:result===destination,name:data(destination.name),usage:data(destination.usage),target:data(destination.uniforms),original:data(original),replacement:data(replacement),trace:data(trace)};
}
function operationsCase(mode){
    const group=new UniformsGroup(),a=new Uniform(1),b=new Uniform(2),trace=[];let result;
    group.add(a);group.add(a);group.add(b);
    if(mode===0)result=group.remove(a);
    if(mode===1)result=group.remove(new Uniform(1));
    if(mode===2)result=group.copy(group);
    if(mode===3){getter(group.uniforms,'indexOf',()=>{trace.push('indexOf');group.uniforms=[8,9,10];return ()=>1;});result=group.remove(a);}
    if(mode===4){group.uniforms.indexOf=()=>'-1';result=group.remove(a);}
    if(mode===5){group.setName(null);result=group.setUsage('dynamic');}
    return {audit:'operations',mode,chain:result===group,uniforms:data(group.uniforms),name:data(group.name),usage:data(group.usage),trace:data(trace)};
}
function mapCase(mode){
    const map=Array.prototype.map,trace=[],thisArg={};let receiver=new Array(4);receiver[0]=2;receiver[2]=6;
    if(mode===1){const proto=Object.create(Object.getPrototypeOf(receiver));proto[1]=4;Object.setPrototypeOf(receiver,proto);}
    if(mode===2)receiver='A😀';if(mode===3)receiver=17;if(mode===4)receiver=null;if(mode===5)receiver=undefined;
    if(mode>=6&&mode<=8)receiver={0:3,2:9,length:mode===6?'3.8':mode===7?-2:4294967296};
    if(mode===9||mode===10||mode===11)getter(receiver,'constructor',()=>{trace.push('constructor');if(mode===11)throw TypeError('ctor');return mode===10?1:undefined;});
    if(mode===12)getter(receiver,'0',()=>{trace.push('get0');receiver[1]=4;return 2;});
    if(mode===16){receiver={};getter(receiver,'length',()=>{trace.push('length');throw TypeError('length');});}
    if(mode===17)getter(receiver,'constructor',()=>{trace.push('unexpected constructor');throw TypeError('ctor');});
    if(mode===18)receiver=false;
    if(mode===19){receiver={length:3,0:5};getter(receiver,'constructor',()=>{throw TypeError('must not read');});}
    let callback=function(value,index,array){trace.push([value,index,this===thisArg,typeof array==='object',array.length]);
        if(mode===13&&index===0){delete array[2];array[1]=4;array[4]=8;}
        if(mode===14)throw TypeError('callback');if(mode===15)return undefined;return value;
    };
    if(mode===16||mode===17)callback=null;
    let result;const error=attempt(()=>{result=map.call(receiver,callback,thisArg);});
    return {audit:'map',mode,error,result:data(result),trace:data(trace)};
}
function cloneCase(mode){
    const source=new UniformsGroup();source.setName('source');source.add(new Uniform(3));const trace=[],start=source.id;let ctor=UniformsGroup;
    if(mode===1)ctor=null;if(mode===2)ctor=()=>undefined;
    if(mode>=3&&mode<=6){ctor=class Custom{constructor(...args){trace.push(['construct',args.length]);this.marker=17;
        if(mode===5)throw TypeError('construct');
        if(mode===6)return undefined;
        if(mode===4)return {marker:19,copy(source){trace.push(['copy',this.marker,source.name]);return this.marker;}};
    }copy(source){trace.push(['copy',this.marker,source.name]);return this.marker;}};}
    if(mode===7)source.uniforms[0].value=null;
    getter(source,'constructor',()=>{trace.push('getConstructor');if(mode===8)throw TypeError('get');return ctor;});
    let result;const error=attempt(()=>{result=source.clone();});const after=new UniformsGroup();
    const packed=result&&typeof result==='object'&&Object.hasOwn(result,'uniforms')?{name:data(result.name),uniforms:data(result.uniforms)}:data(result);
    return {audit:'clone',mode,error,result:packed,allocated:after.id-start-1,trace:data(trace)};
}
function eventCase(mode){
    const group=new UniformsGroup(),trace=[],events=[];
    const second=function(event){trace.push(['second',this===event.target]);};
    const first=function(event){trace.push(['first',this===event.target]);events.push(event);if(mode===1)this.removeEventListener('dispose',second);if(mode===2)this.addEventListener('dispose',second);if(mode===3)throw TypeError('event');};
    group.addEventListener('dispose',first);group.addEventListener('dispose',first);if(mode!==2)group.addEventListener('dispose',second);
    if(mode===4)group.dispatchEvent=function(event){trace.push(['override',event.type,event.target===undefined]);return 88;};
    let result;const error=attempt(()=>{result=group.dispose();});
    return {audit:'events',mode,error,undefinedReturn:result===undefined,trace:data(trace),retainedTarget:events.length?events[0].target===group:false,hasSecond:group.hasEventListener('dispose',second)};
}
function deepData(value){
    if(!value||typeof value!=='object')return data(value);
    if(Array.isArray(value))return {length:value.length,keys:Object.keys(value),slots:Array.from({length:value.length},(_,i)=>Object.hasOwn(value,i)?deepData(value[i]):'<hole>')};
    return Object.fromEntries(Object.keys(value).map(key=>[key,deepData(value[key])]));
}
const definition=value=>({value});
function utilityCloneCase(mode){
    const trace=[],ordinary={marker:5},three=cloneable(17,trace);three.isVector3=true;let source={},value=[ordinary,three];
    if(mode===1)value=[three,cloneable(18,trace)];if(mode===2)value=three;
    if(mode===3){three.isRenderTargetTexture=true;value=three;}
    if(mode===4)value=[null,three];if(mode===5)value=[three,null];
    if(mode===6||mode===7){value=[three,cloneable(18,trace)];three.clone=()=>{trace.push('clone');if(mode===6)value.length=1;else value.push(cloneable(19));return 17;};}
    if(mode===8)value.slice=function(...args){trace.push(['slice',args.length,this.length]);return 'override';};
    if(mode===9)value.constructor=4;
    if(mode===16){value=new Array(3);value[2]=ordinary;}
    if(mode===18){value=three;getter(three,'isRenderTargetTexture',()=>{trace.push('targetFlag');return 'truthy';});}
    if(mode===19)value=ordinary;
    source.a=definition(value);
    if(mode===10){Object.setPrototypeOf(source,{inherited:definition(three)});Object.setPrototypeOf(source.a,{extra:7});}
    if(mode===11){let n=0;const initial={value:1,second:2},replacement={value:9,second:10};getter(source,'a',()=>{trace.push(n);return n++===0?initial:replacement;});}
    if(mode===12){source.b=definition(9);getter(source.a,'value',()=>{trace.push('delete');delete source.b;return 3;});}
    if(mode===13)source='abc';if(mode===14)source=null;
    if(mode===15){source.a='ab';source.b=4;source.c=null;}
    if(mode===17){Object.setPrototypeOf(source,{a:definition(19),b:definition(21)});Object.defineProperty(source,'a',{enumerable:false});}
    let result;const error=attempt(()=>{result=cloneUniforms(source);});
    return {audit:'cloneUniforms',mode,error,result:deepData(result),shared:!!result&&typeof result.a==='object'&&result.a.value===ordinary,sharesElement:!!result&&typeof result.a==='object'&&Array.isArray(result.a.value)&&result.a.value[0]===ordinary,trace:data(trace)};
}
function utilityMergeCase(mode){
    const trace=[],first={a:definition(1),b:definition(2)},second={b:definition(3),c:definition(4)},third={a:definition(8)};let source=[first,second];
    if(mode>=1&&mode<=3)first.a=definition({isColor:true,clone(){trace.push('clone');if(mode===1)source.push(third);if(mode===2)source.length=1;if(mode===3)source[1]=third;return 7;}});
    if(mode===4){let n=0;source={0:first,1:second};getter(source,'length',()=>{trace.push(n);return n++===0?1:2;});}
    if(mode===5)source=null;
    if(mode===6)source=[null,'ab',first];
    if(mode===7)getter(source,'1',()=>{trace.push('second');throw TypeError('second');});
    let result;const error=attempt(()=>{result=UniformsUtils.merge(source);});
    return {audit:'mergeUniforms',mode,error,result:deepData(result),trace:data(trace)};
}
function utilityGroupsCase(mode){
    const trace=[],group=new UniformsGroup();group.setName('group');group.add(new Uniform(1));let source=[];const captured=source;
    const first={clone(){trace.push('first');if(mode===1)captured.push(cloneable(30,trace));if(mode===2)captured.length=1;if(mode===3)delete captured[1];if(mode===4)captured[1]=cloneable(40,trace);return 10;}};
    source.push(first,cloneable(20,trace));
    if(mode===0)source=[group];if(mode===5)source=[null];
    if(mode===6){source={0:first};getter(source,'length',()=>{trace.push('length');return 1;});}
    let result;const error=attempt(()=>{result=cloneUniformsGroups(source);});
    if(mode===0&&result!==undefined){const clone=result[0];result=[clone.name,clone.uniforms,clone.id-group.id];}
    return {audit:'cloneGroups',mode,error,result:data(result),trace:data(trace)};
}
function colorSpaceCase(mode){
    const trace=[],renderer={},target={},texture={};
    getter(renderer,'outputColorSpace',()=>{trace.push('outputColorSpace');return 'output';});
    getter(texture,'colorSpace',()=>{trace.push('colorSpace');if(mode===9)throw TypeError('color');return 'texture';});
    getter(target,'texture',()=>{trace.push('texture');return mode===5?null:texture;});
    getter(target,'isXRRenderTarget',()=>{trace.push('isXR');if(mode===7)throw TypeError('XR');if(mode===2)return 1;if(mode===3)return 'true';if(mode===4)return undefined;return true;});
    getter(renderer,'getRenderTarget',()=>{trace.push('get method');return function(...args){trace.push(['call',args.length,this===renderer]);if(mode===8)throw TypeError('getTarget');if(mode===0)return null;if(mode===6)return undefined;return target;};});
    const previous=ColorManagement.workingColorSpace;ColorManagement.workingColorSpace='working';let result;const error=attempt(()=>{result=getUnlitUniformColorSpace(renderer);});ColorManagement.workingColorSpace=previous;
    return {audit:'colorSpace',mode,error,result:data(result),trace:data(trace)};
}
const expected=[metadata()];
for(let i=0;i<16;i++)expected.push(uniformCase(i));
for(let i=0;i<20;i++)expected.push(copyCase(i));
for(let i=0;i<6;i++)expected.push(operationsCase(i));
for(let i=0;i<20;i++)expected.push(mapCase(i));
for(let i=0;i<9;i++)expected.push(cloneCase(i));
for(let i=0;i<5;i++)expected.push(eventCase(i));
for(let i=0;i<20;i++)expected.push(utilityCloneCase(i));
for(let i=0;i<8;i++)expected.push(utilityMergeCase(i));
for(let i=0;i<7;i++)expected.push(utilityGroupsCase(i));
for(let i=0;i<10;i++)expected.push(colorSpaceCase(i));
const run=spawnSync(process.argv[3],[],{encoding:'utf8',maxBuffer:32*1024*1024});assert.equal(run.status,0,run.stderr||String(run.error));assert.deepEqual(run.stderr.split(/\r?\n/).filter(Boolean),warnings);
const actual=JSON.parse(run.stdout);let comparisons=0;
function compare(a,b,where){if(a&&typeof a==='object'){assert.equal(Array.isArray(a),Array.isArray(b),where);assert.deepEqual(Object.keys(a),Object.keys(b),where);for(const k of Object.keys(a))compare(a[k],b[k],`${where}.${k}`);}else{assert.deepEqual(a,b,where);comparisons++;}}
assert.equal(actual.length,expected.length);const failures=[];
for(let i=0;i<expected.length;i++){try{compare(actual[i],expected[i],`${expected[i].audit}.${expected[i].mode??0}`);}catch(error){failures.push(error.message);}}
assert.equal(failures.length,0,failures.join('\n'));
console.log(`Uniforms parity: ${expected.length} scenarios / ${comparisons} exact comparisons; ${lock.commit}`);
