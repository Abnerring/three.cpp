// Compile the pinned Curve algorithms into native C++ at development time.
// No source or AST is interpreted by the library.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';

const [upstreamArg] = process.argv.slice(2);
if (!upstreamArg) throw new Error('Usage: generate-curve-runtime.mjs pinned-threejs');
const upstream = path.resolve(upstreamArg), repo = path.resolve(import.meta.dirname, '..');
const lock = JSON.parse(fs.readFileSync(path.join(repo, 'upstream.json')));
if (execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding:'utf8'}).trim() !== lock.commit)
    throw new Error('Unexpected upstream commit');
const {parse} = createRequire(path.join(upstream, 'package.json'))('acorn');
const q = JSON.stringify;
const names = ['Curve'];
const knownImports = {
    clamp:['math/MathUtils.h','nativeMathUtilsClamp()'],
    Vector2:['math/Vector2.h','nativeVector2Constructor()'],
    Vector3:['math/Vector3.h','nativeVector3Constructor()'],
    Matrix4:['math/Matrix4.h','nativeMatrix4Constructor()'],
    warn:['utils.h','ThreeUtils::nativeWarnFunction()']
};

for (const name of names) {
    const source = fs.readFileSync(path.join(upstream, 'src/extras/core', name+'.js'), 'utf8');
    const ast = parse(source, {sourceType:'module',ecmaVersion:'latest'});
    const cls = ast.body.find(n=>n.type==='ClassDeclaration');
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
    const includes = new Set(['extras/core/CurveRuntime.h','geometries/GeometryRuntime.h']);
    const importLines=[];
    for(const n of ast.body.filter(n=>n.type==='ImportDeclaration'))for(const s of n.specifiers) {
        const actual=s.imported?.name??s.local.name, info=knownImports[actual];
        if(!info)throw new Error('Missing Curve import '+actual);
        includes.add(info[0]);importLines.push(`bindImport(env,${q(s.local.name)},${info[1]});`);
    }
    const className=cls.id.name, methodLines=[];
    const ctor=cls.body.body.find(n=>n.kind==='constructor');
    // The ordinary constructor intrinsic allocates the base receiver, including
    // GetPrototypeFromConstructor fallback. Its callback then runs the source
    // statements, without an extra observable newTarget.prototype read.
    const constructorDelegate=makeFunction(ctor.value,'baseConstructor');
    for(const m of cls.body.body.filter(n=>n.kind!=='constructor')) {
        m.value.id={type:'Identifier',name:m.key.name};
        const rec=makeFunction(m.value,'method'),metadata=slice(m.static?{...m,start:m.key.start}:m);
        methodLines.push(`{auto home=prototype;auto function=${rec};function.setFunctionSourceText(${metadata});JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;home.defineProperty(${q(m.key.name)},d);}`);
    }
    const namespace='CurveManagedGenerated';
    const output='// Generated from the pinned Curve.js. No source or AST interpreter.\n#ifdef _MSC_VER\n#pragma warning(push)\n#pragma warning(disable:4456)\n#endif\n'+[...includes].map(i=>'#include "'+i+'"').join('\n')+`\nnamespace ${namespace}{using namespace GeometryRuntime;\n`+
        sources.map(([k,v])=>`inline constexpr char ${k}[]=${q(v)};`).join('\n')+'\n'+
        functions.map(f=>`JSValue ${f.id}(const JSValue&,const Args&,const Args&);`).join('\n')+'\n'+
        functions.map(f=>`JSValue ${f.id}(const JSValue& receiver,const Args& args,const Args& captures){${f.body}}`).join('\n')+`\n}\n`+
        `void installNativeCurveNumericMethods(JSValue constructor,JSValue curveClassTable){using namespace ${namespace};auto env=scope();auto prototype=constructor.get("prototype");JSValue home=prototype;${importLines.join('')}bindImport(env,"Curve",constructor);bindImport(env,"CurveClasses",curveClassTable);constructor.setInternalSlot("nativeCurveConstructor",${constructorDelegate});for(const auto& key:prototype.ownPropertyKeys()){if(!key.strictEquals("constructor"))prototype.erase(key);}${methodLines.join('')}}\n`+
        '#ifdef _MSC_VER\n#pragma warning(pop)\n#endif\n';
    const file=path.join(repo,'src/extras/core/CurveRuntime.cpp');
    if(!fs.existsSync(file)||fs.readFileSync(file,'utf8')!==output)fs.writeFileSync(file,output);
    console.log(name,functions.length,'native functions',createHash('sha256').update(source).digest('hex'));
}
