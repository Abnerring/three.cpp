// Differential comparison for the ported examples/jsm/shaders cluster.
//
//   node compare-jsm-shaders.mjs <three.js checkout> <probe executable>
//
// The script imports every pinned shader module, builds the same JSON description the C++
// probe prints and compares both trees with assert.deepEqual (no tolerances). Strings are
// escaped character by character (`\uXXXX` outside printable ASCII) so byte differences in
// the GLSL are reported instead of hidden by JSON escaping; numbers use the ES
// Number::toString spelling so -0/NaN/Infinity stay distinguishable.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {spawnSync,execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

const upstream=path.resolve(process.argv[2]),probe=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);

const shadersDir=path.join(upstream,'examples/jsm/shaders');
const files=fs.readdirSync(shadersDir).filter(name=>name.endsWith('.js')).sort();
// The addons import the bare specifier 'three', which the checkout's package.json maps to
// the prebuilt build/three.cjs. The port targets the pinned source, so every module is
// loaded with that specifier rewritten to src/Three.js instead.
const threeUrl=pathToFileURL(path.join(upstream,'src/Three.js')).href;
const three=await import(threeUrl);
async function importShaderModule(file){
 const source=fs.readFileSync(path.join(shadersDir,file),'utf8');
 const rewritten=source.replace(/'three'/g,JSON.stringify(threeUrl));
 return import('data:text/javascript;base64,'+Buffer.from(rewritten).toString('base64'));
}

function escapeText(text){
 let out='';
 for(const ch of text){const code=ch.codePointAt(0);
  if(ch==='\\')out+='\\\\';
  else if(ch==='"')out+='\\"';
  else if(code>=0x20&&code<=0x7e)out+=ch;
  else if(code>0xffff){const rest=code-0x10000;out+='\\u'+((rest>>10)+0xd800).toString(16).padStart(4,'0')+'\\u'+((rest&0x3ff)+0xdc00).toString(16).padStart(4,'0');}
  else out+='\\u'+code.toString(16).padStart(4,'0');}
 return out;
}
function numberNode(value){
 if(Number.isNaN(value))return{t:'num',v:'NaN'};
 if(value===Infinity)return{t:'num',v:'Infinity'};
 if(value===-Infinity)return{t:'num',v:'-Infinity'};
 if(Object.is(value,-0))return{t:'num',v:'-0'};
 return{t:'num',v:String(value)};
}
function stringNode(value){return{t:'str',v:escapeText(value)};}
function elementsNode(object,type,key){return{t:type,values:object[key].slice().map(numberNode)};}
function textureNode(texture){
 return{t:'DataTexture',width:numberNode(texture.image.width),height:numberNode(texture.image.height),
  wrapS:numberNode(texture.wrapS),wrapT:numberNode(texture.wrapT),version:numberNode(texture.version),
  data:Array.from(texture.image.data).map(numberNode)};
}
function describe(value){
 if(value===undefined)return{t:'undefined'};
 if(value===null)return{t:'null'};
 const type=typeof value;
 if(type==='boolean')return{t:'bool',v:value};
 if(type==='number')return numberNode(value);
 if(type==='string')return stringNode(value);
 if(type==='function')return{t:'fn',name:value.name,length:value.length};
 if(Array.isArray(value))return{t:'array',values:value.map(describe)};
 if(value.isTexture)return textureNode(value);
 if(value.isVector2)return{t:'Vector2',values:[numberNode(value.x),numberNode(value.y)]};
 if(value.isVector3)return{t:'Vector3',values:[numberNode(value.x),numberNode(value.y),numberNode(value.z)]};
 if(value.isVector4)return{t:'Vector4',values:[numberNode(value.x),numberNode(value.y),numberNode(value.z),numberNode(value.w)]};
 if(value.isColor)return{t:'Color',values:[numberNode(value.r),numberNode(value.g),numberNode(value.b)]};
 if(value.isMatrix3)return elementsNode(value,'Matrix3','elements');
 if(value.isMatrix4)return elementsNode(value,'Matrix4','elements');
 const keys=Object.keys(value);
 return{t:'object',keys,values:keys.map(key=>describe(value[key]))};
}

