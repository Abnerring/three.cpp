import fs from 'node:fs';import path from 'node:path';import{pathToFileURL,fileURLToPath}from'node:url';import{spawnSync}from'node:child_process';
const upstream=process.env.THREEJS_SOURCE_DIR;if(!upstream)throw Error('THREEJS_SOURCE_DIR required');const modules=['materials/nodes/NodeMaterial','materials/Material','materials/MeshBasicMaterial','objects/Mesh','core/BufferGeometry','core/BufferAttribute','nodes/core/NodeBuilder','nodes/tsl/TSLBase','math/Color','math/Vector3','math/Plane','textures/Texture','textures/CubeTexture','utils'];
if(process.argv[2]==='--worker'){
 let randomCalls=0;Math.random=()=>++randomCalls%257/257;
 const count=Number(process.argv[3]);if(!Number.isInteger(count)||count<0||count>modules.length)throw Error('Bad prefix');
 for(const module of modules.slice(0,count))await import(pathToFileURL(path.join(upstream,'src',module+'.js')));
 const {default:Node}=await import(pathToFileURL(path.join(upstream,'src/nodes/core/Node.js')));const node=new Node();
 console.log(JSON.stringify({prefix:count,lastModule:count?modules[count-1]:null,nextNodeId:node.id,randomCalls}));
}else{
 const results=[];for(let count=0;count<=modules.length;++count){const processResult=spawnSync(process.execPath,[fileURLToPath(import.meta.url), '--worker',String(count)],{env:process.env,encoding:'utf8'});if(processResult.status!==0)throw Error(processResult.stderr);results.push(JSON.parse(processResult.stdout));}
 let nativeExecuted=false,nativeResults=null,differences=null;if(process.argv[3]){const {strictDifferences}=await import('./strict-parity.mjs');nativeResults=[];for(let count=0;count<=modules.length;++count){const result=spawnSync(process.argv[3],[String(count)],{env:process.env,encoding:'utf8'});if(!Number.isInteger(result.pid)||result.pid<=0||result.status!==0)throw Error('Native prefix '+count+' failed: '+result.stderr);nativeExecuted=true;nativeResults.push(JSON.parse(result.stdout));}differences=strictDifferences(results,nativeResults);if(differences.length)process.exitCode=1;}const output={node:process.version,upstream,results,nativeExecuted,nativeResults,differences};fs.writeFileSync(process.argv[2],JSON.stringify(output,null,2)+'\n');console.log(JSON.stringify(results));
}
