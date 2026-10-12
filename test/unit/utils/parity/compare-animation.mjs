import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

const upstream=path.resolve(process.argv[2]), binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const source=name=>import(pathToFileURL(path.join(upstream,`src/${name}.js`)));
const {AnimationClip}=await source('animation/AnimationClip');
const {KeyframeTrack}=await source('animation/KeyframeTrack');
const {LinearInterpolant}=await source('math/interpolants/LinearInterpolant');
const utils=await source('animation/AnimationUtils');
const classes={base:KeyframeTrack};
for(const [name,prefix] of [['number','Number'],['vector','Vector'],['color','Color'],['quaternion','Quaternion'],['bool','Boolean'],['string','String']])
    classes[name]=(await source(`animation/tracks/${prefix}KeyframeTrack`))[`${prefix}KeyframeTrack`];
const constructors={int8:Int8Array,uint8:Uint8Array,int16:Int16Array,uint16:Uint16Array,int32:Int32Array,uint32:Uint32Array,float32:Float32Array,float64:Float64Array,clamped:Uint8ClampedArray,array:Array};
console.warn=()=>{};console.error=()=>{};
const wire=(key,v)=>v===undefined?{$undefined:true}:typeof v==='number' && (!Number.isFinite(v)||Object.is(v,-0))?{$number:Object.is(v,-0)?'-0':String(v)}:v;
const encode=v=>v===undefined?'$undefined':typeof v==='number'?(Number.isNaN(v)?'$NaN':v===Infinity?'$Infinity':v===-Infinity?'$-Infinity':Object.is(v,-0)?'$-0':v):
    Array.isArray(v)?Array.from(v,encode):v!==null&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
