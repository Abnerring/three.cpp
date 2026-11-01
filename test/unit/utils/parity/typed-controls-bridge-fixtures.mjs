import {fixtures as flyFixtures,run as flyRun,lifecycle} from './fly-controls-fixtures.mjs';
import {upstreamModule} from './threejs-source.mjs';
const {Object3D,Vector3,Quaternion,PerspectiveCamera,Mesh,InstancedMesh}=await upstreamModule('src/Three.js');
const {FlyControls}=await upstreamModule('examples/jsm/controls/FlyControls.js');
export {lifecycle};
const methods=[['object','translateOnAxis',Object3D],['object','translateX',Object3D],['object','translateY',Object3D],['object','translateZ',Object3D],['position','add',Vector3],['quaternion','multiply',Quaternion],['quaternion','multiplyQuaternions',Quaternion]];
const number=x=>Number.isNaN(x)?'#NaN':x===Infinity?'#Infinity':x===-Infinity?'#-Infinity':Object.is(x,-0)?'#-0':x;
const array=a=>a.map(number);
const normal=x=>x===undefined?'#undefined':typeof x==='number'?number(x):x;
export function fixtures(){const extra=[{mode:'metadata'},{mode:'motion',position:[1,2,3],quaternion:[.1,.2,.3,.9]},{mode:'motion',position:[-3,1,0],quaternion:[0,Math.SQRT1_2,0,Math.SQRT1_2]},...['get:quaternion','get:position','get:add','call:add','callback'].map(throwAt=>({mode:'protocol',throwAt})),{mode:'protocol'},{mode:'protocol',reenterAt:'get:add'},{mode:'protocol',reenterAt:'call:add'},...['translateX','translateY','translateZ'].map(method=>({mode:'delegate',method})),...[0,.25,1,-1].map(delta=>({mode:'fly',delta})),{mode:'protocol',coerce:true},{mode:'protocol',coerce:true,reenterAt:'coerce:number'},...['PerspectiveCamera','Mesh','InstancedMesh'].map(ownerType=>({mode:'fly',delta:.25,ownerType}))];return [...flyFixtures(),...extra.map((q,i)=>({name:'typed.bridge.'+i,kind:'bridge',...q}))];}
export function run(q){if(q.kind!=='bridge')return flyRun(q);const Type={Object3D,PerspectiveCamera,Mesh,InstancedMesh}[q.ownerType??'Object3D'];const object=new Type(),position=object.position,quaternion=object.quaternion,events=[],marker={};
 const sourceObjects={object,position,quaternion};
 if(q.mode==='metadata')return methods.map(([target,key,Type])=>{let prototype=sourceObjects[target];while(!Object.hasOwn(prototype,key))prototype=Object.getPrototypeOf(prototype);const d=Object.getOwnPropertyDescriptor(prototype,key),fn=sourceObjects[target][key];return {target,key,sameSource:fn===Type.prototype[key],name:fn.name,length:fn.length,source:Function.prototype.toString.call(fn),constructible:!!Object.getOwnPropertyDescriptor(fn,'prototype'),writable:d.writable,enumerable:d.enumerable,configurable:d.configurable};});
 const positionDescriptor=Object.getOwnPropertyDescriptor(object,'position'),quaternionDescriptor=Object.getOwnPropertyDescriptor(object,'quaternion');
 let error='#undefined',returned;
 try{
  if(q.mode==='motion'){position.set(...q.position);quaternion.set(...q.quaternion);object.translateX(2);object.translateY(-1);returned=object.translateZ(.25);position.x+=.5;quaternion.y+=.125;}
  else if(q.mode==='fly'){position.set(1,2,3);quaternion.set(0,Math.SQRT1_2,0,Math.SQRT1_2);const c=new FlyControls(object);c._moveVector.set(.5,-.25,1);c._rotationVector.set(.1,.2,-.3);c.movementSpeed=2;c.rollSpeed=.5;returned=c.update(q.delta);}
  else if(q.mode==='delegate'){Object.defineProperty(object,'translateOnAxis',{get(){events.push('get:translateOnAxis');return function(axis,distance){events.push(['call:translateOnAxis',this===object,array(axis.toArray()),distance===marker]);return marker;};}});returned=object[q.method](marker);}
  else if(q.mode==='protocol'){
   let entered=false;const signal=label=>{events.push(label);if(q.throwAt===label)throw marker;if(q.reenterAt===label&&!entered){entered=true;events.push('reentry:begin');object.translateZ(.25);events.push('reentry:end');}};
   Object.defineProperty(object,'quaternion',{configurable:true,get(){signal('get:quaternion');return quaternion;}});Object.defineProperty(object,'position',{configurable:true,get(){signal('get:position');return position;}});
   const add=position.add;Object.defineProperty(position,'add',{configurable:true,get(){signal('get:add');return function(...args){signal('call:add');return add.apply(this,args);};}});
   const callback=quaternion._onChangeCallback;quaternion._onChangeCallback=function(){signal('callback');return callback.call(this);};
   const delta=q.coerce?{[Symbol.toPrimitive](hint){signal('coerce:'+hint);return .5;}}:.5;
   returned=object.translateOnAxis(new Vector3(1,2,3),delta);
   if(!q.coerce)quaternion.multiply(new Quaternion(.1,.2,.3,.9));
  }else throw Error('Unknown mode');
 }catch(e){error=e===marker?'#marker':'#'+e.name;}
 // Restore the original descriptors before observing aliases, including throws.
 Object.defineProperty(object,'position',positionDescriptor);Object.defineProperty(object,'quaternion',quaternionDescriptor);
 object.updateMatrix();
 const observed={position:array(position.toArray()),quaternion:array(quaternion.toArray()),rotation:array(object.rotation.toArray()),positionAlias:object.position===position,quaternionAlias:object.quaternion===quaternion,objectAlias:object===sourceObjects.object,matrix:array(object.matrix.elements),matrixWorldNeedsUpdate:object.matrixWorldNeedsUpdate};
 return {events,error,returned:returned===object?'#object':returned===marker?'#marker':normal(returned),backing:observed};
}