const expectedModules=[];
let exportCount=0,fieldCount=0,glslCount=0,glslChars=0,uniformCount=0;
for(const file of files){
 const module=await importShaderModule(file);
 expectedModules.push({file,exports:Object.keys(module).map(name=>({name,value:describe(module[name])}))});
 for(const name of Object.keys(module)){
  exportCount++;
  const value=module[name];
  if(value===null||typeof value!=='object')continue;
  fieldCount+=Object.keys(value).length;
  for(const key of Object.keys(value)){
   if(key==='vertexShader'||key==='fragmentShader'){glslCount++;glslChars+=String(value[key]).length;}
   if(key==='uniforms')uniformCount+=Object.keys(value[key]).length;
  }
 }
}

const blurUtils=(await importShaderModule('DepthLimitedBlurShader.js')).BlurShaderUtils;
const gtao=await importShaderModule('GTAOShader.js');
const poisson=await importShaderModule('PoissonDenoiseShader.js');
const halfPixel=new three.Vector2(1/512,0);
const expectedHelpers=[];
const pushHelper=(name,value)=>expectedHelpers.push({name,value:describe(value)});
pushHelper('BlurShaderUtils.createSampleWeights(4, 1)',blurUtils.createSampleWeights(4,1));
pushHelper('BlurShaderUtils.createSampleWeights(0, 2.5)',blurUtils.createSampleWeights(0,2.5));
pushHelper('BlurShaderUtils.createSampleOffsets(4, Vector2(1/512, 0))',blurUtils.createSampleOffsets(4,halfPixel));
const material={defines:{},uniforms:{sampleUvOffsets:{value:null},sampleWeights:{value:null}}};
blurUtils.configure(material,4,1,halfPixel);
pushHelper('BlurShaderUtils.configure(material, 4, 1, Vector2(1/512, 0))',material);
pushHelper('generateMagicSquareNoise()',gtao.generateMagicSquareNoise());
pushHelper('generateMagicSquareNoise(4)',gtao.generateMagicSquareNoise(4));
pushHelper('generateMagicSquareNoise(5)',gtao.generateMagicSquareNoise(5));
pushHelper('generatePdSamplePointInitializer(16, 2, 1)',poisson.generatePdSamplePointInitializer(16,2,1));
pushHelper('generatePdSamplePointInitializer(8, 3, 1.5)',poisson.generatePdSamplePointInitializer(8,3,1.5));
pushHelper('generatePdSamplePointInitializer(4, 1, 1)',poisson.generatePdSamplePointInitializer(4,1,1));
const expected={modules:expectedModules,helpers:expectedHelpers};

const run=spawnSync(probe,[],{encoding:'utf8',maxBuffer:256*1024*1024});
if(run.status!==0)throw Error(`probe failed ${run.status}: ${run.stderr}`);
const actual=JSON.parse(run.stdout);

let comparisons=0;
function compare(a,e,where){
 comparisons++;
 if(e&&typeof e==='object'){
  assert.equal(typeof a,'object',where);assert.ok(a!==null,where);
  assert.deepEqual(Object.keys(a).sort(),Object.keys(e).sort(),where+' keys');
  for(const key of Object.keys(e))compare(a[key],e[key],where+'.'+key);
 }else assert.deepEqual(a,e,where);
}
try{
 compare(actual,expected,'root');
}catch(error){
 const directory=fs.mkdtempSync(path.join(process.env.TEMP||'.','threecpp-jsm-shaders-'));
 fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,1));
 fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,1));
 throw Error(`${error.message}\nDiagnostic: ${directory}`,{cause:error});
}
console.log(`JSM shaders parity: ${expected.modules.length} modules / ${exportCount} exports / ${fieldCount} fields / ${uniformCount} uniforms / ${glslCount} GLSL strings (${glslChars} chars), ${comparisons} comparisons against ${lock.commit}`);
