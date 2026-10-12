import assert from 'node:assert/strict';import fs from 'node:fs';import path from 'node:path';import {execFileSync} from 'node:child_process';import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=p=>import(pathToFileURL(path.join(upstream,`src/${p}.js`)));
const {AnimationMixer}=await load('animation/AnimationMixer'),{AnimationClip}=await load('animation/AnimationClip'),{AnimationObjectGroup}=await load('animation/AnimationObjectGroup'),{AnimationAction}=await load('animation/AnimationAction'),{Mesh}=await load('objects/Mesh');
console.warn=()=>{};console.error=()=>{};
const wire=(k,v)=>v===undefined?{$undefined:true}:typeof v==='number'&&(!Number.isFinite(v)||Object.is(v,-0))?{$number:Object.is(v,-0)?'-0':String(v)}:v;
function encode(v){if(v===undefined)return '$undefined';if(typeof v==='number')return Number.isNaN(v)?'$NaN':v===Infinity?'$Infinity':v===-Infinity?'$-Infinity':Object.is(v,-0)?'$-0':v;if(Array.isArray(v))return Array.from(v,encode);if(v!==null&&typeof v==='object'){const out={};for(const k of Object.keys(v)){const c=v[k];if(typeof c!=='function')out[k]=encode(c);}return out;}return v;}
function id(list,value){if(value===null||value===undefined)return value;const i=list.indexOf(value);if(i!==-1)return i;list.push(value);return list.length-1;}
const numbers=a=>Array.from(a,encode),components=(v,ks)=>ks.map(k=>encode(v[k]));
function prototypeMetadata(){return [AnimationAction,AnimationObjectGroup,AnimationMixer].map(constructor=>{
    const p=constructor.prototype,keys=Object.getOwnPropertyNames(p),constructorKeys=Object.getOwnPropertyNames(constructor);
    const properties=keys.map(key=>{const d=Object.getOwnPropertyDescriptor(p,key);return {key,writable:d.writable,enumerable:d.enumerable,configurable:d.configurable,name:d.value.name,length:d.value.length};});
    const constructorProperties=constructorKeys.map(key=>{const d=Object.getOwnPropertyDescriptor(constructor,key);return {key,writable:d.writable,enumerable:d.enumerable,configurable:d.configurable};});
    let call=null;try{constructor.call({});}catch(e){call={name:e.name,message:e.message};}
    return {name:constructor.name,length:constructor.length,keys,constructorKeys,properties,constructorProperties,prototypeIdentity:constructor.prototype===p,baseObject:Object.getPrototypeOf(p)===Object.prototype,baseFunction:Object.getPrototypeOf(constructor)===Function.prototype,constructorInheritsPrototypeConstructor:Object.getPrototypeOf(constructor)===Object.getPrototypeOf(p).constructor,call};
});}
function runEnumeration(r){
    const bindings=r.forIn==='bindings',root={uuid:'root'},m=new AnimationMixer(root),prototype={},table=Object.create(prototype),trace=[];
    const entry=name=>bindings?{name,restoreOriginalState:callback}:{actionByRoot:{root:{name}}};
    function callback(value){const a=value??this;trace.push(a.name);if(a.name!=='a')return;
        if(r.mutation==='deleteOwn')delete table.b;
        else if(r.mutation==='nonenumOwn')Object.defineProperty(table,'b',{enumerable:false});
        else if(r.mutation==='nonenumProto')Object.defineProperty(prototype,'c',{enumerable:false});
        else if(r.mutation==='deleteProto')delete prototype.c;
        else if(r.mutation==='addProto')prototype.z=entry('z');
        else if(r.mutation==='replaceProto')Object.setPrototypeOf(table,{z:entry('z')});
        else if(r.mutation==='hideInherited')Object.defineProperty(table,'c',{value:entry('hidden'),enumerable:false,configurable:true});
        else if(r.mutation==='readdOwn'){delete table.b;table.b=entry('readded');}
    }
    Object.defineProperty(prototype,'b',{value:entry('inherited'),enumerable:r.enumerable,configurable:true,writable:true});prototype.c=entry('c');table.a=entry('a');table.b=entry('b');
    const collection=Object.hasOwn(r,'value')?r.value:table;
    if(bindings){m._bindingsByRootAndName={root:collection};m._removeInactiveBinding=()=>{};m._actionsByClip=Object.create(null);}
    else{m._actionsByClip=collection;m._deactivateAction=callback;m._removeInactiveAction=()=>{};}
    if(r.prototypeGetter)Object.defineProperty(Object.prototype,'enumInherited',{enumerable:true,configurable:true,get(){trace.push(typeof this);return entry('primitiveInherited');}});
    let error=null;try{m.uncacheRoot(root);}catch(e){error=e.name;}finally{if(r.prototypeGetter)delete Object.prototype.enumInherited;}return {trace,error};
}
function snapshot(m,roots,actions,bindingIds,group){
    const out={time:encode(m.time),timeScale:encode(m.timeScale),accu:encode(m._accuIndex),stats:encode(m.stats)};
    out.objects=roots.map(r=>({position:components(r.position,['x','y','z']),quaternion:components(r.quaternion,['x','y','z','w']),rotation:components(r.rotation,['x','y','z']),visible:encode(r.visible),label:encode(r.label),matrixWorldNeedsUpdate:encode(r.matrixWorldNeedsUpdate),morph:numbers(r.morphTargetInfluences),opacity:encode(r.material.opacity),color:components(r.material.color,['r','g','b']),materialVersion:encode(r.material.version)}));
    out.actions=actions.map(a=>{const e={};for(const k of ['time','timeScale','weight','enabled','paused','blendMode','loop','repetitions','_loopCount','_startTime','_effectiveTimeScale','_effectiveWeight','_cacheIndex','_byClipCacheIndex','_restoreTimeScale'])e[k]=encode(a[k]);e.localRoot=a._localRoot===null?null:encode(a._localRoot.uuid);e.bindings=a._propertyBindings.map(b=>id(bindingIds,b));e.sharedResult=a._interpolants.map((x,i)=>x.resultBuffer===a._propertyBindings[i].buffer);e.settings=encode(a._interpolantSettings);return e;});
    out.actionOrder=m._actions.map(a=>id(actions,a));out.bindings=m._bindings.map(b=>({id:id(bindingIds,b),cache:encode(b._cacheIndex),use:encode(b.useCount),refs:encode(b.referenceCount),buffer:numbers(b.buffer)}));
    out.controls=m._controlInterpolants.map(c=>({cache:encode(c.__cacheIndex),times:numbers(c.parameterPositions),values:numbers(c.sampleValues),result:numbers(c.resultBuffer),shared:c.resultBuffer===m._controlInterpolants[0].resultBuffer}));
    if(group){out.groupStats=encode(group.stats);out.groupOrder=group._objects.map(o=>o.uuid);}return out;
}
function run(r){
    if(Object.hasOwn(r,'metadata'))return prototypeMetadata();
    if(Object.hasOwn(r,'forIn'))return runEnumeration(r);
    const roots=Array.from({length:3},(_,i)=>{const o=new Mesh();o.uuid=`root${i}`;o.name=`root${i}`;o.position.set(1,2,3);o.morphTargetInfluences=[.1,.2];o.morphTargetDictionary={smile:0,frown:1};o.material.opacity=.7;o.material.color.setRGB(.2,.4,.6);o.label='initial';return o;});
    if(r.hierarchy)roots[0].add(roots[2]);
    const clips=r.clips.map(c=>AnimationClip.parse(structuredClone(c)));for(const root of roots)root.animations.push(...clips);
    const group=r.group?new AnimationObjectGroup(roots[0],roots[1]):undefined;if(group)group.uuid='group';const root=group??roots[0],m=new AnimationMixer(root),slots={},actions=[],bindingIds=[],events=[];let fired=false;
    function listener(e){const a=e.action;events.push({type:e.type,direction:encode(e.direction),loopDelta:encode(e.loopDelta),action:id(actions,a),time:encode(a.time),enabled:encode(a.enabled),target:e.target===this});
        if(!fired){fired=true;if(r.eventOp==='stopSelf')a.stop();else if(r.eventOp==='stopAll')this.stopAllAction();else if(r.eventOp==='playOther')slots.b.play();else if(r.eventOp==='uncacheSelf')this.uncacheAction(a._clip,a._localRoot);else if(r.eventOp==='update')this.update(.125);}
    }
    m.addEventListener('loop',listener);m.addEventListener('finished',listener);const steps=[];
    for(const op of r.operations){let result;const clip=typeof op.clip==='string'?op.clip:clips[op.clip??0],local=op.root===undefined?undefined:roots[op.root];
        try{
            if(op.op==='action'||op.op==='existing'){const a=m[op.op==='action'?'clipAction':'existingAction'](clip,local,op.blend);if(Object.hasOwn(op,'slot'))slots[op.slot]=a;result=encode(id(actions,a));}
            else if(op.op==='actionMethod'){const a=slots[op.slot],v=a[op.method](...(op.args??[]).map(x=>x&&typeof x==='object'&&Object.hasOwn(x,'action')?slots[x.action]:x));result=v!==null&&typeof v==='object'?id(actions,v):encode(v);}
            else if(op.op==='set'){Object.assign(Object.hasOwn(op,'slot')?slots[op.slot]:m,op.values);result='$undefined';}
            else if(op.op==='uncacheClip'){m.uncacheClip(clip);result='$undefined';}
            else if(op.op==='uncacheAction'){m.uncacheAction(clip,local);result='$undefined';}
            else if(op.op==='uncacheRoot'){m.uncacheRoot(local??root);result='$undefined';}
            else if(op.op==='groupAdd'||op.op==='groupRemove'||op.op==='groupUncache'){group[op.op==='groupAdd'?'add':op.op==='groupRemove'?'remove':'uncache'](local);result='$undefined';}
            else if(op.op==='gc')result='$undefined';
            else{const v=m[op.op](...(Object.hasOwn(op,'value')?[op.value]:[]));result=v===m?'$mixer':encode(v);}
        }catch(e){result={error:e.name};}
        steps.push({result,state:snapshot(m,roots,actions,bindingIds,group)});
    }
    return {steps,events};
}
const action=(slot='a',clip=0,extra={})=>({op:'action',slot,clip,...extra}),method=(slot,name,...args)=>({op:'actionMethod',slot,method:name,args}),update=value=>({op:'update',value});
function clip(profile,index=0,extra={}){const offset=index?10:0;const track=(name,type,values)=>({name,type,times:[0,1,2],values});let tracks;
    if(profile==='vector')tracks=[track('.position','vector',[0,1,2,2,3,4,4,5,6].map(x=>x+offset))];
    else if(profile==='quaternion')tracks=[track('.quaternion','quaternion',[0,0,0,1,0,.7071067811865476,0,.7071067811865476,0,1,0,0])];
    else if(profile==='color')tracks=[track('.material.color','color',[.1,.2,.3,.4,.5,.6,.7,.8,.9])];
    else if(profile==='material')tracks=[track('.material.opacity','number',[.2,.4,.8])];
    else if(profile==='morph')tracks=[track('.morphTargetInfluences[smile]','number',[0,.5,1])];
    else if(profile==='bool')tracks=[track('.visible','bool',[false,true,false])];
    else if(profile==='string')tracks=[track('.label','string',['a','b','c'])];
    else if(profile==='multi')tracks=[track('.position[x]','number',[offset,2+offset,4+offset]),track('.material.opacity','number',[.2,.4,.8]),track('.morphTargetInfluences[smile]','number',[0,.5,1])];
    else tracks=[track('.position[x]','number',[offset,2+offset,4+offset])];
    return {name:`clip${index}`,uuid:`clip${index}`,duration:2,tracks,...extra};
}
const cases=[];
for(const profile of ['scalar','vector','quaternion','color','material','morph','bool','string','multi'])for(const blend of [2500,2501])cases.push({clips:[clip(profile)],operations:[action('a',0,{blend}),method('a','play'),update(.25),update(.75),{op:'setTime',value:.5},{op:'gc'},method('a','stop'),update(.1),{op:'uncacheClip'}]});
for(const loop of [2200,2201,2202])for(const timeScale of [-2,-1,0,1,2])for(const clampWhenFinished of [false,true])cases.push({clips:[clip('scalar')],operations:[action(),{op:'set',slot:'a',values:{loop,timeScale,repetitions:2,clampWhenFinished}},method('a','play'),update(0),update(.5),update(2),update(7),update(-1)]});
for(const profile of ['scalar','vector','quaternion','string'])cases.push({clips:[clip(profile),clip(profile,1)],operations:[action('a'),action('b',1),method('a','play'),update(.25),method('b','play'),update(.25),method('b','setEffectiveWeight',.25),update(.25),method('a','stop'),update(.25),method('b','stop'),{op:'uncacheAction',clip:0},{op:'gc'},{op:'uncacheClip',clip:1}]});
for(const warp of [false,true])for(const duration of [0,.5,2])cases.push({clips:[clip('scalar'),clip('scalar',1,{duration:4})],operations:[action('a'),action('b',1),method('a','play'),method('a','fadeIn',duration),update(.25),method('b','play'),method('b','crossFadeFrom',{action:'a'},duration,warp),update(.5),update(2),method('b','halt',.5),update(.25),update(.5),{op:'stopAllAction'}]});
for(const eventOp of ['stopSelf','stopAll','playOther','uncacheSelf','update'])for(const loop of [2200,2201,2202])cases.push({eventOp,clips:[clip('scalar'),clip('scalar',1)],operations:[action('a'),action('b',1),{op:'set',slot:'a',values:{loop}},method('a','play'),update(2.5),update(.5),{op:'gc'}]});
for(const op of ['uncacheAction','uncacheClip','uncacheRoot'])cases.push({clips:[clip('multi')],operations:[action('a'),action('same'),action('b',0,{root:1}),method('a','play'),method('b','play'),update(.5),{op,root:1},update(.5),method('a','stop'),{op},method('a','play'),update(.25),{op:'gc'}]});
cases.push({clips:[clip('scalar')],operations:[{op:'existing',clip:'missing'},action('a', 'clip0'),action('same','clip0'),{op:'existing',clip:'clip0'},action('other',0,{blend:2501}),method('a','play'),method('other','play'),update(.5),{op:'uncacheClip'},{op:'existing',clip:0}]});
for(const profile of ['scalar','vector','quaternion','material','morph','multi'])cases.push({group:true,clips:[clip(profile)],operations:[action(),method('a','play'),update(.25),{op:'groupRemove',root:0},update(.25),{op:'groupAdd',root:2},update(.25),{op:'groupAdd',root:0},update(.25),{op:'groupUncache',root:1},{op:'gc'},update(.25),method('a','stop'),{op:'uncacheClip'}]});
for(const delta of [NaN,Infinity,-Infinity,-0,'0.25',null])cases.push({clips:[clip('scalar')],operations:[action(),method('a','play'),update(delta)]});
for(const interpolation of [2300,2301,2302]){const c=clip('vector');c.tracks[0].interpolation=interpolation;cases.push({clips:[c],operations:[action(),method('a','play'),update(.125),update(.625),update(1.5)]});}
for(const start of [-1,0,.5,2])cases.push({clips:[clip('scalar')],operations:[action(),method('a','startAt',start),method('a','play'),method('a','isScheduled'),method('a','isRunning'),update(.25),update(.5),{op:'set',slot:'a',values:{paused:true}},update(2),{op:'set',slot:'a',values:{paused:false}},update(.25),method('a','reset'),update(.125)]});
for(const mode of [2200,2201,2202]){const c=clip('scalar');c.duration=0;c.tracks[0].times=[0];c.tracks[0].values=[7];cases.push({clips:[c],operations:[action(),method('a','setLoop',mode,2),method('a','play'),update(0),update(.1),update(-.1),method('a','stop')]});}
for(const profile of ['scalar','vector','quaternion','material','morph']){const c=clip(profile);c.tracks[0].name='root2'+c.tracks[0].name;cases.push({hierarchy:true,clips:[c],operations:[action(),method('a','play'),update(.25),{op:'gc'},update(.5),method('a','stop')]});}
for(const profile of ['scalar','vector','quaternion','string']){const c=clip(profile);const second=structuredClone(clip(profile,1).tracks[0]);c.tracks.push(second);cases.push({clips:[c],operations:[action(),method('a','play'),update(.25),update(.5),method('a','stop'),{op:'uncacheClip'}]});}
cases.push({clips:[clip('scalar')],operations:[action('a'),action('b',0,{root:1}),method('a','play'),method('b','play'),update(.5),{op:'uncacheRoot',root:1},method('b','play')]});
cases.push({clips:[clip('scalar',0,{tracks:[]})],operations:[action(),method('a','play'),update(.25),update(2),{op:'stopAllAction'},{op:'uncacheClip'}]});
for(const forIn of ['actions','bindings']){
    for(const enumerable of [false,true])for(const mutation of ['deleteOwn','nonenumOwn','nonenumProto','deleteProto','addProto','replaceProto','hideInherited','readdOwn'])cases.push({forIn,enumerable,mutation});
    for(const value of [undefined,null,0,false,'','x'])cases.push({forIn,value});
    for(const value of [undefined,null,0,false,''])cases.push({forIn,value,prototypeGetter:true});
}
cases.push({metadata:true});
const expected=cases.map(r=>{try{return run(r);}catch(e){return {error:e.name};}});
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases,wire),encoding:'utf8',maxBuffer:128*1024*1024}));assert.equal(actual.length,expected.length);let comparisons=0,failures=[];
function compare(a,b,p){if(a!==null&&b!==null&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),`${p} keys`);for(const k of Object.keys(a))compare(a[k],b[k],`${p}.${k}`);return;}assert.deepEqual(a,b,p);comparisons++;}
for(let i=0;i<cases.length;i++)try{compare(actual[i],expected[i],`case[${i}]`);}catch(e){failures.push({i,case:cases[i],message:e.message});}
if(failures.length){process.stderr.write(JSON.stringify(failures.slice(0,12),wire,2)+'\n');throw new Error(`${failures.length} Mixer parity failures`);}
console.log(`AnimationMixer parity: ${cases.length} scenarios / ${comparisons} exact comparisons; upstream ${lock.commit}`);
