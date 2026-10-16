import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]);
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const {WebGLProgram}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLProgram.js')));
if(!global.gc)throw new Error('Run with node --expose-gc');
let detached,instance;const weak=[];
function create(){
 const renderer={},binding={},gl={},handle={},vertex={},fragment={};renderer.debug={checkShaderErrors:false};renderer.getContext=()=>gl;
 binding.releaseStatesOfProgram=()=>{};
 Object.assign(gl,{VERTEX_SHADER:1,FRAGMENT_SHADER:2,ACTIVE_UNIFORMS:3,ACTIVE_ATTRIBUTES:4});
 gl.createProgram=()=>handle;gl.createShader=type=>type===1?vertex:fragment;
 for(const name of ['shaderSource','compileShader','attachShader','linkProgram','deleteShader','deleteProgram'])gl[name]=()=>{};
 gl.getProgramParameter=()=>0;
 const parameters={isRawShaderMaterial:true,vertexShader:'void main(){}',fragmentShader:'void main(){}',defines:{},shaderType:'Fixture',shaderName:'fixture',rendererExtensionParallelShaderCompile:false};
 const program=new WebGLProgram(renderer,{},parameters,binding);
 for(const item of [renderer,binding,gl,handle,vertex,fragment,program,parameters])weak.push(new WeakRef(item));
 detached=program.isReady;
}
create();
async function snapshot(){for(let i=0;i<8;i++){await new Promise(r=>setImmediate(r));gc();}return weak.map(w=>w.deref()!==undefined);}
const expected=[];expected.push(await snapshot());instance=new detached();detached=undefined;expected.push(await snapshot());instance=undefined;expected.push(await snapshot());
if(!process.argv[3])console.log(JSON.stringify(expected));
else{
 const actual=execFileSync(path.resolve(process.argv[3]),[],{encoding:'utf8'}).trim().split(/\r?\n/).map(JSON.parse);
 assert.deepEqual(actual,expected);console.log('WebGLProgram actual Node GC: 3 snapshots / 24 reachability checks passed.');
}
