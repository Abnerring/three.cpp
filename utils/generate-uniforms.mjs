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
const ast=parse(fs.readFileSync(path.join(upstream,'src/renderers/shaders/UniformsLib.js'),'utf8'),{ecmaVersion:'latest',sourceType:'module'});
const declaration=ast.body.flatMap(n=>n.type==='VariableDeclaration'?n.declarations:[]).find(n=>n.id.name==='UniformsLib');
assert.ok(declaration);
function expression(n,depth=0){
    const indent='    '.repeat(depth);
    if(n.type==='Literal'){
        assert.ok(n.value===null||typeof n.value==='number'||typeof n.value==='boolean'||typeof n.value==='string');
        return n.value===null?'nullptr':JSON.stringify(n.value);
    }
    if(n.type==='ArrayExpression'){
        assert.equal(n.elements.length,0);
        return 'JSValue::array()';
    }
    if(n.type==='ObjectExpression')return 'object({\n'+n.properties.map(p=>{
        assert.equal(p.type,'Property');assert.equal(p.kind,'init');assert.equal(p.computed,false);
        return indent+'    {'+JSON.stringify(p.key.name??p.key.value)+', '+expression(p.value,depth+1)+'}';
    }).join(',\n')+'\n'+indent+'})';
    if(n.type==='NewExpression'){
        const constructor=n.callee.name;
        const wrap={Color:'nativeColor',Vector2:'nativeVector',Vector3:'nativeVector',Matrix3:'nativeMatrix'}[constructor];
        assert.ok(wrap,`Unsupported uniform constructor: ${constructor}`);
        return `${wrap}(std::make_shared<${constructor}>(${n.arguments.map(a=>expression(a,depth)).join(', ')}))`;
    }
    throw Error(`Unsupported uniform initializer: ${n.type}`);
}
const output=`// Generated from the pinned three.js UniformsLib.js by utils/generate-uniforms.mjs.
#include "renderers/shaders/UniformsLib.h"
#include "core/NativeObject.h"
#include "math/Color.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Matrix3.h"
#include <initializer_list>
#include <utility>

namespace {
JSValue object(std::initializer_list<std::pair<const char*,JSValue>> fields) {
    auto result = JSValue::object();
    for (const auto& [name,value] : fields) result.set(name,value);
    return result;
}
}
thread_local JSValue UniformsLib = ${expression(declaration.init)};
`;
const destination=path.join(root,'src/renderers/shaders/UniformsLib.cpp');
if(process.argv.includes('--check'))assert.equal(fs.readFileSync(destination,'utf8').replaceAll('\r\n','\n'),output);
else fs.writeFileSync(destination,output);
console.log(`UniformsLib: ${declaration.init.properties.length} groups ${process.argv.includes('--check')?'verified':'generated'}`);
