// Runs the pinned upstream modules with a recording GL host. No GPU behavior is
// inferred from this trace: it checks resource and binding callback order only.
import assert from 'node:assert/strict';import fs from 'node:fs';import path from 'node:path';import {execFileSync} from 'node:child_process';import {pathToFileURL} from 'node:url';
if(process.argv.length<3)throw Error('Usage: node instancing-upstream-oracle.mjs <three.js> [--json]');
const upstream=path.resolve(process.argv[2]),lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=async p=>import(pathToFileURL(path.join(upstream,p)));
const {WebGLObjects}=await load('src/renderers/webgl/WebGLObjects.js');const {WebGLBindingStates}=await load('src/renderers/webgl/WebGLBindingStates.js');
const {WebGLBufferRenderer}=await load('src/renderers/webgl/WebGLBufferRenderer.js');const {WebGLIndexedBufferRenderer}=await load('src/renderers/webgl/WebGLIndexedBufferRenderer.js');const {WebGLInfo}=await load('src/renderers/webgl/WebGLInfo.js');const {EventDispatcher}=await load('src/core/EventDispatcher.js');
const encode=v=>typeof v==='number'?(Number.isNaN(v)?'NaN':v===Infinity?'Infinity':v===-Infinity?'-Infinity':Object.is(v,-0)?'-0':v):v===undefined?'undefined':v;
function host(){const calls=[],gl={MAX_VERTEX_ATTRIBS:16,ARRAY_BUFFER:34962,ELEMENT_ARRAY_BUFFER:34963,FLOAT:5126,INT:5124,UNSIGNED_INT:5125,TRIANGLES:4,LINES:1,POINTS:0,LINE_STRIP:3,LINE_LOOP:2,getParameter:()=>16};let vao=0;
 gl.createVertexArray=()=>{const name='vao'+(++vao);calls.push(['createVertexArray',name]);return name;};for(const name of ['bindVertexArray','deleteVertexArray','enableVertexAttribArray','disableVertexAttribArray','vertexAttribDivisor','bindBuffer','vertexAttribPointer','vertexAttribIPointer','vertexAttrib1fv','vertexAttrib2fv','vertexAttrib3fv','vertexAttrib4fv','drawArraysInstanced','drawElementsInstanced'])gl[name]=(...args)=>calls.push([name,...args.map(encode)]);return {gl,calls};}
