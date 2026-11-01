import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import {spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const modes=['fresh','preventExtensions','freeze','nonconfigurable','replaceConstructor','deleteConstructor','replaceGlobal','deleteGlobal'];
export function oracle(mode){return vm.runInNewContext(`
const first=function sample(){};
const fp=Object.getPrototypeOf(first),earlyDescriptor=Object.getOwnPropertyDescriptor(fp,'constructor'),earlyConstructor=first.constructor,privateFunction=Function;
const out={earlyOwnConstructor:earlyDescriptor!==undefined,earlyConstructorName:earlyConstructor.name==='Function',earlyPrototypeIdentity:earlyConstructor.prototype===fp,earlyPrivateIdentity:earlyConstructor===privateFunction,earlyDescriptor:earlyDescriptor.writable&&!earlyDescriptor.enumerable&&earlyDescriptor.configurable,initialGlobalIdentity:globalThis.Function===privateFunction};
const marker=Object.create(null);marker.marker=17;
if(mode==='preventExtensions')Object.preventExtensions(fp);else if(mode==='freeze')Object.freeze(fp);else if(mode==='nonconfigurable')Object.defineProperty(fp,'constructor',{value:marker,writable:false,enumerable:false,configurable:false});else if(mode==='replaceConstructor')Object.defineProperty(fp,'constructor',{value:marker,writable:true,enumerable:false,configurable:true});else if(mode==='deleteConstructor')delete fp.constructor;else if(mode==='replaceGlobal')globalThis.Function=marker;else if(mode==='deleteGlobal')delete globalThis.Function;
const before=Object.getOwnPropertyDescriptor(fp,'constructor'),extensibleBefore=Object.isExtensible(fp);
try{
 const fixture=function* fixture(){},gfp=Object.getPrototypeOf(fixture),gf=gfp.constructor,generator=fixture();const after=Object.getOwnPropertyDescriptor(fp,'constructor');
 let same=(before===undefined)===(after===undefined);if(before&&after)same=same&&before.value===after.value&&before.writable===after.writable&&before.enumerable===after.enumerable&&before.configurable===after.configurable;
 Object.assign(out,{imported:true,prototypeDescriptorUnchanged:same,prototypeExtensibilityUnchanged:Object.isExtensible(fp)===extensibleBefore,privateFunctionStable:Object.getPrototypeOf(gf)===privateFunction,generatorConstructorParent:Object.getPrototypeOf(gf)===privateFunction,generatorFunctionPrototypeParent:Object.getPrototypeOf(gfp)===fp,newOrdinaryFunctionParent:Object.getPrototypeOf(function(){})===fp,generatorWorks:generator.next().done===true,globalUnchanged:mode==='replaceGlobal'?globalThis.Function===marker:mode==='deleteGlobal'?!Object.hasOwn(globalThis,'Function'):globalThis.Function===privateFunction});
}catch(e){out.error='#'+e.name;}
out`,{mode});}
const args=process.argv.slice(2),opts=new Map();for(let i=0;i<args.length;i+=2){if(!['--probe','--output'].includes(args[i])||!args[i+1]||opts.has(args[i]))throw Error('Usage: compare-function-bootstrap.mjs [--probe binary] --output new-directory');opts.set(args[i],args[i+1]);}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 if(!opts.has('--output'))throw Error('--output is required');const output=path.resolve(opts.get('--output'));if(fs.existsSync(output))throw Error('Evidence already exists');fs.mkdirSync(output,{recursive:true});
 const expected=modes.map(mode=>({mode,value:oracle(mode)}));fs.writeFileSync(path.join(output,'expected.json'),JSON.stringify(expected,null,2)+'\n');
 const summary={mode:opts.has('--probe')?'native-differential':'oracle-only',binaryExecuted:false,cases:modes.length,node:process.version,scope:'Fresh ordinary function before generator import; nonextensible/frozen/replaced Function.prototype; private Function global replacement; no dynamic source compiler claim'};
 if(opts.has('--probe')){const binary=path.resolve(opts.get('--probe')),before=createHash('sha256').update(fs.readFileSync(binary)).digest('hex');const actual=[],differences=[];for(const {mode,value}of expected){const r=spawnSync(binary,[mode],{encoding:'utf8',maxBuffer:1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});summary.binaryExecuted||=r.pid!==undefined;let decoded;try{decoded=JSON.parse(r.stdout);}catch{decoded={parseError:true};}actual.push({mode,value:decoded,status:r.status,stderr:r.stderr});for(const key of new Set([...Object.keys(value),...Object.keys(decoded)]))if(!Object.is(value[key],decoded[key]))differences.push({mode,key,expected:value[key],actual:decoded[key]});if(r.status!==0)differences.push({mode,status:r.status,error:r.error?.message});}
 const lifetime=spawnSync(binary,['--lifetime'],{encoding:'utf8',maxBuffer:1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2)+'\n');fs.writeFileSync(path.join(output,'differences.json'),JSON.stringify(differences,null,2)+'\n');fs.writeFileSync(path.join(output,'lifetime.txt'),String(lifetime.stdout??'')+String(lifetime.stderr??''));Object.assign(summary,{binary,binaryBeforeSHA256:before,binaryAfterSHA256:createHash('sha256').update(fs.readFileSync(binary)).digest('hex'),failedFields:differences.length,lifetimePassed:lifetime.status===0&&/3 passed/.test(lifetime.stdout)});process.exitCode=differences.length||!summary.lifetimePassed?1:0;}
 fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2)+'\n');console.log(JSON.stringify(summary));
}
