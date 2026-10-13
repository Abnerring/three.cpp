import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url),'utf8'));
const revision=spawnSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'});
assert.equal(revision.status,0);assert.equal(revision.stdout.trim(),lock.commit);
const {WebGLBindingStates}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLBindingStates.js')));
function encode(value){
    if(value===undefined)return 'undefined';if(value===null||typeof value==='boolean'||typeof value==='string')return value;
    if(typeof value==='number'){if(Number.isNaN(value))return 'NaN';if(!Number.isFinite(value))return value<0?'-Infinity':'Infinity';if(Object.is(value,-0))return '-0';return value;}
    if(typeof value==='function')return 'function';if(Object.hasOwn(value,'tag'))return {ref:String(value.tag)};
    if(Array.isArray(value))return Array.from(value,encode);
    const out=Object.create(null);for(const key of Object.keys(value)){const d=Object.getOwnPropertyDescriptor(value,key);out[key]=Object.hasOwn(d,'value')?encode(d.value):'accessor';}return out;
}
function outcome(action){try{return encode(action());}catch(e){return e.name==='Error'?{error:'Error',message:e.message}:{error:e.name};}}
const programAttribute=(location=0,locationSize=1)=>({location,locationSize});
const attribute=(name='a')=>({tag:`attr:${name}`,normalized:false,itemSize:3,count:7,cache:{buffer:`buffer:${name}`,type:5126,bytesPerElement:4}});
const scalars=()=>[undefined,null,false,true,0,-0,-1,1,2,3,4,1.5,'0','1','2','',NaN];
function once(state){if(state.busy||state.acted)return false;state.acted=true;state.busy=true;return true;}
class Fixture {
    constructor(max){
        if(arguments.length===0)max=8;
        const state=this.state=Object.assign(Object.create(null),{log:[],max,next:0,attributesCalls:0,busy:false,acted:false,behavior:0,throw:''});
        const gl=this.gl={},attributeManager=this.attributeManager={},object=this.object={id:4,isInstancedMesh:false},material=this.material={wireframe:false};
        const program=this.program={id:2},geometry=this.geometry={id:1},programAttributes=this.programAttributes={},geometryAttributes=this.geometryAttributes={};
        const source=this.source=attribute();Object.assign(state,{gl,attributeManager,object,material,program,geometry,programAttributes});
        for(const name of ['getParameter','createVertexArray','bindVertexArray','deleteVertexArray','enableVertexAttribArray','disableVertexAttribArray','vertexAttribDivisor','bindBuffer','vertexAttribPointer','vertexAttribIPointer','vertexAttrib1fv','vertexAttrib2fv','vertexAttrib3fv','vertexAttrib4fv'])gl[name]=function(...args){
            state.log.push([name,this===state.gl,...args.map(encode)]);
            if(state.throw===name)throw RangeError('command');
            if(name==='getParameter')return state.max;if(name==='createVertexArray')return `vao:${++state.next}`;
            if((state.behavior===1&&name==='bindVertexArray')||(state.behavior===4&&name==='vertexAttribPointer')){
                if(once(state)){if(state.behavior===1)state.manager.reset();else state.manager.releaseStatesOfGeometry(state.geometry);state.busy=false;}
            }
            if(state.behavior===5&&name==='deleteVertexArray'&&once(state)){state.manager.setup(state.object,state.material,state.program,state.geometry,null);state.busy=false;}
        };
        Object.assign(gl,{MAX_VERTEX_ATTRIBS:34921,ARRAY_BUFFER:34962,ELEMENT_ARRAY_BUFFER:34963,INT:5124,UNSIGNED_INT:5125});
        for(const name of ['get','update'])attributeManager[name]=function(...args){
            state.log.push([name,this===attributeManager,...args.map(encode)]);
            if(state.throw===name)throw RangeError('attribute');
            if(name==='get'){
                if(state.behavior===3&&once(state)){state.manager.reset();state.busy=false;}
                return args[0].cache;
            }
        };
        program.getAttributes=function(){
            state.log.push(['getAttributes',this===program]);const n=++state.attributesCalls;
            if(state.throw==='getAttributes')throw RangeError('program');
            if(state.behavior===2&&once(state)){state.manager.reset();state.busy=false;}
            return state.behavior===6?(n===2?state.alternate:state.programAttributes):state.programAttributes;
        };
        geometry.attributes=geometryAttributes;programAttributes.a=programAttribute();geometryAttributes.a=source;
        this.manager=state.manager=WebGLBindingStates(gl,attributeManager);
    }
    setup(index=null){return this.manager.setup(this.object,this.material,this.program,this.geometry,index);}
    trace(){return encode(this.state.log);}maxCount(){return encode(this.geometry._maxInstanceCount);}clearLog(){this.state.log.length=0;}
}
function attributeCase(mode,locationSize,field,valueIndex){
    const f=new Fixture(),values=scalars(),pa=f.programAttributes.a;pa.locationSize=values[locationSize];
    if(mode===1||mode===3)Object.assign(f.source,{data:{stride:6,count:11,meshPerAttribute:2,isInstancedInterleavedBuffer:mode===3},isInterleavedBufferAttribute:true,offset:1});
    if(mode===2)Object.assign(f.source,{isInstancedBufferAttribute:true,meshPerAttribute:2});if(mode===4)f.source.cache=undefined;
    switch(field){
    case 0:pa.location=values[valueIndex];break;case 1:f.source.itemSize=values[valueIndex];break;case 2:f.source.normalized=values[valueIndex];break;
    case 3:f.source.cache.type=values[valueIndex];break;case 4:f.source.cache.bytesPerElement=values[valueIndex];break;case 5:f.source.gpuType=values[valueIndex];break;
    case 6:if(mode===2)f.source.meshPerAttribute=values[valueIndex];else if(mode===3)f.source.data.meshPerAttribute=values[valueIndex];break;
    case 7:if(mode===1||mode===3)f.source.offset=values[valueIndex];break;
    }
    const result=outcome(()=>f.setup()),repeated=outcome(()=>f.setup());return {kind:'attribute',mode,locationSize,field,value:valueIndex,result,repeated,max:f.maxCount(),trace:f.trace()};
}
function sequenceCase(variant){
    const f=new Fixture(),results=[],index=attribute('index');
    for(let step=0;step<6;step++){
        if(variant===0)f.geometry.id=step%2+1;if(variant===1)f.program.id=step%2+2;if(variant===2)f.material.wireframe=step%2===1;
        if(variant===3){f.object.isInstancedMesh=true;f.object.id=step%2+4;}if(variant===4)f.object.id=step%2+4;
        if(variant===5){f.material.wireframe=step%2?'true':false;f.object.isInstancedMesh='true';}
        if(variant===6&&step===2)f.geometryAttributes.a=attribute('replacement');if(variant===7&&step===2)f.source.data={};
        if(variant===8&&step===2)Object.assign(f.source,{itemSize:4,normalized:true,version:7});
        if(variant===9&&step===2)f.programAttributes.b=programAttribute(3,1);if(variant===9&&step===4)delete f.programAttributes.b;
        if(variant===10&&step===2)f.programAttributes.a.location=-1;if(variant===10&&step===4)f.programAttributes.a.location=1;
        if(variant===11&&step===2)delete f.geometryAttributes.a;if(variant===11&&step===4)f.geometryAttributes.a=f.source;
        if(variant===12&&step===2)f.manager.reset();if(variant===13&&step===2)f.manager.resetDefaultState();
        const nextIndex=variant===14?(step%2?null:index):variant===15?index:null;results.push(outcome(()=>f.setup(nextIndex)));
    }
    return {kind:'sequence',variant,results,trace:f.trace()};
}
function defaultCase(length,mode){
    const f=new Fixture();delete f.geometryAttributes.a;let value=Array.from({length},(_,i)=>i+.25);
    if(mode===1)value={length:String(length),x:1};if(mode===2)value=null;if(mode===3)value=undefined;
    const defaults=f.material.defaultAttributeValues={a:value},first=outcome(()=>f.setup());defaults.a=[9,8,7,6];
    const second=outcome(()=>f.setup());f.manager.reset();const forced=outcome(()=>f.setup());return {kind:'default',length,mode,first,second,forced,trace:f.trace()};
}
function releaseCase(variant){
    const f=new Fixture(),results=[];
    for(let g=0;g<2;g++)for(let o=0;o<2;o++)for(let p=0;p<2;p++)for(const wire of [false,true]){
        f.geometry.id=g+1;f.object.id=o+4;f.object.isInstancedMesh=true;f.program.id=p+2;f.material.wireframe=wire;f.setup();
    }
    f.clearLog();const name=['releaseStatesOfGeometry','releaseStatesOfProgram','releaseStatesOfObject','dispose'][variant%4],value=variant%4===0?f.geometry:variant%4===1?f.program:f.object;
    if(variant>=4)f.state.behavior=5;
    results.push(outcome(()=>f.manager[name](...(variant%4===3?[]:[value]))));results.push(outcome(()=>f.setup()));results.push(outcome(()=>f.manager.dispose()));
    return {kind:'release',variant,results,trace:f.trace()};
}
function behaviorCase(behavior,fail){
    const f=new Fixture();f.state.behavior=behavior;
    const failures=['','createVertexArray','bindVertexArray','getAttributes','get','update','enableVertexAttribArray','vertexAttribDivisor','bindBuffer','vertexAttribPointer','deleteVertexArray'];
    f.state.throw=failures[fail];if(fail===7)Object.assign(f.source,{isInstancedBufferAttribute:true,meshPerAttribute:2});
    if(behavior===6)f.state.alternate={a:programAttribute(-1,1)};
    const index=fail===5?attribute('index'):null,first=outcome(()=>f.setup(index));f.state.throw='';const second=outcome(()=>f.setup(index));
    if(fail===10)f.state.throw=failures[fail];const release=outcome(()=>f.manager.dispose());f.state.throw='';const final=outcome(()=>f.setup());
    return {kind:'behavior',behavior,fail,first,second,release,final,trace:f.trace()};
}
function manualCase(max,index){
    const values=scalars(),f=new Fixture(values[max]),results=[];
    for(const name of ['enableAttribute','enableAttribute','disableUnusedAttributes','initAttributes','disableUnusedAttributes','enableAttribute','reset','disableUnusedAttributes'])
        results.push(outcome(()=>f.manager[name](...(name==='enableAttribute'?[values[index]]:[]))));
    return {kind:'manual',max,index,results,trace:f.trace()};
}
function metadataCase(method,mode){
    const f=new Fixture(),names=['setup','reset','resetDefaultState','dispose','releaseStatesOfGeometry','releaseStatesOfObject','releaseStatesOfProgram','initAttributes','enableAttribute','disableUnusedAttributes'];
    const fn=f.manager[names[method]],proto=fn.prototype,targetCalls=[];let target=fn,expected=proto;
    if(mode){target=function Other(){targetCalls.push('target-body');};if(mode===1){expected={};target.prototype=expected;}else{target.prototype=null;expected=Object.prototype;}}
    const args=method===0?[f.object,f.material,f.program,f.geometry,null]:method===4?[f.geometry]:method===5?[f.object]:method===6?[f.program]:method===8?[1]:[];
    const result=Reflect.construct(fn,args,target),d=Object.getOwnPropertyDescriptor(fn,'prototype');
    return {kind:'metadata',method,mode,name:fn.name,length:fn.length,own:Object.getOwnPropertyNames(fn),prototypeFlags:[d.writable,d.enumerable,d.configurable],constructorIdentity:proto.constructor===fn,
        resultPrototype:Object.getPrototypeOf(result)===expected,resultOwn:Object.getOwnPropertyNames(result),targetCalls,trace:f.trace()};
}
function watch(f,object,name,label,action=0){
    const value=object[name];Object.defineProperty(object,name,{enumerable:true,configurable:true,get(){
        f.state.log.push(['property',label,name]);if(action===1)f.state.gl.ARRAY_BUFFER=99;
        if(action===2&&once(f.state)){f.state.manager.reset();f.state.busy=false;}
        if(action===3)delete f.state.programAttributes.b;return value;
    }});
}
function numberValue(f,label,result,action=0){return {tag:label,[Symbol.toPrimitive](hint){
    f.state.log.push(['convert',label,hint]);
    if(action===1)Object.defineProperty(f.state.programAttributes.a,'locationSize',{value:2,writable:true,enumerable:true,configurable:true});
    if(action===2)throw RangeError('coercion');if(action===3&&once(f.state)){f.state.manager.reset();f.state.busy=false;}return result;
}};}
function getterCase(variant){
    const f=new Fixture();
    if(variant===0){
        watch(f,f.geometry,'attributes','geometry');watch(f,f.geometry,'id','geometry');watch(f,f.program,'id','program');
        watch(f,f.program,'getAttributes','program');watch(f,f.material,'wireframe','material');watch(f,f.material,'defaultAttributeValues','material');
        watch(f,f.programAttributes,'a','layout');watch(f,f.programAttributes.a,'location','layout-a');watch(f,f.programAttributes.a,'locationSize','layout-a');
        for(const name of ['normalized','itemSize','gpuType','data','isInterleavedBufferAttribute','isInstancedBufferAttribute'])watch(f,f.source,name,'source');
        for(const name of ['buffer','type','bytesPerElement'])watch(f,f.source.cache,name,'cache');
    }
    if(variant===1)watch(f,f.gl,'bindBuffer','gl',1);if(variant===2)watch(f,f.gl,'vertexAttribPointer','gl',2);
    if(variant===3)watch(f,f.attributeManager,'get','attributes',2);if(variant===4)watch(f,f.program,'getAttributes','program',2);
    if(variant===5){f.geometry.id=numberValue(f,'geometry-id','1');f.program.id=numberValue(f,'program-id','2');}
    if(variant===6)f.programAttributes.a.location=numberValue(f,'location',1);
    if(variant===7)f.source.itemSize=numberValue(f,'size','4');
    if(variant===8){f.source.itemSize=numberValue(f,'size',4,1);watch(f,f.programAttributes.a,'locationSize','layout-a');}
    if(variant===9)f.source.itemSize=numberValue(f,'size',4,2);
    if(variant===10)f.geometry.id=numberValue(f,'geometry-id','1',3);
    if(variant===11)Object.assign(f.source,{isInterleavedBufferAttribute:true,data:{stride:numberValue(f,'stride','8'),count:4},offset:numberValue(f,'offset','1')});
    if(variant===12){watch(f,f.gl,'getParameter','gl');watch(f,f.gl,'MAX_VERTEX_ATTRIBS','gl');f.manager=f.state.manager=WebGLBindingStates(f.gl,f.attributeManager);}
    if(variant===13){Object.assign(f.source,{isInstancedBufferAttribute:true,meshPerAttribute:numberValue(f,'divisor',2)});f.object.isInstancedMesh='true';}
    const first=outcome(()=>f.setup()),repeated=outcome(()=>f.setup());return {kind:'getter',variant,first,repeated,max:f.maxCount(),trace:f.trace()};
}
function enumerationCase(variant){
    const f=new Fixture(),inherited={b:programAttribute(2,1)};f.geometryAttributes.b=attribute('b');
    if(variant===0)Object.setPrototypeOf(f.programAttributes,inherited);
    if(variant===1){Object.setPrototypeOf(f.programAttributes,inherited);Object.defineProperty(f.programAttributes,'b',{value:programAttribute(3,1),enumerable:false,configurable:true});}
    if(variant===2){f.programAttributes.b=programAttribute(2,1);watch(f,f.programAttributes,'a','layout',3);}
    if(variant===3){Object.setPrototypeOf(f.programAttributes,inherited);watch(f,inherited,'b','inherited');}
    if(variant===4){f.programAttributes['2']=programAttribute(3,1);f.programAttributes['1']=programAttribute(2,1);f.geometryAttributes['1']=attribute('1');f.geometryAttributes['2']=attribute('2');}
    if(variant===5)f.programAttributes.b=null;if(variant===6)f.state.programAttributes=undefined;if(variant===7)f.state.programAttributes=null;if(variant===8)f.state.programAttributes='a';
    if(variant===9){f.state.programAttributes=[programAttribute()];f.geometryAttributes['0']=f.source;}
    if(variant===10){f.state.programAttributes=function(){};f.state.programAttributes.a=programAttribute();}
    const first=outcome(()=>f.setup()),repeated=outcome(()=>f.setup());return {kind:'enumeration',variant,first,repeated,trace:f.trace()};
}
function dynamicMutation(object,prototype,variant){
    const define=(target,key,value,enumerable)=>Object.defineProperty(target,key,{value,writable:true,enumerable,configurable:true});
    if(variant===0)prototype.b=programAttribute(2);
    if(variant===1)object.d=programAttribute(4);
    if(variant===2||variant===14)Object.defineProperty(object,'b',{enumerable:variant===14});
    if(variant===3||variant===10)Object.defineProperty(prototype,'b',{enumerable:variant===10});
    if(variant===4||variant===5||variant===6)delete object.b;
    if(variant===7)delete prototype.b;
    if(variant===8)define(object,'b',programAttribute(3),false);
    if(variant===9)object.b=programAttribute(3);
    if(variant===11)Object.setPrototypeOf(object,{b:programAttribute(3),d:programAttribute(4)});
    if(variant===12)Object.setPrototypeOf(object,null);
    if(variant===13)Object.setPrototypeOf(object,define({},'b',programAttribute(3),false));
    if(variant===15){delete object.b;object.b=programAttribute(3);}
}
function dynamicEnumerationCase(variant,phase){
    const f=new Fixture(),prototype={b:programAttribute(2),c:programAttribute(3)},second=Object.create(prototype),a=programAttribute();
    if(variant===0)delete prototype.b;
    if(variant===2||variant===5||variant===6||variant===15)second.b=programAttribute(2);
    if(variant===4||variant===14)Object.defineProperty(second,'b',{value:programAttribute(2),writable:true,enumerable:false,configurable:true});
    if(variant===6||variant===10)Object.defineProperty(prototype,'b',{enumerable:false});
    Object.assign(f.state,{dynamicCalls:0,dynamicDone:false});
    Object.defineProperty(second,'a',{enumerable:true,configurable:true,get(){f.state.log.push(['dynamic-a']);if(!f.state.dynamicDone){f.state.dynamicDone=true;dynamicMutation(second,prototype,variant);}return a;}});
    if(Object.hasOwn(second,'b')){const d=Object.getOwnPropertyDescriptor(second,'b');delete second.b;Object.defineProperty(second,'b',d);}
    f.program.getAttributes=function(){const n=++f.state.dynamicCalls;f.state.log.push(['dynamic-attributes',this===f.program,n]);return n<phase?f.programAttributes:second;};
    for(const name of ['b','c','d'])f.geometryAttributes[name]=attribute(name);
    const first=outcome(()=>f.setup()),firstTrace=f.trace();f.clearLog();
    const secondResult=outcome(()=>f.setup()),secondTrace=f.trace();f.clearLog();
    f.manager.reset();const forced=outcome(()=>f.setup());
    return {kind:'dynamic-enumeration',variant,phase,first,firstTrace,second:secondResult,secondTrace,forced,forcedTrace:f.trace()};
}
function invalidCase(variant){
    const f=new Fixture();
    if(variant===0)f.geometry.attributes=null;if(variant===1)f.geometryAttributes.a=null;if(variant===2)f.source.cache=null;
    if(variant===3){delete f.geometryAttributes.a;f.material.defaultAttributeValues=null;}if(variant===4)f.program.getAttributes=0;
    if(variant===5)f.gl.vertexAttribPointer=0;if(variant===6)f.attributeManager.get=0;if(variant===7)f.gl.bindVertexArray=0;
    const first=outcome(()=>variant===8?f.manager.setup():f.manager.setup(f.object,f.material,f.program,f.geometry,variant===9?undefined:null));
    const repeated=outcome(()=>f.setup());return {kind:'invalid',variant,first,repeated,trace:f.trace()};
}
const expected=[];
for(let mode=0;mode<4;mode++)for(const size of [4,7,8,10,11,14,0])for(let field=0;field<8;field++)for(let value=0;value<scalars().length;value++)expected.push(attributeCase(mode,size,field,value));
expected.push(attributeCase(4,7,0,7));
for(let variant=0;variant<16;variant++)expected.push(sequenceCase(variant));
for(let length=0;length<6;length++)for(let mode=0;mode<4;mode++)expected.push(defaultCase(length,mode));
for(let variant=0;variant<8;variant++)expected.push(releaseCase(variant));
for(let behavior=0;behavior<7;behavior++)for(let fail=0;fail<11;fail++)expected.push(behaviorCase(behavior,fail));
for(const max of [4,7,8,10,14,0])for(let index=0;index<scalars().length;index++)expected.push(manualCase(max,index));
for(let method=0;method<10;method++)for(let mode=0;mode<3;mode++)expected.push(metadataCase(method,mode));
for(let variant=0;variant<14;variant++)expected.push(getterCase(variant));
for(let variant=0;variant<11;variant++)expected.push(enumerationCase(variant));
for(let variant=0;variant<16;variant++)for(let phase=1;phase<5;phase++)expected.push(dynamicEnumerationCase(variant,phase));
for(let variant=0;variant<10;variant++)expected.push(invalidCase(variant));
const native=spawnSync(path.resolve(process.argv[3]),[],{encoding:'utf8',maxBuffer:128*1024*1024});
assert.equal(native.status,0,native.stderr);const actual=native.stdout.trim().split(/\r?\n/).map(line=>JSON.parse(line));assert.equal(actual.length,expected.length);
let comparisons=0;
function compare(a,b,label){comparisons++;if(Array.isArray(b)){assert.ok(Array.isArray(a),label);assert.equal(a.length,b.length,`${label}.length`);b.forEach((v,i)=>compare(a[i],v,`${label}[${i}]`));}
    else if(b&&typeof b==='object'){assert.ok(a&&typeof a==='object',label);assert.deepEqual(Object.keys(a),Object.keys(b),`${label}.keys`);for(const key of Object.keys(b))compare(a[key],b[key],`${label}.${key}`);}
    else assert.deepEqual(a,b,label);
}
for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`${i}:${expected[i].kind}`);
console.log(`WebGLBindingStates parity: ${expected.length} scenarios / ${comparisons} comparisons passed against ${lock.commit} (CPU recording transport)`);
