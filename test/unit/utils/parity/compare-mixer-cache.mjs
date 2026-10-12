import { spawnSync, execFileSync } from 'node:child_process';
import fs from 'node:fs';
import { pathToFileURL } from 'node:url';
import path from 'node:path';
import assert from 'node:assert/strict';
const [upstream,binary]=process.argv.slice(2);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {AnimationMixer}=await import(pathToFileURL(path.join(upstream,'src/animation/AnimationMixer.js')));
const ref=name=>({ref:name});
function encode(v) {
 if(v===undefined)return '$undefined';if(typeof v==='function')return '$function';
 if(typeof v==='number'){if(Number.isNaN(v))return '$NaN';if(!Number.isFinite(v))return v<0?'$-Infinity':'$Infinity';if(Object.is(v,-0))return '$-0';}
 if(v===null||typeof v!=='object')return v;if(Object.hasOwn(v,'label'))return v.label;
 if(Array.isArray(v))return Array.from({length:v.length},(_,i)=>Object.hasOwn(v,i)?encode(v[i]):'$hole');
 return Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])]));
}
function run(request){
 const state={enabled:false,trace:[]},record=e=>{if(state.enabled)state.trace.push(e);},refs={};
 for(let i=0;i<2;++i)refs['r'+i]={uuid:'r'+i,label:'r'+i};
 const m=refs.m=new AnimationMixer(refs.r0);m.label='m';
 for(let i=0;i<2;++i)refs['c'+i]={uuid:'c'+i,tracks:[],label:'c'+i};
 for(let i=0;i<2;++i){const b=refs['b'+i]={label:'b'+i,binding:{rootNode:refs['r'+i],path:'.value'},referenceCount:i?1:2,useCount:0,_cacheIndex:null};
  for(const method of ['saveOriginalState','restoreOriginalState','apply'])b[method]=function(x){record(['call',this.label,method,x]);};}
 for(let i=0;i<3;++i)refs['a'+i]={label:'a'+i,_localRoot:i===1?refs.r1:null,_clip:refs[i===2?'c1':'c0'],_propertyBindings:[refs[i===1?'b1':'b0']],_interpolants:[],_cacheIndex:null,_byClipCacheIndex:null,time:0,
  stop(){record(['call',this.label,'stop']);m._deactivateAction(this);return this;},_update(...a){record(['call',this.label,'_update',...a]);}};
 function watch(o,name,keys){for(const key of keys){let value=o[key];Object.defineProperty(o,key,{configurable:true,enumerable:true,get(){record(['get',name,key]);return value;},set(v){record(['set',name,key,v&&typeof v==='object'&&'label'in v?v.label:v]);value=v;}});}}
 if(request.watch){watch(m,'m',Object.keys(m).filter(k=>k!=='label'));for(const name of ['r0','r1','c0','c1'])watch(refs[name],name,['uuid']);
  for(const name of ['a0','a1','a2'])watch(refs[name],name,['_localRoot','_clip','_propertyBindings','_interpolants','_cacheIndex','_byClipCacheIndex','time','stop','_update']);
  for(const name of ['b0','b1'])watch(refs[name],name,['binding','referenceCount','useCount','_cacheIndex','saveOriginalState','restoreOriginalState','apply']);
  watch(m,'m',['_isActiveAction','_bindAction','_addInactiveAction','_lendAction','_takeBackAction','_lendBinding','_takeBackBinding','_removeInactiveBinding','_removeInactiveBindingsForAction','_deactivateAction','_removeInactiveAction','existingAction','update']);}
 function resolve(v){if(v&&typeof v==='object'){if('ref'in v)return refs[v.ref];if('number'in v)return Number(v.number);if('undefined'in v)return undefined;}return v;}
 function snapshot(){const s={};for(const k of ['_actions','_nActiveActions','_actionsByClip','_bindings','_nActiveBindings','_bindingsByRootAndName','time','timeScale','_accuIndex','stats'])s[k]=m[k];s.objects={};
  for(const name of ['a0','a1','a2','b0','b1']){s.objects[name]={};for(const k of ['_cacheIndex','_byClipCacheIndex','time','referenceCount','useCount'])s.objects[name][k]=refs[name][k];}return encode(s);}
 return request.steps.map(step=>{state.trace=[];state.enabled=true;let result,error='';try{const target=refs[step.target??'m'];if('set'in step)target[step.set]=resolve(step.value);else result=target[step.method](...(step.args??[]).map(resolve));}catch(e){error=e.name;}
  state.enabled=false;return {error,result:encode(result),trace:encode(state.trace),state:snapshot()};});
}
const call=(method,...args)=>({method,args});
const initial=[call('_addInactiveBinding',ref('b0'),'r0','.value'),call('_addInactiveBinding',ref('b1'),'r1','.value'),...['a0','a1','a2'].map((a,i)=>call('_addInactiveAction',ref(a),i===2?'c1':'c0',i===1?'r1':'r0'))];
const requests=[];
for(const watch of [false,true]){
 const basic=[...initial,call('_activateAction',ref('a0')),call('_activateAction',ref('a1')),call('_activateAction',ref('a2')),call('update',.25),call('_deactivateAction',ref('a0')),call('stopAllAction'),call('setTime',2),call('uncacheAction',ref('c0'),ref('r1')),call('uncacheClip',ref('c1')),call('uncacheRoot',ref('r0'))];requests.push({watch,steps:basic});
 for(const method of ['uncacheClip','uncacheRoot','uncacheAction'])for(let i=0;i<3;++i)requests.push({watch,steps:[...initial,call('_activateAction',ref('a'+i)),call(method,ref(method==='uncacheRoot'?'r0':'c0')),call('stopAllAction')]});
 for(const value of [0,-2,'3',null,{undefined:true},{number:'NaN'},{number:'Infinity'},{number:'-0'}])requests.push({watch,steps:[...initial,call('_activateAction',ref('a0')),{set:'time',value},{set:'timeScale',value},call('update',.25),call('setTime',.5)]});
 for(const value of [0,1,2,'1',null,{undefined:true}])requests.push({watch,steps:[...initial,{target:'a0',set:'_cacheIndex',value},call('_isActiveAction',ref('a0')),call('_activateAction',ref('a0')),call('_deactivateAction',ref('a0'))]});
 for(let seed=1;seed<=35;++seed){let n=seed;const rand=k=>(n=(Math.imul(n,1664525)+1013904223)>>>0)%k;const steps=[...initial];for(let j=0;j<35;++j){const op=rand(6),a=ref('a'+rand(3));steps.push(op===0?call('update',(rand(40)-20)/8):op===1?call('setTime',rand(20)/7):op===2?call('stopAllAction'):call(op===3?'_activateAction':op===4?'_deactivateAction':'_isActiveAction',a));}requests.push({watch,steps});}
}
const expected=requests.map(run),p=spawnSync(binary,[],{input:requests.map(x=>JSON.stringify(x)).join('\n')+'\n',encoding:'utf8',maxBuffer:64*1024*1024});
if(p.status!==0)throw new Error(p.stderr||`probe exit ${p.status}`);const actual=p.stdout.trim().split(/\r?\n/).map(x=>JSON.parse(x));assert.equal(actual.length,expected.length);
let comparisons=0;
function compare(a,b,where){if(a===null||b===null||typeof a!=='object'||typeof b!=='object'){++comparisons;assert.deepEqual(a,b,where);return;}assert.deepEqual(Object.keys(a),Object.keys(b),where+' keys');for(const key of Object.keys(a))compare(a[key],b[key],where+'.'+key);}
for(let i=0;i<actual.length;++i)compare(actual[i],expected[i],'case'+i);
console.log(`${requests.length} mixer cache scenarios / ${comparisons} exact comparisons passed`);
