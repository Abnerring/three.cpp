import assert from 'node:assert/strict';
import {registerHooks} from 'node:module';
import {upstreamModule,upstreamModuleURL} from './threejs-source.mjs';
registerHooks({resolve(specifier,context,next){if(specifier==='three')return {url:upstreamModuleURL('src/Three.js'),shortCircuit:true};return next(specifier,context);}});
const {FlyControls}=await upstreamModule('examples/jsm/controls/FlyControls.js');
const {Object3D}=await upstreamModule('src/core/Object3D.js');
assert.equal(process.version,'v26.5.0');assert.equal(typeof globalThis.gc,'function');
assert(process.argv.length===2||(process.argv.length===3&&process.argv[2]==='--lifetime'));
const gc=async()=>{for(let i=0;i<12;i++){globalThis.gc();await new Promise(resolve=>setImmediate(resolve));}};
// Test-only event targets implement exactly the listener boundary exercised here.
// The actual FlyControls/Object3D are imported from the pinned upstream source.
function target(){const listeners=new Map();return {
 style:{},innerWidth:800,innerHeight:600,offsetWidth:800,offsetHeight:600,offsetLeft:0,offsetTop:0,
 addEventListener(type,callback){let list=listeners.get(type);if(!list)listeners.set(type,list=[]);if(!list.some(entry=>!entry.removed&&entry.callback===callback))list.push({callback,removed:false});},
 removeEventListener(type,callback){for(const entry of listeners.get(type)??[])if(!entry.removed&&entry.callback===callback){entry.removed=true;entry.callback=undefined;break;}},
 dispatch(type,event={}){for(const entry of [...(listeners.get(type)??[])])if(!entry.removed)entry.callback.call(this,event);}
};}
const observations=[];
for(let round=0;round<3;round++){
 const oldWindow=Object.getOwnPropertyDescriptor(globalThis,'window'),oldDocument=Object.getOwnPropertyDescriptor(globalThis,'document');
 const window=target(),document=target(),surface=target();
 Object.defineProperty(globalThis,'window',{value:window,writable:true,configurable:true});Object.defineProperty(globalThis,'document',{value:document,writable:true,configurable:true});
 try{
  let control=new FlyControls(new Object3D(),surface),callback=control._onKeyDown;const weak=new WeakRef(control);
  window.dispatch('keydown',{code:'KeyW',altKey:false});control.update(.25);const zAfterDown=control.object.position.z;
  window.dispatch('keyup',{code:'KeyW'});control.update(.5);const zAfterUp=control.object.position.z;
  control.cycle=control;control=null;await gc();const aliveWhileConnected=!!weak.deref();await new Promise(resolve=>setImmediate(resolve));
  weak.deref().dispose();await gc();const callbackKeepsReceiver=!!weak.deref();callback=null;await new Promise(resolve=>setImmediate(resolve));await gc();const released=!weak.deref();
  assert(aliveWhileConnected&&callbackKeepsReceiver&&released);observations.push({round,zAfterDown,zAfterUp,aliveWhileConnected,callbackKeepsReceiver,released});
 }finally{if(oldWindow)Object.defineProperty(globalThis,'window',oldWindow);else delete globalThis.window;if(oldDocument)Object.defineProperty(globalThis,'document',oldDocument);else delete globalThis.document;}
}
console.log(JSON.stringify({rounds:3,observations}));
