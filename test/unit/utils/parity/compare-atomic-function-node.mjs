import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/gpgpu/AtomicFunctionNode.js')).href);
const AtomicFunctionNode = mod.default;

const pointer = () => ({ getNodeType: () => 'uint', build: () => 'ptr' });
const value = () => ({ build: () => 'val' });


const pointerRef = pointer();
const valueRef = value();
const atomic = new AtomicFunctionNode('atomicAdd', pointerRef, valueRef);
const builder = {};
const inputType = atomic.getInputType(builder);
const generatedType = atomic.generateNodeType(builder);


const constants = {};
for (const name of [ 'ATOMIC_LOAD', 'ATOMIC_STORE', 'ATOMIC_ADD', 'ATOMIC_SUB', 'ATOMIC_MAX',
  'ATOMIC_MIN', 'ATOMIC_AND', 'ATOMIC_OR', 'ATOMIC_XOR' ]) {
  constants[name] = AtomicFunctionNode[name];
}


// The module level factory builds a node through nodeProxy and calls toStack on it.
let atomicLoadBuilt = true;
let atomicLoadResult;
try {
  atomicLoadResult = mod.atomicLoad(pointerRef);
} catch {
  atomicLoadBuilt = false;
}
const atomicLoadIsNode = atomicLoadBuilt && atomicLoadResult.isNode === true;
const atomicLoadIsStackNode = atomicLoadBuilt && atomicLoadResult.isStackNode === true;
const makeGenerateBuilder = (parents, lines) => ({
  shaderStage: 'compute',
  getNodeProperties: () => ({ parents }),
  getDataFromNode: () => ({ nodeType: 'uint' }),
  getNodeType: () => 'uint',
  getMethod: (method) => method,
  getVarFromNode: () => ({}),
  getPropertyName: () => 'v0',
  generateLetStatement: (type, name) => `let ${type} ${name}`,
  addLineFlowCode: (snippet) => { lines.push(snippet); }
});
const stackParents = [ { isStackNode: true } ];
const voidLines = [];
const voidAtomic = new AtomicFunctionNode('atomicStore', pointerRef, valueRef);
const voidResult = voidAtomic.generate(makeGenerateBuilder(stackParents, voidLines));

const emptyParents = [];
const valueLines = [];
const valueAtomic = new AtomicFunctionNode('atomicAdd', pointerRef, valueRef);
const valueResult = valueAtomic.generate(makeGenerateBuilder(emptyParents, valueLines));
const expected = {
  name: AtomicFunctionNode.name,
  length: AtomicFunctionNode.length,
  staticType: AtomicFunctionNode.type,
  constants,
  type: atomic.type,
  nodeType: atomic.nodeType,
  method: atomic.method,
  pointerSame: atomic.pointerNode === pointerRef,
  valueSame: atomic.valueNode === valueRef,
  parents: atomic.parents,
  cacheable: atomic.isCacheable(),
  inputType,
  generatedType,
  generateLength: atomic.generate.length,
  generateExecuted: false,

  atomicFuncLength: mod.atomicFunc.length,
  atomicLoadLength: mod.atomicLoad.length,
  atomicAddLength: mod.atomicAdd.length,

  atomicLoadBuilt,
  atomicLoadIsNode,
  atomicLoadIsStackNode,
  voidResultUndefined: voidResult === undefined,
  voidLine: voidLines[0] ?? '',
  valueResult,
  valueLine: valueLines[0] ?? '',
  toStackCallable: typeof atomic.toStack === 'function',
  toStackReturnsSelf: typeof atomic.toStack === 'function' && atomic.toStack() === atomic,
  source: AtomicFunctionNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('AtomicFunctionNode parity passed');
