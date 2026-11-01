import assert from 'node:assert/strict';import path from 'node:path';import{pathToFileURL}from'node:url';
const upstream=process.argv[2];assert.equal(typeof global.gc,'function');let randomCalls=0;Math.random=()=>((++randomCalls)%257)/257;
const load=async name=>import(pathToFileURL(path.join(upstream,'src',name+'.js'))),pmrem=await load('nodes/pmrem/PMREMUtils'),tsl=await load('nodes/tsl/TSLBase'),{default:Node}=await load('nodes/core/Node'),{default:StackNode}=await load('nodes/core/StackNode'),{CubeTexture}=await load('textures/CubeTexture'),{uniform}=await load('nodes/core/UniformNode'),{cubeTexture}=await load('nodes/accessors/CubeTextureNode');
const tick=()=>new Promise(resolve=>setImmediate(resolve));async function gone(refs){for(let i=0;i<80;i++){await tick();global.gc();await tick();if(refs.every(ref=>ref.deref()===undefined))return true;}return false;}
const observations=[];
for(let round=0;round<3;round++){
 const previous=tsl.getCurrentStack();let stack=new StackNode();tsl.setCurrentStack(stack);
 let direction=new Node('vec3'),directionProxy=tsl.nodeObject(direction),sigma=uniform(0.5),texture=new CubeTexture(),envMap=cubeTexture(texture),graph=pmrem.sphericalGaussianBlur.shaderNode.jsFunc({SAMPLES:tsl.int(4),sigma,direction:directionProxy,envMap});
 const refs=[direction,sigma,texture,envMap].map(value=>new WeakRef(value));let loop=stack.nodes.find(node=>node.type==='LoopNode'),held=loop.params[loop.params.length-1];assert.equal(held.isFn,true);tsl.setCurrentStack(previous);stack=direction=directionProxy=sigma=texture=envMap=graph=loop=null;
 global.gc();assert.ok(refs.every(ref=>ref.deref()!==undefined));observations.push({round,phase:'held-real-loop-callback',alive:true});held=null;assert.equal(await gone(refs),true);observations.push({round,phase:'dropped-real-loop-callback',gone:true});
}
process.stdout.write(JSON.stringify({rounds:3,observations}));
