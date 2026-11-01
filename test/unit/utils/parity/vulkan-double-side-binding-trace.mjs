import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';
const args=Object.fromEntries(process.argv.slice(2).reduce((all,x,i,a)=>i%2?all:[...all,[x,a[i+1]]],[]));
assert.ok(args['--upstream']&&args['--input']);
const root=resolve(args['--upstream']);
const load=f=>import(pathToFileURL(resolve(root,'src',f)).href);
const [{parse},{WebGLAttributes},{WebGLBindingStates},{WebGLGeometries},{WebGLObjects},{WebGLIndexedBufferRenderer},{WebGLPrograms},{WebGLLights},{MeshBasicMaterial},{PlaneGeometry},{Mesh},{Scene},{OrthographicCamera},constants]=await Promise.all([
 import(pathToFileURL(resolve(root,'node_modules/acorn/dist/acorn.mjs')).href),
 load('renderers/webgl/WebGLAttributes.js'),load('renderers/webgl/WebGLBindingStates.js'),
 load('renderers/webgl/WebGLGeometries.js'),load('renderers/webgl/WebGLObjects.js'),
 load('renderers/webgl/WebGLIndexedBufferRenderer.js'),load('renderers/webgl/WebGLPrograms.js'),load('renderers/webgl/WebGLLights.js'),load('materials/MeshBasicMaterial.js'),
 load('geometries/PlaneGeometry.js'),load('objects/Mesh.js'),load('scenes/Scene.js'),
 load('cameras/OrthographicCamera.js'),load('constants.js')]);
