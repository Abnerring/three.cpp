import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),probe=path.resolve(process.argv[3]),input=path.resolve(process.argv[4]);
assert.equal(spawnSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).stdout.trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const constants=await import(pathToFileURL(path.join(upstream,'src/constants.js')));
const {getByteLength}=await import(pathToFileURL(path.join(upstream,'src/extras/TextureUtils.js')));
const source=fs.readFileSync(path.join(upstream,'src/extras/TextureUtils.js'),'utf8');
// The private helper is executed verbatim from the pinned module, with the actual imported constants.
const start=source.indexOf('function getTextureTypeByteLength('),end=source.indexOf('\n}',start)+2;
const getType=new Function(...Object.keys(constants),source.slice(start,end)+';return getTextureTypeByteLength;')(...Object.values(constants));
function encode(value){if(value===undefined)return 'undefined';if(typeof value==='number'){if(Number.isNaN(value))return 'NaN';if(!Number.isFinite(value))return value<0?'-Infinity':'Infinity';if(Object.is(value,-0))return '-0';}if(typeof value==='symbol')return 'symbol';if(typeof value==='function')return 'function';if(Array.isArray(value))return value.map(encode);if(value&&typeof value==='object')return Object.fromEntries(Object.keys(value).map(k=>[k,encode(value[k])]));return value;}
function decode(value,trace){
 if(value===null||typeof value!=='object')return value;
 if(Object.hasOwn(value,'$'))return {undefined:undefined,NaN,Infinity,'-Infinity':-Infinity,'-0':-0,symbol:Symbol('offset')}[value.$];
 function methodGetter(spec){return function(){trace.push([spec.label,'get']);if(spec.getThrow)throw RangeError('conversion getter');if(Object.hasOwn(spec,'methodValue'))return decode(spec.methodValue,trace);return function(hint){trace.push([spec.label,'call',encode(hint)]);if(spec.gc)global.gc?.();if(spec.throw)throw RangeError('conversion');return decode(spec.value,trace);};};}
 if(Object.hasOwn(value,'$convert'))return Object.defineProperty({},Symbol.toPrimitive,{get:methodGetter(value.$convert),enumerable:true,configurable:true});
 if(Object.hasOwn(value,'$ordinary')){const out={};for(const [key,spec]of Object.entries(value.$ordinary))Object.defineProperty(out,key,{get:methodGetter(spec),enumerable:true,configurable:true});return out;}
 return Array.isArray(value)?value.map(v=>decode(v,trace)):Object.fromEntries(Object.entries(value).map(([key,v])=>[key,decode(v,trace)]));
}
function scenario(spec){const trace=[];let result;try{const args=spec.args.map(v=>decode(v,trace));if(spec.mode==='type'){const a=getType(...args),b=getType(...args);result={value:encode(a),own:Object.getOwnPropertyNames(a),descriptors:Object.getOwnPropertyNames(a).map(n=>{const d=Object.getOwnPropertyDescriptor(a,n);return [d.writable,d.enumerable,d.configurable];}),fresh:a!==b,ordinaryPrototype:Object.getPrototypeOf(a)===Object.prototype};}else result=encode(getByteLength(...args));}catch(e){result=e instanceof TypeError?{error:'TypeError'}:e instanceof RangeError?{error:'RangeError'}:{error:e.name,message:e.message};}return {name:spec.name,result,trace};}
const cases=fs.readFileSync(input,'utf8').trim().split(/\r?\n/).map(JSON.parse);
const native=spawnSync(probe,[input],{encoding:'utf8',maxBuffer:128*1024*1024,timeout:900000});
fs.writeFileSync(process.argv[5]??probe+'.actual.jsonl',native.stdout);fs.writeFileSync(probe+'.stderr.log',native.stderr);assert.equal(native.status,0,native.stderr);
const actual=native.stdout.trim().split(/\r?\n/).map(JSON.parse);assert.equal(actual.length,cases.length);
const expected=cases.map(scenario);fs.writeFileSync(probe+'.expected.jsonl',expected.map(v=>JSON.stringify(v)).join('\n')+'\n');
let comparisons=0;function compare(a,b,location){comparisons++;if(a!==null&&typeof a==='object'&&b!==null&&typeof b==='object'){assert.equal(Array.isArray(a),Array.isArray(b),location);assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),location);for(const key of Object.keys(b))compare(a[key],b[key],location+'.'+key);}else assert.deepEqual(a,b,location);}
for(let i=0;i<cases.length;i++)compare(actual[i],expected[i],cases[i].name);console.log(JSON.stringify({scenarios:cases.length,comparisons}));
