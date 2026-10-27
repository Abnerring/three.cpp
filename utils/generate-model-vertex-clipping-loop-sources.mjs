// Development-only source metadata; the resulting text is never executed.
import fs from 'node:fs';import path from 'node:path';import {createRequire} from 'node:module';import {execFileSync} from 'node:child_process';
const args=process.argv.slice(2),check=args.includes('--check'),upstream=path.resolve(args.find(x=>x!=='--check')??'../three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw new Error('Wrong upstream revision');
const require=createRequire(path.join(upstream,'package.json')),acorn=require('acorn');
for(const [name,folder]of [['ModelViewProjectionNode','accessors'],['VertexColorNode','accessors'],['ClippingNode','accessors'],['LoopNode','utils']]){
    const source=fs.readFileSync(path.join(upstream,`src/nodes/${folder}/${name}.js`),'utf8'),ast=acorn.parse(source,{ecmaVersion:'latest',sourceType:'module'}),out=['// Exact pinned-upstream function metadata; never executed; see THREEJS-LICENSE.',`namespace ${name}Source {`];
    const cls=ast.body.find(n=>n.type==='ClassDeclaration');if(cls){out.push(`inline constexpr const char* classSource=${JSON.stringify(source.slice(cls.start,cls.end))};`);for(const m of cls.body.body.filter(n=>n.type==='MethodDefinition'&&n.kind!=='constructor'))out.push(`inline constexpr const char* ${m.key.name}Source=${JSON.stringify(source.slice(m.start,m.end).replace(/^static\s+/,''))};`);}
    const arrows=[];function visit(n){if(!n||typeof n!=='object')return;if(n.type==='ArrowFunctionExpression')arrows.push(n);for(const [k,v]of Object.entries(n))if(k!=='start'&&k!=='end'){if(Array.isArray(v))for(const x of v)visit(x);else visit(v);}}visit(ast);arrows.sort((a,b)=>a.start-b.start);arrows.forEach((n,i)=>out.push(`inline constexpr const char* arrow${i}=${JSON.stringify(source.slice(n.start,n.end))};`));out.push('}');
    const text=out.join('\n')+'\n',target=new URL(`../src/nodes/${folder}/${name}Source.inc`,import.meta.url);if(check){if(fs.readFileSync(target,'utf8').replaceAll('\r\n','\n')!==text)throw new Error('Stale '+name);}else fs.writeFileSync(target,text);
    console.log(name+': '+arrows.map((n,i)=>`${i}:${source.slice(n.start,Math.min(n.end,n.start+38)).replaceAll('\r','').replaceAll('\n',' ')}`).join(' | '));
}
