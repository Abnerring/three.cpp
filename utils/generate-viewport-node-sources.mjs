// Development-time source metadata only; emitted strings are never executed.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
const args=process.argv.slice(2),check=args.includes('--check'),upstream=path.resolve(args.find(x=>x!=='--check')??'../three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw new Error('Unexpected upstream');
const require=createRequire(path.join(upstream,'package.json')),acorn=require('acorn'),lines=['// Source metadata from pinned three.js; see THREEJS-LICENSE.'];
for(const name of ['ViewportTextureNode','ViewportDepthTextureNode','ViewportDepthNode']){
 const text=fs.readFileSync(path.join(upstream,`src/nodes/display/${name}.js`),'utf8'),ast=acorn.parse(text,{ecmaVersion:'latest',sourceType:'module'}),cls=ast.body.find(n=>n.type==='ClassDeclaration');
 lines.push(`namespace ${name}Source {`);
 const emit=(key,source)=>lines.push(`constexpr const char* ${key}=${JSON.stringify(source)};`);
 emit('classSource',text.slice(cls.start,cls.end));
 for(const m of cls.body.body.filter(n=>n.type==='MethodDefinition'&&n.kind!=='constructor'))emit(m.key.name+'Source',text.slice(m.start,m.end).replace(/^static\s+/,''));
 for(const n of ast.body){if(n.type==='ExportNamedDeclaration'&&n.declaration?.type==='VariableDeclaration')for(const d of n.declaration.declarations){if(d.init?.type==='ArrowFunctionExpression')emit(d.id.name+'Source',text.slice(d.init.start,d.init.end));if(d.init?.type==='CallExpression'&&d.init.callee.name==='Fn')emit(d.id.name+'CallbackSource',text.slice(d.init.arguments[0].start,d.init.arguments[0].end));}if(n.type==='ExpressionStatement'&&n.expression.type==='AssignmentExpression'&&n.expression.left.type==='MemberExpression'&&n.expression.left.object.name==='depth'&&n.expression.left.property.name==='assign')emit('depthAssignSource',text.slice(n.expression.right.start,n.expression.right.end));}
 lines.push('}');
}
const result=lines.join('\n')+'\n',output=new URL('../src/nodes/display/ViewportNodeSources.inc',import.meta.url);if(check){if(fs.readFileSync(output,'utf8').replaceAll('\r\n','\n')!==result)throw new Error('Stale Viewport source metadata');}else fs.writeFileSync(output,result);console.log('Viewport source metadata verified');
