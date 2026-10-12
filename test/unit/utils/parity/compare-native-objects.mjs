import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if (process.argv.length !== 4) throw Error('Usage: node compare-native-objects.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const T=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const result={};
const root=new T.Object3D();root.name='root';const mesh=new T.Mesh();mesh.name='mesh';root.add(mesh);
result.wrapperIdentity=root===root;result.componentIdentity=mesh.position===mesh.position;
result.methodIdentity=mesh.position.toArray===root.position.toArray;result.ownMethod=Object.hasOwn(mesh.position,'toArray');
const buffer=new Float64Array([9,1,2,3,9]);const position=new T.PropertyBinding(root,'mesh.position');position.setValue(buffer,1);
result.position=mesh.position.toArray();result.positionDirty=mesh.matrixWorldNeedsUpdate;
const output=new Float64Array(5);position.getValue(output,1);result.positionRead=Array.from(output);
result.typedIdentity=buffer===buffer;result.typedSubarrayIdentity=buffer===buffer.subarray();result.typedRoundtrip=true;
const positionObject=mesh.position,events=[],input=Array(3);
for(let i=0;i<3;i++)Object.defineProperty(input,String(i),{enumerable:true,configurable:true,get(){events.push([i,positionObject.x,positionObject.y,positionObject.z]);return 4+i;}});
const returned=positionObject.fromArray(input);result.fromArrayEvents=events;result.fromArrayIdentity=returned===positionObject;result.fromArrayPosition=mesh.position.toArray();
Object.defineProperty(positionObject,'x',{configurable:true,enumerable:true,get(){return 17;}});
const plain=[];const arrayReturn=positionObject.toArray(plain,1);result.redefinedComponent=plain;result.toArrayIdentity=arrayReturn===plain;
const q=mesh.quaternion,qevents=[],qinput=Array(4);
for(let i=0;i<4;i++)Object.defineProperty(qinput,String(i),{enumerable:true,configurable:true,get(){qevents.push([i,q._x,q._y,q._z,q._w]);return i===3?1:.1*(i+1);}});
q.fromArray(qinput);result.quaternionEvents=qevents;result.quaternion=mesh.quaternion.toArray();
const material=mesh.material,opacity=new T.PropertyBinding(root,'mesh.material.opacity');opacity.setValue(new Float64Array([.25]),0);
mesh.material=new T.MeshBasicMaterial();opacity.setValue(new Float64Array([.75]),0);
result.oldMaterialOpacity=material.opacity;result.newMaterialOpacity=mesh.material.opacity;result.materialVersion=material.version;result.oldMaterialIdentity=opacity.targetObject===material;
mesh.geometry.morphAttributes.position=[new T.Float32BufferAttribute([0,0,0],3)];mesh.geometry.morphAttributes.position[0].name='smile';mesh.updateMorphTargets();
const morph=mesh.morphTargetInfluences,morphBinding=new T.PropertyBinding(root,'mesh.morphTargetInfluences[smile]');morphBinding.setValue(new Float64Array([.4]),0);
result.morphValue=mesh.morphTargetInfluences[0];result.morphIdentity=morph===mesh.morphTargetInfluences;
const oldMorph=mesh.morphTargetInfluences;mesh.updateMorphTargets();morphBinding.setValue(new Float64Array([.8]),0);
result.oldMorphValue=oldMorph[0];result.newMorphValue=mesh.morphTargetInfluences[0];
const children=root.children,extra=new T.Object3D();extra.name='extra';root.add(extra);
result.childrenIdentity=children===root.children;result.childrenGrowth=children.length;result.findAddedChild=T.PropertyBinding.findNode(root,'extra')===extra;
mesh.customWeight=2;const dynamic=new T.PropertyBinding(root,'mesh.customWeight');dynamic.setValue(new Float64Array([7]),0);result.dynamicProperty=mesh.customWeight;
let conversions=0;mesh.morphTargetInfluences.length={valueOf(){return conversions++===0?4294967298:2;}};result.lengthConversion=mesh.morphTargetInfluences.length;
mesh.material.map=new T.Texture();const textureOffset=new T.PropertyBinding(root,'mesh.map.offset');textureOffset.setValue(new Float64Array([.25,.75]),0);
result.textureOffset=mesh.material.map.offset.toArray();result.textureIdentity=mesh.material.map===mesh.material.map;
const typed=new Float64Array([1]);result.typedSet=[];
for(const key of ['0','-1','-0','NaN','2','0.5']) {
 const calls=[],value={valueOf(){calls.push('convert');return 7;}};typed[key]=value;
 const item={key,calls,value:typed[0]},receiver={},receiverCalls=[],receiverValue={valueOf(){receiverCalls.push('convert');return 9;}};
 item.reflect=Reflect.set(typed,key,receiverValue,receiver);item.receiverOwn=Object.hasOwn(receiver,key);item.receiverIdentity=receiver[key]===receiverValue;item.receiverCalls=receiverCalls;result.typedSet.push(item);
}
try {typed[-1]={valueOf(){throw TypeError('conversion')}};result.typedInvalidThrows=false;}catch {result.typedInvalidThrows=true;}
result.typedOwnLength=Object.hasOwn(typed,'length');Object.defineProperty(typed,'length',{value:7,configurable:true});result.typedShadowLength=typed.length;delete typed.length;result.typedRestoredLength=typed.length;
const math=new T.Vector3(1,2,3),offsetEvents=[],offsetValue={toString(){offsetEvents.push('string');math.x=99;return 'slot';},valueOf(){offsetEvents.push('number');return 0;}};
const destination=[];math.toArray(destination,offsetValue);result.computedFirstValue=destination.slot;result.computedEvents=offsetEvents;
const euler=new T.Euler(),eulerInput=[1,2,3],eulerEvents=[];Object.defineProperty(eulerInput,'3',{get(){eulerEvents.push('order');return eulerEvents.length===1?'XYZ':'ZYX';}});
euler.fromArray(eulerInput,{valueOf(){throw TypeError('offset')}});result.eulerFromArray=euler.toArray();result.eulerOrderReads=eulerEvents;
for(const world of [false,true]) {
 const parent=new T.Object3D(),first=new T.Object3D(),second=new T.Object3D(),events=[];parent.children=[first,second];
 first.updateMatrixWorld=first.updateWorldMatrix=()=>{events.push('first');parent.children=[];};second.updateMatrixWorld=second.updateWorldMatrix=()=>events.push('second');
 if(world)parent.updateWorldMatrix(false,true);else parent.updateMatrixWorld();result[world?'worldChildrenSnapshot':'matrixChildrenSnapshot']=events;
}
const ancestorRoot=new T.Object3D(),ancestorParent=new T.Object3D(),ancestorChild=new T.Object3D();ancestorRoot.name='grand';ancestorParent.name='parent';ancestorRoot.add(ancestorParent);ancestorParent.add(ancestorChild);
const ancestors=[];ancestorChild.traverseAncestors(visited=>{ancestors.push(visited.name);ancestorChild.parent=null;});result.ancestorSnapshot=ancestors;
const copySource=new T.Object3D(),copyTarget=new T.Object3D(),firstClone=new T.Object3D(),secondClone=new T.Object3D();firstClone.name='first';secondClone.name='second';copySource.children=[firstClone];
firstClone.clone=()=>{copySource.children.push(secondClone);const copy=new T.Object3D();copy.name='first';return copy;};copyTarget.copy(copySource);result.copyDynamicLength=copyTarget.children.map(child=>child.name);
const removeParent=new T.Object3D(),removeFirst=new T.Object3D(),removeSecond=new T.Object3D();removeParent.add(removeFirst,removeSecond);Object.defineProperty(removeParent.children,'length',{writable:false});
const removeResult=[];try{removeParent.remove(removeFirst);removeResult.push(false);}catch{removeResult.push(true);}removeResult.push(removeFirst.parent===null,removeParent.children.length,removeParent.children[0]===removeSecond,Object.hasOwn(removeParent.children,'1'));result.removePartialWrites=removeResult;
const expected=JSON.parse(JSON.stringify(result));
const actual=JSON.parse(execFileSync(path.resolve(process.argv[3]),[],{encoding:'utf8'}));
let comparisons=0;
function compare(a,b,where='$') { comparisons++;
 if(typeof b==='number')assert.ok(typeof a==='number'&&Math.abs(a-b)<=1e-13*Math.max(1,Math.abs(b)),`${where}: ${a} != ${b}`);
 else if(b&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),`${where}: keys`);for(const k of Object.keys(b))compare(a[k],b[k],`${where}.${k}`);}
 else assert.equal(a,b,where);
}
compare(actual,expected);console.log(`Native object parity: ${Object.keys(expected).length} scenarios / ${comparisons} comparisons passed`);
