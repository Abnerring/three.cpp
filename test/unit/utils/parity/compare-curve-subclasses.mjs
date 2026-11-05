import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

const [upstream,probe,requestedOutput]=process.argv.slice(2);
assert(upstream&&probe,'Usage: compare-curve-subclasses.mjs <pinned> <probe|--oracle-only> [fresh-directory]');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const names=['ArcCurve','CatmullRomCurve3','CubicBezierCurve','CubicBezierCurve3','EllipseCurve','LineCurve','LineCurve3','QuadraticBezierCurve','QuadraticBezierCurve3','SplineCurve'];
const classes={};
for(const name of names)classes[name]=(await import(pathToFileURL(path.join(upstream,'src/extras/curves',name+'.js')).href))[name];
const {Vector2}=await import(pathToFileURL(path.join(upstream,'src/math/Vector2.js')).href);
const {Vector3}=await import(pathToFileURL(path.join(upstream,'src/math/Vector3.js')).href);
const requests=JSON.parse(fs.readFileSync(new URL('./curve-subclasses-fixtures.json',import.meta.url)));
const create=request=>new classes[request.curve]().fromJSON(structuredClone(request.json));
const vector=request=>new (request.curve.endsWith('3')?Vector3:Vector2)();
const getter=(self,key,fn)=>Object.defineProperty(self,key,{get:fn,enumerable:true,configurable:true});
function clean(value){
    if(value===undefined)return {$undefined:true};
    if(typeof value==='number'&&!Number.isFinite(value))return {$number:Number.isNaN(value)?'NaN':value<0?'-Infinity':'Infinity'};
    if(typeof value==='number'&&Object.is(value,-0))return {$number:'-0'};
    if(Array.isArray(value))return value.map(clean);
    if(value&&typeof value==='object')return Object.fromEntries(Object.keys(value).map(key=>[key,clean(value[key])]));
    return value;
}
function metadata(){return names.map(name=>({name,constructor:{name:classes[name].name,length:classes[name].length,source:classes[name].toString(),parent:Object.getPrototypeOf(classes[name]).name,prototypeFlags:['writable','enumerable','configurable'].map(key=>Object.getOwnPropertyDescriptor(classes[name],'prototype')[key]),constructorFlags:['writable','enumerable','configurable'].map(key=>Object.getOwnPropertyDescriptor(classes[name].prototype,'constructor')[key])},keys:Object.getOwnPropertyNames(classes[name].prototype),methods:Object.getOwnPropertyNames(classes[name].prototype).filter(key=>key!=='constructor').map(key=>{
    const fn=classes[name].prototype[key],d=Object.getOwnPropertyDescriptor(classes[name].prototype,key);
    let constructible=false;try{Reflect.construct(function(){},[],fn);constructible=true;}catch{}
    return {key,name:fn.name,length:fn.length,source:fn.toString(),flags:[d.writable,d.enumerable,d.configurable],constructible};
})}));}
function construct(request,events){
    const Ctor=classes[request.curve],variant=request.variant,token={},marker={};let result;
    if(variant==='newTarget'||variant==='newTargetFallback'){
        function Alternate(){}Alternate.prototype=variant==='newTargetFallback'?7:marker;
        result=Reflect.construct(Ctor,[],Alternate);
    }else if(variant==='constructorIgnored'){
        const proto=Ctor.prototype,saved=Object.getOwnPropertyDescriptor(proto,'constructor');
        proto.constructor=function(){throw 'wrong-constructor';};
        try{result=new Ctor();}finally{Object.defineProperty(proto,'constructor',saved);}
    }else if(['superObject','superThrow','fieldThrow'].includes(variant)){
        const parent=Object.getPrototypeOf(Ctor);
        for(const key of ['type','arcLengthDivisions','isArcCurve','isEllipseCurve','isCatmullRomCurve3','isCubicBezierCurve','isCubicBezierCurve3','isLineCurve','isLineCurve3','isQuadraticBezierCurve','isQuadraticBezierCurve3','isSplineCurve','v0','v1','v2','v3','points','closed','curveType','tension','aX','aY','xRadius','yRadius','aStartAngle','aEndAngle','aClockwise','aRotation'])Object.defineProperty(token,key,{configurable:true,set(value){events.push(['set:'+key,clean(value)]);if(variant==='fieldThrow'&&key==='type')throw 'field';}});
        Object.setPrototypeOf(Ctor,function(...args){events.push(['super',args.length]);if(variant==='superThrow')throw 'super';return token;});
        try{result=new Ctor();}finally{Object.setPrototypeOf(Ctor,parent);}
    }else if(variant==='rawRadius')result=new Ctor(2,3,token,0,1,false);
    else result=new Ctor();
    const fields=Object.keys(result).map(key=>[key,clean(result[key])]);
    return {same:result===token,prototype:Object.getPrototypeOf(result)===(variant==='newTarget'?marker:['superObject','newTargetFallback'].includes(variant)?Object.prototype:Ctor.prototype),fields,...(variant==='rawRadius'?{radiusAlias:result.xRadius===token&&result.yRadius===token}:{})};
}
function observable(request,events){
    const self=create(request),variant=request.variant;
    let target=vector(request),t=request.t,token={},received;
    if(variant==='coercion'||variant==='coercionThrow')t={valueOf(){events.push('coerce:t');if(variant==='coercionThrow')throw 'coercion';return request.t;}};
    if(variant==='getterOrder'){
        const fields=request.curve==='ArcCurve'||request.curve==='EllipseCurve'?['aEndAngle','aStartAngle','aClockwise','aX','aY','xRadius','yRadius','aRotation']:request.curve==='SplineCurve'||request.curve==='CatmullRomCurve3'?['points','closed','curveType','tension']:Object.keys(request.json).filter(key=>/^v\d/.test(key));
        for(const key of fields){const value=self[key];getter(self,key,()=>{events.push('get:'+key);return value;});}
        if(self.points){const points=self.points,list={};getter(list,'length',()=>{events.push('get:points.length');return points.length;});points.forEach((p,i)=>getter(list,String(i),()=>{events.push('get:point:'+i);return p;}));Object.defineProperty(self,'points',{get(){events.push('get:points');return list;},configurable:true});}
    }
    if(variant==='targetSet'||variant==='targetReentry'){
        target={};
        getter(target,'set',()=>{
            events.push('get:set');
            if(variant==='targetReentry'){const inner=new classes.CatmullRomCurve3([new Vector3(1,2,3),new Vector3(4,1,0),new Vector3(9,5,-2)],false,'catmullrom',.7);events.push(['inner',inner.getPoint(.2).toArray()]);}
            return function(...values){events.push(['call:set',this===target]);received=values;return token;};
        });
    }
    if(variant==='targetNull')target=null;
    if(variant==='lineChain'){
        const stage={},stage2={};target={};
        for(const [obj,key,ret]of[[target,'copy',stage],[stage,'sub',{}],[target,'multiplyScalar',stage2],[stage2,'add',{}]])getter(obj,key,()=>{events.push('get:'+key);return function(value){events.push(['call:'+key,this===obj,value===self.v1?'v1':value===self.v2?'v2':value]);return ret;};});
        for(const key of ['v1','v2']){const v=self[key];getter(self,key,()=>{events.push('get:'+key);return v;});}
    }
    if(variant==='superCopyHome'){
        const proto=classes.LineCurve.prototype,parent=Object.getPrototypeOf(proto),alternate={};
        getter(alternate,'copy',function(){events.push(['get:super',this===self]);return function(source){events.push(['call:super',this===self,source===token]);return null;};});
        const source=create(request);token=source;
        Object.setPrototypeOf(proto,alternate);
        let result;try{result=proto.copy.call(self,source);}finally{Object.setPrototypeOf(proto,parent);}return {same:result===self,json:self.toJSON()};
    }
    const result=self.getPoint(t,target);
    if(variant==='targetSet'||variant==='targetReentry'||variant==='lineChain')return {same:result===target,token:result===token,received};
    return {same:result===target,point:result.toArray()};
}
function run(request){
    if(request.op==='metadata')return metadata();
    const events=[];
    try{
        let value;
        if(request.op==='construct')value=construct(request,events);
        else if(request.op==='sample'){
            const self=create(request);
            if(request.seed)new classes.CatmullRomCurve3([new Vector3(1,2,3),new Vector3(4,1,0),new Vector3(9,5,-2)],false,'catmullrom',.7).getPoint(.2);
            const target=request.target?vector(request):undefined;
            const result=self[request.method](request.t,target);
            value={point:result.toArray(),same:result===target};
        }else if(request.op==='copy'){
            const self=new classes[request.curve](),source=create(request),result=self.copy(source);
            const keys=Object.keys(request.json).filter(key=>/^v\d/.test(key));
            value={same:result===self,json:self.toJSON(),aliases:keys.map(key=>self[key]===source[key])};
            if(source.points)value.aliases=[self.points===source.points,...self.points.map((p,i)=>p===source.points[i])];
        }else value=observable(request,events);
        return clean({value,events});
    }catch(error){return clean({error:error&&['object','function'].includes(typeof error)?{name:error.name}:{name:'ThrownValue',raw:error},events});}
}
const expected=requests.map(run);
for(let i=0;i<requests.length;i++){
    if(requests[i].expectedError)assert.deepEqual(expected[i].error,requests[i].expectedError,requests[i].case);
    else if(expected[i].error&&!['targetNull','coercionThrow','superThrow','fieldThrow'].includes(requests[i].variant))assert.fail('Unexpected oracle error '+requests[i].case+': '+JSON.stringify(expected[i]));
}
const output=requestedOutput??path.join('D:/Temp/surui','curve-subclasses-'+Date.now());
assert(!fs.existsSync(output),'Output must be fresh');fs.mkdirSync(output,{recursive:true});
for(const [name,value]of[['requests',requests],['expected',expected]])fs.writeFileSync(path.join(output,name+'.json'),JSON.stringify(value,null,2));
const failures=[];
function compare(actual,wanted,where){
    if(typeof actual==='number'&&typeof wanted==='number')assert(Math.abs(actual-wanted)<=1e-12*Math.max(1,Math.abs(actual),Math.abs(wanted)),where+': '+actual+' != '+wanted);
    else if(Array.isArray(wanted)){assert(Array.isArray(actual),where);assert.equal(actual.length,wanted.length,where);wanted.forEach((v,i)=>compare(actual[i],v,where+'['+i+']'));}
    else if(wanted&&typeof wanted==='object'){assert.deepEqual(Object.keys(actual).sort(),Object.keys(wanted).sort(),where);for(const key of Object.keys(wanted))compare(actual[key],wanted[key],where+'.'+key);}
    else assert.deepEqual(actual,wanted,where);
}
if(probe!=='--oracle-only'){
    const actual=JSON.parse(execFileSync(path.resolve(probe),{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:64*1024*1024,timeout:300000,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}}).trim().split(/\r?\n/).at(-1));
    fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);
    requests.forEach((request,i)=>{try{compare(actual[i],expected[i],request.case);}catch(error){failures.push({case:request.case,message:error.message});}});
}
const summary={cases:requests.length,successes:expected.filter(v=>!v.error).length,expectedErrors:expected.filter(v=>v.error).length,failures};
fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));
console.log(JSON.stringify({output,...summary,failures:failures.length}));assert.equal(failures.length,0,JSON.stringify(failures.slice(0,5)));