const mkArray=a=>a&&typeof a==='object'&&Object.hasOwn(a,'kind')?(a.kind==='array'?a.data:new constructors[a.kind](a.data)):a;
const typeName=a=>Object.entries(constructors).find(([n,T])=>n!=='array'&&a instanceof T)?.[0]??'array';
const dumpArray=a=>ArrayBuffer.isView(a)?Array.from(a,encode):encode(a);
const trackState=t=>({name:encode(t.name),type:t.ValueTypeName,timeType:typeName(t.times),valueType:typeName(t.values),times:dumpArray(t.times),values:dumpArray(t.values),valueSize:encode(t.getValueSize()),interpolation:encode(t.getInterpolation()),defaultInterpolation:encode(t.DefaultInterpolation),valid:t.validate(),json:encode(KeyframeTrack.toJSON(t))});
function trackCase(r) {
    const times=mkArray(r.times),values=mkArray(r.values),t=new classes[r.type](r.name,times,values,r.interpolation);
    if(Object.hasOwn(r,'settings'))t.settings=r.settings===null?null:{inTangents:mkArray(r.settings.inTangents),outTangents:mkArray(r.settings.outTangents)};
    const out={sameTimes:t.times===times,sameValues:t.values===values,initial:trackState(t)},states=[];
    for(const a of r.actions??[]) {
        if(a.op==='shift')t.shift(a.value);
        else if(a.op==='scale')t.scale(a.value);
        else if(a.op==='trim')t.trim(a.start,a.end);
        else if(a.op==='optimize')t.optimize();
        else if(a.op==='interpolation')t.setInterpolation(a.value);
        else if(a.op==='clearFactory')t.createInterpolant=undefined;
        else if(a.op==='times')t.times=mkArray(a.value);
        else if(a.op==='sample'){const i=t.createInterpolant();states.push(a.times.map(x=>dumpArray(i.evaluate(x))));continue;}
        else if(a.op==='clone'){const clone=t.clone();if(Object.hasOwn(a,'change'))clone.values[0]=a.change;
            states.push({sameTimes:clone.times===t.times,sameValues:clone.values===t.values,sameFactory:clone.createInterpolant===t.createInterpolant,state:trackState(clone)});continue;}
        states.push(trackState(t));
    }
    return {...out,states,sourceTimes:dumpArray(times),sourceValues:dumpArray(values)};
}
const clipState=c=>({json:encode(c.toJSON()),valid:c.validate()});
function markGLTF(c){for(const t of c.tracks){t.createInterpolant=function(result){return new LinearInterpolant(this.times,this.values,this.getValueSize()/3,result);};t.createInterpolant.isInterpolantFactoryMethodGLTFCubicSpline=true;}}
function clipCase(r){
    const input=structuredClone(r.json),c=AnimationClip.parse(input);if(r.gltf)markGLTF(c);
    const out={initial:clipState(c),input:encode(input)},states=[];
    for(const a of r.actions??[]){
        if(a.op==='trim')c.trim();else if(a.op==='optimize')c.optimize();else if(a.op==='reset')c.resetDuration();
        else if(a.op==='clone'){const clone=c.clone();clone.uuid='derived';states.push(clipState(clone));continue;}
        else if(a.op==='subclip'){const part=utils.subclip(c,'part',a.start,a.end,a.fps);part.uuid='derived';states.push(clipState(part));continue;}
        else if(a.op==='additive'){const reference=a.reference?AnimationClip.parse(a.reference):undefined;if(reference&&r.gltf)markGLTF(reference);utils.makeClipAdditive(c,a.frame,reference,a.fps);}
        states.push(clipState(c));
    }
    return {...out,states};
}
function callbackCase(kind) {
    const trace=[],t=new classes.number('.x',[0,1],[2,6]);
    if(kind==='settings'){
        const replacement={inTangents:[7,8],outTangents:[9,10]};
        t.settings={get inTangents(){trace.push('in');t.settings=replacement;return [];},get outTangents(){trace.push('out');return [];}};
        t.scale(2);return {trace,times:dumpArray(t.times),settings:replacement};
    }
    if(kind==='slice'){
        const s=new classes.string('.s',[0,1],['a','b']);
        Object.defineProperty(s.values,'0',{get(){trace.push('first');s.values=['newA','newB'];return 'a';},configurable:true});
        const clone=s.clone();return {trace,clone:clone.values,source:s.values};
    }
    if(kind==='size'){
        t.values={get length(){trace.push('valueLength');return {valueOf(){trace.push('valueOf');t.times=[0,1,2,3];return 6;}};}};
        t.times={get length(){trace.push('timeLength');return 2;}};
        const size=t.getValueSize();return {trace,size,times:t.times};
    }
    if(kind==='duration'){
        t.times={get length(){trace.push('length');t.times=[0,100];return 2;},get 1(){trace.push('last');return 7;}};
        const clip=new AnimationClip('clip',0,[t]);clip.resetDuration();return {trace,duration:clip.duration,times:t.times};
    }
    if(kind==='durationTracks'){
        const clip=new AnimationClip('clip',0,[t,new classes.number('.y',[0,100],[2,6])]);
        t.times={get length(){trace.push('replace');clip.tracks=[t,new classes.number('.z',[0,20],[2,6])];return 2;},1:7};
        clip.resetDuration();return {trace,duration:clip.duration,second:clip.tracks[1].name};
    }
    if(kind==='iterator'||kind==='arrayLike'){
        const source=kind==='iterator'?new Array(2):{length:2};source.last=5;
        const element={valueOf(){trace.push('valueOf');source.last=90;source[2]=90;return 3;}};
        Object.defineProperty(source,'0',{get(){trace.push('get0');if(kind==='iterator')source[2]=7;return element;},configurable:true});
        Object.defineProperty(source,'1',{get(){trace.push('get1');return source.last;},configurable:true});
        const values=utils.convertArray(source,Float32Array);return {trace,values:dumpArray(values),last:source.last};
    }
    if(kind==='spread'){
        const times=[],values=[42],source=new Array(2);
        Object.defineProperty(source,'0',{get(){trace.push(values.length);source[2]=7;return 3;},configurable:true});
        Object.defineProperty(source,'1',{get(){trace.push(values.length);return 5;},configurable:true});
        utils.flattenJSON([{time:4,value:source}],times,values,'value');return {trace,times,values};
    }
    if(kind==='toArray'){
        const times=[],values=[42],source={x:7,toArray(destination,offset){trace.push(this.x,offset);destination[offset]=this.x;return destination;}};
        utils.flattenJSON([{time:4,value:source}],times,values,'value');return {trace,times,values};
    }
    throw new Error('Unknown callback fixture');
}
function run(r){
    if(r.mode==='track')return trackCase(r);
    if(r.mode==='clip')return clipCase(r);
    if(r.mode==='callback')return callbackCase(r.kind);
    if(r.mode==='sortTrace'){
        const backing=r.times.slice(),times=new Array(backing.length),trace=[];let changed=false;
        for(let i=0;i<times.length;i++)Object.defineProperty(times,i,{get(){
            trace.push(`get:${i}`);const value=backing[i];return {valueOf(){
                trace.push(`number:${i}`);if(r.mutate&&!changed){changed=true;backing[0]=-100;}return value;
            }};
        }});
        return {order:utils.getKeyframeOrder(times),trace,backing:encode(backing)};
    }
    if(r.mode==='convert'){const a=mkArray(r.array),v=utils.convertArray(a,constructors[r.type]);return {same:a===v,type:typeName(v),value:dumpArray(v)};}
    if(r.mode==='sort'){const t=mkArray(r.times),v=mkArray(r.values),order=utils.getKeyframeOrder(t);return {order:dumpArray(order),times:dumpArray(utils.sortedArray(t,1,order)),values:dumpArray(utils.sortedArray(v,r.stride,order))};}
    if(r.mode==='flatten'){const times=[],values=[];utils.flattenJSON(r.keys,times,values,r.property);return {times:encode(times),values:encode(values)};}
    if(r.mode==='morph'){
        if(!r.groups){const c=AnimationClip.CreateFromMorphTargetSequence('morph',r.targets,r.fps,r.noLoop);c.uuid='morph';return clipState(c);}
        return AnimationClip.CreateClipsFromMorphTargetSequences(r.targets,r.fps,r.noLoop).map(c=>{c.uuid='morph';return clipState(c);});
    }
    if(r.mode==='json'){const value=JSON.parse(r.text);return {value:encode(value),text:JSON.stringify(value)};}
    throw new Error('unknown case');
}
const cases=[];
for(const kind of ['settings','slice','size','duration','durationTracks','iterator','arrayLike','spread','toArray'])cases.push({mode:'callback',kind});
const track=(type='number',extra={})=>({mode:'track',type,name:'.value',times:[0,1,3,5],values:type==='quaternion'?[0,0,0,1,0,1,0,0,0,0,1,0,1,0,0,0]:type==='string'?['a','b','b','d']:type==='bool'?[false,true,true,false]:[0,10,-5,20],...extra});
for(const type of Object.keys(classes))for(const interpolation of [undefined,0,-0,NaN,2300,2301,2302,2303,999,'2301'])
    cases.push(track(type,{interpolation,actions:[{op:'sample',times:[-1,0,.25,1,2,5,NaN]},{op:'clone',change:type==='string'?'changed':type==='bool'?true:8}]}));
