// Static translation of the fixed Material data/JSON statements into C++.
// The library contains native statements, never an AST evaluator or JS engine.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
const args=process.argv.slice(2),check=args.includes('--check'),upstream=path.resolve(args.find(x=>x!=='--check')??'../three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw new Error('Unexpected upstream');
const require=createRequire(path.join(upstream,'package.json')),acorn=require('acorn');
const source=fs.readFileSync(path.join(upstream,'src/materials/Material.js'),'utf8'),ast=acorn.parse(source,{ecmaVersion:'latest',sourceType:'module'}),cls=ast.body.find(n=>n.type==='ClassDeclaration');
const q=JSON.stringify,fail=n=>{throw new Error(`Unsupported ${n.type}: ${source.slice(n.start,n.end)}`);};
let serial=0;
const temp=()=>`v${++serial}`;
const iife=(body,result)=>`([&]() -> JSValue { ${body} return ${result}; }())`;
function expr(n){
 if(n.type==='Literal')return n.value===null?'JSValue(nullptr)':`JSValue(${q(n.value)})`;
 if(n.type==='ThisExpression')return 'self';
 if(n.type==='Identifier'){
  if(n.name==='undefined')return 'JSValue{}';
  if(['Color','Vector2','Plane','Array','Object'].includes(n.name))return `imports.get(${q(n.name)})`;
  return n.name;
 }
 if(n.type==='MemberExpression'){
  if(!n.computed)return `${expr(n.object)}.get(${q(n.property.name)})`;
  const a=temp(),b=temp();return iife(`auto ${a}=${expr(n.object)}; auto ${b}=${expr(n.property)};`,`${a}.get(${b})`);
 }
 if(n.type==='LogicalExpression'){
  const a=temp();return iife(`auto ${a}=${expr(n.left)}; if(${n.operator==='&&'?'!':''}${a}.toBoolean()) return ${a};`,expr(n.right));
 }
 if(n.type==='BinaryExpression'){
  const a=temp(),b=temp();let value;
  if(n.operator==='==='||n.operator==='!==')value=`JSValue(${n.operator==='!=='?'!':''}${a}.strictEquals(${b}))`;
  else if(n.operator==='>')value=`JSValue(greater(${a},${b}))`;
  else if(n.operator==='+')value=`add(${a},${b})`;
  else return fail(n);
  return iife(`auto ${a}=${expr(n.left)}; auto ${b}=${expr(n.right)};`,value);
 }
 if(n.type==='UnaryExpression'){
  if(n.operator==='typeof')return `JSValue(typeOf(${expr(n.argument)}))`;
  if(n.operator==='!')return `JSValue(!${expr(n.argument)}.toBoolean())`;
  return fail(n);
 }
 if(n.type==='ObjectExpression'){
  const value=temp();let body=`auto ${value}=JSValue::object();`;
  for(const p of n.properties){if(p.type!=='Property'||p.computed||p.kind!=='init')fail(p);body+=`own(${value},${q(p.key.name??p.key.value)},${expr(p.value)});`;}
  return iife(body,value);
 }
 if(n.type==='ArrayExpression'){
  const value=temp();let body=`auto ${value}=JSValue::array(${n.elements.length});`;
  for(let i=0;i<n.elements.length;i++)if(n.elements[i])body+=`own(${value},${q(String(i))},${expr(n.elements[i])});`;
  return iife(body,value);
 }
 if(n.type==='AssignmentExpression'&&n.operator==='='){
  if(n.left.type==='Identifier')return `(${n.left.name}=${expr(n.right)})`;
  if(n.left.type!=='MemberExpression')return fail(n);
  const a=temp(),b=temp(),v=temp();return iife(`auto ${a}=${expr(n.left.object)}; auto ${b}=${n.left.computed?expr(n.left.property):`JSValue(${q(n.left.property.name)})`}; auto ${v}=${expr(n.right)}; ${a}.set(${b},${v});`,v);
 }
 if(n.type==='NewExpression'){
  const ctor=temp();let body=`auto ${ctor}=${expr(n.callee)};`;const argv=[];
  for(const x of n.arguments){const a=temp();body+=`auto ${a}=${expr(x)};`;argv.push(a);}
  return iife(body,`${ctor}.construct({${argv.join(',')}})`);
 }
 if(n.type==='CallExpression'){
  if(n.callee.type==='Super')return 'JSValue{}';
  if(n.callee.type==='Identifier'&&n.callee.name==='generateUUID')return 'JSValue(MathUtils::generateUUID())';
  if(n.callee.type==='Identifier'&&n.callee.name==='extractFromCache')return `extractFromCache(${expr(n.arguments[0])})`;
  if(n.callee.type!=='MemberExpression')return fail(n);
  if(n.callee.object.type==='Identifier'&&n.callee.object.name==='Object'&&n.callee.property.name==='defineProperty')return 'defineMaterialId(self,state,imports)';
  const object=temp(),fn=temp();let body=`auto ${object}=${expr(n.callee.object)}; auto ${fn}=${object}.get(${n.callee.computed?expr(n.callee.property):q(n.callee.property.name)});`;const argv=[];
  for(const x of n.arguments){const a=temp();body+=`auto ${a}=${expr(x)};`;argv.push(a);}
  return iife(body,`${fn}.call(${object},{${argv.join(',')}})`);
 }
 if(n.type==='ArrowFunctionExpression'){
  if(n.params.length!==1||n.params[0].type!=='Identifier'||n.body.type==='BlockStatement')return fail(n);
  const fn=temp();return iife(`auto ${fn}=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue { auto imports=captures[0]; auto ${n.params[0].name}=arg(args,0); return ${expr(n.body)}; },{imports},"",1); ${fn}.setFunctionSourceText(${q(source.slice(n.start,n.end))});`,fn);
 }
 return fail(n);
}
function statement(n){
 if(n.type==='BlockStatement')return `{\n${n.body.map(statement).join('\n')}\n}`;
 if(n.type==='ExpressionStatement')return `${expr(n.expression)};`;
 if(n.type==='VariableDeclaration')return n.declarations.map(d=>{if(d.id.type!=='Identifier'||!d.init)return fail(d);return `auto ${d.id.name}=${expr(d.init)};`;}).join('\n');
 if(n.type==='IfStatement')return `if(${expr(n.test)}.toBoolean()) ${statement(n.consequent)}${n.alternate?' else '+statement(n.alternate):''}`;
 if(n.type==='ReturnStatement')return `return ${n.argument?expr(n.argument):'JSValue{}'};`;
 if(n.type==='FunctionDeclaration'&&n.id.name==='extractFromCache')return '// extractFromCache is implemented by the native for-in/delete helper.';
 return fail(n);
}
const methods=new Map(cls.body.body.map(m=>[m.kind==='get'||m.kind==='set'?m.kind+'_'+m.key.name:m.key.name,m]));
const text=['// Generated native statements from fixed Material.js; do not edit.',`// source SHA-256: ${createHash('sha256').update(source).digest('hex')}`];
for(const [name,parameters] of [['constructor','JSValue self,const JSValue& imports,JSValue state'],['toJSON','JSValue self,JSValue meta,const JSValue& imports'],['fromJSON','JSValue self,JSValue json,JSValue textures,const JSValue& imports']]){
 text.push(`JSValue material_${name}(${parameters}) ${statement(methods.get(name).value.body)}`);
 if(name==='constructor')text[text.length-1]=text[text.length-1].replace(/\n\}$/,'\nreturn self;\n}');
}
const metadata=['// Exact source text is observable function metadata; never executed.','namespace MaterialSource {',`inline constexpr const char* Material=${q(source.slice(cls.start,cls.end))};`];
for(const [name,m] of methods)if(name!=='constructor')metadata.push(`inline constexpr const char* ${name}=${q(source.slice(m.start,m.end))};`);
metadata.push('}');
for(const [file,value] of [['MaterialRuntimeStatements.inc',text.join('\n\n')+'\n'],['MaterialSource.inc',metadata.join('\n')+'\n']]){
 const target=new URL('../src/materials/'+file,import.meta.url);
 if(check){if(fs.readFileSync(target,'utf8').replaceAll('\r\n','\n')!==value)throw new Error('Stale '+file);}else fs.writeFileSync(target,value);
}
console.log('Material: constructor/toJSON/fromJSON native statements and exact method metadata verified');
