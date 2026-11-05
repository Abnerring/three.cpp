import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const mod = await import(pathToFileURL(path.join(upstream, 'src/loaders/nodes/NodeMaterialLoader.js')).href);
const NodeMaterialLoader = mod.default;

const materialStub = () => class StubNodeMaterial {
  constructor() {
    this.type = 'StubNodeMaterial';
    this.isMaterial = true;
    this.isNodeMaterial = true;
  }
};

const loader = new NodeMaterialLoader({});
const nodes = {};
const setNodesSame = loader.setNodes(nodes) === loader;
const nodesStored = loader.nodes === nodes;

const registry = { StubNodeMaterial: materialStub() };
const setMaterialsSame = loader.setNodeMaterials(registry) === loader;
const materialsStored = loader.nodeMaterials === registry;

const created = loader.createMaterialFromType('StubNodeMaterial');
const createdType = created.type;
const createdIsNodeMaterial = created.isNodeMaterial;

// An unregistered type falls back to the base loader, which answers a plain material and only logs.
let unknownThrows = false;
let unknownResult;
try {
  unknownResult = loader.createMaterialFromType('DefinitelyNotAMaterialType');
} catch {
  unknownThrows = true;
}
const unknownIsMaterial = !unknownThrows && unknownResult.isMaterial === true;
const unknownType = unknownThrows ? '' : unknownResult.type;

const childNode = { isNode: true, type: 'StubNode' };
loader.setNodes({ 'node-uuid': childNode });
const json = {
  type: 'MeshBasicMaterial',
  uuid: 'material-uuid',
  inputNodes: { colorNode: 'node-uuid' }
};
let parseBuilt = true;
let material;
try {
  material = loader.parse(json);
} catch {
  parseBuilt = false;
}
const parsedIsMaterial = parseBuilt && material.isMaterial === true;
const parsedInputSame = parseBuilt && material.colorNode === childNode;

const arityTable = {};
for (const name of [ 'parse', 'setNodes', 'setNodeMaterials', 'createMaterialFromType' ]) {
  arityTable[name] = loader[name].length;
}

const expected = {
  name: NodeMaterialLoader.name,
  length: NodeMaterialLoader.length,
  nodesIsObject: typeof loader.nodes === 'object',
  nodeMaterialsIsObject: typeof loader.nodeMaterials === 'object',
  setNodesSame,
  nodesStored,
  setMaterialsSame,
  materialsStored,
  createdType,
  createdIsNodeMaterial,
  unknownThrows,
  unknownIsMaterial,
  unknownType,
  parseBuilt,
  parsedIsMaterial,
  parsedInputSame,
  arityTable,
  source: NodeMaterialLoader.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('NodeMaterialLoader parity passed');
