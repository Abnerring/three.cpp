import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {UniformsLib}=await import(pathToFileURL(path.join(upstream,'src/renderers/shaders/UniformsLib.js')));
const {cloneUniforms}=await import(pathToFileURL(path.join(upstream,'src/renderers/shaders/UniformsUtils.js')));
function encode(value){
    if(value===null||typeof value!=='object')return value??(value===null?null:'undefined');
    for(const flag of ['isColor','isVector2','isVector3','isMatrix3'])if(value[flag])return {kind:flag,data:value.toArray()};
    if(Array.isArray(value))return value.map(encode);
    return {properties:Object.keys(value).map(key=>[key,encode(value[key])])};
}
const expected={defaults:encode(UniformsLib),clones:[]};
const seen=new Map(),identities=[];
function visit(value,path){
    if(value===null||typeof value!=='object')return;
    if(seen.has(value)){identities.push([path,seen.get(value)]);return;}
    seen.set(value,path);identities.push([path,path]);
    for(const key of Object.keys(value))visit(value[key],path+'.'+key);
}
visit(UniformsLib,'lib');expected.identities=identities;
for(const [name,source] of Object.entries(UniformsLib)){
    const cloned=cloneUniforms(source);
    const aliases=Object.keys(source).map(uniform=>({uniform,same:source[uniform]===cloned[uniform],fields:Object.keys(source[uniform]).map(field=>[field,Object.is(source[uniform][field],cloned[uniform][field])])}));
    expected.clones.push({name,clone:encode(cloned),aliases});
}
const common=UniformsLib.common,copied=cloneUniforms(common);
common.diffuse.value.r=.25;
expected.mutation={shared:UniformsLib.common===common,original:UniformsLib.common.diffuse.value.r,clone:copied.diffuse.value.r};
const actual=JSON.parse(execFileSync(binary,[],{encoding:'utf8'}));
assert.deepEqual(actual,expected);
const uniformCount=Object.values(UniformsLib).reduce((n,group)=>n+Object.keys(group).length,0);
console.log(`UniformsLib parity: ${Object.keys(UniformsLib).length} groups / ${uniformCount} uniforms; defaults, key order, clone aliases and mutation passed`);
