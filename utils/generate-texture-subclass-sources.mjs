// Development-time source metadata only. The generated text is never executed.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
const args=process.argv.slice(2),check=args.includes('--check');
const upstream=path.resolve(args.find(x=>x!=='--check')??'../three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw new Error('Unexpected upstream checkout');
const require=createRequire(path.join(upstream,'package.json')),acorn=require('acorn');
for(const name of ['DepthTexture','FramebufferTexture']){
    const text=fs.readFileSync(path.join(upstream,`src/textures/${name}.js`),'utf8');
    const cls=acorn.parse(text,{ecmaVersion:'latest',sourceType:'module'}).body.find(n=>n.type==='ClassDeclaration');
    const lines=['// Source metadata from the pinned upstream; see THREEJS-LICENSE.'];
    lines.push(`constexpr const char* classSource=${JSON.stringify(text.slice(cls.start,cls.end))};`);
    for(const method of cls.body.body.filter(n=>n.type==='MethodDefinition'&&n.kind!=='constructor'))lines.push(`constexpr const char* ${method.key.name}Source=${JSON.stringify(text.slice(method.start,method.end))};`);
    const result=lines.join('\n')+'\n',file=new URL(`../src/textures/${name}Source.inc`,import.meta.url);
    if(check){if(fs.readFileSync(file,'utf8').replaceAll('\r\n','\n')!==result)throw new Error('Stale '+name+' source metadata');}else fs.writeFileSync(file,result);
}
console.log('DepthTexture/FramebufferTexture source metadata verified');
