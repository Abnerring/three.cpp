import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node compare-animation-runtime.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]),lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const T=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const {PropertyMixer}=await import(pathToFileURL(path.join(upstream,'src/animation/PropertyMixer.js')));
const expected={},root={x:2},binding=new T.PropertyBinding(root,'.x'),b2=new T.PropertyBinding(root,'.x');
expected.bindingInitial={keys:Object.keys(binding),sharedPrototype:Object.getPrototypeOf(binding)===Object.getPrototypeOf(b2),
    initialGet:binding.getValue===binding._getValue_unbound,initialSet:binding.setValue===binding._setValue_unbound};
const buffer=new Array(2);binding.getValue(buffer,1);
expected.bindingBound={keys:Object.keys(binding),buffer:[...buffer].map(x=>x??null),targetIdentity:binding.targetObject===root,
    getIdentity:binding.getValue===binding._getValue_direct,propertyName:binding.propertyName};
buffer[1]=7;binding.setValue(buffer,1);binding.unbind();expected.bindingAfter={x:root.x,nodeNull:binding.node===null,getIdentity:binding.getValue===binding._getValue_unbound};
const group={nCachedObjects_:0,subscribe_(){return [binding];}},composite=new T.PropertyBinding.Composite(group,'.x');composite.getValue(buffer,0);
expected.composite={keys:Object.keys(composite),rootUndefined:composite.rootNode===undefined,pathUndefined:composite.path===undefined,value:buffer[0]};
const mixer=new PropertyMixer(binding,'number',1);
expected.mixerInitial={keys:Object.keys(mixer),cacheUndefined:mixer._cacheIndex===undefined,mixIdentity:mixer._mixBufferRegion===mixer._lerp};
mixer.saveOriginalState();const storage=mixer.buffer;storage[0]=11;mixer.accumulate(0,.5);mixer.apply(0);
expected.mixerApplied={x:root.x,buffer:[...storage],bufferIdentity:storage===mixer.buffer,weight:mixer.cumulativeWeight};
mixer.restoreOriginalState();expected.restored=root.x;
const interpolant=new T.LinearInterpolant(new Float32Array([0,1,2]),new Float32Array([2,6,14]),1),settings={};interpolant.settings=settings;
const result=interpolant.resultBuffer,samples=[-.5,0,.25,1,1.5,2,3].map(t=>[...interpolant.evaluate(t)]);
expected.interpolant={keys:Object.keys(interpolant),samples,resultIdentity:result===interpolant.resultBuffer,settingsIdentity:interpolant.getSettings_()===settings,cached:interpolant._cachedIndex};
const track=new T.NumberKeyframeTrack('.x',[0,1],[2,10]),curve=track.createInterpolant(),factory=track.createInterpolant;track.shift(2);
const clip=new T.AnimationClip('clip',-1,[track]),clone=clip.clone();
expected.clipTrack={duration:clip.duration,valueType:track.ValueTypeName,shiftedTimes:[...track.times],sharedTimes:curve.parameterPositions===track.times,
    factoryIdentity:factory===track.InterpolantFactoryMethodLinear,trackIdentity:clip.tracks[0]===track,arrayIdentity:clip.tracks===clip.tracks,
    cloneArrayDistinct:clone.tracks!==clip.tracks,cloneTrackDistinct:clone.tracks[0]!==track};
for(const kind of ['interpolate','copy','interval','settings','throwInterval','getterReplace']){
    const value=kind==='settings'?new T.CubicInterpolant(new Float32Array([0,1]),new Float32Array([2,6]),1):
        new T.LinearInterpolant(new Float32Array([0,1,2]),new Float32Array([2,6,10]),1);
    const trace=[],returned=[0];
    const fn=function(...args){
        trace.push(`call:${kind}:${args.length?args[0]:''}`);globalThis.gc?.();
        if(kind==='interval'){
            this.interpolate_=function(i,t0,t,t1){trace.push(`replacement:${t}`);globalThis.gc?.();returned[0]=77+t;return returned};return;
        }
        if(kind==='settings')return {endingStart:T.ZeroSlopeEnding,endingEnd:T.ZeroSlopeEnding};
        returned[0]=kind==='copy'?100+args[0]:40+args[2];return returned;
    };
    const name=kind==='copy'?'copySampleValue_':kind==='interval'||kind==='throwInterval'?'intervalChanged_':kind==='settings'?'getSettings_':'interpolate_';
    Object.defineProperty(value,name,{configurable:true,get(){
        trace.push(`get:${kind}`);globalThis.gc?.();
        if(kind==='getterReplace')Object.defineProperty(this,'interpolate_',{writable:true,enumerable:true,configurable:true,
            value(i,t0,t,t1){trace.push(`installed:${t}`);globalThis.gc?.();returned[0]=900+t;return returned}});
        return kind==='throwInterval'?null:fn;
    }});
    const times=kind==='copy'?[-1,3]:kind==='settings'?[.25,.75]:[.5,.75,1.5],samples=[],identities=[];
    for(const time of times){try{const result=value.evaluate(time);samples.push([...result]);identities.push(result===returned)}catch(e){samples.push(e.name);identities.push(false)}}
    expected[`hook:${kind}`]={samples,identities,trace,cached:value._cachedIndex};
}
const actual=JSON.parse(execFileSync(path.resolve(process.argv[3]),[],{encoding:'utf8'}));assert.deepEqual(actual,expected);
// The probe output is compared as one JSON blob, so the strength of this test is the number of
// scalar values inside it, not the number of top-level scenarios; report both.
const leaves = value => value !== null && typeof value === 'object'
    ? Object.values(value).reduce((sum, entry) => sum + leaves(entry), 0)
    : 1;
console.log(`Animation runtime parity: ${Object.keys(expected).length} scenarios / ${leaves(expected)} compared values; live fields, prototypes, buffers and identity match ${lock.commit}`);
