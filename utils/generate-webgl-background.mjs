// Compile the pinned background factory and all of its closures to native C++.
// No production source/AST interpreter is used.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
const upstream=path.resolve(process.argv[2]??'');
if(!process.argv[2])throw new Error('Usage: generate-webgl-background.mjs <pinned>');
const repo=path.resolve(import.meta.dirname,'..');
const lock=JSON.parse(fs.readFileSync(path.join(repo,'upstream.json')));
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!==lock.commit)throw new Error('Unexpected source commit');
const {parse}=createRequire(path.join(upstream,'package.json'))('acorn');
const q=JSON.stringify,name='WebGLBackground';
const source=fs.readFileSync(path.join(upstream,'src/renderers/webgl/WebGLBackground.js'),'utf8');
const ast=parse(source,{sourceType:'module',ecmaVersion:'latest'});
    let serial=0; const temp=()=>`g${++serial}`;
    const fail=n=>{throw new Error(name+': unsupported '+n.type+' '+source.slice(n.start,n.end).slice(0,160));};
    const iife=(body,ret)=>`([&]() -> JSValue { ${body} return ${ret}; }())`;
    const functions=[], sources=[];
    const slice=n=>{const k='source'+sources.length;sources.push([k,source.slice(n.start,n.end)]);return k;};
    function patternNames(n) {
        if(n.type==='Identifier')return[n.name];
        if(n.type==='AssignmentPattern')return patternNames(n.left);
        if(n.type==='ObjectPattern')return n.properties.flatMap(p=>patternNames(p.value));
        fail(n);
    }
    function declarePattern(n,mutable=true) {return patternNames(n).map(s=>`declare(env,${q(s)},${mutable});`).join('');}
    function bind(n,value) {
        if(n.type==='Identifier')return `initialize(env,${q(n.name)},${value});`;
        if(n.type==='AssignmentPattern') {
            const v=temp();return `auto ${v}=${value};if(${v}.isUndefined())${v}=${expr(n.right)};`+bind(n.left,v);
        }
        if(n.type==='ObjectPattern') {
            const v=temp();return `const auto ${v}=${value};`+n.properties.map(p=>bind(p.value,`${v}.get(${p.computed?expr(p.key):q(p.key.name??p.key.value)})`)).join('');
        }
        fail(n);
    }
    function prelude(statements) {
        let result='';
        for(const s of statements) {
            if(s.type==='VariableDeclaration')for(const d of s.declarations)result+=declarePattern(d.id,s.kind!=='const');
            if(s.type==='FunctionDeclaration')result+=`declare(env,${q(s.id.name)},true);`;
        }
        for(const s of statements)if(s.type==='FunctionDeclaration')result+=`initialize(env,${q(s.id.name)},${makeFunction(s,'function')});`;
        return result;
    }
    function makeFunction(n,kind='function',arityOverride) {
        const id='function'+functions.length, text=slice(n);functions.push({id,body:''});
        const record=functions.at(-1);
        const parameterNames=n.params.flatMap(patternNames);
        let body='auto env=scope(captures[0]);auto home=captures[1];';
        body+=`declare(env,"$this",false);${kind==='constructor'?'':`initialize(env,"$this",receiver);`}`;
        if(kind==='constructor')body+='declare(env,"$newTarget",false);initialize(env,"$newTarget",receiver);';
        for(const p of n.params)body+=declarePattern(p);
        n.params.forEach((p,i)=>body+=bind(p,`arg(args,${i})`));
        if(n.body.type==='BlockStatement')body+=block(n.body,kind);
        else body+='return '+expr(n.body)+';';
        body+=kind==='constructor'?'return read(env,"$this");':'return {};';
        record.body=body;
        const arity=arityOverride??n.params.findIndex(p=>p.type==='AssignmentPattern');
        const length=arity===-1?n.params.length:arity;
        if(kind==='constructor')return {id,text,length};
        const v=temp();return iife(`auto ${v}=JSValue::${kind==='method'?'function':'constructor'}(${id},{env,home},${q(n.id?.name??'')},${length}${kind==='method'?'':',JSValue::ConstructorKind::Callable'});${v}.setFunctionSourceText(${text});`,v);
    }
    function call(n,construct=false) {
        const fn=temp(),receiver=temp(),argv=temp();let body='';
        if(n.callee.type==='Super') {
            body+=`const auto ${fn}=home.getPrototype();Args ${argv};`;
        } else if(n.callee.type==='MemberExpression') {
            const member=n.callee;
            body+=`const auto ${receiver}=${member.object.type==='Super'?'read(env,"$this")':expr(member.object)};`;
            body+=`const auto ${fn}=${member.object.type==='Super'?'home.getPrototype()':receiver}.get(${member.computed?expr(member.property):q(member.property.name)}${member.object.type==='Super'?','+receiver:''});Args ${argv};`;
        } else body+=`const auto ${fn}=${expr(n.callee)};Args ${argv};`;
        for(const a of n.arguments)body+=a.type==='SpreadElement'?`spread(${argv},${expr(a.argument)});`:`${argv}.push_back(${expr(a)});`;
        if(n.callee.type==='Super')return iife(body+`const auto instance=${fn}.construct(${argv},read(env,"$newTarget"));initialize(env,"$this",instance);`,'instance');
        return iife(body,construct?`${fn}.construct(${argv})`:`${fn}.call(${n.callee.type==='MemberExpression'?receiver:'JSValue{}'},${argv})`);
    }
    function expr(n) {
        switch(n.type) {
        case 'Identifier':return n.name==='undefined'?'JSValue{}':`read(env,${q(n.name)})`;
        case 'Literal':return n.value===null?'JSValue(nullptr)':typeof n.value==='number'?`JSValue(${Number.isInteger(n.value)?n.value+'.0':n.value})`:`JSValue(${q(n.value)})`;
        case 'ThisExpression':return 'read(env,"$this")';
        case 'MemberExpression': {
            const a=temp(),k=temp();return iife(`const auto ${a}=${n.object.type==='Super'?'home.getPrototype()':expr(n.object)};const auto ${k}=${n.computed?expr(n.property):`JSValue(${q(n.property.name)})`};`,`${a}.get(${k}${n.object.type==='Super'?',read(env,"$this")':''})`);
        }
        case 'CallExpression':return call(n);
        case 'NewExpression':return call(n,true);
        case 'BinaryExpression': {
            const a=temp(),b=temp();return iife(`const auto ${a}=${expr(n.left)};const auto ${b}=${expr(n.right)};`,`binary(${q(n.operator)},${a},${b})`);
        }
        case 'LogicalExpression': {
            const a=temp();return iife(`const auto ${a}=${expr(n.left)};if(${n.operator==='&&'?'!'+a+'.toBoolean()':n.operator==='||'?a+'.toBoolean()':`!${a}.isNull()&&!${a}.isUndefined()`})return ${a};`,expr(n.right));
        }
        case 'ConditionalExpression':return `(${expr(n.test)}.toBoolean()?${expr(n.consequent)}:${expr(n.alternate)})`;
        case 'UnaryExpression': {
            const a=temp();if(n.operator==='!')return `JSValue(!${expr(n.argument)}.toBoolean())`;
            if(n.operator==='-'||n.operator==='+')return `JSValue(${n.operator}${expr(n.argument)}.toNumber())`;
            if(n.operator==='typeof')return iife(`const auto ${a}=${expr(n.argument)};`,`JSValue(${a}.isUndefined()?"undefined":${a}.isCallable()?"function":${a}.isSymbol()?"symbol":${a}.type()==JSValue::Type::String?"string":${a}.type()==JSValue::Type::Number?"number":${a}.type()==JSValue::Type::Boolean?"boolean":"object")`);
            if(n.operator==='delete'&&n.argument.type==='MemberExpression')return iife(`auto ${a}=${expr(n.argument.object)};`,`deleteReference(${a},${n.argument.computed?expr(n.argument.property):q(n.argument.property.name)})`);
            fail(n);
        }
        case 'AssignmentExpression': {
            const a=temp(),key=temp(),value=temp();let body='',target;
            if(n.left.type==='Identifier')target=`write(env,${q(n.left.name)},${value})`;
            else if(n.left.type==='MemberExpression') {
                body+=`auto ${a}=${expr(n.left.object)};const auto ${key}=${n.left.computed?expr(n.left.property):`JSValue(${q(n.left.property.name)})`};`;
                target=`setReference(${a},${key},${value})`;
            }else fail(n.left);
            if(n.operator==='=')body+=`const auto ${value}=${expr(n.right)};`;
            else {
                const old=temp(),rhs=temp();body+=`const auto ${old}=${n.left.type==='Identifier'?expr(n.left):`${a}.get(${key})`};const auto ${rhs}=${expr(n.right)};const auto ${value}=binary(${q(n.operator.slice(0,-1))},${old},${rhs});`;
            }
            return iife(body,target);
        }
        case 'UpdateExpression': {
            const old=temp(),value=temp(),a=temp(),key=temp();let body='',target;
            if(n.argument.type==='Identifier')target=`write(env,${q(n.argument.name)},${value})`;
            else if(n.argument.type==='MemberExpression') {
                body+=`auto ${a}=${expr(n.argument.object)};const auto ${key}=${n.argument.computed?expr(n.argument.property):`JSValue(${q(n.argument.property.name)})`};`;
                target=`setReference(${a},${key},${value})`;
            }else fail(n.argument);
            body+=`const auto ${old}=${n.argument.type==='Identifier'?expr(n.argument):`${a}.get(${key})`}.toNumber();const JSValue ${value}(${old}${n.operator==='++'?'+':'-'}1);${target};`;
            return iife(body,n.prefix?value:`JSValue(${old})`);
        }
        case 'ObjectExpression': {
            const v=temp();let body=`auto ${v}=JSValue::object();`;
            for(const p of n.properties) {
                if(p.type==='SpreadElement'){body+=`copyProperties(${v},${expr(p.argument)});`;continue;}
                if(p.kind!=='init')fail(p);
                if(p.value.type==='FunctionExpression'&&!p.value.id&&!p.computed)p.value.id={type:'Identifier',name:p.key.name??String(p.key.value)};
                const k=temp();body+=`const auto ${k}=${p.computed?expr(p.key):`JSValue(${q(p.key.name??p.key.value)})`};own(${v},${k},${expr(p.value)});`;
            }
            return iife(body,v);
        }
        case 'ArrayExpression': {
            const v=temp();let body=`auto ${v}=JSValue::array(${n.elements.length});`;
            n.elements.forEach((e,i)=>{if(e)body+=`own(${v},JSValue(${i}.0),${expr(e)});`;});return iife(body,v);
        }
        case 'FunctionExpression':return makeFunction(n);
        case 'SequenceExpression':return iife(n.expressions.slice(0,-1).map(e=>expr(e)+';').join(''),expr(n.expressions.at(-1)));
        case 'TemplateLiteral': {
            const v=temp();let body=`std::string ${v}=${q(n.quasis[0].value.cooked)};`;
            n.expressions.forEach((e,i)=>body+=`${v}+=${expr(e)}.toString();${v}+=${q(n.quasis[i+1].value.cooked)};`);return iife(body,`JSValue(${v})`);
        }
        default:fail(n);
        }
    }
    function block(n,kind='method') {
        const parent=temp();return `{auto ${parent}=env;auto env=scope(${parent});${prelude(n.body)}${n.body.map(s=>stmt(s,kind)).join('\n')}}`;
    }
    function stmt(n,kind='method') {
        switch(n.type) {
        case 'BlockStatement':return block(n,kind);
        case 'FunctionDeclaration':return '';
        case 'ExpressionStatement':return expr(n.expression)+';';
        case 'VariableDeclaration':return n.declarations.map(d=>bind(d.id,d.init?expr(d.init):'JSValue{}')).join('');
        case 'ReturnStatement':return 'return '+(n.argument?expr(n.argument):kind==='constructor'?'read(env,"$this")':'JSValue{}')+';';
        case 'IfStatement':return `if(${expr(n.test)}.toBoolean()){${stmt(n.consequent,kind)}}${n.alternate?'else{'+stmt(n.alternate,kind)+'}':''}`;
        case 'ForStatement': {
            const parent=temp(),next=temp();let declarations='',initializer='',update='';const bindings=n.init?.type==='VariableDeclaration'?n.init.declarations.flatMap(d=>patternNames(d.id)):[];
            if(n.init?.type==='VariableDeclaration'){declarations=n.init.declarations.map(d=>declarePattern(d.id,n.init.kind!=='const')).join('');initializer=stmt(n.init,kind);}
            else if(n.init)initializer=expr(n.init)+';';
            if(bindings.length)update+=`auto ${next}=scope(${parent});`+bindings.map(s=>`declare(${next},${q(s)},true);initialize(${next},${q(s)},read(env,${q(s)}));`).join('')+`env=${next};`;
            if(n.update)update+=expr(n.update)+';';
            return `{auto ${parent}=env;auto env=scope(${parent});${declarations}${initializer}for(;${n.test?expr(n.test)+'.toBoolean()':'true'};[&](){${update}}()){${stmt(n.body,kind)}}}`;
        }
        case 'WhileStatement':return `while(${expr(n.test)}.toBoolean()){${stmt(n.body,kind)}}`;
        case 'ContinueStatement':return 'continue;';
        case 'BreakStatement':return 'break;';
        case 'SwitchStatement': {
            const v=temp(),index=temp(),parent=temp();let body=`{auto ${parent}=env;auto env=scope(${parent});${prelude(n.cases.flatMap(c=>c.consequent))}const auto ${v}=${expr(n.discriminant)};int ${index}=-1;`;
            for(const [i,c]of n.cases.entries())if(c.test)body+=`${i?'else ':''}if(${v}.strictEquals(${expr(c.test)}))${index}=${i};`;
            const fallback=n.cases.findIndex(c=>!c.test);if(fallback>=0)body+=`if(${index}<0)${index}=${fallback};`;
            body+=`switch(${index}){`+n.cases.map((c,i)=>`case ${i}:{${c.consequent.map(s=>stmt(s,kind)).join('')}}`).join('')+'}}';return body;
        }
        case 'ForOfStatement': {
            const it=temp(),value=temp(),parent=temp();if(n.left.type!=='VariableDeclaration'||n.left.declarations.length!==1)fail(n);
            const pattern=n.left.declarations[0].id;
            return `{auto ${it}=getJSIterator(${expr(n.right)});try{while(const auto ${value}=stepJSIterator(${it})){auto ${parent}=env;auto env=scope(${parent});${declarePattern(pattern,n.left.kind!=='const')}${bind(pattern,'*'+value)}${stmt(n.body,kind)}}}catch(...){if(!${it}.done)closeJSIterator(${it},std::current_exception());throw;}if(!${it}.done)closeJSIterator(${it});}`;
        }
        default:fail(n);
        }
    }

