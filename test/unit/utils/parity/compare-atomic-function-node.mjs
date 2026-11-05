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

  source: AtomicFunctionNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('AtomicFunctionNode parity passed');
