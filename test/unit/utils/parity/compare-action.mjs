import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {AnimationAction}=await import(pathToFileURL(path.join(upstream,'src/animation/AnimationAction.js')));
const wire=(k,v)=>v===undefined?{$undefined:true}:typeof v==='number'&&(!Number.isFinite(v)||Object.is(v,-0))?{$number:Object.is(v,-0)?'-0':String(v)}:v;
function encode(v){if(v===undefined)return '$undefined';if(typeof v==='number')return Number.isNaN(v)?'$NaN':v===Infinity?'$Infinity':v===-Infinity?'$-Infinity':Object.is(v,-0)?'$-0':v;if(Array.isArray(v))return Array.from(v,encode);if(v!==null&&typeof v==='object'){const out={};for(const k of Object.keys(v)){const child=v[k];if(typeof child!=='function')out[k]=encode(child);}return out;}return v;}
function snapshot(a,m){
    const result={};
    for(const key of ['blendMode','loop','_loopCount','_startTime','time','timeScale','_effectiveTimeScale','weight','_effectiveWeight','repetitions','paused','enabled','clampWhenFinished','zeroSlopeAtStart','zeroSlopeAtEnd','_restoreTimeScale','_cacheIndex','_byClipCacheIndex','_interpolantSettings'])result[key]=encode(a[key]);
    for(const key of ['_timeScaleInterpolant','_weightInterpolant']){const c=a[key];result[key]=c===null?null:{id:encode(c.id),times:encode(c.parameterPositions),values:encode(c.sampleValues)};}
    result.active=encode(m.active);result.now=encode(m.time);return result;
}
function run(r){
    const trace=[],state={controlValue:1,eventOp:r.eventOp};
    const mixer={time:Object.hasOwn(r,'now')?r.now:0,_root:{},active:false,next:0,
        _activateAction(){this.active=true;trace.push(['activate']);},
        _deactivateAction(){this.active=false;trace.push(['deactivate']);},
        _isActiveAction(){trace.push(['isActive']);return this.active;},
        _lendControlInterpolant(){const id=this.next++;trace.push(['lend',id]);return {id,parameterPositions:[0,0],sampleValues:[0,0],evaluate(time){trace.push(['control',id,time]);return [state.controlValue];}};},
        _takeBackControlInterpolant(i){trace.push(['takeBack',i.id]);},
        dispatchEvent(e){const a=e.action;trace.push(['event',e.type,e.direction,e.loopDelta,a.time,a._loopCount,a.enabled,a.paused]);if(state.eventOp==='reset')a.reset();if(state.eventOp==='stop')a.stop();if(state.eventOp==='change'){a.weight=.25;a.time=99;a.enabled=true;}}
    };
    const clip=(duration,count)=>({duration,blendMode:2500,tracks:Array.from({length:count},(_,i)=>({createInterpolant(result){trace.push(['create',i,result===null]);return {evaluate(t){trace.push(['evaluate',i,t]);return[t];}};}}))});
    const count=r.count??2,source=clip(Object.hasOwn(r,'duration')?r.duration:2,count);
    const a=new AnimationAction(mixer,source,r.localRoot,r.blendMode),other=new AnimationAction(mixer,clip(4,0));
    for(let i=0;i<count;i++)a._propertyBindings[i]={accumulate(accu,w){trace.push(['accumulate',i,accu,w]);},accumulateAdditive(w){trace.push(['additive',i,w]);}};
    if(r.fields)Object.assign(a,r.fields);
    const coercion=(value,label)=>({value,valueOf(){trace.push(['coerce',label]);return this.value;}});
    if(r.coerce)for(const [key,value]of Object.entries(r.coerce))a[key]=coercion(value,key);
    const watch=(object,keys)=>{const storage={};for(const key of keys){storage[key]=object[key];Object.defineProperty(object,key,{configurable:true,enumerable:true,get(){trace.push(['get',key]);return storage[key];},set(value){trace.push(['set',key]);storage[key]=value;}});}};
    if(r.watch){
        watch(a,['enabled','paused','time','timeScale','weight','repetitions','_loopCount','_startTime','_mixer','_clip','_localRoot','_effectiveTimeScale','_effectiveWeight','_restoreTimeScale','_timeScaleInterpolant','_weightInterpolant','_interpolants','_propertyBindings','blendMode','loop','clampWhenFinished','zeroSlopeAtStart','zeroSlopeAtEnd','_interpolantSettings','warp','stopFading','stopWarping','reset','fadeIn','fadeOut','_setEndings','_updateTime','_updateTimeScale','_updateWeight','_scheduleFading']);
        watch(mixer,['time','_activateAction','_deactivateAction','_isActiveAction','_lendControlInterpolant','_takeBackControlInterpolant','dispatchEvent']);
        watch(a._interpolantSettings,['endingStart','endingEnd']);
    }
    const initial=snapshot(a,mixer),steps=[];
    for(const op of r.operations){let result;
        try{
            if(Object.hasOwn(op,'set')){Object.assign(op.target==='other'?other:op.target==='mixer'?mixer:op.target==='state'?state:a,op.set);result='$undefined';}
            else{const args=(op.args??[]).map((x,j)=>x&&typeof x==='object'&&Object.hasOwn(x,'$coerce')?coercion(x.$coerce,`arg${j}`):x==='$other'?other:x==='$self'?a:x);const v=a[op.method](...args);result=v===a?'$self':v===other?'$other':v===mixer?'$mixer':v===source?'$clip':v===mixer._root?'$root':encode(v);}
        }catch(e){result={error:e.name};}
        steps.push({result,state:snapshot(a,mixer),other:snapshot(other,mixer)});
    }
    return {initial,steps,trace:encode(trace)};
}
const op=(method,...args)=>({method,args}),cases=[];
cases.push({operations:[op('getMixer'),op('getClip'),op('getRoot'),op('isRunning'),op('isScheduled'),op('play'),op('isRunning'),op('startAt',2),op('isRunning'),op('reset'),op('stop')]});
for(const value of [0,-0,1,-1,.25,NaN,Infinity,-Infinity,'2','',null,undefined,false,true]){
    for(const method of ['setEffectiveWeight','setEffectiveTimeScale','setDuration','startAt','fadeIn','fadeOut','halt'])cases.push({operations:[op(method,value),op('getEffectiveWeight'),op('getEffectiveTimeScale'),op('isRunning'),op('stopFading'),op('stopWarping')]});
    cases.push({fields:{timeScale:value,enabled:value},operations:[op('isRunning'),op('play'),op('isRunning'),op('_updateWeight',1),op('_updateTimeScale',1)]});
}
for(const mode of [2200,2201,2202,999,'2200',null])for(const duration of [0,2,-2,NaN,Infinity,'2'])for(const repetitions of [0,1,2,Infinity]){
    cases.push({duration,fields:{loop:mode,repetitions},operations:[op('play'),op('_update',0,0,0,0),op('_update',.5,.5,1,1),op('_update',3,2.5,1,0),op('_update',-1,-4,-1,1)]});
}
for(const loop of [2200,2201,2202])for(const clampWhenFinished of [false,true])for(const eventOp of [undefined,'reset','stop','change'])cases.push({fields:{loop,repetitions:2,clampWhenFinished},eventOp,operations:[op('play'),op('_update',3,3,1,0),op('_update',5,2,1,1),op('_update',-1,-6,-1,0)]});
for(const delta of [0,-0,.1,-.1,2,-2,17,-17,NaN,Infinity,-Infinity,'1',null,undefined])for(const loop of [2200,2201,2202])cases.push({fields:{loop},operations:[op('_updateTime',delta),op('_updateTime',delta)]});
for(const time of [0,-0,'2',NaN,Infinity])for(const count of [-1,0,1,2,'1',NaN,Infinity])cases.push({fields:{time,_loopCount:count,loop:2202},operations:[op('_updateTime',0),op('_updateTime',-0)]});
for(const enabled of [false,true,0,''])for(const paused of [false,true])for(const blendMode of [2500,2501,999,'2501'])cases.push({blendMode,fields:{enabled,paused},operations:[op('startAt',2),op('_update',1,.25,1,0),op('_update',3,.25,1,1),op('_update',-1,-2,-1,0)]});
for(const controlValue of [0,-0,.25,1,2,'0','2',NaN,Infinity])for(const scale of [0,1,-1,2])cases.push({fields:{timeScale:scale},operations:[op('fadeIn',2),op('warp',.5,2,2),{target:'state',set:{controlValue}},op('_updateWeight',2),op('_updateTimeScale',2),op('_updateWeight',2.1),op('_updateTimeScale',2.1)]});
for(const warp of [false,true,1,'true',null,undefined])for(const duration of [0,2,'2',NaN])cases.push({operations:[op('setEffectiveTimeScale',2),{target:'other',set:{timeScale:.5}},op('crossFadeFrom','$other',duration,warp),op('_updateTimeScale',3),op('crossFadeTo','$other',duration,warp),op('syncWith','$other')]});
for(const zeroSlopeAtStart of [false,true])for(const zeroSlopeAtEnd of [false,true])for(const a of [false,true])for(const b of [false,true])for(const p of [false,true])cases.push({fields:{zeroSlopeAtStart,zeroSlopeAtEnd},operations:[op('_setEndings',a,b,p)]});
for(const localRoot of [undefined,null,false,0,'local',{}])cases.push({localRoot,operations:[op('getRoot')]});
for(const loop of [2200,2201,2202])for(const eventOp of [undefined,'reset','change'])cases.push({watch:true,eventOp,fields:{loop,repetitions:2},operations:[op('play'),op('fadeIn',2),op('warp',.5,2,2),op('_update',1,1,1,0),op('_update',5,4,1,1),op('reset'),op('crossFadeFrom','$other',1,true),op('halt',2),op('stop')]});
for(const field of ['time','timeScale','weight','repetitions','_loopCount','_startTime'])for(const value of [0,1,'2',NaN])cases.push({watch:true,coerce:{[field]:value},operations:[op('isRunning'),op('_update',3,.5,1,0),op('_updateTime',3),op('setDuration',{$coerce:4}),op('warp',{$coerce:.5},{$coerce:2},{$coerce:3})]});
for(const value of [0,1,'2',NaN])for(const loop of [2200,2201,2202])cases.push({watch:true,fields:{loop},operations:[op('_updateTime',{$coerce:value}),op('fadeIn',{$coerce:value}),op('_updateWeight',{$coerce:value}),op('_updateTimeScale',{$coerce:value})]});
const expected=cases.map(r=>{try{return run(r);}catch(e){return {error:e.name};}});
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases,wire),encoding:'utf8',maxBuffer:128*1024*1024}));
assert.equal(actual.length,expected.length);let comparisons=0;
function compare(a,b,p){if(a!==null&&b!==null&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),`${p} keys`);for(const k of Object.keys(a))compare(a[k],b[k],`${p}.${k}`);return;}assert.deepEqual(a,b,p);comparisons++;}
for(let i=0;i<cases.length;i++)try{compare(actual[i],expected[i],`case[${i}]`);}catch(e){console.log(JSON.stringify(cases[i],wire));throw e;}
console.log(`AnimationAction parity: ${cases.length} scenarios / ${comparisons} exact comparisons; upstream ${lock.commit}`);
