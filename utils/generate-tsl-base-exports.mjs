import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {fileURLToPath,pathToFileURL} from 'node:url';

// Development-only inventory. Generated C++ contains binding names, never JS.
const root=fileURLToPath(new URL('../',import.meta.url));
const upstream=path.resolve(process.argv[2]??'../three.js');
const check=process.argv.includes('--check');
const pin=JSON.parse(fs.readFileSync(path.join(root,'upstream.json'),'utf8'));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),pin.commit);
const getters={
 './TSLCore.js':'nativeTSLCoreBinding',
 '../core/ArrayNode.js':'nativeArrayNodeBinding',
 '../core/UniformNode.js':'uniformBinding',
 '../core/PropertyNode.js':'nativePropertyNodeBinding',
 '../core/AssignNode.js':'nativeAssignNodeBinding',
 '../code/FunctionCallNode.js':'nativeFunctionCallNodeBinding',
 '../math/OperatorNode.js':'nativeOperatorNodeBinding',
 '../math/MathNode.js':'nativeMathNodeBinding',
 '../math/ConditionalNode.js':'nativeConditionalNodeBinding',
 '../core/ContextNode.js':'nativeContextNodeBinding',
 '../core/VarNode.js':'nativeVarNodeBinding',
 '../core/VaryingNode.js':'nativeVaryingNodeBinding',
 '../display/ColorSpaceNode.js':'nativeColorSpaceNodeBinding',
 '../display/ToneMappingNode.js':'nativeToneMappingNodeBinding',
 '../accessors/BufferAttributeNode.js':'nativeBufferAttributeNodeBinding',
 '../gpgpu/ComputeNode.js':'nativeComputeNodeBinding',
 '../core/IsolateNode.js':'nativeIsolateNodeBinding',
 '../core/BypassNode.js':'nativeBypassNodeBinding',
 '../utils/Remap.js':'nativeRemapBinding',
 '../code/ExpressionNode.js':'nativeExpressionNodeBinding',
 '../utils/Discard.js':'nativeDiscardBinding',
 '../display/RenderOutputNode.js':'nativeRenderOutputNodeBinding',
 '../utils/DebugNode.js':'nativeDebugNodeBinding',
 '../core/SubBuildNode.js':'nativeSubBuildNodeBinding',
 '../core/InspectorNode.js':'nativeInspectorNodeBinding'
};
const source=path.join(upstream,'src/nodes/tsl/TSLBase.js');
const declarations=[...fs.readFileSync(source,'utf8').matchAll(/export \* from '([^']+)'/g)].map(x=>x[1]);
assert.deepEqual(new Set(declarations),new Set(Object.keys(getters)));
const bindings=new Map();
const rows=[];
for(const declaration of declarations){
 const module=await import(pathToFileURL(path.resolve(path.dirname(source),declaration)));
 const names=Object.keys(module).filter(name=>name!=='default');
 for(const name of names){if(bindings.has(name))assert.equal(bindings.get(name),module[name],`Ambiguous star export ${name}`);bindings.set(name,module[name]);}
 rows.push(`merge(${getters[declaration]}, {${names.map(x=>JSON.stringify(x)).join(',')}});`);
}
const actual=await import(pathToFileURL(source));
assert.deepEqual([...bindings.keys(),'addNodeElement'].sort(),Object.keys(actual));
const generated=`// Generated from pinned three.js ${pin.commit} by utils/generate-tsl-base-exports.mjs.\n${rows.join('\n')}\n`;
const target=path.join(root,'src/nodes/tsl/TSLBaseExports.inc');
if(check)assert.equal(fs.readFileSync(target,'utf8').replaceAll('\r\n','\n'),generated);else fs.writeFileSync(target,generated);
console.log(`TSLBase: ${declarations.length} real source modules, ${Object.keys(actual).length} named exports ${check?'verified':'generated'}`);
