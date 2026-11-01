import {strictDifferences,inspectNumberTransport} from './strict-parity.mjs';
import {upstreamModule} from './threejs-source.mjs';
import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {fixtures} from './camera-fixtures.mjs';
const {Camera}=await upstreamModule('src/cameras/Camera.js');
const {PerspectiveCamera}=await upstreamModule('src/cameras/PerspectiveCamera.js');
const {OrthographicCamera}=await upstreamModule('src/cameras/OrthographicCamera.js');
const {ArrayCamera}=await upstreamModule('src/cameras/ArrayCamera.js');
const {CubeCamera}=await upstreamModule('src/cameras/CubeCamera.js');
const {StereoCamera}=await upstreamModule('src/cameras/StereoCamera.js');
const {Object3D}=await upstreamModule('src/core/Object3D.js');
const {Vector2}=await upstreamModule('src/math/Vector2.js');
const {Vector3}=await upstreamModule('src/math/Vector3.js');
const {Matrix4}=await upstreamModule('src/math/Matrix4.js');
const constructors={Camera,PerspectiveCamera,OrthographicCamera,ArrayCamera,CubeCamera,StereoCamera};
const scalar=v=>v===undefined?'#undefined':typeof v==='function'?'#function':typeof v==='symbol'?'#symbol':typeof v==='number'?(Number.isNaN(v)?'#NaN':v===Infinity?'#Infinity':v===-Infinity?'#-Infinity':Object.is(v,-0)?'#-0':v):v;
const error=e=>e instanceof Error?'#'+e.name:{thrown:scalar(e)};
function normal(v,seen=new Set()){
 if(v===null||typeof v!=='object')return scalar(v);
 if(seen.has(v))return '#cycle';seen=new Set(seen);seen.add(v);
 if(v.isVector2||v.isVector3||v.isQuaternion||v.isEuler)return normal(v.toArray(),seen);
 if(v.isMatrix4||v.isMatrix3)return normal(v.elements,seen);
 if(Array.isArray(v))return Array.from(v,x=>normal(x,seen));
 const out={};for(const key of Object.keys(v))out[key]=key==='uuid'?'#uuid':normal(v[key],seen);return out;
}
const scalarFields=['type','isCamera','isPerspectiveCamera','isOrthographicCamera','isArrayCamera','isMultiViewCamera','coordinateSystem','_reversedDepth','reversedDepth','fov','aspect','near','far','zoom','focus','filmGauge','filmOffset','left','right','top','bottom','activeMipmapLevel','eyeSep','matrixAutoUpdate','matrixWorldAutoUpdate','matrixWorldNeedsUpdate'];
function snapshot(v,depth=0){
 const out={keys:Object.keys(v)};for(const key of scalarFields)if(key in v)out[key]=scalar(v[key]);
 for(const key of ['position','rotation','quaternion','scale','up','matrix','matrixWorld','matrixWorldInverse','projectionMatrix','projectionMatrixInverse','view','_cache'])if(key in v)out[key]=normal(v[key]);
 if(v.layers)out.layers=v.layers.mask;
 if(v.children)out.children=depth<2?v.children.map(x=>snapshot(x,depth+1)):v.children.length;
 if(v.cameras)out.cameras=v.cameras.map(x=>x.name);
 if(v.cameraL)out.cameraL=snapshot(v.cameraL,depth+1);
 if(v.cameraR)out.cameraR=snapshot(v.cameraR,depth+1);
 return out;
}
function decode(v,s){if(v&&typeof v==='object'){if('$undefined'in v)return undefined;if('$ref'in v)return s[v.$ref];if('$number'in v)return v.$number==='-0'?-0:Number(v.$number);if('$vector2'in v)return new Vector2(...v.$vector2);if(Array.isArray(v))return v.map(x=>decode(x,s));const r={};for(const[k,x]of Object.entries(v))r[k]=decode(x,s);return r;}return v;}
function descriptor(d){const out={};for(const[k,v]of Object.entries(d))out[k]=scalar(v);return out;}
function metadata(C){const out={name:C.name,length:C.length,source:Function.prototype.toString.call(C),constructorKeys:Reflect.ownKeys(C),keys:Reflect.ownKeys(C.prototype),methods:[]};for(const key of out.keys){if(key==='constructor')continue;const d=Object.getOwnPropertyDescriptor(C.prototype,key),fn=d.value??d.get;out.methods.push({key,descriptor:descriptor(d),name:fn.name,length:fn.length,source:Function.prototype.toString.call(fn)});}return out;}
function special(q){const C=constructors[q.class],out={};
 if(q.special==='metadata')return metadata(C);
 if(q.special==='module'){const facade={[q.class]:C};facade[q.class]=null;return {unchanged:C===constructors[q.class],facade:facade[q.class]};}
 if(q.special==='newTarget'){function Target(){}Target.prototype=Object.create(C.prototype);const args=q.class==='CubeCamera'?[.3,150,{texture:{}}]:[];const v=Reflect.construct(C,args,Target);return {prototype:Object.getPrototypeOf(v)===Target.prototype,ownConstructor:Object.hasOwn(v,'constructor'),value:snapshot(v)};}
 if(q.special==='readonly'){const v=new C();let failure;try{v.reversedDepth=true;}catch(e){failure=error(e);}v._reversedDepth='flag';return {failure,value:v.reversedDepth,descriptor:descriptor(Object.getOwnPropertyDescriptor(Camera.prototype,'reversedDepth'))};}
 if(q.special==='arrayIdentity'){const a=[new PerspectiveCamera()],v=new ArrayCamera(a);a.push(new OrthographicCamera());return {same:v.cameras===a,count:v.cameras.length,cloneShared:v.clone().cameras===a,defaultArrays:new ArrayCamera().cameras!==new ArrayCamera().cameras};}
 if(q.special==='superHome'){const v=new C(),source=new C(),home=C.prototype,previous=Object.getPrototypeOf(home),events=[];const replacement=Object.create(previous);Object.defineProperty(replacement,'copy',{get(){events.push(['get',this===v]);return function(s,r){events.push(['call',this===v,s===source,r]);return 99;};},configurable:true});Object.setPrototypeOf(home,replacement);try{const result=home.copy.call(v,source,false);return {events,result:result===v};}finally{Object.setPrototypeOf(home,previous);}}
 if(q.special==='dynamicMath'){const v=new PerspectiveCamera(),tan=Math.tan,atan=Math.atan,events=[];try{Math.tan=function(x){events.push(['tan',this===Math,x]);return .25;};Math.atan=function(x){events.push(['atan',this===Math,x]);return .5;};const focal=v.getFocalLength(),effective=v.getEffectiveFOV();v.setFocalLength(20);return {events,focal,effective,fov:v.fov};}finally{Math.tan=tan;Math.atan=atan;}}
 if(q.special==='boundsReentry'){const v=new PerspectiveCamera(70,1.3),other=new PerspectiveCamera(35,.7),min=new Vector2(),max=new Vector2(),events=[];const old=min.set;min.set=function(x,y){events.push([x,y]);other.getViewBounds(9,new Vector2(),new Vector2());return old.call(this,x,y);};v.getViewBounds(3,min,max);return {events,min:normal(min),max:normal(max)};}
 if(q.special==='stereoReentry'){const v=new StereoCamera(),other=new StereoCamera(),camera=new PerspectiveCamera(75,1.7),second=new PerspectiveCamera(35,.6);camera.position.set(2,3,4);camera.updateMatrixWorld();const target=v.cameraL.projectionMatrix,old=target.copy,events=[];target.copy=function(m){events.push('copyL');other.eyeSep=.2;other.update(second);return old.call(this,m);};v.update(camera);return {events,value:snapshot(v)};}
 if(q.special==='iterator'){const events=[],cameras=Array.from({length:6},()=>({up:{set(){}},lookAt(){},updateMatrixWorld(){}}));let serial=0;const iterable={[Symbol.iterator](){const id=++serial;events.push('iterator:'+id);let index=0;return {next(){events.push('next:'+id+':'+index);if(q.fault==='next'&&id===1&&index===2)throw 71;return index<6?{value:cameras[index++],done:false}:{done:true};},return(){events.push('return:'+id);if(q.fault==='bindingReturn'||q.fault==='bodyReturn'&&id===2)throw 72;return {};}};}};const self={coordinateSystem:2000,children:{concat(){events.push('concat');return iterable;}},remove(){events.push('remove');if(q.fault==='body'||q.fault==='bodyReturn')throw 73;},add(){events.push('add');}};try{CubeCamera.prototype.updateCoordinateSystem.call(self);}catch(e){out.error=error(e);}return {...out,events};}
 if(q.special==='mathMethod'){const target=q.mathClass==='Matrix4'?new Matrix4():q.mathClass==='Vector2'?new Vector2():Math,args=q.args.map(x=>decode(x,{}));try{const result=target[q.method](...args);out.result=normal(result===target?target:result);}catch(e){out.error=error(e);}if(q.mathClass==='Matrix4')out.elements=normal(target.elements);return out;}
 throw Error('Unknown fixture special '+q.special);
}
function run(q){if(q.special)return special(q);const s={active:false,entered:false,events:[],scene:{tag:'scene'},min:new Vector2(),max:new Vector2(),size:new Vector2(),direction:new Vector3()};
 const emit=(label,extra)=>{if(!s.active)return;s.events.push(extra===undefined?label:[label,...extra]);if(q.throwAt===label)throw new TypeError('fixture');if(q.reenterAt===label&&!s.entered){s.entered=true;s.camera.update(s.renderer,s.scene);}};
 let mipmaps=true,coordinate=q.renderer?.coordinateSystem??2000,currentTarget={tag:'previous'},face=3,level=2;
 s.target={tag:'target',texture:{needsPMREMUpdate:false}};Object.defineProperty(s.target.texture,'mipmapsAutoUpdate',{enumerable:true,configurable:true,get(){emit('get:mipmaps');return mipmaps;},set(v){emit('set:mipmaps',[v]);mipmaps=v;}});
 const config=q.renderer??{};s.renderer={xr:{enabled:true},isWebGLRenderer:!!config.webgl,reversedDepthBuffer:!!config.reversed,autoClear:config.autoClear??true,state:{buffers:{depth:{getReversed(){emit('getReversed');return !!config.reversed;}}}},getRenderTarget(){emit('getRenderTarget');return currentTarget;},getActiveCubeFace(){emit('getActiveCubeFace');return face;},getActiveMipmapLevel(){emit('getActiveMipmapLevel');return level;},setRenderTarget(t,f,m){emit('setRenderTarget:'+f,[t.tag,f,m]);currentTarget=t;face=f;level=m;},clearDepth(){emit('clearDepth');},render(scene,camera){emit('render:'+face,[scene===s.scene,s.camera.children.indexOf(camera),mipmaps,this.xr.enabled]);}};
 Object.defineProperty(s.renderer,'coordinateSystem',{get(){emit('get:renderer.coordinateSystem');return coordinate;},set(v){coordinate=v;}});
 const C=constructors[q.class];s.camera=q.constructorArgs?new C(...q.constructorArgs.map(v=>decode(v,s))):q.class==='CubeCamera'?new C(.3,150,s.target):new C();s.source=q.class==='StereoCamera'?new PerspectiveCamera(65,1.6,.2,800):q.class==='CubeCamera'?new C(.4,200,s.target):new C();
 if(s.source.isObject3D){s.source.name='source';s.source.position.set(-2,4,1);s.source.rotation.set(.1,-.3,.2);s.source.scale.set(2,3,4);s.source._reversedDepth=true;s.source.coordinateSystem=2001;s.source.add(new Object3D());s.source.updateMatrixWorld(true);if(s.source.isPerspectiveCamera){s.source.fov=65;s.source.aspect=1.6;s.source.filmOffset=2;s.source.setViewOffset(1200,800,50,100,600,400);}if(s.source.isOrthographicCamera)s.source.setViewOffset(1000,800,10,20,500,400);}
 if(s.camera.isObject3D)s.camera.name='camera';
 for(const[k,v]of Object.entries(q.properties??{}))s.camera[k]=decode(v,s);
 if(q.transform)for(const[k,v]of Object.entries(q.transform))s.camera[k].set(...v);
 if(q.parent){s.parent=new Object3D();s.parent.position.set(1,2,-4);s.parent.rotation.set(.1,-.2,.3);s.parent.scale.set(1.2,1.3,1.4);s.parent.add(s.camera);}
 const observed=[];for(const key of q.observed??[]){if(!(key in s.camera))continue;let value=s.camera[key];const d=Object.getOwnPropertyDescriptor(s.camera,key);Object.defineProperty(s.camera,key,{configurable:true,enumerable:d?.enumerable??false,get(){emit('get:'+key);return value;},set(v){emit('set:'+key);value=v;}});observed.push(key);}
 const out={results:[]};s.active=true;for(const step of q.steps??[]){try{const result=step.get?s.camera[step.get]:s.camera[step.method](...(step.args??[]).map(v=>decode(v,s)));out.results.push(result===s.camera?'#self':result?.isObject3D||result?.cameraL?snapshot(result):normal(result));}catch(e){out.error=error(e);break;}}
 s.active=false;out.events=s.events;out.camera=snapshot(s.camera);out.source=snapshot(s.source);out.targets={min:normal(s.min),max:normal(s.max),size:normal(s.size),direction:normal(s.direction)};
 if(q.class==='CubeCamera')out.renderer={target:currentTarget.tag,face,level,xr:s.renderer.xr.enabled,mipmaps,needsPMREMUpdate:s.target.texture.needsPMREMUpdate};
 return out;
}
const argv=process.argv.slice(2),options={};for(let i=0;i<argv.length;i++){const key=argv[i];if(key==='--oracle-only'){if(options.oracleOnly)throw Error('Duplicate --oracle-only');options.oracleOnly=true;}else if(['--output','--probe','--input'].includes(key)){if(options[key]||!argv[i+1]||argv[i+1].startsWith('--'))throw Error('Missing/duplicate '+key);options[key]=argv[++i];}else throw Error('Unknown argument '+key);}
if(!options['--output']||Boolean(options.oracleOnly)===Boolean(options['--probe']))throw Error('Use --output NEW-DIR and exactly one of --oracle-only / --probe ABSOLUTE-EXE');
if(process.env.TEMP?.replaceAll('\\','/').toLowerCase()!=='d:/temp/surui'||process.env.TMP?.replaceAll('\\','/').toLowerCase()!=='d:/temp/surui')throw Error('TEMP/TMP must be D:/Temp/surui');
const output=path.resolve(options['--output']);if(fs.existsSync(output))throw Error('Output directory already exists');fs.mkdirSync(output,{recursive:true});
const input=options['--input']?JSON.parse(fs.readFileSync(options['--input'],'utf8')):fixtures;if(!Array.isArray(input))throw Error('Input must be array');
const expected=input.map(q=>({name:q.name,value:run(q)}));const wireAudit=inspectNumberTransport(expected);wireAudit.strictJSONRoundTripDifferences=strictDifferences(expected,JSON.parse(JSON.stringify(expected)));fs.writeFileSync(output+'/wire-audit.json',JSON.stringify(wireAudit,null,2)+'\n');if(wireAudit.rawSpecialNumberCount||wireAudit.strictJSONRoundTripDifferences.length)throw Error('Lossy output transport; see wire-audit.json');
fs.writeFileSync(output+'/input.json',JSON.stringify(input,null,2)+'\n');fs.writeFileSync(output+'/expected.json',JSON.stringify(expected,null,2)+'\n');
const sha=p=>createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const audit={upstream:'11738a9d55e502fb484cefffcea3a66dc44e9fe9',node:process.version,arguments:argv,cwd:process.cwd(),TEMP:process.env.TEMP,TMP:process.env.TMP,cases:input.length,nativeExecuted:false,oracleOnly:!!options.oracleOnly};
if(options['--probe']){const binary=options['--probe'];if(!path.isAbsolute(binary))throw Error('Probe path must be absolute');const before=sha(binary),start=Date.now(),result=spawnSync(binary,[],{input:JSON.stringify(input),encoding:'utf8',maxBuffer:64*1024*1024,env:process.env});audit.nativeExecuted=Number.isInteger(result.pid)&&result.pid>0;audit.binary={path:binary,beforeSHA256:before,afterSHA256:sha(binary),exitCode:result.status,signal:result.signal,error:result.error?.message,elapsedMs:Date.now()-start};fs.writeFileSync(output+'/native.stdout',result.stdout??'');fs.writeFileSync(output+'/native.stderr',result.stderr??'');const mismatches=[];if(!result.error&&result.status===0){let actual;try{actual=JSON.parse(result.stdout);}catch(e){mismatches.push({parseError:e.message});}const diff=(a,b,p)=>strictDifferences(a,b,p,mismatches);if(actual!==undefined)diff(expected,actual,'$');}else mismatches.push({nativeExit:result.status,error:result.error?.message});if(before!==audit.binary.afterSHA256)mismatches.push({binaryChanged:true});fs.writeFileSync(output+'/mismatches.json',JSON.stringify(mismatches,null,2)+'\n');audit.mismatches=mismatches.length;const lifetime=spawnSync(binary,['--lifetime'],{encoding:'utf8',maxBuffer:1024*1024,env:process.env});audit.lifetime={executed:Number.isInteger(lifetime.pid)&&lifetime.pid>0,exitCode:lifetime.status,signal:lifetime.signal,error:lifetime.error?.message,binarySHA256:sha(binary)};fs.writeFileSync(output+'/lifetime.stdout',lifetime.stdout??'');fs.writeFileSync(output+'/lifetime.stderr',lifetime.stderr??'');if(mismatches.length||lifetime.status!==0||audit.lifetime.binarySHA256!==before)process.exitCode=1;}
fs.writeFileSync(output+'/audit.json',JSON.stringify(audit,null,2)+'\n');console.log(JSON.stringify(audit));
