// Development metadata extraction only; the generated source text is never executed.
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const args=process.argv.slice(2),check=args.includes('--check'),upstream=path.resolve(args.find(x=>x!=='--check')??'../three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw new Error('Unexpected upstream checkout');
const require=createRequire(path.join(upstream,'package.json')),acorn=require('acorn'),q=x=>JSON.stringify(x);
function source(name){const text=fs.readFileSync(path.join(upstream,'src/nodes/accessors/'+name+'.js'),'utf8');return {text,ast:acorn.parse(text,{ecmaVersion:'latest',sourceType:'module'})};}
const cube=source('CubeTextureNode'),material=source('MaterialProperties'),cls=cube.ast.body.find(n=>n.type==='ClassDeclaration'),actualCube=await import(pathToFileURL(path.join(upstream,'src/nodes/accessors/CubeTextureNode.js')));
const cubeLines=['// Exact pinned-upstream source metadata, never executed; see THREEJS-LICENSE.','namespace CubeTextureNodeSource {',`inline constexpr const char* classSource=${q(cube.text.slice(cls.start,cls.end))};`];
for(const m of cls.body.body.filter(n=>n.type==='MethodDefinition'&&n.kind!=='constructor'))cubeLines.push(`inline constexpr const char* ${m.key.name}Source=${q(cube.text.slice(m.start,m.end).replace(/^static\s+/,''))};`);
for(const e of cube.ast.body.filter(n=>n.type==='ExportNamedDeclaration'))for(const d of e.declaration.declarations)if(d.init.type==='ArrowFunctionExpression')cubeLines.push(`inline constexpr const char* ${d.id.name}Source=${q(cube.text.slice(d.init.start,d.init.end))};`);
cubeLines.push(`inline constexpr const char* cubeTextureBaseSource=${q(Function.prototype.toString.call(actualCube.cubeTextureBase))};`,'}');
const materialLines=['// Exact pinned-upstream callback metadata, never executed; see THREEJS-LICENSE.','namespace MaterialPropertiesSource {'];
for(const e of material.ast.body.filter(n=>n.type==='ExportNamedDeclaration'))for(const d of e.declaration.declarations){const update=d.init.arguments[0],reference=d.init.callee.object.arguments[0];for(const [key,n]of [['reference',reference],['update',update]])materialLines.push(`inline constexpr const char* ${d.id.name}_${key}=${q(material.text.slice(n.start,n.end))};`);}
materialLines.push('}');
const matrixText=fs.readFileSync(path.join(upstream,'src/math/Matrix4.js'),'utf8'),matrixAst=acorn.parse(matrixText,{ecmaVersion:'latest',sourceType:'module'}),matrixMethod=matrixAst.body.find(n=>n.type==='ClassDeclaration').body.body.find(n=>n.key?.name==='makeRotationFromEuler');
const matrixLines=['// Exact pinned-upstream function metadata; never executed; see THREEJS-LICENSE.',`inline constexpr const char* matrix4RotationSource=${q(matrixText.slice(matrixMethod.start,matrixMethod.end))};`];
for(const [name,folder,lines]of [['CubeTextureNode','nodes/accessors',cubeLines],['MaterialProperties','nodes/accessors',materialLines],['Matrix4Rotation','math',matrixLines]]){const target=new URL('../src/'+folder+'/'+name+'Source.inc',import.meta.url),result=lines.join('\n')+'\n';if(check){if(fs.readFileSync(target,'utf8').replaceAll('\r\n','\n')!==result)throw new Error('Stale '+name+' metadata');}else fs.writeFileSync(target,result);}
console.log('CubeTextureNode and MaterialProperties exact source metadata verified');
