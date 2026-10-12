import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding:'utf8'}).trim(), lock.commit);
const source = name => import(pathToFileURL(path.join(upstream, `src/math/${name}.js`)));
const {Interpolant} = await source('Interpolant');
const classes = await Promise.all(['Discrete', 'Linear', 'Cubic', 'QuaternionLinear', 'Bezier'].map(async name =>
    (await source(`interpolants/${name}Interpolant`))[`${name}Interpolant`]));
const [Discrete, Linear, Cubic, QuaternionLinear, Bezier] = classes;
const value = v => v === undefined ? '$undefined' : Number.isNaN(v) ? '$NaN' :
    v === Infinity ? '$Infinity' : v === -Infinity ? '$-Infinity' : Object.is(v,-0) ? '$-0' : v;
const array = a => Array.from(a,value);
const run = (i,times) => times.map(t => {const r=i.evaluate(t);return {value:array(r),index:i._cachedIndex,same:r===i.resultBuffer};});
class Trace extends Interpolant {
    calls=[];
    intervalChanged_(i,t0,t1){this.calls.push(['interval',i,value(t0),value(t1)]);}
    interpolate_(i,t0,t,t1){this.calls.push(['interpolate',i,value(t0),value(t),value(t1)]);return this.copySampleValue_(i-1);}
}
function intervals(C) {
    const regular=new C([0,1,2.5,4,8],[1,-1,3,2,-4,7,10,-2,9,5],2);
    const duplicate=new C([0,1,1,2],[1,5,9,13],1);
    const missing=new C([0,undefined,2,3],[1,5,9,13],1);
    const nonfinite=new C([0,NaN,2,3],[1,5,9,13],1);
    const empty=new C([],[],2),single=new C([2],[7,-0],2);
    const missingSamples=new C([0,1],[1,undefined,NaN],2);
    return {regular:run(regular,[-Infinity,-1,0,.25,.75,1,2.2,7,3.5,.1,8,Infinity,4,NaN,0]),
        duplicate:run(duplicate,[-1,0,.5,1,1.5,2,1,NaN]),missing:run(missing,[.2,2.5,.2,NaN,3]),
        nanTime:run(nonfinite,[.2,2.5,.2,NaN,3]),empty:run(empty,[-1,0,1,NaN]),
        single:run(single,[-Infinity,0,2,3,Infinity,NaN]),missingSamples:run(missingSamples,[-1,.5,1,2])};
}
function storage(T) {
    const values=new T([-3,257,8,-129]),positions=new Float64Array([0,1]),i=new Linear(positions,values,2);
    const result={samePositions:i.parameterPositions===positions,sameValues:i.sampleValues===values,initial:array(i.resultBuffer),samples:run(i,[-1,.25,.5,.75,2])};
    values[0]=30;positions[1]=2;result.mutated=run(i,[.5,1.5]);return result;
}
const expected={};
const trace=new Trace([11,22,33,44,55,66,77,88,99],[1,2,3,4,5,6,7,8,9],1);
expected.trace=run(trace,[11,12,22,21,20,43,12,77,80,36,24,16,100,100,89,-1,11,NaN]);expected.calls=trace.calls;
for(const [name,C] of [['discrete',Discrete],['linear',Linear],['cubic',Cubic],['bezierFallback',Bezier]])expected[name]=intervals(C);
for(const [name,T] of [['int8',Int8Array],['uint8',Uint8Array],['int16',Int16Array],['uint16',Uint16Array],['int32',Int32Array],['uint32',Uint32Array],['float32',Float32Array],['float64',Float64Array],['clamped',Uint8ClampedArray]])expected[name]=storage(T);
const result=[],supplied=new Linear([0,1],[1,2,3,4],2,result);
expected.suppliedIdentity=supplied.evaluate(.25)===result;expected.supplied=array(result);
expected.shortTyped=run(new Linear([0,1],[1,2,3,4],2,new Float32Array(1)),[.5,-1,2]);
const aliasValues=new Float64Array([1,2,3,4,5,6]),alias=new Linear([0,1],aliasValues,3,aliasValues.subarray(1,4));
expected.aliasedLinear=run(alias,[.5,-1,.25]);expected.aliasedValues=array(aliasValues);
const raw=new Discrete(null,[undefined,NaN,null,'7',true,-0],2,[]);
expected.rawCopies=[-1,0,1,2,3].map(i=>array(raw.copySampleValue_(i)));
const base=new Interpolant([0,1],[5,9],1);
try{base.evaluate(.5);expected.abstract='no error';}catch(e){expected.abstract=e.message;}
expected.baseBoundaries=run(base,[-1,2]);
const settings=new Cubic([0,1,3,5],[0,10,-5,20],1);
expected.endings=[2400,2401,2402,999].map(mode=>{settings.settings={endingStart:mode,endingEnd:mode};settings._cachedIndex=0;return run(settings,[.2,.6,4.5,4.8,.1]);});
const prototype={endingStart:2401,endingEnd:2402};settings.settings=Object.create(prototype);settings._cachedIndex=0;
const dynamic=[array(settings.evaluate(.25))];prototype.endingStart=2402;
dynamic.push(array(settings.evaluate(.5)),array(settings.evaluate(2)),array(settings.evaluate(.5)));
expected.dynamicSettings=dynamic;expected.settingsIdentity=settings.getSettings_()===settings.settings;
settings.settings=false;expected.defaultIdentity=settings.getSettings_()===settings.DefaultSettings_;
expected.quaternion=run(new QuaternionLinear([0,1,2],[0,0,0,1,0,.7071067811865476,0,.7071067811865476,0,0,0,-1],4),[0,.25,.5,.99,1,1.5,2,NaN,.1]);
expected.quaternionSmall=run(new QuaternionLinear([0,1],[0,0,0,1,.001,0,0,.9999995],4),[.1,.5,.9]);
const qValues=new Float64Array([0,0,0,1,0,1,0,0,0,0,1,0,1,0,0,0]);
expected.quaternionMulti=run(new QuaternionLinear([0,1],qValues,8,qValues.subarray(4,12)),[.5,.25]);expected.quaternionMultiValues=array(qValues);
expected.quaternionUndefined=run(new QuaternionLinear([0,1],[],4),[.5]);
const bezier=new Bezier([0,1,3],[0,10,1,-5,4,2],2);
const tangentIn=new Float64Array([0,0,0,10,.9,0,.8,0,2.7,3,2.2,4]),tangentOut=new Float64Array([.2,2,.3,15,1.3,-1,1.8,-7,3,4,3,2]);
bezier.inTangents=tangentIn;bezier.outTangents=tangentOut;
expected.bezier=run(bezier,[0,.1,.4,.8,1,1.2,2.4,3,.5]);tangentOut[1]=8;expected.bezierMutated=run(bezier,[.5]);
bezier.inTangents=[];expected.bezierMissing=run(bezier,[.5]);bezier.inTangents=null;expected.bezierNullFallback=run(bezier,[.5]);
const fractional=new Linear(new Float32Array([0,1]),new Float32Array([0,1,2,3]),1.5);
expected.fractionalConstructor=[fractional.valueSize,fractional.resultBuffer.length];
const observedNumber=(trace,label,number)=>({valueOf(){trace.push(`number:${label}`);return number;}});
function observedArray(trace,label,entries) {
    const result=new Array(entries.length);
    entries.forEach((entry,index)=>Object.defineProperty(result,index,{enumerable:true,configurable:true,
        get(){trace.push(`get:${label}.${index}`);return entry;}}));
    return result;
}
function parameterCallbacks(C,quaternion=false) {
    const trace=[];
    const positions=[0,1,3,5].map(t=>observedNumber(trace,`time${t}`,t));
    const values=quaternion?[0,0,0,1,0,1,0,0,0,0,1,0,1,0,0,0]:[0,10,-5,20];
    const interpolant=new C(observedArray(trace,'positions',positions),
        observedArray(trace,'values',values),quaternion?4:1);
    const samples=run(interpolant,[.25,.75,2,4.5,.2,-1,6,NaN,.5]);
    return {samples,trace};
}
expected.discreteCallbacks=parameterCallbacks(Discrete);
expected.linearCallbacks=parameterCallbacks(Linear);
expected.cubicCallbacks=parameterCallbacks(Cubic);
expected.quaternionCallbacks=parameterCallbacks(QuaternionLinear,true);
expected.bezierFallbackCallbacks=parameterCallbacks(Bezier);
{
    const trace=[];let c;
    const previous={valueOf(){trace.push('stride=2');c.valueSize=2;return 1;}};
    c=new Cubic([0,previous,2,3,4],[0,10,20,30,40,50,60,70,80,90],1);
    c.intervalChanged_(3,2,3);
    const sample=array(c.interpolate_(3,2,2.5,3));
    expected.cubicStrideCallback={stride:c.valueSize,offsetPrev:c._offsetPrev,offsetNext:c._offsetNext,sample,trace};
}
{
    class SettingsCallbacks extends Cubic {
        trace=[];
        getSettings_(){this.trace.push('getSettings');return super.getSettings_();}
    }
    const c=new SettingsCallbacks([0,1,3,5],[0,10,-5,20],1);
    c.settings={get endingStart(){c.trace.push('endingStart');
        const nested=array(c.evaluate(2));c.trace.push(['nested',nested,c._cachedIndex]);return 2401;}};
    const samples=run(c,[.5,.75,4.5]);
    expected.cubicSettingsCallback={samples,trace:c.trace};
}
{
    const c=new Cubic([0,'1','3',5],[0,10,-5,20],1);
    c.settings={endingStart:2402,endingEnd:2402};expected.cubicStringTimes=run(c,[4,.25,4.5,2]);
}
{
    const trace=[],out=[1/3,0];
    const first={valueOf(){trace.push('sample');out[1]=100;return 0;}};
    const c=new Bezier([0,1],[first,8],1);c.outTangents=out;c.inTangents=[0,0,2/3,0];
    const sample=array(c.evaluate(.5));expected.bezierReadCallback={sample,out:array(out),trace};
}
{
    const trace=[],number=(name,v)=>observedNumber(trace,name,v);
    const c=new Bezier([0,1],observedArray(trace,'values',[number('v0',2),number('v1',8)]),1);
    c.outTangents=observedArray(trace,'out',[number('c0x',.1),number('c0y',3)]);
    c.inTangents=observedArray(trace,'in',[0,0,number('c1x',.8),number('c1y',6)]);
    const sample=array(c.evaluate(.35));expected.bezierArithmeticCallbacks={sample,trace};
}
{
    const trace=[];let count=0;
    const values=[1,0,0,0,0,0,0,1].map((n,i)=>observedNumber(trace,`q${i}`,n));
    values[0].valueOf=()=>{trace.push('number:q0');return ++count;};
    const c=new QuaternionLinear([0,1],observedArray(trace,'values',values),4);
    const sample=array(c.evaluate(.5));expected.quaternionChangingCallback={sample,trace};
}
{
    const trace=[];
    const values=[0,0,0,1,.001,0,0,-.9999995].map((n,i)=>observedNumber(trace,`q${i}`,n));
    const c=new QuaternionLinear([0,1],observedArray(trace,'values',values),4);
    const sample=array(c.evaluate(.5));expected.quaternionNegativeCallbacks={sample,trace};
}
{
    const trace=[],samples=[1,2,3,4],result=new Array(2);let stored;
    Object.defineProperty(result,0,{get(){return stored;},set(v){trace.push(v);stored=v;samples[1]=20;}});
    const c=new Linear([0,1],samples,2,result),sample=array(c.evaluate(.5));
    expected.resultSetterCallback={sample,samples:array(samples),trace};
}
const actual=JSON.parse(execFileSync(path.resolve(process.argv[3]),[],{encoding:'utf8'}));
let compared=0;
function compare(a,b,location='root') {
    if(typeof a==='number' && typeof b==='number') {
        assert.ok(Math.abs(a-b)<=3e-14*Math.max(1,Math.abs(a),Math.abs(b)),`${location}: ${a} != ${b}`);++compared;return;
    }
    if(a!==null && b!==null && typeof a==='object' && typeof b==='object') {
        assert.deepEqual(Object.keys(a),Object.keys(b),`${location} keys`);
        for(const k of Object.keys(a))compare(a[k],b[k],`${location}.${k}`);
    } else {assert.deepEqual(a,b,location);++compared;}
}
compare(actual,expected);
console.log(`Interpolant parity: ${compared} values across ${Object.keys(expected).length} scenarios`);