for(const kind of Object.keys(constructors))cases.push(track('number',{times:{kind,data:[0,1,3,5]},values:{kind,data:[0,10,-5,20]},actions:[{op:'shift',value:2},{op:'scale',value:.1},{op:'clone'}]}));
for(const op of ['shift','scale'])for(const value of [0,-0,1,2,-2,'2',null,NaN,Infinity])cases.push(track('number',{actions:[{op,value}]}));
for(const [start,end] of [[1,3],[2,2],[6,8],[-8,-2],[4,1],[NaN,3],[1,NaN],[-Infinity,Infinity]])cases.push(track('number',{actions:[{op:'trim',start,end}]}));
for(const interpolation of [2300,2301,2302,2303])for(const times of [[0,0,1,2,3,3],[0,1,2,3,4,5],[0,0,0,0,0,0]])cases.push(track('number',{times,values:[0,9,1,1,1,5],interpolation,actions:[{op:'optimize'},{op:'clone'}]}));
for(const values of [[NaN,1,2,3],[Infinity,1,-Infinity,0],[],[1,2,3],[0,0,0,0]])cases.push(track('number',{values}));
for(const type of ['number','string','bool'])cases.push(track(type,{actions:[{op:'times',value:[]}]}));
for(const type of ['number','string','bool','quaternion'])cases.push(track(type,{actions:[{op:'clearFactory'}]}));
for(const name of [undefined,null,17])cases.push(track('number',{name}));
cases.push(track('number',{times:[]}));cases.push(track('number',{times:undefined}));
const tangents={inTangents:{kind:'float32',data:[0,0,.8,1,2.8,2,4.8,3]},outTangents:{kind:'float64',data:[.2,1,1.2,2,3.2,3,5,4]}};
cases.push(track('number',{settings:tangents,interpolation:2303,actions:[{op:'sample',times:[.2,.5,2]},{op:'shift',value:2},{op:'scale',value:2},{op:'trim',start:5,end:11},{op:'clone'},{op:'optimize'}]}));
cases.push(track('number',{settings:null}));
for(const sourceKind of Object.keys(constructors))for(const type of Object.keys(constructors))cases.push({mode:'convert',array:{kind:sourceKind,data:[0,-0,-129,257,.5,NaN,Infinity]},type});
for(const array of [null,false,0,3,'2'])for(const type of ['float32','array'])cases.push({mode:'convert',array,type});
for(const times of [[4,1,3,1,-2],[0,Infinity,-Infinity,3,3],[],[2,1]]){
    const values=times.flatMap((t,i)=>[i,-i]);cases.push({mode:'sort',times,values:{kind:'float32',data:values},stride:2});
}
let sortSeed=0x11738a9d;
const sortRandom=()=>{sortSeed=(Math.imul(sortSeed,1664525)+1013904223)>>>0;return sortSeed/4294967296;};
for(let fixture=0;fixture<200;fixture++){
    const times=Array.from({length:fixture<15?fixture:32+fixture},(_,i)=>{
        const value=Math.floor(sortRandom()*41)-20;
        return fixture%3===0&&i%7===1?NaN:fixture%5===0&&i%11===2?Infinity:fixture%7===0&&i%13===3?-Infinity:value;
    });
    cases.push({mode:'sort',times,values:times.map((_,i)=>i),stride:1});
}
for(const n of [0,1,2,4,31,32,63,64,65,127,128,257])for(const mutate of [false,true])
    cases.push({mode:'sortTrace',times:Array.from({length:n},(_,i)=>i%9===3?NaN:Math.floor(sortRandom()*41)-20),mutate});
