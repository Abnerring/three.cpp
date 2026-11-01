import {upstreamModule} from './threejs-source.mjs';
import fs from 'node:fs';
const q=JSON.parse(fs.readFileSync(0,'utf8')),events=[],originalMath=globalThis.Math,mathDescriptor=Object.getOwnPropertyDescriptor(globalThis,'Math'),numberToString=Number.prototype.toString;
const token={tag:'failure'};let gets=0,current;
const make=which=>{const object=Object.create(originalMath);Object.defineProperty(object,'PI',{configurable:true,get(){gets++;events.push('PI:'+which);if(q.throwAt===gets)throw token;if(q.symbolAt===gets)return Symbol('pi');if(q.kind==='replaceOnConvert'&&which===1)return {[Symbol.toPrimitive](hint){events.push('convert:'+hint);current=second;return 3;}};if(q.kind==='coercion')return which===1?'6':null;return which===1?3:6;}});return object;};
const first=make(1),second=make(2);current=first;
// A direct second lookup sees the same Math object unless conversion replaced it.
if(q.kind==='twoValues')Object.defineProperty(first,'PI',{get(){events.push('PI:'+(++gets));return gets===1?3:6;},configurable:true});
Object.defineProperty(globalThis,'Math',{configurable:true,get(){events.push('Math');return current;}});
Number.prototype.toString=function(...args){events.push('Number.toString');return numberToString.apply(this,args);};
const scalar=v=>typeof v==='number'?(Number.isNaN(v)?'#NaN':v===Infinity?'#Infinity':v===-Infinity?'#-Infinity':Object.is(v,-0)?'#-0':v):v;
const encode=e=>e instanceof Error?'#'+e.name:e;const out={};let firstError;
try{const mod=await upstreamModule('src/math/MathUtils.js');const fn=q.factory==='generateUUID'?mod.generateUUID:mod.clamp;out.constants=[scalar(mod.DEG2RAD),scalar(mod.RAD2DEG)];out.identity=fn===mod.MathUtils[q.factory??'clamp'];out.constantStable=mod.degToRad(1)===mod.DEG2RAD;}
catch(e){firstError=e;out.error=encode(e);try{await upstreamModule('src/math/MathUtils.js');}catch(secondError){out.secondError=encode(secondError);out.sameError=secondError===firstError;}}
finally{Object.defineProperty(globalThis,'Math',mathDescriptor);Number.prototype.toString=numberToString;}
out.events=events;process.stdout.write(JSON.stringify(out));
