// Differential check for the native AnimationClipCreator port.
//
// The pinned module is imported from the checkout, Math.random is stubbed with the same sequence
// the probe injects, and every clip is compared with assert.deepEqual (no tolerances).
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-animation-clip-creator.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const THREE=await import(url('build/three.module.js'));
const {AnimationClipCreator}=await import(url('examples/jsm/animation/AnimationClipCreator.js'));
const {Color,Vector3}=THREE;

const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);
const clipJSON=clip=>({name:clip.name,duration:encode(clip.duration),
    tracks:clip.tracks.map(track=>({name:track.name,times:encodeArray(track.times),
        // The pinned BooleanKeyframeTrack keeps booleans (ValueBufferType Array); the native
        // track exposes the same information as 0/1 through number(), so booleans are
        // normalised to their exact numeric value while every other value stays untouched.
        values:encodeArray(track.values).map(value=>typeof value==='boolean'?Number(value):value)}))});
// The palette the probe derives from the index with the same integer maths.
const palette=count=>{
    const colors=[];
    for(let i=0;i<count;++i) colors.push(new Color((i%7)/7,((i*3)%5)/5,((i*5)%11)/11));
    return colors;
};

const draws=[];
let seed=987654321;
for(let i=0;i<200;++i){seed=(seed*1103515245+12345)%2147483648;draws.push(seed/2147483648);}
const axes=['x','y','z'];
const scenarios=[];
for(const axis of [0,1,2]){
    for(const period of [0,1,2.5,10]) scenarios.push({kind:0,first:period,second:axis,args:[],values:draws});
    for(const period of [0,1,2.5,10]) scenarios.push({kind:1,first:period,second:axis,args:[],values:draws});
}
for(const duration of [0,1,2.5]) for(const scale of [[1,1,1],[0.5,-2,3]]){
    scenarios.push({kind:2,first:duration,second:scale[0],args:scale.slice(1),values:draws});
}
for(const duration of [0,1,2.5]) for(const scale of [1,0.5,3]){
    scenarios.push({kind:3,first:duration,second:scale,args:[],values:draws});
}
// The pinned BooleanKeyframeTrack keeps booleans and the native track exposes the same values
// as 0/1; clipJSON normalises exactly that representation difference. duration 0 stays in: all
// three times collapse to 0 there, which is worth pinning down rather than skipping.
for(const duration of [0,1,2.5,7]) scenarios.push({kind:4,first:duration,second:0,args:[],values:draws});
for(const duration of [1,5]) for(const count of [0,1,2,5]){
    scenarios.push({kind:5,first:duration,second:count,args:[],values:draws});
}

const runScenario=({kind,first,second,args,values})=>{
    let cursor=0;
    const original=Math.random;
    Math.random=()=>values[cursor++%values.length];
    try{
        if(kind===0) return AnimationClipCreator.CreateRotationAnimation(first,axes[second]);
        if(kind===1) return AnimationClipCreator.CreateScaleAxisAnimation(first,axes[second]);
        if(kind===2) return AnimationClipCreator.CreateShakeAnimation(first,new Vector3(second,...args));
        if(kind===3) return AnimationClipCreator.CreatePulsationAnimation(first,second);
        if(kind===4) return AnimationClipCreator.CreateVisibilityAnimation(first);
        return AnimationClipCreator.CreateMaterialColorAnimation(first,palette(second));
    }finally{
        Math.random=original;
    }
};

// One line per scenario: the kind and its numbers, then the injected random sequence.
const lines=scenarios.map(({kind,first,second,args,values})=>
    ['C',kind,token(first),token(second),...args.map(token),...values.map(token)].join(' '));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:256*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');

let tracks=0,entries=0,errors=0;
for(let index=0;index<scenarios.length;++index){
    const scenario=scenarios[index];
    let expected,failure;
    try{
        expected=clipJSON(runScenario(scenario));
    }catch(error){
        // The pinned KeyframeTrack rejects an empty times array (duration 0); the native core
        // throws the identical message, so the failure is part of the compared contract.
        failure={probeError:error.message};
    }
    const actual=JSON.parse(output[index]);
    if(failure){
        assert.deepEqual(actual,failure,`scenario ${index}: kind ${scenario.kind} ${scenario.first}`);
        errors+=1;
        continue;
    }
    assert.deepEqual(actual,expected,`scenario ${index}: kind ${scenario.kind} ${scenario.first} ${scenario.second}`);
    tracks+=expected.tracks.length;
    for(const track of expected.tracks) entries+=track.times.length+track.values.length;
}
console.log(`AnimationClipCreator parity: ${scenarios.length} scenarios / ${tracks} tracks / ${entries} time and`
    +` value entries${errors?` (plus ${errors} matched throw paths)`:''} match the pinned checkout bit for bit;`
    +` ${process.version}`);
