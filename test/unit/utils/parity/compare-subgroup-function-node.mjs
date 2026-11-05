import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/gpgpu/SubgroupFunctionNode.js')).href);
const SubgroupFunctionNode = mod.default;

const nodeStub = (type, snippet) => ({
  getNodeType: () => type,
  build: (builder, buildType) => `${snippet}:${buildType}`
});

const typeBuilder = () => ({
  isMatrix: (type) => typeof type === 'string' && type.startsWith('mat'),
  getTypeLength: (type) => {
    if ([ 'float', 'int', 'uint', 'bool' ].includes(type)) return 1;
    const last = String(type).slice(-1);
    return '234'.includes(last) ? Number(last) : 0;
  }
});

const generateBuilder = (formatted) => ({
  shaderStage: 'compute',
  isMatrix: (type) => typeof type === 'string' && type.startsWith('mat'),
  getTypeLength: (type) => {
    if ([ 'float', 'int', 'uint', 'bool' ].includes(type)) return 1;
    const last = String(type).slice(-1);
    return '234'.includes(last) ? Number(last) : 0;
  },
  getNodeType: () => 'float',
  getDataFromNode: () => ({}),
  getMethod: (method) => method,
  format: (snippet) => { formatted.snippet = snippet; return snippet; }
});

const typeB = typeBuilder();
const floatNode = nodeStub('float', 'a');
const vec4Node = nodeStub('vec4', 'b');
const add = new SubgroupFunctionNode('subgroupAdd', floatNode, vec4Node);
const inputType = add.getInputType(typeB);
const generatedType = add.generateNodeType(typeB);
const matrixNode = nodeStub('mat4', 'm');
const mixed = new SubgroupFunctionNode('subgroupAdd', matrixNode, floatNode);
const mixedInputType = mixed.getInputType(typeB);
const elect = new SubgroupFunctionNode('subgroupElect', floatNode, null);
const electType = elect.generateNodeType(typeB);
const ballot = new SubgroupFunctionNode('subgroupBallot', floatNode, null);
const ballotType = ballot.generateNodeType(typeB);

const formatted = {};
const electNode = new SubgroupFunctionNode('subgroupElect', null, null);
const electSnippet = electNode.generate(generateBuilder(formatted), '');

const a = nodeStub('float', 'a');
const addNode = new SubgroupFunctionNode('subgroupAdd', a, null);
const addSnippet = addNode.generate(generateBuilder(formatted), '');

const bFloat = nodeStub('float', 'b');
const broadcastNode = new SubgroupFunctionNode('subgroupBroadcast', a, bFloat);
const broadcastSnippet = broadcastNode.generate(generateBuilder(formatted), '');

const shuffleNode = new SubgroupFunctionNode('subgroupShuffleXor', a, bFloat);
const shuffleSnippet = shuffleNode.generate(generateBuilder(formatted), '');

const data = {};
addNode.serialize(data);
const target = new SubgroupFunctionNode('subgroupAny');
target.deserialize({ method: 'subgroupBallot' });

const arityTable = {};
for (const name of [ 'subgroupElect', 'subgroupBallot', 'subgroupAdd', 'subgroupBroadcast',
  'subgroupShuffleDown', 'quadBroadcast' ]) {
  arityTable[name] = mod[name].length;
}
const constants = {};
for (const name of [ 'SUBGROUP_ELECT', 'SUBGROUP_BALLOT', 'SUBGROUP_ADD', 'SUBGROUP_INCLUSIVE_ADD',
  'SUBGROUP_EXCLUSIVE_ADD', 'SUBGROUP_MUL', 'SUBGROUP_INCLUSIVE_MUL', 'SUBGROUP_EXCLUSIVE_MUL',
  'SUBGROUP_AND', 'SUBGROUP_OR', 'SUBGROUP_XOR', 'SUBGROUP_MIN', 'SUBGROUP_MAX', 'SUBGROUP_ALL',
  'SUBGROUP_ANY', 'SUBGROUP_BROADCAST_FIRST', 'QUAD_SWAP_X', 'QUAD_SWAP_Y', 'QUAD_SWAP_DIAGONAL',
  'SUBGROUP_BROADCAST', 'SUBGROUP_SHUFFLE', 'SUBGROUP_SHUFFLE_XOR', 'SUBGROUP_SHUFFLE_UP',
  'SUBGROUP_SHUFFLE_DOWN', 'QUAD_BROADCAST' ]) {
  constants[name] = SubgroupFunctionNode[name];
}

const expected = {
  name: SubgroupFunctionNode.name,
  length: SubgroupFunctionNode.length,
  staticType: SubgroupFunctionNode.type,
  type: add.type,
  method: add.method,
  aNull: add.aNode === null,
  constants,
  inputType,
  generatedType,
  mixedInputType,
  electType,
  ballotType,
  electSnippet,
  addSnippet,
  broadcastSnippet,
  shuffleSnippet,
  serializedMethod: data.method,
  deserializedMethod: target.method,
  arityTable,
  electCallable: typeof mod.subgroupElect === 'function',
  source: SubgroupFunctionNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('SubgroupFunctionNode parity passed');
