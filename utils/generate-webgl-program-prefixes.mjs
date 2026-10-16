// Development-only source translation. The generated C++ contains direct
// expressions and calls; no JavaScript parser or interpreter ships at runtime.
import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import { pathToFileURL } from 'node:url';
const upstream=path.resolve(process.argv[2]);
const stage=path.resolve(process.argv[3]);
const revision='11738a9d55e502fb484cefffcea3a66dc44e9fe9';
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),revision);
const {parse}=await import(pathToFileURL(path.join(upstream,'node_modules/acorn/dist/acorn.mjs')));
const source=fs.readFileSync(path.join(upstream,'src/renderers/webgl/WebGLProgram.js'),'utf8');
assert.equal(source.replaceAll('\r\n','\n'),execFileSync('git',['-C',upstream,'show','HEAD:src/renderers/webgl/WebGLProgram.js'],{encoding:'utf8'}));
const ast=parse(source,{ecmaVersion:'latest',sourceType:'module'});
const ctor=ast.body.find(n=>n.type==='FunctionDeclaration'&&n.id.name==='WebGLProgram');
const quote=s=>JSON.stringify(s);
function expr(n){
  if(n.type==='Literal') return 'JSValue('+quote(n.value)+')';
  if(n.type==='Identifier'){
    if(n.name==='ShaderChunk')return 'module.get("ShaderChunk")';
    if(n.name==='filterEmptyLine')return 'module.get("filterEmptyLine")';
    return n.name;
  }
  if(n.type==='MemberExpression')return n.computed?
    '([&] { const auto base = '+expr(n.object)+'; const auto key = '+expr(n.property)+'; return base.get(key); }())':
    '('+expr(n.object)+').get('+quote(n.property.name)+')';
  if(n.type==='BinaryExpression'){
    const a=expr(n.left),b=expr(n.right);
    const body={'+':'add(left, right)','-':'subtract(left, right)','>':'JSValue(greater(left, right))',
      '===':'JSValue(left.strictEquals(right))','!==':'JSValue(!left.strictEquals(right))'}[n.operator];
    if(!body)throw Error('binary '+n.operator);
    return '([&] { const auto left = '+a+'; const auto right = '+b+'; return '+body+'; }())';
  }
  if(n.type==='LogicalExpression')return '([&] { const auto left = '+expr(n.left)+'; return '+
    (n.operator==='&&'?'left.toBoolean() ? '+expr(n.right)+' : left':'left.toBoolean() ? left : '+expr(n.right))+'; }())';
  if(n.type==='ConditionalExpression')return '(('+expr(n.test)+').toBoolean() ? '+expr(n.consequent)+' : '+expr(n.alternate)+')';
  if(n.type==='ArrayExpression')return '([&] { auto array = JSValue::array(); '+n.elements.map((e,i)=>'literal(array, "'+i+'", '+expr(e)+');').join(' ')+' return array; }())';
  if(n.type==='CallExpression'){
    const args=n.arguments.map(expr).join(', ');
    if(n.callee.type==='MemberExpression')return '([&] { const auto receiver = '+expr(n.callee.object)+
      '; const auto method = receiver.get('+ (n.callee.computed?expr(n.callee.property):quote(n.callee.property.name))+
      '); const Args args{'+args+'}; return method.call(receiver, args); }())';
    const name=n.callee.name;
    if(!['generatePrecision','getToneMappingFunction','getTexelEncodingFunction','getLuminanceFunction'].includes(name))throw Error('call '+name);
    return '([&] { const Args args{'+args+'}; return '+name+'(module, args); }())';
  }
  throw Error('Unhandled expression '+n.type+' '+source.slice(n.start,n.end));
}
const raw=ctor.body.body.find(n=>n.type==='IfStatement'&&source.slice(n.test.start,n.test.end)==='parameters.isRawShaderMaterial');
const version=ctor.body.body.find(n=>n.type==='IfStatement'&&source.slice(n.test.start,n.test.end)==='parameters.isRawShaderMaterial !== true');
const assignments=block=>block.body.filter(n=>n.type==='ExpressionStatement'&&n.expression.type==='AssignmentExpression').map(n=>n.expression);
const select=(block,name)=>assignments(block).find(n=>n.left.name===name);
const list=[['buildRawPrefix',select(raw.consequent,'prefixVertex')],['buildVertexPrefix',select(raw.alternate,'prefixVertex')],
 ['buildFragmentPrefix',select(raw.alternate,'prefixFragment')],['buildVersionVertexPrefix',select(version.consequent,'prefixVertex')],
 ['buildVersionFragmentPrefix',select(version.consequent,'prefixFragment')]];
let output='// Generated from pinned WebGLProgram.js prefix expressions. See THREEJS-LICENSE.\n';
for(const [name,node] of list){
 output+='JSValue '+name+'(const JSValue& module, const JSValue& parameters, const JSValue& customDefines,\n'+
 ' const JSValue& customVertexExtensions, const JSValue& shadowMapTypeDefine, const JSValue& envMapTypeDefine,\n'+
 ' const JSValue& envMapModeDefine, const JSValue& envMapBlendingDefine, const JSValue& prefixVertex = {}, const JSValue& prefixFragment = {}) {\n';
 output+='    return '+expr(node.right)+';\n}\n';
}
const destination=path.join(stage,'src/renderers/webgl/WebGLProgramPrefixes.inc');
// Pass the project's formatter as argument 4 for the checked-in spelling.
// Generation itself needs only the pinned upstream development dependencies.
if(process.argv[4])output=execFileSync(path.resolve(process.argv[4]),
 ['--style=file',`--assume-filename=${destination}`],{input:output,encoding:'utf8'});
fs.writeFileSync(destination,output,'utf8');
console.log(JSON.stringify({functions:list.length,source:'11738a9d55e502fb484cefffcea3a66dc44e9fe9',bytes:Buffer.byteLength(output)}));
