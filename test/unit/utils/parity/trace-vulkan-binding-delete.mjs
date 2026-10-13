import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
assert.equal(process.argv.length,3,'Usage: node trace-vulkan-binding-delete.mjs <three.js checkout>');
const checkout=resolve(process.argv[2]);
const revision=spawnSync('git',['-C',checkout,'rev-parse','HEAD'],{encoding:'utf8'});
assert.equal(revision.status,0,revision.stderr);
assert.equal(revision.stdout.trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const source=relative=>import(pathToFileURL(resolve(checkout,relative)).href);
const {WebGLBindingStates}=await source('src/renderers/webgl/WebGLBindingStates.js');
const {WebGLGeometries}=await source('src/renderers/webgl/WebGLGeometries.js');
const {WebGLAttributes}=await source('src/renderers/webgl/WebGLAttributes.js');
const {BufferGeometry}=await source('src/core/BufferGeometry.js');
const {Float32BufferAttribute,Uint16BufferAttribute}=await source('src/core/BufferAttribute.js');
for(const dispose of ['A','B']) {
 let current=null,next=0;const commands=[];
 const gl={MAX_VERTEX_ATTRIBS:34921,ARRAY_BUFFER:34962,ELEMENT_ARRAY_BUFFER:34963,FLOAT:5126,INT:5124,UNSIGNED_INT:5125,
  getParameter(){return 16},createBuffer(){return {buffer:++next}},createVertexArray(){return {vao:++next}},
  bindVertexArray(v){current=v;commands.push(['bindVAO',v?.vao??null])},
  deleteVertexArray(v){commands.push(['deleteVAO',v.vao,'current',current?.vao??null]);if(current===v)current=null},
  deleteBuffer(b){commands.push(['deleteBuffer',b.buffer,'current',current?.vao??null])},
  bindBuffer(t,b){commands.push(['bindBuffer',t,b?.buffer??null,'current',current?.vao??null])},
  bufferData(){},bufferSubData(){},enableVertexAttribArray(){},disableVertexAttribArray(){},vertexAttribDivisor(){},
  vertexAttribPointer(...args){commands.push(['pointer',...args,'current',current?.vao??null])},vertexAttribIPointer(){}};
 const attributes=WebGLAttributes(gl),bindings=WebGLBindingStates(gl,attributes),info={memory:{geometries:0}},geometries=WebGLGeometries(gl,attributes,info,bindings);
 const p=new Float32BufferAttribute([0,0,0,1,0,0,0,1,0],3),c=new Float32BufferAttribute([0,1,0,0,1,0,0,1,0],3),i=new Uint16BufferAttribute([0,1,2],1);
 const A=new BufferGeometry().setAttribute('position',p).setAttribute('color',c).setIndex(i),B=new BufferGeometry().setAttribute('position',p).setAttribute('color',c).setIndex(i);
 const program={id:1,getAttributes(){return {position:{location:0,locationSize:1},color:{location:1,locationSize:1}}}},object={id:1},material={wireframe:false};
 const vaos={};for(const [name,g] of [['A',A],['B',B]]){geometries.get(object,g);geometries.update(g);bindings.setup(object,material,program,g,i);vaos[name]=current.vao;}
 bindings.resetDefaultState();assert.equal(current.vao,vaos.B);commands.length=0;
 (dispose==='A'?A:B).dispose();const disposal=[...commands];assert.equal(disposal.filter(x=>x[0]==='deleteBuffer').length,3);
 for(const command of disposal.filter(x=>x[0]==='deleteBuffer'))assert.equal(command[3],vaos.B);
 commands.length=0;const surviving=dispose==='A'?B:A;geometries.update(surviving);bindings.setup(object,material,program,surviving,i);
 assert.equal(commands.filter(x=>x[0]==='pointer').length,0);
 console.log(JSON.stringify({dispose,vaos,disposal,nextSetup:commands,memory:info.memory.geometries}));
 bindings.dispose();geometries.dispose();
}
console.log('2 pinned source command-order scenarios passed. This is a recording adapter, not a WebGL driver pixel oracle.');