for(const keys of [[],[{time:0},{time:1,value:3},{time:2},{time:4,value:5}],[{time:0,value:[1,2]},{time:1,value:[3,4]}],[{time:0,value:[1]},{time:1,value:'💩x'}],[{time:0,value:'ab'},{time:1,value:'cd'}],[{time:0,value:null}]])cases.push({mode:'flatten',keys,property:'value'});
const clip=(extra={})=>({name:'clip',uuid:'clip-id',tracks:[{name:'.x',type:'number',times:[0,1,2,3],values:[2,4,6,8]},{name:'.visible',type:'bool',times:[0,3],values:[false,true]}],userData:'{"ratio":0.1,"text":"hello"}',...extra});
for(const duration of [undefined,-1,0,1.5,10,NaN,Infinity])for(const fps of [undefined,0,2,-2,'0'])cases.push({mode:'clip',json:clip({duration,fps}),actions:[{op:'clone'},{op:'trim'},{op:'optimize'},{op:'reset'}]});
for(const type of ['scalar','double','float','NUMBER','integer','vector2','vector3','vector4','color','quaternion','boolean','string','missing',undefined])cases.push({mode:'clip',json:clip({tracks:[{name:'.v',type,keys:[{time:0,value:1},{time:4,value:5}]}],fps:2})});
for(const userData of ['[1,null,true]','0','false','null','"hello"','{"__proto__":{"x":1},"1":0.1,"emoji":"\\ud83d\\udca9","lone":"\\ud800"}','bad','[1,]',null,undefined])cases.push({mode:'clip',json:clip({userData}),actions:[{op:'clone'}]});
for(const [start,end,fps] of [[0,90,30],[30,90,30],[90,100,30],[-1,0,30],[100,101,30],[0,1,0],[NaN,90,30],[0,90,NaN]])cases.push({mode:'clip',json:clip(),actions:[{op:'subclip',start,end,fps}]});
for(const frame of [-30,0,15,30,1000,NaN])for(const fps of [30,0,-1])cases.push({mode:'clip',json:clip(),actions:[{op:'additive',frame,fps}]});
for(const frame of [-30,0,15,60,1000])cases.push({mode:'clip',json:clip({tracks:[{name:'.q',type:'quaternion',times:[0,1,2],values:[0,0,0,1,0,.7071067811865476,0,.7071067811865476,0,1,0,0]}]}),actions:[{op:'additive',frame}]});
for(const frame of [-1,0,1000])cases.push({mode:'clip',gltf:true,json:clip({tracks:[{name:'.x',type:'number',times:[0,1],values:[100,2,200,300,8,400]}]}),actions:[{op:'additive',frame}]});
cases.push({mode:'clip',json:clip(),actions:[{op:'additive',frame:15,reference:clip({tracks:[{name:'.x',type:'number',times:[0,1,2],values:[10,20,30]}]})}]});
for(let n=0;n<6;n++)for(const noLoop of [false,true])for(const fps of [24,0,-1])cases.push({mode:'morph',targets:Array.from({length:n},(_,i)=>({name:`target${i}`})),fps,noLoop});
for(const names of [['Walk_002','Run_001','Walk_001','Run_002','ignored'],['flamingo_flyA_003','flamingo_run1_003','crdeath0059'],['__proto__001'],['constructor001'],['001','002','a-1','a-2']])cases.push({mode:'morph',groups:true,targets:names.map(name=>({name})),fps:24,noLoop:false});
for(const text of ['0','-0','1e309','1e-9999','0.1','[1,2,true,null]','{"b":2,"1":1,"a":0}','{"a":1,"a":2}','"\\ud83d\\udca9"','"\\ud800"','[1,]','{"a":1,}','01','+1','NaN','"x\n"','true false'])cases.push({mode:'json',text});
const expected=cases.map(r=>{try{return run(r);}catch(e){return {error:e.name};}});
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases,wire),encoding:'utf8',maxBuffer:32*1024*1024,stdio:['pipe','pipe','pipe']}));
let comparisons=0;
function compare(a,b,where){
    if(typeof a==='number'&&typeof b==='number'){assert.ok(Math.abs(a-b)<=3e-14*Math.max(1,Math.abs(a),Math.abs(b)),`${where}: ${a} != ${b}`);comparisons++;return;}
    if(a!==null&&b!==null&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),`${where} keys`);for(const key of Object.keys(a))compare(a[key],b[key],`${where}.${key}`);return;}
    assert.deepEqual(a,b,where);comparisons++;
}
assert.equal(actual.length,cases.length);
for(let i=0;i<cases.length;i++){try{
    if(cases[i].mode==='sort'||cases[i].mode==='sortTrace')assert.deepEqual(actual[i],expected[i],`exact sort case[${i}]`);
    compare(actual[i],expected[i],`case[${i}]`);
}catch(e){console.log(JSON.stringify(cases[i],wire));throw e;}}
console.log(`Animation parity: ${cases.length} scenarios / ${comparisons} comparisons; upstream ${lock.commit}`);
