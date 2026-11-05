import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/gpgpu/ComputeBuiltinNode.js')).href);
const ComputeBuiltinNode = mod.default;

const formatted = { value: false };
const makeBuilder = (hasBuiltin) => ({
  shaderStage: 'compute',
  format: () => { formatted.value = true; return 'formatted'; },
  generateConst: () => 'const',
  getDataFromNode: () => ({}),
  getNodeType: () => 'uvec3',
  hasBuiltin: () => hasBuiltin
});

const builtin = new ComputeBuiltinNode('globalId', 'uvec3');
const cacheable = builtin.isCacheable();
const renamed = builtin.setBuiltinName('localId');
const renamedSame = renamed === builtin;
const nameAfterSet = builtin._builtinName;
builtin.setBuiltinName('globalId');

const builder = makeBuilder(true);
const hash = builtin.getHash(builder);
const hasBuiltinValue = builtin.hasBuiltin(builder);
const generated = builtin.generate(builder, 'uvec3');
const formatWasUsed = formatted.value;
const generatedType = builtin.generateNodeType();

const outputStage = {
  shaderStage: 'fragment',
  generateConst: () => 'const',
  getDataFromNode: () => ({}),
  getNodeType: () => 'uvec3'
};
const generatedOther = builtin.generate(outputStage, 'uvec3');

const data = {};
builtin.serialize(data);
const serializedGlobalIsUndefined = data.global === undefined;
const serializedBuiltin = data._builtinName;

const target = new ComputeBuiltinNode('localId', 'vec3');
target.deserialize({ global: true, _builtinName: 'globalId' });

const builtins = {};
for (const name of [ 'numWorkgroups', 'workgroupId', 'globalId', 'localId', 'subgroupSize' ]) {
  const entry = mod[name];
  builtins[name] = { builtinName: entry._builtinName, nodeType: entry.nodeType, isNode: entry.isNode };
}

const expected = {
  name: ComputeBuiltinNode.name,
  length: ComputeBuiltinNode.length,
  staticType: ComputeBuiltinNode.type,
  type: builtin.type,
  isNode: builtin.isNode,
  builtinName: builtin._builtinName,
  nodeType: builtin.nodeType,
  cacheable,
  renamedSame,
  nameAfterSet,
  hash,
  hasBuiltinValue,
  generated,
  formatWasUsed,
  generatedType,
  generatedOther,
  serializedBuiltin,
  serializedGlobalIsUndefined,
  deserializedBuiltin: target._builtinName,
  deserializedGlobal: target.global,
  builtins,
  constructionLength: builtin.setBuiltinName.length,
  source: ComputeBuiltinNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('ComputeBuiltinNode parity passed');
