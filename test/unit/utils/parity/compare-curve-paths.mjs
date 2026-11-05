import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

const [upstream,probe,requestedOutput]=process.argv.slice(2);
assert(upstream&&probe,'Usage: compare-curve-paths.mjs <pinned> <probe|--oracle-only> [fresh-directory]');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=relative=>import(pathToFileURL(path.join(upstream,'src',relative)).href);
const {CurvePath}=await load('extras/core/CurvePath.js'),{Path}=await load('extras/core/Path.js'),{Shape}=await load('extras/core/Shape.js');
const {Vector2}=await load('math/Vector2.js'),{Vector3}=await load('math/Vector3.js'),Curves=await load('extras/curves/Curves.js');
const classes={CurvePath,Path,Shape},requests=JSON.parse(fs.readFileSync(new URL('./curve-paths-fixtures.json',import.meta.url)));
const getter=(self,key,get)=>Object.defineProperty(self,key,{get,enumerable:true,configurable:true});
const traceGet=(self,key,events,label,value)=>getter(self,key,()=>{events.push(label);return value;});
const validUUID=value=>typeof value==='string'&&/^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value);
function clean(value){
    if(value===undefined)return {$undefined:true};
    if(typeof value==='number'&&(!Number.isFinite(value)||Object.is(value,-0)))return {$number:Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':value<0?'-Infinity':'Infinity'};
    if(typeof value==='function')return {$function:true};
    if(Array.isArray(value))return value.map(clean);
    if(value&&typeof value==='object')return Object.fromEntries(Object.keys(value).map(key=>[key,clean(value[key])]));
    return value;
}
const pointValues=value=>Array.isArray(value)?value.map(pointValues):value&&(value.isVector2===true||value.isVector3===true)?value.toArray():value;
function curvePath(dim){
    const out=new CurvePath();
    if(dim===3){out.add(new Curves.LineCurve3(new Vector3(0,0,0),new Vector3(3,0,4)));out.add(new Curves.LineCurve3(new Vector3(3,0,4),new Vector3(3,4,4)));}
    else{out.add(new Curves.LineCurve(new Vector2(0,0),new Vector2(3,0)));out.add(new Curves.QuadraticBezierCurve(new Vector2(3,0),new Vector2(4,4),new Vector2(6,2)));}return out;
}
function populated(name){
    if(name==='CurvePath')return curvePath(2);
    const out=new classes[name]([new Vector2(0,0),new Vector2(4,0),new Vector2(4,3)]);
    out.quadraticCurveTo(2,5,0,3);out.closePath();out.autoClose=true;
    if(name==='Shape'){out.uuid='source-shape';const hole=new Path([new Vector2(1,1),new Vector2(1,2),new Vector2(2,1)]);hole.closePath();out.holes.push(hole);}return out;
}
function metadata(){return Object.entries(classes).map(([name,Ctor])=>({name,length:Ctor.length,source:Ctor.toString(),keys:Object.getOwnPropertyNames(Ctor.prototype),methods:Object.getOwnPropertyNames(Ctor.prototype).filter(key=>key!=='constructor').map(key=>{
    const fn=Ctor.prototype[key],d=Object.getOwnPropertyDescriptor(Ctor.prototype,key);let constructible=false;try{Reflect.construct(function(){},[],fn);constructible=true;}catch{}
    return {key,name:fn.name,length:fn.length,source:fn.toString(),flags:[d.writable,d.enumerable,d.configurable],constructible};
})}));}
function construct(request,events){
    const name=request.class,Ctor=classes[name],variant=request.variant,token={},marker={};let result;
    if(variant==='newTarget'){function Alternate(){}Alternate.prototype=marker;result=Reflect.construct(Ctor,[],Alternate);}
    else if(variant==='constructorIgnored'){
        const descriptor=Object.getOwnPropertyDescriptor(Ctor.prototype,'constructor');Ctor.prototype.constructor=function(){throw 'wrong-constructor';};
        try{result=new Ctor();}finally{Object.defineProperty(Ctor.prototype,'constructor',descriptor);}
    }else if(['superObject','superThrow','fieldThrow'].includes(variant)){
        const parent=Object.getPrototypeOf(Ctor);
        for(const key of ['type','curves','autoClose','currentPoint','uuid','holes'])Object.defineProperty(token,key,{configurable:true,set(value){events.push(['set:'+key,key==='uuid'?validUUID(value):clean(value)]);if(variant==='fieldThrow'&&key==='type')throw 'field';}});
        Object.setPrototypeOf(Ctor,function(...args){events.push(['super',args.length]);if(variant==='superThrow')throw 'super';return token;});
        try{result=new Ctor();}finally{Object.setPrototypeOf(Ctor,parent);}
    }else if(variant==='points')result=new Ctor([new Vector2(1,2),new Vector2(3,4),new Vector2(-1,5)]);
    else if(variant==='emptyPoints')result=new Ctor([]);
    else if(variant==='falsePoints')result=new Ctor(false);
    else result=new Ctor();
    return {same:result===token,fields:Object.keys(result).map(key=>[key,key==='uuid'?'$uuid':clean(result[key])]),prototype:Object.getPrototypeOf(result)===(variant==='newTarget'?marker:variant==='superObject'?Object.prototype:Ctor.prototype),...(name==='Shape'&&variant!=='superObject'?{uuidValid:validUUID(result.uuid)}:{})};
}
function numeric(request){
    const self=curvePath(request.dim);self.autoClose=!!request.close;const args=Object.hasOwn(request,'arg')?[request.arg]:[],target=request.dim===3?new Vector3():new Vector2();if(request.target)args.push(target);
    const result=self[request.method](...args);return {value:result===self?'$self':pointValues(result),target:result===target,cache:result===self.cacheLengths,json:self.toJSON(),...(Array.isArray(result)&&result.length?{closedAlias:result[0]===result.at(-1)}:{})};
}
function sequence(request){
    const self=new Path(),returns=[];
    for(const action of request.actions){const args=action.args.map(value=>value&&typeof value==='object'&&Object.hasOwn(value,'points')?value.points.map(p=>new Vector2(p[0],p[1])):value);returns.push(self[action.method](...args)===self);}
    return {returns,json:self.toJSON(),points:pointValues(self.getPoints(3))};
}
function copyOrJSON(request){
    const name=request.class,source=populated(name),target=new classes[name]();if(name==='Shape')target.uuid='target-shape';const out={};
    if(request.op==='roundtrip'){out.same=target.fromJSON(source.toJSON())===target;out.json=target.toJSON();}
    else{out.same=target.copy(source)===target;out.aliases=[target.curves===source.curves,...target.curves.map((c,i)=>c===source.curves[i])];if(name!=='CurvePath')out.aliases.push(target.currentPoint===source.currentPoint);if(name==='Shape')out.aliases.push(target.holes===source.holes,target.holes[0]===source.holes[0]);out.json=target.toJSON();}
    out.points=pointValues(target.getPoints(3));return out;
}
function observe(request,events){
    const variant=request.variant,cp=CurvePath.prototype,pp=Path.prototype,sp=Shape.prototype,token={};let self={},result;
    if(variant==='addDispatch'){
        const curves={};traceGet(self,'curves',events,'get:curves',curves);traceGet(curves,'push',events,'get:push',function(value){events.push(['push',this===curves,value===token]);return 13;});result=cp.add.call(self,token);
    }else if(['cacheHit','cacheMiss','cacheThrow'].includes(variant)){
        const curves=[],cache=[7,9];for(let i=0;i<2;i++){const child={};traceGet(child,'getLength',events,'get:length:'+i,function(){events.push('call:length:'+i);return i===0?'2':3;});curves.push(child);}traceGet(self,'curves',events,'get:curves',curves);
        if(variant==='cacheThrow')getter(self,'cacheLengths',()=>{events.push('get:cache');throw 'cache';});else traceGet(self,'cacheLengths',events,'get:cache',variant==='cacheHit'?cache:null);
        if(variant==='cacheMiss')Object.defineProperty(self,'cacheLengths',{configurable:true,get(){events.push('get:cache');return null;},set(value){events.push(['set:cache',value]);}});
        result=cp.getCurveLengths.call(self);return {value:result,same:result===cache};
    }else if(variant==='updateDispatch'){
        for(const key of ['needsUpdate','cacheLengths'])Object.defineProperty(self,key,{configurable:true,set(value){events.push(['set:'+key,value]);}});
        traceGet(self,'getCurveLengths',events,'get:lengths',function(){events.push(['call:lengths',this===self]);return 3;});result=cp.updateArcLengths.call(self);
    }else if(variant==='lengthDispatch'){
        const lengths={};traceGet(lengths,'length',events,'get:length',3);traceGet(lengths,'2',events,'get:2','tail');traceGet(self,'getCurveLengths',events,'get:lengths',function(){events.push(['call:lengths',this===self]);return lengths;});result=cp.getLength.call(self);
    }else if(['pointDispatch','pointZero','pointOutside','pointCoercionThrow'].includes(variant)){
        const zero=variant==='pointZero',lengths=[zero?0:2,5],curves=[];for(let i=0;i<2;i++){const child={};traceGet(child,'getLength',events,'get:segmentLength',function(){events.push('call:segmentLength');return zero?0:i===0?2:3;});traceGet(child,'getPointAt',events,'get:pointAt',function(u,target){events.push(['call:pointAt',this===child,u,target===token]);return token;});curves.push(child);}
        traceGet(self,'getLength',events,'get:totalLength',function(){events.push('call:totalLength');return 5;});traceGet(self,'getCurveLengths',events,'get:lengths',function(){events.push('call:lengths');return lengths;});traceGet(self,'curves',events,'get:curves',curves);
        const t={valueOf(){events.push('coerce:t');if(variant==='pointCoercionThrow')throw 't';return variant==='pointOutside'?2:variant==='pointZero'?0:.6;}};result=cp.getPoint.call(self,t,token);return result===token?'$token':result;
    }else if(variant==='spacedDispatch'||variant==='spacedThrow'){
        traceGet(self,'getPoint',events,'get:point',function(t){events.push(['call:point',this===self,t]);if(variant==='spacedThrow'&&t===.5)throw 'point';return t;});traceGet(self,'autoClose',events,'get:autoClose',true);
        result=cp.getSpacedPoints.call(self,{valueOf(){events.push('coerce:divisions');return 2;}});
    }else if(variant==='pointsResolution'){
        const curves=[],a=new Vector2(0,0),b=new Vector2(1,0),c=new Vector2(1,1),kinds=['ellipse','line','line3','spline','other'];
        for(const [i,kind]of kinds.entries()){const child={};for(const flag of ['isEllipseCurve','isLineCurve','isLineCurve3','isSplineCurve'])traceGet(child,flag,events,'get:'+kind+':'+flag,kind==='ellipse'&&flag==='isEllipseCurve'||kind==='line'&&flag==='isLineCurve'||kind==='line3'&&flag==='isLineCurve3'||kind==='spline'&&flag==='isSplineCurve');child.points=[0,1,2];traceGet(child,'getPoints',events,'get:'+kind+':getPoints',function(resolution){events.push(['call:'+kind,this===child,resolution]);return i===0?[a,b]:[b,c];});curves.push(child);}self.curves=curves;self.autoClose=true;result=cp.getPoints.call(self,2);return {points:pointValues(result),closedAlias:result[0]===result.at(-1)};
    }else if(variant==='closeEmpty'){self.curves=[];result=cp.closePath.call(self);}
    else if(['close2','close3','closeEqual'].includes(variant)){
        const dim3=variant==='close3',a=dim3?new Vector3(1,2,3):new Vector2(1,2),b=variant==='closeEqual'?a:dim3?new Vector3(4,5,6):new Vector2(4,5),curves=[];
        for(let i=0;i<2;i++){const child={};traceGet(child,'getPoint',events,'get:point:'+i,function(t){events.push(['call:point',t]);return i===0?a:b;});curves.push(child);}self.curves=curves;result=cp.closePath.call(self);const out={same:result===self,count:curves.length};if(curves.length===3){const line=curves[2];out.type=line.type;out.aliases=[line.v1===b,line.v2===a];}return out;
    }else if(['forwardArc','forwardEllipse','forwardAbsarc'].includes(variant)){
        const current={};traceGet(current,'x',events,'get:x',10);traceGet(current,'y',events,'get:y',20);traceGet(self,'currentPoint',events,'get:currentPoint',current);const destination=variant==='forwardArc'?'absarc':'absellipse';traceGet(self,destination,events,'get:'+destination,function(...args){events.push(['forward',this===self,args]);return token;});
        const x={valueOf(){events.push('coerce:x');return 'A';}},y={valueOf(){events.push('coerce:y');return 2;}},args=variant==='forwardArc'||variant==='forwardAbsarc'?[x,y,3,.2,1,true]:[x,y,3,4,.2,1,false,.3];result=pp[variant==='forwardArc'?'arc':variant==='forwardEllipse'?'ellipse':'absarc'].call(self,...args);return result===self;
    }else if(variant==='setFromPointsDispatch'||variant==='setFromPointsThrow'){
        const input={};traceGet(input,'length',events,'get:points.length',3);for(let i=0;i<3;i++){const p={};traceGet(p,'x',events,'get:x:'+i,i);if(variant==='setFromPointsThrow'&&i===1)getter(p,'y',()=>{events.push('get:y:1');throw 'y';});else traceGet(p,'y',events,'get:y:'+i,i+4);traceGet(input,String(i),events,'get:point:'+i,p);}for(const key of ['moveTo','lineTo'])traceGet(self,key,events,'get:'+key,function(...args){events.push(['call:'+key,this===self,args]);return null;});result=pp.setFromPoints.call(self,input);return result===self;
    }else if(['moveDispatch','lineDispatch','quadraticDispatch','cubicDispatch','splineDispatch','splineEmpty'].includes(variant)){
        const current={},curves={},clone=new Vector2(1,2),input=variant==='splineEmpty'?[]:[new Vector2(3,4),new Vector2(5,6)];traceGet(self,'currentPoint',events,'get:currentPoint',current);traceGet(self,'curves',events,'get:curves',curves);traceGet(current,'clone',events,'get:clone',function(){events.push(['clone',this===current]);return clone;});
        for(const key of ['set','copy'])traceGet(current,key,events,'get:'+key,function(...args){events.push(['call:'+key,this===current,key==='copy'?pointValues(args[0]):args]);if(variant==='splineEmpty'&&args[0]===undefined)throw 'empty';return null;});
        traceGet(curves,'push',events,'get:push',function(curve){events.push(['push',this===curves,curve.type,curve.isSplineCurve?[curve.points[0]===clone,curve.points[1]===input[0]]:curve[curve.isLineCurve?'v1':'v0']===clone]);return 0;});
        const method=variant==='moveDispatch'?'moveTo':variant==='lineDispatch'?'lineTo':variant==='quadraticDispatch'?'quadraticCurveTo':variant==='cubicDispatch'?'bezierCurveTo':'splineThru',args=variant==='quadraticDispatch'?[3,4,5,6]:variant==='cubicDispatch'?[3,4,5,6,7,8]:variant==='splineDispatch'||variant==='splineEmpty'?[input]:[9,10];result=pp[method].call(self,...args);return result===self;
    }else if(['absellipseJoin','absellipseNoJoin','absellipseEmpty'].includes(variant)){
        const proto=Curves.EllipseCurve.prototype,saved=Object.getOwnPropertyDescriptor(proto,'getPoint'),start=new Vector2(1,2),end=new Vector2(3,4),current=variant==='absellipseNoJoin'?start:new Vector2(8,9),curves={};
        traceGet(curves,'length',events,'get:curves.length',variant==='absellipseEmpty'?0:1);traceGet(self,'curves',events,'get:curves',curves);traceGet(self,'currentPoint',events,'get:currentPoint',current);proto.getPoint=function(t){events.push(['ellipse:point',this.type,t]);return t===0?start:end;};
        traceGet(self,'lineTo',events,'get:lineTo',function(...args){events.push(['lineTo',this===self,args]);});traceGet(curves,'push',events,'get:push',function(curve){events.push(['push',this===curves,curve.type]);return 0;});
        try{result=pp.absellipse.call(self,0,0,2,3,0,1,false,0);}finally{Object.defineProperty(proto,'getPoint',saved);}return {same:result===self,point:current.toArray()};
    }else if(variant==='extractDispatch'||variant==='extractThrow'){
        const shape=[1],holes=[2];for(const [key,value]of [['getPoints',shape],['getPointsHoles',holes]])traceGet(self,key,events,'get:'+key,function(divisions){events.push(['call:'+key,this===self,divisions===token]);if(variant==='extractThrow')throw 'extract';return value;});result=sp.extractPoints.call(self,token);return [result.shape===shape,result.holes===holes];
    }else if(['holesDispatch','holesReenter','holesThrow'].includes(variant)){
        const input={};traceGet(input,'length',events,'get:holes.length',2);for(let i=0;i<2;i++){const hole={};traceGet(hole,'getPoints',events,'get:holePoints:'+i,function(divisions){events.push(['holePoints',this===hole,divisions===token,i]);if(variant==='holesThrow')throw 'hole';if(variant==='holesReenter'&&i===0)events.push(['inner',sp.getPointsHoles.call({holes:[]},1)]);return [i];});traceGet(input,String(i),events,'get:hole:'+i,hole);}traceGet(self,'holes',events,'get:holes',input);result=sp.getPointsHoles.call(self,token);
    }else if(variant==='copySuperHome'||variant==='copySuperThrow'){
        const name=request.class,proto=classes[name].prototype,parent=Object.getPrototypeOf(proto),alternate={},source=populated(name);self=new classes[name]();if(name==='Shape')self.uuid='target-shape';const original=proto.copy;
        getter(alternate,'copy',function(){events.push('get:superCopy');return function(s){events.push(['superCopy',this===self,s===source]);if(variant==='copySuperThrow')throw 'supercopy';return null;};});Object.setPrototypeOf(proto,alternate);
        try{result=original.call(self,source);}finally{Object.setPrototypeOf(proto,parent);}return {same:result===self,json:self.toJSON()};
    }else throw new Error('Unknown CurvePaths observable '+variant);
    return result;
}
function run(request){
    if(request.op==='metadata')return metadata();const events=[];
    try{
        let value;if(request.op==='construct')value=construct(request,events);else if(request.op==='numeric')value=numeric(request);else if(request.op==='sequence')value=sequence(request);else if(['copy','roundtrip'].includes(request.op))value=copyOrJSON(request);
        else if(request.op==='shapePoints'){const self=populated('Shape'),args=Object.hasOwn(request,'arg')?[request.arg]:[],result=self.extractPoints(...args);value={shape:pointValues(result.shape),holes:pointValues(result.holes)};}
        else value=observe(request,events);return clean({value,events});
    }catch(error){return clean({error:error!==null&&['object','function'].includes(typeof error)?{name:error.name}:{name:'ThrownValue',raw:error},events});}
}
const expected=requests.map(run);
requests.forEach((request,i)=>assert.deepEqual(expected[i].error,request.expectedError,'Unexpected pinned result '+request.case));
const output=requestedOutput??path.join('D:/Temp/surui','curve-paths-'+Date.now());assert(!fs.existsSync(output),'Output must be fresh');fs.mkdirSync(output,{recursive:true});
for(const [name,value]of [['requests',requests],['expected',expected]])fs.writeFileSync(path.join(output,name+'.json'),JSON.stringify(value,null,2));
const failures=[];
function compare(actual,wanted,where){
    if(typeof actual==='number'&&typeof wanted==='number')assert(Math.abs(actual-wanted)<=1e-12*Math.max(1,Math.abs(actual),Math.abs(wanted)),where+': '+actual+' != '+wanted);
    else if(Array.isArray(wanted)){assert(Array.isArray(actual),where);assert.equal(actual.length,wanted.length,where);wanted.forEach((v,i)=>compare(actual[i],v,where+'['+i+']'));}
    else if(wanted&&typeof wanted==='object'){assert(actual&&typeof actual==='object',where);assert.deepEqual(Object.keys(actual).sort(),Object.keys(wanted).sort(),where);for(const key of Object.keys(wanted))compare(actual[key],wanted[key],where+'.'+key);}
    else assert.deepEqual(actual,wanted,where);
}
if(probe!=='--oracle-only'){
    const actual=JSON.parse(execFileSync(path.resolve(probe),{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:64*1024*1024,timeout:300000,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}}).trim().split(/\r?\n/).at(-1));fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);
    requests.forEach((request,i)=>{try{compare(actual[i],expected[i],request.case);}catch(error){failures.push({case:request.case,message:error.message});}});
}
const summary={cases:requests.length,successes:expected.filter(v=>!v.error).length,expectedErrors:expected.filter(v=>v.error).length,failures};fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));
console.log(JSON.stringify({output,...summary,failures:failures.length}));assert.equal(failures.length,0,JSON.stringify(failures.slice(0,5)));
