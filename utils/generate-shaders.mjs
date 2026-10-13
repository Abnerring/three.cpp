import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
const root=fileURLToPath(new URL('../',import.meta.url));
const upstream=path.resolve(process.argv[2]??'../three.js');
const lock=JSON.parse(fs.readFileSync(path.join(root,'upstream.json'),'utf8'));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const require=createRequire(path.join(upstream,'package.json'));
const {parse}=require('acorn');
const parseSource=file=>parse(fs.readFileSync(path.join(upstream,file),'utf8'),{ecmaVersion:'latest',sourceType:'module'});
const check=process.argv.includes('--check'), outputs=new Map(), inputs=[];
const prefix='src/renderers/shaders/';
const comment=file=>`// Generated from three.js ${file} at ${lock.commit}.\n// Regenerate with utils/generate-shaders.mjs. See THREEJS-LICENSE.\n`;
const put=(file,text)=>{outputs.set(file,text);};
const namespace=(group,base)=>`ShaderSource::${group==='ShaderChunk'?'Chunk':'Lib'}::${base}`;
const literal=(value)=>{for(let i=0;;++i){const tag='three_glsl_'+i;if(!value.includes(')'+tag+'"'))return `R"${tag}(${value})${tag}"`;}};
for(const group of ['ShaderChunk','ShaderLib']){
    for(const name of fs.readdirSync(path.join(upstream,prefix,group)).filter(x=>x.endsWith('.glsl.js')).sort()){
        const file=prefix+group+'/'+name, ast=parseSource(file), base=name.slice(0,-'.glsl.js'.length), exported=[];
        assert.match(base,/^[A-Za-z_][A-Za-z_0-9]*$/);
        for(const node of ast.body){
            if(node.type==='ExportDefaultDeclaration'){assert.equal(node.declaration.type,'TemplateLiteral');assert.equal(node.declaration.expressions.length,0);exported.push(['defaultSource',node.declaration.quasis[0].value.cooked,'default']);}
            else {assert.equal(node.type,'ExportNamedDeclaration');assert.equal(node.declaration.type,'VariableDeclaration');assert.equal(node.declaration.kind,'const');for(const d of node.declaration.declarations){assert.equal(d.init.type,'TemplateLiteral');assert.equal(d.init.expressions.length,0);exported.push([d.id.name,d.init.quasis[0].value.cooked,d.id.name]);}}
        }
        for(const [name,value] of exported){assert.equal(typeof value,'string');assert.match(name,/^[A-Za-z_][A-Za-z_0-9]*$/);assert.ok(!value.includes('\0'));}
        const stem=prefix+group+'/'+base+'.glsl', ns=namespace(group,base);
        put(stem+'.h',comment(file)+`#pragma once\n#include <string_view>\nnamespace ${ns} {\n`+exported.map(([name])=>`std::string_view ${name}();\n`).join('')+'}\n');
        put(stem+'.cpp',comment(file)+`#include "renderers/shaders/${group}/${base}.glsl.h"\nnamespace ${ns} {\n`+exported.map(([name,value])=>`std::string_view ${name}() { return ${literal(value)}; }\n`).join('')+'}\n');
        inputs.push({file,group,base,exports:exported.map(([name,,original])=>({name,original}))});
    }
}
const chunkAst=parseSource(prefix+'ShaderChunk.js'), imports=new Map();
for(const node of chunkAst.body)if(node.type==='ImportDeclaration'){
    assert.equal(node.specifiers.length,1);const spec=node.specifiers[0], parts=node.source.value.split('/'), group=parts[1],base=parts[2].slice(0,-'.glsl.js'.length);
    imports.set(spec.local.name,{group,base,kind:spec.type});
}
const chunk=chunkAst.body.find(n=>n.type==='ExportNamedDeclaration'&&n.declaration?.type==='VariableDeclaration').declaration.declarations[0];
assert.equal(chunk.id.name,'ShaderChunk');assert.equal(chunk.init.type,'ObjectExpression');
function chunkExpression(n){
    if(n.type==='Identifier'){const source=imports.get(n.name);assert.equal(source.kind,'ImportDefaultSpecifier');return `std::string(${namespace(source.group,source.base)}::defaultSource())`;}
    assert.equal(n.type,'MemberExpression');assert.equal(n.computed,false);const source=imports.get(n.object.name);assert.equal(source.kind,'ImportNamespaceSpecifier');return `std::string(${namespace(source.group,source.base)}::${n.property.name}())`;
}
const objectHelper=`namespace {\nJSValue object(std::initializer_list<std::pair<const char*,JSValue>> fields) {\n    auto result=JSValue::object();\n    for(const auto& [key,value]:fields){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;result.defineProperty(key,d);}\n    return result;\n}\n`;
const factory=(name,expression,pre='',post='')=>`JSValue native${name}() {\n    static thread_local JSWeakValue cache;\n    if(const auto existing=cache.lock();existing&&existing->isObject())return *existing;\n    static thread_local bool initializing=false;\n    JSModuleInitializationScope initialization(initializing,"${name}");\n${pre}    auto result=${expression};\n${post}    result.retainForHeapLifetime();cache=JSWeakValue(result);return result;\n}\n`;
for(const name of ['ShaderChunk','ShaderLib'])put(prefix+name+'.h',`#pragma once\n#include "core/JSModuleObject.h"\nJSValue native${name}();\n// Stateless resolver; explicit JSValue handles retain the current realm object.\ninline constexpr JSModuleObject ${name}(native${name});\n`);
const chunkIncludes=[...imports.values()].map(x=>`#include "renderers/shaders/${x.group}/${x.base}.glsl.h"\n`).join('');
const chunkInit='object({\n'+chunk.init.properties.map(p=>{assert.equal(p.type,'Property');assert.equal(p.kind,'init');assert.equal(p.computed,false);return `        {${JSON.stringify(p.key.name)},${chunkExpression(p.value)}}`;}).join(',\n')+'\n    })';
put(prefix+'ShaderChunk.cpp',comment(prefix+'ShaderChunk.js')+'#include "renderers/shaders/ShaderChunk.h"\n'+chunkIncludes+'#include <initializer_list>\n#include <utility>\n'+objectHelper+'}\n'+factory('ShaderChunk',chunkInit));
const libAst=parseSource(prefix+'ShaderLib.js');
const lib=libAst.body.flatMap(n=>n.type==='VariableDeclaration'?n.declarations:[]).find(d=>d.id.name==='ShaderLib');
function expression(n,depth=1){
    const indent='    '.repeat(depth);
    if(n.type==='Literal')return n.value===null?'JSValue(nullptr)':typeof n.value==='boolean'?String(n.value):JSON.stringify(n.value);
    if(n.type==='UnaryExpression'){assert.equal(n.operator,'-');assert.equal(n.argument.type,'Literal');return '-'+expression(n.argument,depth);}
    if(n.type==='ArrayExpression')return 'array({\n'+n.elements.map(e=>indent+'    '+expression(e,depth+1)).join(',\n')+'\n'+indent+'})';
    if(n.type==='ObjectExpression')return 'object({\n'+n.properties.map(p=>{assert.equal(p.type,'Property');assert.equal(p.kind,'init');assert.equal(p.computed,false);return indent+'    {'+JSON.stringify(p.key.name??p.key.value)+','+expression(p.value,depth+1)+'}';}).join(',\n')+'\n'+indent+'})';
    if(n.type==='NewExpression'){const name=n.callee.name,wrapper={Color:'nativeColor',Vector2:'nativeVector',Vector3:'nativeVector',Matrix3:'nativeMatrix'}[name];assert.ok(wrapper,name);return `${wrapper}(std::make_shared<${name}>(${n.arguments.map(x=>expression(x,depth)).join(',')}))`;}
    if(n.type==='CallExpression'){assert.equal(n.callee.name,'mergeUniforms');assert.equal(n.arguments.length,1);return 'mergeUniforms('+expression(n.arguments[0],depth)+')';}
    if(n.type==='Identifier'){const names={UniformsLib:'uniforms',ShaderChunk:'chunks',ShaderLib:'result'};assert.ok(names[n.name],n.name);return names[n.name];}
    if(n.type==='MemberExpression'){assert.equal(n.computed,false);return expression(n.object,depth)+'.get('+JSON.stringify(n.property.name)+')';}
    throw Error('Unsupported ShaderLib initializer '+n.type);
}
const assignments=libAst.body.filter(n=>n.type==='ExpressionStatement').map(n=>{const e=n.expression;assert.equal(e.type,'AssignmentExpression');assert.equal(e.operator,'=');assert.equal(e.left.object.name,'ShaderLib');assert.equal(e.left.computed,false);return `    result.set(${JSON.stringify(e.left.property.name)},${expression(e.right)});\n`;}).join('');
const libIncludes=['ShaderLib.h','ShaderChunk.h','UniformsLib.h','UniformsUtils.h'].map(x=>`#include "renderers/shaders/${x}"\n`).join('')+'#include "core/NativeObject.h"\n'+['Color','Vector2','Vector3','Matrix3'].map(x=>`#include "math/${x}.h"\n`).join('')+'#include <initializer_list>\n#include <utility>\n';
const arrayHelper=`JSValue array(std::initializer_list<JSValue> values) {\n    auto result=JSValue::array();std::size_t index=0;\n    for(const auto& value:values){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;result.defineProperty(std::to_string(index++),d);}\n    return result;\n}\n}\n`;
put(prefix+'ShaderLib.cpp',comment(prefix+'ShaderLib.js')+libIncludes+objectHelper+arrayHelper+factory('ShaderLib',expression(lib.init),'    const auto chunks=nativeShaderChunk();\n    const auto uniforms=nativeUniformsLib();\n',assignments));
put(prefix+'ShaderSources.cmake','# Generated by utils/generate-shaders.mjs.\nset(THREECPP_SHADER_STRING_SOURCES\n'+[...outputs.keys()].filter(x=>x.endsWith('.cpp')).map(x=>'    "${CMAKE_CURRENT_LIST_DIR}/'+x.slice(prefix.length)+'"').join('\n')+'\n)\n');
put('utils/shader-sources.json',JSON.stringify({commit:lock.commit,modules:inputs,chunkKeys:chunk.init.properties.map(p=>p.key.name)},null,2)+'\n');
const fixtureIncludes=inputs.map(m=>`#include "renderers/shaders/${m.group}/${m.base}.glsl.h"`).join('\n');
const fixtureCases=inputs.map(m=>`emit(JsonValue::Object{{"kind","source"},{"file",${JSON.stringify(m.file)}},{"exports",JsonValue::Object{${m.exports.map(e=>`{${JSON.stringify(e.original)},std::string(${namespace(m.group,m.base)}::${e.name}())}`).join(',')}}}});`).join('\n');
put('test/unit/utils/parity/ShaderStrings.generated.h','// Generated fixture input inventory; expected text comes from actual imported upstream modules.\n'+fixtureIncludes+'\ninline void shaderStringCases(){\n'+fixtureCases+'\n}\n');
for(const [file,text] of outputs){const dest=path.join(root,file);if(check)assert.equal(fs.readFileSync(dest,'utf8').replaceAll('\r\n','\n'),text,file);else if(!fs.existsSync(dest)||fs.readFileSync(dest,'utf8')!==text){fs.mkdirSync(path.dirname(dest),{recursive:true});fs.writeFileSync(dest,text);}}
console.log(`${inputs.length} GLSL modules / ${chunk.init.properties.length} aggregate keys / ${outputs.size} generated files ${check?'verified':'generated'}`);
