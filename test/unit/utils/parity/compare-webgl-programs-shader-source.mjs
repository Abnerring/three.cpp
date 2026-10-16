import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL,fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
const [checkout,exe,out]=process.argv.slice(2); if(!checkout||!exe||!out)throw Error('checkout executable output-directory required');
const head=spawnSync('git',['-C',checkout,'rev-parse','HEAD'],{encoding:'utf8'});assert.equal(head.status,0);assert.equal(head.stdout.trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const module=await import(pathToFileURL(path.resolve(checkout,'src/renderers/webgl/WebGLPrograms.js')).href);
const three=await import(pathToFileURL(path.resolve(checkout,'src/Three.js')).href);
const cases=JSON.parse(fs.readFileSync(new URL('./WebGLProgramsShaderSourceCases.json',import.meta.url),'utf8'));
function fixture(config){
 const sources=[], gl={VERTEX_SHADER:35633,FRAGMENT_SHADER:35632};
 for(const method of ['createProgram','createShader','shaderSource','compileShader','attachShader','bindAttribLocation','linkProgram','deleteProgram'])gl[method]=function(...args){
  if(method==='createProgram')return {};
  if(method==='createShader')return {type:args[0]};
  if(method==='shaderSource')sources.push({type:args[0].type,source:args[1]});
 };
 const renderer={outputColorSpace:'srgb',toneMapping:config.tone?4:0,shadowMap:{enabled:!!config.shadows,type:1},getContext(){return gl;},getRenderTarget(){return null;},state:{buffers:{depth:{getReversed(){return !!config.reversed;}}}}};
 const environments={get(env){return env;}}, extensions={has(){return false;}}, capabilities={precision:'highp',logarithmicDepthBuffer:!!config.logDepth,getMaxPrecision(v){return v;}},binding={releaseStatesOfProgram(){}};
 const clipping={numPlanes:config.clip?2:0,numIntersection:config.clip?1:0},lights={};
 for(const key of ['sun','directional','point','spot','spotLightMap','rectArea','hemi','sunShadowMap','directionalShadowMap','pointShadowMap','spotShadowMap'])lights[key]=Array(config.lights?1:0);
 lights.numSpotLightShadowsWithMaps=config.lights?1:0;lights.numLightProbes=0;
 const material=new three[config.material]();material.name='source-fixture';material.customProgramCacheKey=()=> 'source-fixture';Object.assign(material,config.props);
 for(const [key,channel]of Object.entries(config.maps||{})){const t=new three.Texture();t.channel=channel;material[key]=t;}
 const object=new three.Mesh(new three.BoxGeometry(),material),scene={environment:null,fog:config.fog?{isFogExp2:true}:null};
 const programs=module.WebGLPrograms(renderer,environments,extensions,capabilities,binding,clipping);
 const p=programs.getParameters(material,lights,Array(config.shadows?1:0),scene,object,[]),key=programs.getProgramCacheKey(p),program=programs.acquireProgram(p,key);
 const result=JSON.parse(JSON.stringify({parameters:p,key,sources}));programs.releaseProgram(program);return result;
}
fs.mkdirSync(out,{recursive:true});fs.writeFileSync(path.join(out,'inputs.json'),JSON.stringify(cases,null,2)+'\n');
const result=spawnSync(exe,[],{input:cases.map(c=>JSON.stringify(c)).join('\n')+'\n',encoding:'utf8',maxBuffer:128*1024*1024});
fs.writeFileSync(path.join(out,'native.stdout'),result.stdout);fs.writeFileSync(path.join(out,'native.stderr'),result.stderr);assert.equal(result.status,0,result.stderr);
const actual=result.stdout.trim().split(/\r?\n/).map(s=>JSON.parse(s));assert.equal(actual.length,cases.length);
const manifest={commit:head.stdout.trim(),cases:[],upstreamSources:{}};
for(const file of ['src/renderers/webgl/WebGLPrograms.js','src/renderers/webgl/WebGLProgram.js','src/renderers/webgl/WebGLShader.js','src/renderers/shaders/ShaderLib.js'])manifest.upstreamSources[file]=crypto.createHash('sha256').update(fs.readFileSync(path.join(checkout,file))).digest('hex');
let comparisons=0;
function compare(a,b,p){++comparisons;assert.equal(typeof a,typeof b,p);if(a&&typeof a==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),p);for(const k of Object.keys(a))compare(a[k],b[k],p+'.'+k);}else assert.equal(a,b,p);}
for(let i=0;i<cases.length;++i){
 const c=cases[i],expected=fixture(c);fs.writeFileSync(path.join(out,c.name+'.node.json'),JSON.stringify(expected,null,2)+'\n');fs.writeFileSync(path.join(out,c.name+'.native.json'),JSON.stringify(actual[i],null,2)+'\n');compare(actual[i],expected,c.name);
 const row={name:c.name,input:c,sources:[]};
 for(const [index,s]of actual[i].sources.entries()){
  const extension=s.type===35633?'vert':'frag',name=c.name+'.'+extension;fs.writeFileSync(path.join(out,name),s.source);
  row.sources.push({file:name,bytes:Buffer.byteLength(s.source),sha256:crypto.createHash('sha256').update(s.source).digest('hex')});
 }
 manifest.cases.push(row);
}
manifest.comparisons=comparisons;fs.writeFileSync(path.join(out,'MANIFEST.json'),JSON.stringify(manifest,null,2)+'\n');
console.log(`Programs → Program real ShaderLib source: ${cases.length} scenarios / ${comparisons} comparisons / ${cases.length*2} shader sources passed (${manifest.commit}).`);
