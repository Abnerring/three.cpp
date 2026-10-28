// Source metadata only; these strings are never executed.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
const args=process.argv.slice(2),check=args.includes('--check'),upstream=path.resolve(args.find(a=>a!=='--check')??'../three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw Error('Unexpected upstream');
const require=createRequire(path.join(upstream,'package.json')),acorn=require('acorn');
const source=fs.readFileSync(path.join(upstream,'src/nodes/accessors/Batch.js'),'utf8'),ast=acorn.parse(source,{ecmaVersion:'latest',sourceType:'module'}),nodes=[];
function walk(n){if(!n||typeof n!=='object')return;if(n.type==='FunctionDeclaration'||n.type==='ArrowFunctionExpression')nodes.push(n);for(const v of Object.values(n)){if(Array.isArray(v))v.forEach(walk);else if(v&&typeof v==='object')walk(v);}}walk(ast);
let index=0;const lines=['// Pinned three.js source metadata; see THREEJS-LICENSE.','namespace BatchSource {'];
for(const n of nodes)lines.push(`constexpr const char* ${n.id?.name??'callback_'+index++}=${JSON.stringify(source.slice(n.start,n.end))};`);
lines.push('}');const result=lines.join('\n')+'\n',output=new URL('../src/nodes/accessors/BatchSources.inc',import.meta.url);
if(check){if(fs.readFileSync(output,'utf8').replaceAll('\r\n','\n')!==result)throw Error('Stale Batch metadata');}else fs.writeFileSync(output,result);
console.log(`Batch metadata: ${nodes.length} source slices verified`);
