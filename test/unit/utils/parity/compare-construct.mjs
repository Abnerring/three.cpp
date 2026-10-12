import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';

// Constructors are language semantics: Node itself is the reference, without a Three.js adapter.
const binary=path.resolve(process.argv[2]);
const descriptor=(object,key)=>{const d=Object.getOwnPropertyDescriptor(object,key);return {writable:d.writable,enumerable:d.enumerable,configurable:d.configurable};};
const type=value=>value===null?'null':Array.isArray(value)?'array':typeof value;
function makeConstructor(callable,body,name='Example',arity=0){
    const value=callable?function(...args){return body(this,args);}:class{constructor(...args){return body(this,args);}};
    Object.defineProperty(value,'name',{value:name});Object.defineProperty(value,'length',{value:arity});return value;
}
function returned(kind){
    switch(kind){case 'null':return null;case 'false':return false;case 'number':return 12;case 'nan':return NaN;case 'negativeZero':return -0;case 'string':return 'return';case 'array':return new Array(2);case 'function':return ()=>{};case 'object':return {return:true};default:return undefined;}
}
function metadata(callable,arity){
    const value=makeConstructor(callable,()=>undefined,'Example',arity);
    return {kind:'metadata',callable,arity,names:Object.getOwnPropertyNames(value),prototypeNames:Object.getOwnPropertyNames(value.prototype),name:value.name,length:value.length,
        nameDescriptor:descriptor(value,'name'),lengthDescriptor:descriptor(value,'length'),prototypeDescriptor:descriptor(value,'prototype'),constructorDescriptor:descriptor(value.prototype,'constructor'),backlink:value.prototype.constructor===value};
}
function scenario(callable,returnKind,prototypeKind){
    const state={},value=returned(returnKind),log=[];
    const constructor=makeConstructor(callable,(self,args)=>{
        state.receiver=self;log.push('body');self.argument=args[0];self.second=args[1];if(returnKind==='throw')throw RangeError('body');return returnKind==='self'?self:value;
    },'Example',2);
    let target=constructor;
    if(prototypeKind!=='self'){
        target=makeConstructor(true,()=>{log.push('target');},'Target');
        if(prototypeKind==='object')target.prototype=Object.create(null);
        else if(prototypeKind==='null')target.prototype=null;
        else if(prototypeKind==='number')target.prototype=17;
        else if(prototypeKind==='array')target.prototype=[];
        else if(prototypeKind==='function')target.prototype=()=>{};
    }
    const expectedPrototype=target.prototype!==null&&['object','function'].includes(typeof target.prototype)?target.prototype:Object.prototype;
    let result,error='';try{result=Reflect.construct(constructor,[7,'argument'],target);}catch(e){error=e.name;}
    const receiver=state.receiver;
    const out={kind:'construct',callable,returned:returnKind,prototype:prototypeKind,error,resultType:type(result),sameReturned:value!==null&&['object','function'].includes(typeof value)&&result===value,
        sameReceiver:result===receiver,receiverPrototype:Object.getPrototypeOf(receiver)===expectedPrototype,receiverKeys:Object.keys(receiver),argument:receiver.argument,second:receiver.second,log:[...log]};
    const callReceiver={};error='';let callResult;try{callResult=constructor.call(callReceiver,3,'call');}catch(e){error=e.name;}
    Object.assign(out,{callError:error,callType:type(callResult),callKeys:Object.keys(callReceiver)});return out;
}
function invalid(targetKind,validBase){
    const log=[],base=validBase?class{}:()=>{};
    const target=targetKind==='function'?()=>{}:targetKind==='object'?{}:returned(targetKind);
    if(target!==null&&['object','function'].includes(typeof target))Object.defineProperty(target,'prototype',{get(){log.push('prototype');throw RangeError('getter');}});
    let error='';try{Reflect.construct(base,[],target);}catch(e){error=e.name;}
    return {kind:'invalid',target:targetKind,validBase,error,log};
}
function reentry(throws){
    const state={depth:0},log=[];
    const constructor=class{constructor(){const depth=state.depth;log.push(depth);state.escaped=this;if(depth===0){state.depth=1;new state.constructor();}if(throws)throw RangeError('body');}};
    state.constructor=constructor;let error='';try{new constructor();}catch(e){error=e.name;}
    return {kind:'reentry',throws,error,log,prototype:Object.getPrototypeOf(state.escaped)===constructor.prototype};
}
function intrinsic(array,prototypeKind,argumentMode){
    const constructor=array?Array:Object;let target=constructor;
    if(prototypeKind!=='self'){target=makeConstructor(true,()=>undefined,'Target');if(prototypeKind==='null')target.prototype=null;}
    const args=[[],[3],[{}],['a','b'],[-1]][argumentMode];let result,error='';try{result=Reflect.construct(constructor,args,target);}catch(e){error=e.name;}
    const prototype=target.prototype!==null&&['object','function'].includes(typeof target.prototype)?target.prototype:(array?Array.prototype:Object.prototype);
    return {kind:'intrinsic',array,prototype:prototypeKind,argumentMode,error,resultType:type(result),prototypeMatches:result!==null&&['object','function'].includes(typeof result)&&Object.getPrototypeOf(result)===prototype,
        sameArgument:args.length>0&&result===args[0],length:Array.isArray(result)?result.length:null,keys:result!==null&&['object','function'].includes(typeof result)?Object.keys(result):[]};
}
const expected=[];
for(const callable of [false,true])for(const arity of [0,2,4294967296])expected.push(metadata(callable,arity));
for(const callable of [false,true])for(const returned of ['undefined','null','false','number','nan','negativeZero','string','array','function','object','self','throw'])for(const prototype of ['self','other','object','null','number','array','function'])expected.push(scenario(callable,returned,prototype));
for(const validBase of [false,true])for(const target of ['undefined','null','false','number','string','function','object'])expected.push(invalid(target,validBase));
for(const throws of [false,true])expected.push(reentry(throws));
for(const array of [false,true])for(const prototype of ['self','other','null'])for(let argument=0;argument<5;++argument){if(!array&&prototype==='self'&&[1,3,4].includes(argument))continue;expected.push(intrinsic(array,prototype,argument));}
const actual=JSON.parse(execFileSync(binary,[],{encoding:'utf8',maxBuffer:16*1024*1024}));
assert.equal(actual.length,expected.length);let comparisons=0;
function compare(a,e,p){comparisons++;if(Array.isArray(e)){assert.ok(Array.isArray(a),p);assert.equal(a.length,e.length,p);e.forEach((value,i)=>compare(a[i],value,`${p}[${i}]`));}else if(e!==null&&typeof e==='object'){assert.deepEqual(Object.keys(a).sort(),Object.keys(e).sort(),p);for(const key of Object.keys(e))compare(a[key],e[key],`${p}.${key}`);}else assert.equal(a,e,p);}
for(let i=0;i<expected.length;++i){try{compare(actual[i],expected[i],`scenario ${i}`);}catch(error){console.error('actual',actual[i]);console.error('expected',expected[i]);throw error;}}
console.log(`Construct parity: ${expected.length} scenarios, ${comparisons} comparisons passed (Node ${process.version}).`);
