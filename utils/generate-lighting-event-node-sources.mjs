// Development-time source metadata only. Strings are never executed.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
const args=process.argv.slice(2),check=args.includes('--check'),upstream=path.resolve(args.find(x=>x!=='--check')??'../three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw new Error('Unexpected upstream');
const require=createRequire(path.join(upstream,'package.json')),acorn=require('acorn'),lines=['// Source metadata from pinned three.js; see THREEJS-LICENSE.'];
for(const [folder,name] of [['lighting','LightingNode'],['lighting','LightingContextNode'],['lighting','AONode'],['lighting','IrradianceNode'],['utils','EventNode']]){
 const text=fs.readFileSync(path.join(upstream,`src/nodes/${folder}/${name}.js`),'utf8'),ast=acorn.parse(text,{ecmaVersion:'latest',sourceType:'module'}),cls=ast.body.find(n=>n.type==='ClassDeclaration');
 const emit=(key,source)=>lines.push(`constexpr const char* ${key}=${JSON.stringify(source)};`);
 lines.push(`namespace ${name}Source {`);emit('classSource',text.slice(cls.start,cls.end));
 for(const m of cls.body.body.filter(n=>n.type==='MethodDefinition'&&n.kind!=='constructor'))emit(m.key.name+'Source',text.slice(m.start,m.end).replace(/^static\s+/,''));
 for(const n of ast.body){const d=n.type==='ExportNamedDeclaration'?n.declaration:n;if(d?.type==='VariableDeclaration')for(const v of d.declarations)if(v.init?.type==='ArrowFunctionExpression')emit(v.id.name+'Source',text.slice(v.init.start,v.init.end));}
 lines.push('}');
}
const result=lines.join('\n')+'\n',output=new URL('../src/nodes/lighting/LightingEventNodeSources.inc',import.meta.url);
if(check){if(fs.readFileSync(output,'utf8').replaceAll('\r\n','\n')!==result)throw new Error('Stale lighting/event source metadata');}else fs.writeFileSync(output,result);
console.log('Lighting/event source metadata verified');
