// Exact source metadata only. Production never executes these strings.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const args=process.argv.slice(2),check=args.includes('--check'),upstream=path.resolve(args.find(value=>value!=='--check')??'D:/project/DX/GitHub/three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw Error('Unexpected upstream checkout');
const require=createRequire(path.join(upstream,'package.json')),{parse}=require('acorn'),root=fileURLToPath(new URL('../',import.meta.url));
const arity=params=>{const index=params.findIndex(value=>value.type==='AssignmentPattern'||value.type==='RestElement');return index<0?params.length:index;};
let count=0;
for(const name of ['Frustum','FrustumArray']){
 const source=fs.readFileSync(path.join(upstream,'src/math',name+'.js'),'utf8'),tree=parse(source,{ecmaVersion:'latest',sourceType:'module'}),cls=tree.body.find(value=>value.type==='ClassDeclaration');
 const lines=['// Pinned three.js source metadata; see THREEJS-LICENSE.',`namespace ${name}Source {`,`constexpr const char* classSource=${JSON.stringify(source.slice(cls.start,cls.end))};`,'struct Method { const char* name; unsigned arity; const char* source; };','constexpr Method methods[]={'];
 for(const method of cls.body.body){lines.push(`{${JSON.stringify(method.key.name)},${arity(method.value.params)},${JSON.stringify(source.slice(method.start,method.end))}},`);++count;}
 lines.push('};','}');const text=lines.join('\n')+'\n',output=path.join(root,'src/math',name+'Source.inc');
 if(check){if(fs.readFileSync(output,'utf8').replaceAll('\r\n','\n')!==text)throw Error('Stale metadata '+output);}else{fs.mkdirSync(path.dirname(output),{recursive:true});fs.writeFileSync(output,text);}
}
console.log(`Frustum source metadata: ${count+2} exact source slices ${check?'verified':'generated'}`);
