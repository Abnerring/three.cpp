import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {fileURLToPath,pathToFileURL} from 'node:url';

export const upstreamCommit='11738a9d55e502fb484cefffcea3a66dc44e9fe9';
export const upstream=path.resolve(process.env.THREEJS_SOURCE_DIR??fileURLToPath(new URL('../../../../../three.js',import.meta.url)));
const actualCommit=execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim();
assert.equal(actualCommit,upstreamCommit,'THREEJS_SOURCE_DIR must identify the pinned three.js commit');
assert.equal(execFileSync('git',['-C',upstream,'status','--porcelain','--untracked-files=no','--','src'],{encoding:'utf8'}).trim(),'','Pinned three.js src must be clean');

export function upstreamModuleURL(relativePath){
 const filename=path.resolve(upstream,relativePath),relative=path.relative(upstream,filename);
 assert(relative!==''&&!relative.startsWith('..')&&!path.isAbsolute(relative),'Module must be within THREEJS_SOURCE_DIR');
 return pathToFileURL(filename).href;
}
export function upstreamModule(relativePath){return import(upstreamModuleURL(relativePath));}