const imports={
BackSide:['constants.h','JSValue(BackSide)'],FrontSide:['constants.h','JSValue(FrontSide)'],SRGBTransfer:['constants.h','JSValue(SRGBTransfer)'],
BoxGeometry:['geometries/BoxGeometry.h','nativeBoxGeometryConstructor()'],PlaneGeometry:['geometries/PlaneGeometry.h','nativePlaneGeometryConstructor()'],
ShaderMaterial:['materials/ShaderMaterial.h','nativeShaderMaterialConstructor()'],Color:['math/Color.h','nativeColorConstructor()'],
ColorManagement:['math/ColorManagement.h','nativeColorManagement()'],Matrix3:['math/Matrix3.h','nativeMatrix3Constructor()'],
Matrix4:['math/Matrix4.h','nativeMatrix4Constructor()'],Mesh:['objects/Mesh.h','nativeMeshConstructor()'],
ShaderLib:['renderers/shaders/ShaderLib.h','nativeShaderLib()'],cloneUniforms:['renderers/shaders/UniformsUtils.h','nativeCloneUniforms()'],
getUnlitUniformColorSpace:['renderers/shaders/UniformsUtils.h','nativeGetUnlitUniformColorSpace()']};
const includes=new Set(['renderers/webgl/WebGLBackground.h','geometries/GeometryRuntime.h']);const importLines=[];
for(const decl of ast.body.filter(n=>n.type==='ImportDeclaration'))for(const spec of decl.specifiers){const actual=spec.imported?.name??spec.local.name;const pair=imports[actual];if(!pair)throw new Error('Missing background import '+actual);includes.add(pair[0]);importLines.push(`bindImport(env,${q(spec.local.name)},${pair[1]});`);}
const top=ast.body.filter(n=>['VariableDeclaration','FunctionDeclaration','ExpressionStatement'].includes(n.type));
const declarations=prelude(top),initializers=top.map(n=>stmt(n)).join('');
let output='// Generated by utils/generate-webgl-background.mjs. See THREEJS-LICENSE.\n#ifdef _MSC_VER\n#pragma warning(push)\n#pragma warning(disable:4456)\n#endif\n'+[...includes].map(i=>'#include "'+i+'"').join('\n')+'\nnamespace WebGLBackgroundGenerated{using namespace GeometryRuntime;\n';
output+=sources.map(([k,v])=>`inline constexpr char ${k}[]=${q(v)};`).join('\n')+'\n';
output+=functions.map(f=>`JSValue ${f.id}(const JSValue&,const Args&,const Args&);`).join('\n')+'\n';
output+=functions.map(f=>`JSValue ${f.id}(const JSValue& receiver,const Args& args,const Args& captures){${f.body}}`).join('\n')+'\n}\n';
output+=`JSValue nativeWebGLBackgroundConstructor(){using namespace WebGLBackgroundGenerated;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isCallable())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope guard(initializing,"WebGLBackground");auto env=scope();JSValue home;${importLines.join('')}${declarations}${initializers}auto ctor=read(env,"WebGLBackground");ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;}\n`;
output+='WebGLBackground::WebGLBackground(JSValue renderer,JSValue environments,JSValue state,JSValue objects,JSValue alpha,JSValue premultipliedAlpha):object_(nativeWebGLBackgroundConstructor().construct({renderer,environments,state,objects,alpha,premultipliedAlpha})){}\n';
output+='JSValue WebGLBackground::getClearColor()const{return object_.get("getClearColor").call(object_); }\nJSValue WebGLBackground::getClearAlpha()const{return object_.get("getClearAlpha").call(object_); }\n';
for(const [method,args]of [['setClearColor',['color','alpha']],['setClearAlpha',['alpha']],['render',['scene']],['addToRenderList',['renderList','scene']],['dispose',[]]])output+=`JSValue WebGLBackground::${method}(${args.map(a=>'JSValue '+a).join(',')}){return object_.get(${q(method)}).call(object_,{${args.join(',')}});}\n`;
output+='#ifdef _MSC_VER\n#pragma warning(pop)\n#endif\n';
const file=path.join(repo,'src/renderers/webgl/WebGLBackground.cpp');if(!fs.existsSync(file)||fs.readFileSync(file,'utf8')!==output)fs.writeFileSync(file,output);
console.log(name,functions.length,'native functions',createHash('sha256').update(source).digest('hex'));