const source=readFileSync(resolve(root,'src/renderers/WebGLRenderer.js'),'utf8');
const ast=parse(source,{ecmaVersion:'latest',sourceType:'module',locations:true});
const all=[];
function walk(n,visitor){if(!n||typeof n!=='object')return;visitor(n);for(const [k,v]of Object.entries(n))if(k!=='loc'){if(Array.isArray(v))v.forEach(x=>walk(x,visitor));else if(v&&typeof v==='object')walk(v,visitor);}}
walk(ast,n=>all.push(n));
const declaration=name=>{const a=all.filter(n=>n.type==='FunctionDeclaration'&&n.id?.name===name);assert.equal(a.length,1);return a[0];};
const assignment=name=>{const a=all.filter(n=>n.type==='AssignmentExpression'&&n.left.type==='MemberExpression'&&n.left.object.type==='ThisExpression'&&n.left.property.name===name);assert.equal(a.length,1);return a[0].right;};
const objectNode=declaration('renderObject'),directNode=assignment('renderBufferDirect'),renderNode=assignment('render');
const slice=n=>source.slice(n.start,n.end);
const bindingCalls=[];let renderTry=0;
function ownWalk(n,visitor,first=true){if(!n||typeof n!=='object')return;if(!first&&/Function/.test(n.type))return;visitor(n);for(const[k,v]of Object.entries(n))if(k!=='loc'){if(Array.isArray(v))v.forEach(x=>ownWalk(x,visitor,false));else if(v&&typeof v==='object')ownWalk(v,visitor,false);}}
ownWalk(renderNode,n=>{if(n.type==='TryStatement')++renderTry;if(n.type==='CallExpression'&&n.callee.type==='MemberExpression'&&n.callee.object.name==='bindingStates')bindingCalls.push({method:n.callee.property.name,line:n.loc.start.line});});
assert.deepEqual(bindingCalls.map(x=>x.method),['resetDefaultState']);assert.equal(renderTry,0);
const defaultResetNode=all.find(n=>n.type==='ExpressionStatement'&&n.expression.type==='CallExpression'&&n.expression.callee.type==='MemberExpression'&&n.expression.callee.object.name==='bindingStates'&&n.expression.callee.property.name==='resetDefaultState');
assert.ok(defaultResetNode);
const resetTail=new Function('bindingStates',slice(defaultResetNode));
const programSource=readFileSync(resolve(root,'src/renderers/webgl/WebGLPrograms.js'),'utf8');
assert.ok(programSource.includes('doubleSided: material.side === DoubleSide'));
assert.ok(programSource.includes('flipSided: material.side === BackSide'));
// Shader compilation/linking is a command-host boundary, not a simulated GPU.
// Both source program-key flags distinguish BackSide from FrontSide. Stable
// descriptors below expose only the actual plain MeshBasic position input.
const results=JSON.parse(readFileSync(args['--input'],'utf8')).map(request=>{
 const material=new MeshBasicMaterial({color:0xff0000,transparent:request.transparent,side:request.side,forceSinglePass:request.forceSinglePass});
 const geometry=new PlaneGeometry(1.6,1.6),object=new Mesh(geometry,material),scene=new Scene(),camera=new OrthographicCamera(-1,1,1,-1,.1,20);
 if(request.backFacing)object.rotation.y=Math.PI;scene.add(object);camera.position.z=5;scene.updateMatrixWorld();camera.updateMatrixWorld();
 const uploads=[],draws=[],frames=[],commands=[];let frame=0,nextBuffer=0,nextVao=0,current=null,arrayBuffer=null,secondUploads=0,callCount=0;
 const blank=id=>({id,index:null,attributes:Array.from({length:16},()=>({enabled:false,buffer:null,divisor:0}))});
 const defaultVao=blank(0),vaos=new Map([[0,defaultVao]]);current=defaultVao;
 const buffers=new Map(),record=(op,extra={})=>commands.push({frame,op,vao:current.id,...extra});
 const gl={MAX_VERTEX_ATTRIBS:0x8869,ARRAY_BUFFER:0x8892,ELEMENT_ARRAY_BUFFER:0x8893,FLOAT:0x1406,UNSIGNED_SHORT:0x1403,UNSIGNED_INT:0x1405,TRIANGLES:4,
  getParameter(p){assert.equal(p,this.MAX_VERTEX_ATTRIBS);return 16;},
  createVertexArray(){const vao=blank(++nextVao);vaos.set(vao.id,vao);record('createVertexArray',{created:vao.id});return vao;},
  bindVertexArray(vao){current=vao??defaultVao;record('bindVertexArray');},deleteVertexArray(vao){vaos.delete(vao.id);},
  createBuffer(){const b={id:++nextBuffer};buffers.set(b.id,null);record('createBuffer',{buffer:b.id});return b;},
  bindBuffer(target,b){if(target===this.ARRAY_BUFFER)arrayBuffer=b;else{assert.equal(target,this.ELEMENT_ARRAY_BUFFER);current.index=b;}record('bindBuffer',{target,buffer:b?.id??null});},
  bufferData(target,data){const b=target===this.ARRAY_BUFFER?arrayBuffer:current.index;assert.ok(b);buffers.set(b.id,Array.from(data));record('bufferData',{target,buffer:b.id});},
  bufferSubData(){throw new Error('Unexpected version update in this fixed trace');},deleteBuffer(b){buffers.delete(b.id);},
  enableVertexAttribArray(i){current.attributes[i].enabled=true;record('enableVertexAttribArray',{location:i});},
  disableVertexAttribArray(i){current.attributes[i].enabled=false;record('disableVertexAttribArray',{location:i});},
  vertexAttribPointer(i,size,type,normalized,stride,offset){assert.ok(arrayBuffer);Object.assign(current.attributes[i],{buffer:arrayBuffer,size,type,normalized,stride,offset});record('vertexAttribPointer',{location:i,buffer:arrayBuffer.id});},
  vertexAttribIPointer(){throw new Error('Unexpected integer position');},vertexAttribDivisor(i,divisor){current.attributes[i].divisor=divisor;},
  drawElements(mode,count,type,offset){assert.equal(mode,4);const p=current.attributes[0];draws.push({frame,side:material.side,vao:current.id,count,type,offset,positionEnabled:p.enabled,positionBuffer:p.buffer?.id??null,indexBuffer:current.index?.id??null,indexData:buffers.get(current.index?.id)??null,constantPositionWhenDisabled:[0,0,0,1]});record('drawElements',{positionEnabled:p.enabled,positionBuffer:p.buffer?.id??null,indexBuffer:current.index?.id??null});}
 };
 const attributes=WebGLAttributes(gl),bindingStates=WebGLBindingStates(gl,attributes);
 const info={memory:{geometries:0},render:{frame:0},update(){++callCount;}};
 const geometries=WebGLGeometries(gl,attributes,info,bindingStates),objects=WebGLObjects(gl,geometries,attributes,bindingStates,info);
 const extensions={has(){return false;}},indexed=new WebGLIndexedBufferRenderer(gl,extensions,info);
 const lights=WebGLLights(extensions);lights.setup([]);
 const rendererConfiguration={getRenderTarget(){return null;},state:{buffers:{depth:{getReversed(){return false;}}}},outputColorSpace:constants.LinearSRGBColorSpace,toneMapping:constants.NoToneMapping,shadowMap:{enabled:false,type:constants.PCFShadowMap}};
 const sourceProgramCache=WebGLPrograms(rendererConfiguration,{get(){return null;}},extensions,{precision:'highp',logarithmicDepthBuffer:false},bindingStates,{numPlanes:0,numIntersection:0});
 const programs=new Map(),programSelections=[];
 const setProgram=()=>{const parameters=sourceProgramCache.getParameters(material,lights.state,[],scene,object,[]),key=sourceProgramCache.getProgramCacheKey(parameters);if(!programs.has(key))programs.set(key,{id:programs.size+1,getAttributes(){return {position:{location:0,locationSize:1}};}});programSelections.push({frame,side:material.side,doubleSided:parameters.doubleSided,flipSided:parameters.flipSided,key,id:programs.get(key).id});record('setProgram',{key,id:programs.get(key).id});return programs.get(key);};
 const direct=new Function('_emptyScene','setProgram','state','geometries','bindingStates','bufferRenderer','indexedBufferRenderer','attributes','_gl','getTargetPixelRatio','extensions','properties',`return (${slice(directNode)});`)(scene,setProgram,{setMaterial(){}},geometries,bindingStates,{},indexed,attributes,gl,()=>1,{},{});
 const host={renderBufferDirect:direct};
 const renderObject=new Function('_nodesHandler','_this','BackSide','FrontSide','DoubleSide',`return (${slice(objectNode)});`)(null,host,constants.BackSide,constants.FrontSide,constants.DoubleSide);
 const reason={originalUploadFailure:true},first=geometry.index,second=request.swapIndex?first.clone():null;
 if(second)for(let i=0;i<second.array.length;i+=3)[second.array[i+1],second.array[i+2]]=[second.array[i+2],second.array[i+1]];
 const observeUpload=which=>{uploads.push({frame,which,side:material.side,vao:current.id,positionEnabled:current.attributes[0].enabled,positionBuffer:current.attributes[0].buffer?.id??null,indexBuffer:current.index?.id??null});record(`upload-${which}`);};
 first.onUpload(()=>{observeUpload('first');if(request.swapIndex&&uploads.length===1){geometry.setIndex(second);material.color.setRGB(0,0,1);}if(request.throwFirst&&uploads.length===1)throw reason;});
 if(second)second.onUpload(()=>{++secondUploads;observeUpload('second');if(request.throwSecond&&secondUploads===1)throw reason;});
 for(frame=0;frame<2;++frame){info.render.frame=frame;callCount=0;let errorSame=false;const drawBegin=draws.length;objects.update(object);try{renderObject(object,scene,camera,geometry,material,null);resetTail(bindingStates);}catch(e){assert.equal(e,reason);errorSame=true;}frames.push({frame,errorSame,calls:callCount,side:material.side,positionEnabled:current.attributes[0].enabled,positionBuffer:current.attributes[0].buffer?.id??null,draws:draws.slice(drawBegin)});}
 if(request.throwSecond){assert.equal(frames[0].errorSame,true);assert.equal(frames[0].calls,1);assert.equal(frames[1].errorSame,false);assert.equal(frames[1].calls,1);assert.equal(frames[1].draws[0].positionEnabled,false);assert.equal(frames[1].draws[0].positionBuffer,null);assert.deepEqual(frames[1].draws[0].indexData,Array.from(second.array));assert.notEqual(programSelections[0].key,programSelections[1].key);assert.equal(programSelections[1].key,programSelections[2].key);}
 return {id:request.id,frames,uploads,programSelections,commands};
});
process.stdout.write(JSON.stringify({scope:'Exact fixed renderObject + renderBufferDirect; real WebGLObjects/Geometries/Attributes/BindingStates/IndexedBufferRenderer; command-recording GL and shader-link host, no WebGL GPU',WebGLRendererInstantiated:false,WebGLGPUExecuted:false,VulkanGPUExecuted:false,rendererSHA256:createHash('sha256').update(source).digest('hex'),renderObject:{loc:objectNode.loc,sha256:createHash('sha256').update(slice(objectNode)).digest('hex')},renderBufferDirect:{loc:directNode.loc,sha256:createHash('sha256').update(slice(directNode)).digest('hex')},renderOwnTryCount:renderTry,renderBindingCalls:bindingCalls,resetDefaultStatement:{loc:defaultResetNode.loc,source:slice(defaultResetNode)},results}));
