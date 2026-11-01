import path from 'node:path';
import {pathToFileURL} from 'node:url';
import assert from 'node:assert/strict';
const [upstream]=process.argv.slice(2);assert(upstream);assert.equal(process.version,'v26.5.0');
let randomCalls=0,randomSeed=0x12345678;
Math.random=()=>{randomCalls++;randomSeed=(Math.imul(randomSeed,1664525)+1013904223)>>>0;return randomSeed/4294967296;};
const load=f=>import(pathToFileURL(path.join(upstream,'src',f+'.js')).href);
await load('nodes/tsl/TSLCore');await load('nodes/math/MathNode');
const {default:Node}=await load('nodes/core/Node');
const before=new Node(),randomBefore=randomCalls;
const module=await load('nodes/display/BlendModes'),after=new Node();
const names=['blendBurn','blendDodge','blendScreen','blendOverlay','blendColor'];
const result={beforeID:before.id,afterID:after.id,allocatedNodes:after.id-before.id-1,randomBefore,randomAfter:randomCalls,
 exports:names.map(name=>({name,id:module[name].id,shaderID:module[name].shaderNode.id,layout:module[name].shaderNode.layout}))};
assert.equal(result.allocatedNodes,10);assert.equal(new Set(result.exports.flatMap(r=>[r.id,r.shaderID])).size,10);
process.stdout.write(JSON.stringify(result));
