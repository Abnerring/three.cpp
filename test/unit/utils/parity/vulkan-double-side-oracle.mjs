import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {pathToFileURL} from 'node:url';
import {createHash} from 'node:crypto';

const options = Object.fromEntries(process.argv.slice(2).reduce((all,value,index,args) => index%2 ? all : [...all,[value,args[index+1]]], []));
assert.ok(options['--upstream'] && options['--input'], 'Usage: --upstream PATH --input JSON');
const upstream = resolve(options['--upstream']);
const importSource = name => import(pathToFileURL(resolve(upstream, 'src', name)).href);
const [{parse}, {WebGLAttributes}, {MeshBasicMaterial}, {PlaneGeometry}, {Mesh}, {Scene}, {OrthographicCamera}, constants] = await Promise.all([
  import(pathToFileURL(resolve(upstream,'node_modules/acorn/dist/acorn.mjs')).href),
  importSource('renderers/webgl/WebGLAttributes.js'), importSource('materials/MeshBasicMaterial.js'),
  importSource('geometries/PlaneGeometry.js'), importSource('objects/Mesh.js'),
  importSource('scenes/Scene.js'), importSource('cameras/OrthographicCamera.js'), importSource('constants.js')
]);
const rendererSource = readFileSync(resolve(upstream,'src/renderers/WebGLRenderer.js'),'utf8');
const ast = parse(rendererSource,{ecmaVersion:'latest',sourceType:'module',locations:true});
const found = [];
function visit(node) {
  if (!node || typeof node!=='object') return;
  if(node.type==='FunctionDeclaration' && node.id?.name==='renderObject') found.push(node);
  for(const [key,value] of Object.entries(node)) if(key!=='loc') {
    if(Array.isArray(value)) value.forEach(visit); else if(value && typeof value==='object') visit(value);
  }
}
visit(ast);assert.equal(found.length,1);
const originalFunctionSource=rendererSource.slice(found[0].start,found[0].end);
const sourceFunctionFactory = new Function('_nodesHandler','_this','BackSide','FrontSide','DoubleSide',`return (${originalFunctionSource});`);
const fixtures=JSON.parse(readFileSync(options['--input'],'utf8'));
const cases=fixtures.map(request=>{
  const material=new MeshBasicMaterial({color:0xff0000,transparent:request.transparent,side:request.side,forceSinglePass:request.forceSinglePass});
  const geometry=new PlaneGeometry(1.6,1.6), object=new Mesh(geometry,material), scene=new Scene(), camera=new OrthographicCamera(-1,1,1,-1,.1,20);
  if(request.backFacing)object.rotation.y=Math.PI;
  scene.add(object);camera.position.z=5;scene.updateMatrixWorld();camera.updateMatrixWorld();
  const events=[], frames=[], initialVersion=material.version, reason={originalUploadFailure:true};
  let frame=0,uploads=0,calls=0,secondUploads=0;
  const observations=[];
  const event=kind=>events.push({frame,kind,side:material.side,versionDelta:material.version-initialVersion});
  // Source-only command recording host. No WebGL context, GPU pixels or Vulkan
  // execution are claimed. The actual fixed WebGLAttributes methods perform all
  // cache/version/upload-callback decisions against this command boundary.
  const gl={FLOAT:0x1406,UNSIGNED_SHORT:0x1403,createBuffer(){return {};},bindBuffer(){},bufferData(){},bufferSubData(){},deleteBuffer(){}};
  const attributes=WebGLAttributes(gl);
  const host={
    renderBufferDirect(activeCamera,activeScene,activeGeometry,activeMaterial,activeObject,group){
      assert.equal(activeCamera,camera);assert.equal(activeScene,scene);assert.equal(activeGeometry,geometry);
      assert.equal(activeMaterial,material);assert.equal(activeObject,object);assert.equal(group,null);
      attributes.update(geometry.index,0x8893);
      ++calls;
    },
    readPixels(){event('readback');}
  };
  const renderObject=sourceFunctionFactory(null,host,constants.BackSide,constants.FrontSide,constants.DoubleSide);
  object.onBeforeRender=()=>event('before');object.onAfterRender=()=>event('after');
  const originalIndex=geometry.index,secondIndex=request.swapIndex?originalIndex.clone():null;
  if(secondIndex)for(let i=0;i<secondIndex.array.length;i+=3){const temp=secondIndex.array[i+1];secondIndex.array[i+1]=secondIndex.array[i+2];secondIndex.array[i+2]=temp;}
  if(request.swapIndex)secondIndex.onUpload(()=>{
    ++uploads;++secondUploads;event('upload-second');
    observations.push({frame,kind:'second-upload',side:material.side,versionDelta:material.version-initialVersion,calls});
    if(request.secondReadback && secondUploads===1){event('second-readback-enter');host.readPixels();event('second-readback-exit');}
    if(request.throwSecond && secondUploads===1)throw reason;
  });
  geometry.index.onUpload(()=>{
    ++uploads;event('upload');
    if(request.swapIndex && uploads===1){geometry.setIndex(secondIndex);material.color.setRGB(0,0,1);}
    if(request.reenter && uploads===1) {event('readback-enter');host.readPixels();event('readback-exit');}
    if(request.throwFirst && uploads===1) throw reason;
  });
  for(frame=0;frame<2;++frame){
    calls=0;let errorSame=false;
    try{renderObject(object,scene,camera,geometry,material,null);}catch(error){assert.equal(error,reason);errorSame=error===reason;}
    frames.push({frame,errorSame,calls,side:material.side,versionDelta:material.version-initialVersion,uploads});
  }
  const result={id:request.id,events,frames};
  if(request.swapIndex)result.observations=observations;
  return result;
});
if(options['--audit']) process.stderr.write(JSON.stringify({scope:'exact fixed renderObject function + actual WebGLAttributes index-upload protocol; command recording host only',rendererSourceSHA256:createHash('sha256').update(rendererSource).digest('hex'),renderObjectSourceSHA256:createHash('sha256').update(originalFunctionSource).digest('hex'),sourceRange:{start:found[0].start,end:found[0].end,loc:found[0].loc},WebGLRendererInstantiated:false,WebGLGPUExecuted:false,VulkanGPUExecuted:false})+'\n');
process.stdout.write(JSON.stringify(cases));
