import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {pathToFileURL,fileURLToPath} from 'node:url';
const upstream=path.resolve(process.argv[2]),probe=path.resolve(process.argv[3]);
const revision=spawnSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'});
assert.equal(revision.status,0);assert.equal(revision.stdout.trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const {WebGLState}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLState.js')));
const {Object3D}=await import(pathToFileURL(path.join(upstream,'src/core/Object3D.js')));
const {Quaternion}=await import(pathToFileURL(path.join(upstream,'src/math/Quaternion.js')));
const {Euler}=await import(pathToFileURL(path.join(upstream,'src/math/Euler.js')));
const {Vector2}=await import(pathToFileURL(path.join(upstream,'src/math/Vector2.js')));
const {Vector3}=await import(pathToFileURL(path.join(upstream,'src/math/Vector3.js')));
const {Vector4}=await import(pathToFileURL(path.join(upstream,'src/math/Vector4.js')));
const {Color}=await import(pathToFileURL(path.join(upstream,'src/math/Color.js')));
const {setConsoleFunction,ReversedDepthFuncs}=await import(pathToFileURL(path.join(upstream,'src/utils.js')));
const scenariosPath=path.resolve(process.argv[4]??fileURLToPath(new URL('./webgl-state-scenarios.jsonl',import.meta.url)));
const constants=JSON.parse(fs.readFileSync(new URL('./WebGLStateConstants.json',import.meta.url),'utf8'));
function encode(value){
    if(value===undefined)return 'undefined';if(value===null||typeof value==='boolean'||typeof value==='string')return value;
    if(typeof value==='number'){if(Number.isNaN(value))return 'NaN';if(!Number.isFinite(value))return value<0?'-Infinity':'Infinity';if(Object.is(value,-0))return '-0';return value;}
    if(typeof value==='symbol')return 'symbol';if(typeof value==='function')return 'function';
    if(Object.hasOwn(value,'tag'))return {ref:String(value.tag)};
    if(value instanceof Uint8Array)return {typed:'Uint8Array',data:Array.from(value,encode)};
    if(Array.isArray(value))return Array.from(value,encode);
    const out=Object.create(null);for(const key of Object.keys(value)){const d=Object.getOwnPropertyDescriptor(value,key);out[key]=Object.hasOwn(d,'value')?encode(d.value):'accessor';}return out;
}
function decode(state,value){
    if(value===null||typeof value!=='object')return value;
    if(Object.hasOwn(value,'$ref'))return state.objects[value.$ref];
    if(Object.hasOwn(value,'$'))return {undefined:undefined,NaN:NaN,Infinity:Infinity,'-Infinity':-Infinity,'-0':-0,symbol:state.symbol}[value.$];
    if(Object.hasOwn(value,'$coerce')){const spec=value.$coerce;return {tag:spec.name,[Symbol.toPrimitive](hint){state.log.push(['coerce',spec.name,hint]);actions(state,spec.action);if(spec.throw)throw RangeError('coercion');return decode(state,spec.value);}};}
    if(Object.hasOwn(value,'$nativeObject3D'))return Object.assign(new Object3D(),{tag:'Object3D'});
    if(Object.hasOwn(value,'$nativeQuaternion')){const out=new Quaternion();if(value.$nativeQuaternion)out._onChange(()=>{state.log.push(['onChange','Quaternion']);});return Object.assign(out,{tag:'Quaternion'});}
    if(Object.hasOwn(value,'$nativeEuler')){const out=new Euler();if(value.$nativeEuler)out._onChange(()=>{state.log.push(['onChange','Euler']);});return Object.assign(out,{tag:'Euler'});}
    if(Object.hasOwn(value,'$vector')||Object.hasOwn(value,'$color')){
        const isColor=Object.hasOwn(value,'$color'),out=Object.create(isColor?Color.prototype:Vector4.prototype),values=value[isColor?'$color':'$vector'];
        if(isColor)out.isColor=true;Array.from(isColor?'rgb':'xyzw').forEach((key,i)=>out[key]=decode(state,values[i]));return out;
    }
    const out=Array.isArray(value)?[]:{};for(const key of Object.keys(value))Object.defineProperty(out,key,{value:decode(state,value[key]),writable:true,enumerable:true,configurable:true});return out;
}
function getPath(state,value){let out=state.objects[typeof value==='string'?value:value[0]];if(Array.isArray(value))for(const key of value.slice(1))out=out[key];return out;}
function actions(state,list){if(list)for(const op of list)execute(state,op);}
function trigger(state,name){if(state.trigger===name&&!state.acted){state.acted=true;actions(state,state.action);}}
function execute(state,op){
    const [kind,path,key,value]=op;
    if(kind==='save'){state.objects[path]=getPath(state,key);return;}
    if(kind==='nativeRegister'){const target=getPath(state,path),spec=value??{};let count=0;target._onChange(()=>{state.log.push(['nativeCallback',key,++count]);actions(state,spec.action);if(spec.throw)throw RangeError('native callback');});return;}
    if(kind==='nativeSet'){getPath(state,path).x=decode(state,key);return;}
    if(kind==='callbackFlags'){Object.defineProperty(getPath(state,path),'_onChangeCallback',key);return;}
    if(kind==='call'||kind==='construct'||kind==='borrow'){
        const self=getPath(state,path),fn=self[key],args=value.map(item=>decode(state,item));
        if(kind==='construct')return Reflect.construct(fn,args);
        return fn.apply(kind==='borrow'?decode(state,op[4]):self,args);
    }
    if(kind==='descriptor'){const d=Object.getOwnPropertyDescriptor(getPath(state,path),key);return d?{own:true,writable:d.writable??false,enumerable:d.enumerable??false,configurable:d.configurable??false,value:Object.hasOwn(d,'value')?encode(d.value):'accessor'}:{own:false};}
    if(kind==='getKey')return getPath(state,path)[decode(state,key)];
    if(kind==='setKey')getPath(state,path)[decode(state,key)]=decode(state,value);
    else if(kind==='set')getPath(state,path)[key]=decode(state,value);
    else if(kind==='define')Object.defineProperty(getPath(state,path),key,{value:decode(state,value),writable:true,enumerable:true,configurable:true});
    else if(kind==='delete')delete getPath(state,path)[key];
    else if(kind==='object')state.objects[path]=decode(state,key);
    else if(kind==='fail')state.fail=path;
    else if(kind==='trigger'){state.trigger=path;state.action=key;state.acted=false;}
    else if(kind==='watch'){
        const target=getPath(state,path),spec=value;let current=target[key];
        Object.defineProperty(target,key,{enumerable:true,configurable:true,get(){state.log.push(['getter',spec.label??path,key]);actions(state,spec.action);if(spec.throw)throw RangeError('getter');return current;},set(value){state.log.push(['setter',spec.label??path,key,encode(value)]);actions(state,spec.setAction);if(spec.setThrow)throw RangeError('setter');current=value;}});
    }else if(kind==='gc')global.gc?.();
    else if(kind==='function'){
        const target=getPath(state,path),original=target[key],spec=value;
        Object.defineProperty(target,key,{writable:true,enumerable:true,configurable:true,value:function(...args){
            state.log.push(['override',spec.label,encode(this),...args.map(encode)]);actions(state,spec.action);if(spec.throw)throw RangeError('override');
            if(spec.forward)return original.apply(this,args);if(spec.returnThis)return this;return decode(state,spec.result);
        }});
    }
    else if(kind==='clear')state.log.length=0;
    else if(kind==='metadata'){
        const fn=getPath(state,path)[key],d=Object.getOwnPropertyDescriptor(fn,'prototype');
        return {name:fn.name,length:fn.length,own:Object.getOwnPropertyNames(fn),prototypeFlags:d?[d.writable,d.enumerable,d.configurable]:[],source:Function.prototype.toString.call(fn),constructible:!!d};
    }
}
class Fixture {
    constructor(config){
        const state=this.state=Object.assign(Object.create(null),{log:[],next:0,max:8,version:'WebGL 2.0',scissor:[0,0,640,480],viewport:[0,0,640,480],blockIndex:7,
            fail:'',trigger:'',acted:false,action:[],thrown:{tag:'thrown'},symbol:Symbol('slot'),objects:Object.create(null)});
        const gl=this.gl={},extensions=this.extensions={},extension=this.extension={};Object.assign(state,{gl,extensions,extension});Object.assign(state.objects,{gl,extensions,extension,state});
        for(const key of Object.keys(config))if(key!=='before')state[key]=decode(state,config[key]);
        for(const name of ['getParameter','createTexture','bindTexture','texParameteri','texImage2D','texImage3D','enable','disable','colorMask','clearColor','depthMask','depthFunc','clearDepth','stencilMask','stencilFunc','stencilOp','clearStencil','frontFace','cullFace','blendEquation','blendFuncSeparate','blendFunc','blendEquationSeparate','blendColor','lineWidth','polygonOffset','activeTexture','bindFramebuffer','useProgram','drawBuffers','pixelStorei','scissor','viewport','getUniformBlockIndex','uniformBlockBinding','bindBufferBase','compressedTexImage2D','compressedTexImage3D','texStorage2D','texStorage3D','texSubImage2D','texSubImage3D','compressedTexSubImage2D','compressedTexSubImage3D'])gl[name]=command(name,'gl');
        function command(name,surface){return function(...args){state.log.push([name,this===state[surface],...args.map(encode)]);trigger(state,name);if(state.fail===name)throw state.thrown;
            if(name==='getParameter'){if(args[0]===gl.MAX_COMBINED_TEXTURE_IMAGE_UNITS)return state.max;if(args[0]===gl.VERSION)return state.version;if(args[0]===gl.SCISSOR_BOX)return state.scissor;if(args[0]===gl.VIEWPORT)return state.viewport;return `parameter:${args[0]}`;}
            if(name==='createTexture')return {tag:`texture:${++state.next}`};if(name==='get')return state.extension;if(name==='getUniformBlockIndex')return state.blockIndex;
        };}
        extensions.get=command('get','extensions');extension.clipControlEXT=command('clipControlEXT','extension');Object.assign(gl,constants);
        Object.assign(extension,{LOWER_LEFT_EXT:36001,ZERO_TO_ONE_EXT:37727,NEGATIVE_ONE_TO_ONE_EXT:37726});
        gl.canvas={width:640,height:480};Object.assign(state.objects,{canvas:gl.canvas,quaternionPrototype:Quaternion.prototype,eulerPrototype:Euler.prototype,vector2Prototype:Vector2.prototype,vector3Prototype:Vector3.prototype,vectorPrototype:Vector4.prototype,colorPrototype:Color.prototype,weakMapPrototype:WeakMap.prototype,stringPrototype:String.prototype,regexpPrototype:RegExp.prototype,reversed:ReversedDepthFuncs});
        state.vectorPrototype=Vector4.prototype;
        setConsoleFunction(function(...args){state.log.push(['console',this===undefined,...args.map(encode)]);trigger(state,'console');if(state.fail==='console')throw state.thrown;});
        actions(state,config.before);state.objects.manager=this.manager=WebGLState(gl,extensions);state.objects.constructorHolder={ctor:WebGLState};
    }
}
function scenario(input){let results=[],trace=null;
    // C++ releases each fixture's application realm. Restore the development
    // oracle realm's shared prototypes between fixtures, preserving mutations
    // and pollution throughout each complete scenario.
    const prototypes=[Object.prototype,Array.prototype,Color.prototype,Quaternion.prototype,Euler.prototype,Vector2.prototype,Vector3.prototype,Vector4.prototype,WeakMap.prototype,String.prototype,RegExp.prototype,ReversedDepthFuncs];
    const before=prototypes.map(value=>Object.getOwnPropertyDescriptors(value));
    try{const f=new Fixture(input.config);for(const op of input.ops){try{results.push(encode(execute(f.state,op)));}catch(e){results.push(e===f.state.thrown?{thrown:encode(e)}:e instanceof Error?{error:e.name}:{thrown:encode(e)});}}trace=encode(f.state.log);}
    catch(e){results.push(e instanceof Error?{constructorError:e.name}:{constructorThrown:encode(e)});}
    finally{
        setConsoleFunction(null);
        prototypes.forEach((value,index)=>{for(const key of Reflect.ownKeys(value))if(!Object.hasOwn(before[index],key))Reflect.deleteProperty(value,key);Object.defineProperties(value,before[index]);});
    }
    return {name:input.name,results,trace};
}
const cases=fs.readFileSync(scenariosPath,'utf8').trim().split(/\r?\n/).map(line=>JSON.parse(line));
const native=spawnSync(probe,[scenariosPath],{encoding:'utf8',maxBuffer:128*1024*1024,timeout:900000});
fs.writeFileSync(process.argv[5]??probe+'.actual.jsonl',native.stdout);fs.writeFileSync(probe+'.stderr.log',native.stderr);
assert.equal(native.status,0,`Native probe exited ${native.status}; ${native.stderr}`);
const actual=native.stdout.trim().split(/\r?\n/).map(line=>JSON.parse(line));assert.equal(actual.length,cases.length);
let comparisons=0;
function compare(a,b,location){comparisons++;if(a!==null&&typeof a==='object'&&b!==null&&typeof b==='object'){assert.equal(Array.isArray(a),Array.isArray(b),location);assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),location);for(const key of Object.keys(b))compare(a[key],b[key],`${location}.${key}`);}else assert.deepEqual(a,b,location);}
const expected=[];
for(let i=0;i<cases.length;i++){expected.push(scenario(cases[i]));try{compare(actual[i],expected[i],cases[i].name);}catch(error){fs.writeFileSync(probe+'.expected.jsonl',expected.map(value=>JSON.stringify(value)).join('\n')+'\n');throw error;}global.gc?.();}
fs.writeFileSync(probe+'.expected.jsonl',expected.map(value=>JSON.stringify(value)).join('\n')+'\n');
console.log(JSON.stringify({scenarios:cases.length,comparisons}));
