import fs from 'node:fs';
import path from 'node:path';
import {registerHooks} from 'node:module';
import {upstream,upstreamModule,upstreamModuleURL} from './threejs-source.mjs';
registerHooks({resolve(specifier,context,next){if(specifier==='three')return {url:upstreamModuleURL('src/Three.js'),shortCircuit:true};return next(specifier,context);}});
const {FlyControls}=await upstreamModule('examples/jsm/controls/FlyControls.js');
const {Controls}=await upstreamModule('src/extras/Controls.js');
const {Object3D}=await upstreamModule('src/core/Object3D.js');
export const callbackNames=['onKeyDown','onKeyUp','onPointerMove','onPointerDown','onPointerUp','onPointerCancel','onContextMenu'];
const scalar=v=>v===undefined?'#undefined':typeof v==='symbol'?'#symbol:'+v.description:typeof v==='number'?(Number.isNaN(v)?'#NaN':v===Infinity?'#Infinity':v===-Infinity?'#-Infinity':Object.is(v,-0)?'#-0':v):typeof v==='function'?'#function:'+v.name:v;
const apply=(o,k,a=[])=>o[k](...a);
function normal(v,s,seen=new Set()){
 if(v===null||typeof v!=='object')return scalar(v);if(v===s.marker)return '#marker';if(v===s.self)return '#self';if(v===s.object)return '#object';
 for(const k of ['window','document','surface'])if(v===s[k])return '#'+k;
 if(v===null||typeof v!=='object')return scalar(v);if(seen.has(v))return '#cycle';seen=new Set(seen);seen.add(v);
 if(Array.isArray(v))return Array.from(v,x=>normal(x,s,seen));const out={};for(const key of Object.keys(v))out[key]=normal(v[key],s,seen);return out;
}
function failure(e,s){return e===s.marker?'#marker':e instanceof Error?'#'+e.name:normal(e,s);}
function decode(v,s){if(v===null||typeof v!=='object')return v;if(v.$ref)return s[v.$ref];if(v.$undefined)return undefined;if(v.$number)return v.$number==='-0'?-0:Number(v.$number);if(v.$symbol)return Symbol(v.$symbol);if(v.$coerce)return {[Symbol.toPrimitive](hint){s.events.push(['coerce',v.$coerce.label,hint]);if(v.$coerce.throw)throw s.marker;return decode(v.$coerce.value,s);}};if(Array.isArray(v))return v.map(x=>decode(x,s));return Object.fromEntries(Object.entries(v).map(([k,x])=>[k,decode(x,s)]));}
function createHost(s){
 const lists=new Map(),metrics={width:800,height:600,left:10,top:20,surfaceWidth:640,surfaceHeight:480};s.lists=lists;s.metrics=metrics;
 for(const [kind,index]of [['window',0],['document',1],['surface',2]]){
  const target={},map=new Map();lists.set(target,map);s[kind]=target;
  target.addEventListener=function addEventListener(type,callback){type=String(type);if(callback==null)return;if(typeof callback!=='function')throw TypeError();let list=map.get(type);if(!list)map.set(type,list=[]);if(list.some(e=>!e.removed&&e.callback===callback))return;list.push({callback,removed:false});};
  target.removeEventListener=function removeEventListener(type,callback){type=String(type);for(const entry of map.get(type)??[])if(!entry.removed&&entry.callback===callback){entry.removed=true;entry.callback=undefined;break;}};
  for(const key of ['innerWidth','innerHeight','offsetWidth','offsetHeight','offsetLeft','offsetTop'])Object.defineProperty(target,key,{configurable:true,enumerable:true,get(){return key==='innerWidth'?metrics.width:key==='innerHeight'?metrics.height:key==='offsetWidth'?(index===2?metrics.surfaceWidth:metrics.width):key==='offsetHeight'?(index===2?metrics.surfaceHeight:metrics.height):key==='offsetLeft'?(index===2?metrics.left:0):(index===2?metrics.top:0);}});
  let touchAction='';target.style={};Object.defineProperty(target.style,'touchAction',{configurable:true,enumerable:true,get(){return touchAction;},set(v){touchAction=String(v);}});
 }
 s.dispatch=(target,type,event={})=>{event.type=type;event.target=target;event.currentTarget=target;event.defaultPrevented=false;event.preventDefault=function preventDefault(){this.defaultPrevented=true;};const snapshot=[...(lists.get(target).get(type)??[])];try{for(const entry of snapshot)if(!entry.removed)entry.callback.call(target,event);}finally{event.currentTarget=null;}return !!event.defaultPrevented;};
 s.oldWindow=Object.getOwnPropertyDescriptor(globalThis,'window');s.oldDocument=Object.getOwnPropertyDescriptor(globalThis,'document');Object.defineProperty(globalThis,'window',{value:s.window,writable:true,configurable:true});Object.defineProperty(globalThis,'document',{value:s.document,writable:true,configurable:true});
}
function cleanup(s){for(const key of ['window','document']){const old=s[key==='window'?'oldWindow':'oldDocument'];if(old)Object.defineProperty(globalThis,key,old);else delete globalThis[key];}}
function locate(s,path){const bits=path.split('.');let object=s[bits.shift()];while(bits.length>1)object=object[bits.shift()];return [object,bits[0]];}
function hook(s,path){const [object,key]=locate(s,path),old=Object.getOwnPropertyDescriptor(object,key);let value=object[key];Object.defineProperty(object,key,{configurable:true,enumerable:old?.enumerable??true,get(){signal(s,'get:'+path);if(typeof value==='function')return function(...args){signal(s,'call:'+path);return Reflect.apply(value,this,args);};return value;},set(next){signal(s,'set:'+path);value=next;}});s.restores.push(()=>old?Object.defineProperty(object,key,old):delete object[key]);}
function signal(s,label){if(!s.active)return;s.events.push(label);if(s.q.throwAt===label)throw s.marker;if(s.q.reenterAt===label&&!s.entered){s.entered=true;s.events.push('reentry:begin');for(const step of s.q.reentrySteps??[])stepRun(s,step);s.events.push('reentry:end');}}
function stepRun(s,step){let value;
 if(step.op==='set'){const [object,key]=locate(s,step.path);object[key]=decode(step.value,s);}
 else if(step.op==='call')value=Reflect.apply(s.self[step.method],Object.hasOwn(step,'receiver')?decode(step.receiver,s):s.self,(step.args??[]).map(x=>decode(x,s)));
 else if(step.op==='callback')value=s.self['_'+step.name].call(step.foreignThis?{}:s.self,decode(step.event??{},s));
 else if(step.op==='dispatch')value=s.dispatch(s[step.target??'surface'],step.type,decode(step.event??{},s));
 else if(step.op==='metrics')Object.assign(s.metrics,step.value);
 else if(step.op==='collect')globalThis.gc?.();
 else throw Error('Unknown step '+step.op);
 return normal(value,s);
}
function listenerState(s,target){const out={};for(const [type,list]of s.lists.get(target)){const current=list.filter(e=>!e.removed);out[type]=current.map(e=>({name:e.callback.name,key:callbackNames.map(n=>'_'+n).find(k=>s.self&&e.callback===s.self[k])??'#foreign'}));}return out;}
function snapshot(s){const c=s.self;if(!c)return null;
 const out={ownKeys:Reflect.ownKeys(c).map(scalar),objectSame:c.object===s.object,domElement:normal(c.domElement,s)};
 for(const key of ['enabled','state','keys','mouseButtons','touches','movementSpeed','rollSpeed','dragToLook','autoForward','_moveState','_status','movementSpeedMultiplier'])out[key]=normal(c[key],s);
 for(const key of ['_moveVector','_rotationVector','_lastQuaternion','_lastPosition'])out[key]=c[key]?.toArray().map(scalar)??'#undefined';
 out.position=c.object?.position?.toArray().map(scalar)??'#undefined';out.quaternion=c.object?.quaternion?.toArray().map(scalar)??'#undefined';
 out.callbacks=callbackNames.map(name=>({key:'_'+name,name:c['_'+name].name,length:c['_'+name].length,same:c['_'+name]===c['_'+name]}));
 return out;
}
function constructible(f){try{Reflect.construct(function(){},[],f);return true;}catch{return false;}}
function functionInfo(f){return {name:f.name,length:f.length,source:Function.prototype.toString.call(f),ownKeys:Reflect.ownKeys(f).map(scalar),constructible:constructible(f)};}
function metadata(C){return {constructor:functionInfo(C),parentName:Object.getPrototypeOf(C).name,prototypeKeys:Reflect.ownKeys(C.prototype).map(scalar),methods:Object.getOwnPropertyNames(C.prototype).filter(k=>k!=='constructor').map(k=>{const d=Object.getOwnPropertyDescriptor(C.prototype,k);return {key:k,writable:d.writable,enumerable:d.enumerable,configurable:d.configurable,fn:functionInfo(d.value)};})};}
export function run(q){if(q.kind==='metadata')return metadata(q.class==='Controls'?Controls:FlyControls);
 const s={q,events:[],outputs:[],marker:{},restores:[],active:true};createHost(s);s.object=Object3D?new Object3D():{};s.self=undefined;if(q.noHost){delete globalThis.window;delete globalThis.document;}
 try{
  if(q.kind==='base'){const arg=Object.hasOwn(q,'object')?decode(q.object,s):s.object;const element=Object.hasOwn(q,'dom')?decode(q.dom,s):null;const c=new Controls(arg,element);s.self=c;s.object=arg;for(const step of q.steps??[])try{s.outputs.push(stepRun(s,step));}catch(e){s.outputs.push({error:failure(e,s)});}s.active=false;return {ownKeys:Reflect.ownKeys(c).map(scalar),objectSame:c.object===arg,domElement:normal(c.domElement,s),enabled:normal(c.enabled,s),state:normal(c.state,s),keys:normal(c.keys,s),mouseButtons:normal(c.mouseButtons,s),touches:normal(c.touches,s),events:s.events,outputs:s.outputs};}
  if(q.kind==='helpers'){const original=Function.prototype.bind,helpers=[];Function.prototype.bind=function(...args){helpers.push(functionInfo(this));return Reflect.apply(original,this,args);};try{s.self=new FlyControls(s.object,null);}finally{Function.prototype.bind=original;}return {helpers,callbacks:callbackNames.map(n=>functionInfo(s.self['_'+n]))};}
  s.active=false;for(const path of q.preHooks??[])hook(s,path);s.active=true;
  let error;
  try{if(q.callWithoutNew)FlyControls(s.object);else{const object=Object.hasOwn(q,'object')?decode(q.object,s):s.object;const element=Object.hasOwn(q,'dom')?decode(q.dom,s):null;if(q.newTarget){function Target(){}Target.prototype=Object.create(FlyControls.prototype);s.expectedPrototype=Target.prototype;s.self=Reflect.construct(FlyControls,[object,element],Target);}else s.self=new FlyControls(object,element);s.object=object;}}
  catch(e){error=failure(e,s);}
  if(s.self){s.active=false;for(const [path,v]of Object.entries(q.set??{})){const [obj,key]=locate(s,path);obj[key]=decode(v,s);}for(const path of q.hooks??[])hook(s,path);s.active=true;
   if(q.change){s.self.addEventListener('change',function(event){s.events.push(['change',this===s.self,event.target===s.self,event===s.lastEvent]);s.lastEvent=event;if(q.change.throw)throw s.marker;if(q.change.reenter&&!s.entered){s.entered=true;for(const step of q.change.steps??[])stepRun(s,step);}});}
   for(const step of q.steps??[])try{s.outputs.push(stepRun(s,step));}catch(e){s.outputs.push({error:failure(e,s)});}
  }
  s.active=false;const out={error:error??'#undefined',events:s.events,outputs:s.outputs,value:snapshot(s),listeners:{window:listenerState(s,s.window),document:listenerState(s,s.document),surface:listenerState(s,s.surface)},touchAction:{window:s.window.style.touchAction,document:s.document.style.touchAction,surface:s.surface.style.touchAction}};if(q.newTarget)out.prototypeIsTarget=Object.getPrototypeOf(s.self)===s.expectedPrototype;if(s.lastEvent)out.changeEventTarget=normal(s.lastEvent.target,s);return out;
 }finally{s.active=false;for(const restore of s.restores.reverse())restore();cleanup(s);}
}
export function fixtures(){const out=[],add=(name,q)=>out.push({name,...q});
 for(const cls of ['Controls','FlyControls'])add(cls+'.metadata',{kind:'metadata',class:cls});add('Fly.helpers-and-bound-identity',{kind:'helpers'});
 add('Fly.defaults',{});add('Fly.no-native-host-default',{noHost:true});add('Fly.no-native-host-connect',{noHost:true,dom:{$ref:'surface'}});add('Fly.no-new',{callWithoutNew:true});add('Fly.newTarget',{newTarget:true});
 for(const dom of [null,{$undefined:true},{$ref:'surface'},{$ref:'document'},false,0,'bad'])add('Fly.constructor.dom.'+JSON.stringify(dom),{dom,preHooks:dom&&dom.$ref?['window.removeEventListener','window.addEventListener','surface.removeEventListener','surface.addEventListener','surface.style','surface.style.touchAction']:[]});
 for(const object of [null,{$undefined:true},17,'object'])add('Fly.object.'+JSON.stringify(object),{object,steps:[{op:'call',method:'update',args:[0.1]}]});
 for(const dom of [null,{$undefined:true},{$ref:'surface'},17])add('Controls.dom.'+JSON.stringify(dom),{kind:'base',dom,steps:[{op:'call',method:'connect',args:[{$ref:'document'}]},{op:'call',method:'disconnect'},{op:'call',method:'dispose'},{op:'call',method:'update',args:[0.1]}]});
 const codes=['ShiftLeft','ShiftRight','KeyW','KeyS','KeyA','KeyD','KeyR','KeyF','ArrowUp','ArrowDown','ArrowLeft','ArrowRight','KeyQ','KeyE','Unknown'];
 for(const code of codes)for(const name of ['onKeyDown','onKeyUp'])add(name+'.'+code,{steps:[{op:'callback',name,event:{code},foreignThis:true}]});
 for(const enabled of [false,true,0,1,null,{$undefined:true},''])for(const name of callbackNames)add('enabled.'+JSON.stringify(enabled)+'.'+name,{set:{'self.enabled':enabled},steps:[{op:'callback',name,event:{code:'KeyW',button:0,pageX:100,pageY:80,altKey:false}}],hooks:['self.enabled']});
 for(const altKey of [true,1,'yes',false,0,null])add('alt.'+JSON.stringify(altKey),{hooks:['self.enabled','self._moveState'],steps:[{op:'callback',name:'onKeyDown',event:{code:'KeyW',altKey}}]});
 for(const dragToLook of [false,true,1,null,''])for(const button of [0,1,2,-1,'0'])for(const name of ['onPointerDown','onPointerUp'])add(name+'.drag.'+JSON.stringify(dragToLook)+'.button.'+JSON.stringify(button),{set:{'self.dragToLook':dragToLook,'self._status':1},steps:[{op:'callback',name,event:{button}}]});
 for(const dragToLook of [false,true])for(const status of [0,1,-1,'2',{$number:'NaN'}])for(const dom of [{$ref:'surface'},{$ref:'document'}])add('pointermove.'+JSON.stringify([dragToLook,status,dom]),{dom:null,set:{'self.domElement':dom,'self.dragToLook':dragToLook,'self._status':status},hooks:['self.enabled','self.dragToLook','self._status','self._getContainerDimensions','self._updateRotationVector'],steps:[{op:'callback',name:'onPointerMove',event:{pageX:330,pageY:140}}]});
 for(const width of [0,1,640])add('pointermove.width.'+width,{set:{'self.domElement':{$ref:'surface'}},steps:[{op:'metrics',value:{surfaceWidth:width}},{op:'callback',name:'onPointerMove',event:{pageX:100,pageY:100}}]});
 for(const dom of [{$ref:'surface'},{$ref:'document'},'[object Object]',null])add('container.'+JSON.stringify(dom),{set:{'self.domElement':dom},steps:[{op:'call',method:'_getContainerDimensions'}]});
 for(const autoForward of [true,false,1,'yes'])for(const back of [0,1,0.5,'0'])add('movement.'+JSON.stringify([autoForward,back]),{set:{'self.autoForward':autoForward,'self._moveState.back':back,'self._moveState.left':'2','self._moveState.right':'3','self._moveState.down':1,'self._moveState.up':4},steps:[{op:'call',method:'_updateMovementVector'}]});
 for(const delta of [0,.1,1,-1,null,'0.25',{$undefined:true},{$number:'NaN'},{$number:'Infinity'},{$symbol:'delta'},{$coerce:{label:'delta',value:.25}},{$coerce:{label:'delta',value:1,throw:true}}])add('update.'+JSON.stringify(delta),{set:{'self._moveVector.x':1,'self._rotationVector.y':.5},change:{},steps:[{op:'call',method:'update',args:[delta]}]});
 const paths=['object.translateX','object.translateY','object.translateZ','self._moveVector','self._rotationVector','self._lastPosition.distanceToSquared','self._lastQuaternion.dot','self.dispatchEvent','self._lastQuaternion.copy','self._lastPosition.copy'];
 add('update.disabled-before-delta',{set:{'self.enabled':false},hooks:paths,steps:[{op:'call',method:'update',args:[{$symbol:'unreached'}]}]});
 add('shift-multiplier-source-behavior',{steps:[{op:'callback',name:'onKeyDown',event:{code:'ShiftLeft'}},{op:'callback',name:'onKeyDown',event:{code:'KeyW'}},{op:'call',method:'update',args:[1]}]});
 add('autoForward-needs-vector-update',{set:{'self.autoForward':true},steps:[{op:'call',method:'update',args:[1]},{op:'callback',name:'onKeyDown',event:{code:'Unknown'}},{op:'call',method:'update',args:[1]}]});
 add('update.full-order',{hooks:paths,set:{'self._moveVector.x':2,'self._rotationVector.y':1},change:{},steps:[{op:'call',method:'update',args:[.25]}]});
 for(const path of paths)for(const kind of ['get:','call:'])if(!(kind==='call:'&&['_moveVector','_rotationVector'].some(x=>path.endsWith(x))))add('update.throw.'+kind+path,{hooks:paths,throwAt:kind+path,set:{'self._moveVector.x':path==='self._lastQuaternion.dot'?0:2,'self._rotationVector.y':1},change:{},steps:[{op:'call',method:'update',args:[.25]}]});
 add('change.throw-preserves-event-target',{set:{'self._moveVector.x':1},change:{throw:true},steps:[{op:'call',method:'update',args:[1]}]});
 add('change.reentry',{set:{'self._moveVector.x':1},change:{reenter:true,steps:[{op:'set',path:'self._moveVector.y',value:1},{op:'call',method:'update',args:[.5]}]},steps:[{op:'call',method:'update',args:[1]},{op:'call',method:'update',args:[0]}]});
 add('update.get-reentry',{hooks:paths,reenterAt:'get:object.translateY',reentrySteps:[{op:'set',path:'self._moveVector.z',value:3},{op:'call',method:'update',args:[.5]}],set:{'self._moveVector.x':1},steps:[{op:'call',method:'update',args:[1]}]});
 const connectHooks=['window.addEventListener','window.removeEventListener','surface.addEventListener','surface.removeEventListener','surface.style','surface.style.touchAction'];
 for(const method of ['connect','disconnect','dispose'])add('lifecycle.'+method,{dom:{$ref:'surface'},hooks:connectHooks,steps:[{op:'call',method,args:method==='connect'?[{$ref:'document'}]:[]}]});
 for(const path of connectHooks)for(const prefix of path.endsWith('touchAction')?['get:','set:']:path.endsWith('.style')?['get:']:['get:','call:'])add('connect.throw.'+prefix+path,{preHooks:connectHooks,throwAt:prefix+path,dom:{$ref:'surface'}});
 add('host.input-roundtrip',{dom:{$ref:'surface'},steps:[{op:'dispatch',target:'window',type:'keydown',event:{code:'KeyW'}},{op:'dispatch',type:'pointermove',event:{pageX:600,pageY:300}},{op:'call',method:'update',args:[.5]},{op:'dispatch',target:'window',type:'keyup',event:{code:'KeyW'}},{op:'dispatch',type:'contextmenu'},{op:'call',method:'dispose'},{op:'dispatch',target:'window',type:'keydown',event:{code:'KeyS'}}]});
 for(const drag of [true,false])add('pointercancel.'+drag,{set:{'self.dragToLook':drag,'self._status':2,'self._moveState.forward':1,'self._moveState.back':1,'self._moveState.yawLeft':1,'self._moveState.pitchDown':1},steps:[{op:'callback',name:'onPointerCancel'}]});
 for(const value of [{$coerce:{label:'status',value:1}},{$symbol:'status'}])for(const method of ['onPointerDown','onPointerUp'])add('status.coerce.'+method+JSON.stringify(value),{set:{'self.dragToLook':true,'self._status':value},steps:[{op:'callback',name:method,event:{button:0}}]});
 return out;
}
export async function lifecycle(){if(typeof globalThis.gc!=='function')throw Error('--expose-gc required');const observations=[];const gc=async()=>{for(let i=0;i<12;i++){globalThis.gc();await new Promise(r=>setImmediate(r));}};
 for(let round=0;round<3;round++){
  const s={q:{},events:[],outputs:[],marker:{},restores:[],active:false};createHost(s);s.object=new Object3D();let control=new FlyControls(s.object,s.surface),callback=control._onKeyDown;const weak=new WeakRef(control);control.cycle=control;
  control=null;await gc();const aliveWhileConnected=!!weak.deref();await new Promise(r=>setImmediate(r));weak.deref().dispose();await gc();const callbackKeepsReceiver=!!weak.deref();callback=null;await new Promise(r=>setImmediate(r));await gc();const released=!weak.deref();observations.push({round,aliveWhileConnected,callbackKeepsReceiver,released});cleanup(s);
 }
 return {rounds:3,observations};
}