const attr=(name,itemSize=16)=>({name,itemSize,normalized:false,meshPerAttribute:1,isInstancedBufferAttribute:true,count:3,array:new Float32Array(itemSize*3)});
function bindingCase(mode){const {gl,calls}=host(),buffers=new Map(),trace=[];const attributes={get(a){trace.push(['get',a?.name]);return buffers.get(a);},update(a){trace.push(['update',a?.name]);}};
 const matrix=attr('matrix'),color=attr('color',3),geometryMatrix=attr('geometryMatrix'),index={name:'index'};for(const a of [matrix,color,geometryMatrix,index])buffers.set(a,{buffer:a.name,type:gl.FLOAT,bytesPerElement:4});
 const object={id:1,isInstancedMesh:true,instanceMatrix:matrix,instanceColor:color},material={wireframe:false},geometry={id:2,attributes:{}},program={id:3,getAttributes:()=>({instanceMatrix:{location:1,locationSize:4},instanceColor:{location:5,locationSize:1}})};
 if(mode===1)geometry.attributes.instanceMatrix=geometryMatrix;
 if(mode===2)buffers.delete(matrix);
 if(mode===3)geometry.attributes.instanceMatrix=null;
 const states=WebGLBindingStates(gl,attributes);let error='';try{states.setup(object,material,program,geometry,null);}catch(e){error=e.name;}
 const first={calls:[...calls],trace:[...trace],error};calls.length=trace.length=0;
 matrix.meshPerAttribute=7;matrix.itemSize=12;matrix.normalized=true;matrix.gpuType=1013;color.meshPerAttribute=3;
 if(mode===4)geometry.attributes.instanceColor=attr('newColor',3);
 if(mode===5)states.reset();
 if(mode===6)object.instanceMatrix=attr('replacement');
 try{states.setup(object,material,program,geometry,mode===7?index:null);}catch(e){error=e.name;}
 return {mode,first,second:{calls,trace,error}};
}
function objectsCase(mode){const {gl}=host(),trace=[],info={render:{frame:0}},matrix=attr('matrix'),replacement=attr('replacement'),color=attr('color',3),geometry={name:'geometry'},object=new EventDispatcher();Object.assign(object,{id:1,isInstancedMesh:true,geometry,instanceMatrix:matrix,instanceColor:color});let once=false;
 const attributes={update(a){trace.push(['attribute',a?.name]);if(a===matrix&&!once){once=true;if(mode===1)object.instanceColor=replacement;if(mode===2)object.instanceMatrix=replacement;if(mode===3)object.instanceColor=null;if(mode===4)throw TypeError('upload');if(mode===5)object.dispatchEvent({type:'dispose'});}},remove(a){trace.push(['remove',a?.name]);}};
 const objects=WebGLObjects(gl,{get(o,g){trace.push(['getGeometry',g.name]);return g;},update(g){trace.push(['geometry',g.name]);}},attributes,{releaseStatesOfObject(o){trace.push(['release',o.id]);}},info);
 const run=()=>{try{objects.update(object);}catch(e){trace.push(['error',e.name]);}};run();run();object.instanceMatrix=replacement;object.dispatchEvent({type:'dispose'});run();info.render.frame++;run();
 return {mode,trace};
}
function renderCase(indexed,primcount,count=7.5){const {gl,calls}=host(),info=WebGLInfo(gl),renderer=indexed?new WebGLIndexedBufferRenderer(gl,{},info):new WebGLBufferRenderer(gl,{},info);renderer.setMode(gl.TRIANGLES);if(indexed)renderer.setIndex({type:5123,bytesPerElement:2});renderer.renderInstances(1.5,count,primcount);return {indexed,primcount:encode(primcount),calls,info:{calls:info.render.calls,triangles:encode(info.render.triangles)}};}
function conversionCase(clear){const {gl,calls}=host(),trace=[],matrix=attr('matrix'),oldColor=attr('oldColor',3),newColor=attr('newColor',3),buffers=new Map();for(const a of [matrix,oldColor,newColor])buffers.set(a,{buffer:a.name,type:gl.FLOAT,bytesPerElement:4});
 const object={id:1,isInstancedMesh:true,instanceMatrix:matrix,instanceColor:oldColor},geometry={id:2,attributes:{}},program={id:3,getAttributes:()=>({instanceMatrix:{location:1,locationSize:4},instanceColor:{location:5,locationSize:1}})};let conversion=0;
 matrix.meshPerAttribute={valueOf(){trace.push(['convert',++conversion]);object.instanceColor=clear?null:newColor;return conversion;}};
 // Model WebIDL conversion explicitly at the host boundary, so getters run at
 // the same place as a real vertexAttribDivisor call.
 gl.vertexAttribDivisor=(location,value)=>calls.push(['vertexAttribDivisor',location,Number(value)>>>0]);
 const states=WebGLBindingStates(gl,{get(a){trace.push(['get',a.name]);return buffers.get(a);},update(){}});states.setup(object,{wireframe:false},program,geometry,null);
 return {clear,calls,trace};}
const bindings=Array.from({length:8},(_,i)=>bindingCase(i)),objects=Array.from({length:6},(_,i)=>objectsCase(i));const renders=[];for(const indexed of [false,true])for(const count of [0,-0,'0',undefined,NaN,-1,2.5])renders.push(renderCase(indexed,count));
assert.equal(bindings[0].second.calls.length,0,'same identity metadata changes do not rebind');assert.equal(bindings[0].second.trace.length,0,'same identity metadata does not fetch buffers');
assert(bindings[1].first.calls.some(c=>c[0]==='bindBuffer'&&c[2]==='geometryMatrix'),'geometry attribute takes precedence');
assert(!bindings[2].first.calls.some(c=>c[0]==='vertexAttribPointer'&&c[1]>=1&&c[1]<=4),'missing matrix GPU cache skips its pointers');
assert.equal(bindings[3].first.error,'TypeError','null geometry override is not undefined fallback');
assert(bindings[6].second.calls.some(c=>c[0]==='disableVertexAttribArray'&&c[1]===1),'uncached replacement disables old matrix location');
assert.equal(renders[0].info.calls,0);assert.equal(renders[2].info.calls,1);assert.equal(renders[6].info.triangles,6.25);
assert.equal(objects[1].trace.filter(t=>t[0]==='attribute'&&t[1]==='replacement').length,3,'matrix upload callback changes subsequent color read');
const conversions=[conversionCase(false),conversionCase(true)];assert.deepEqual(conversions[0].trace,[['get','matrix'],['convert',1],['convert',2],['convert',3],['convert',4],['get','newColor']]);assert.deepEqual(conversions[1].trace,[['get','matrix'],['convert',1],['convert',2],['convert',3],['convert',4]]);assert(!conversions[1].calls.some(c=>c[0]==='vertexAttribPointer'&&c[1]===5));
const result={commit:lock.commit,bindings,objects,renders,conversions};if(process.argv.includes('--json'))console.log(JSON.stringify(result,null,2));else console.log(`Instancing upstream oracle: ${bindings.length} binding / ${objects.length} lifecycle / ${renders.length} count / ${conversions.length} conversion scenarios passed`);
