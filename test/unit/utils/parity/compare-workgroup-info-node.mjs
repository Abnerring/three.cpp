import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/gpgpu/WorkgroupInfoNode.js')).href);
const WorkgroupInfoNode = mod.default;
const { workgroupArray } = mod;

const recordingBuilder = (captured, computeStage = true) => ({
  shaderStage: computeStage ? 'compute' : 'fragment',
  getScopedArray: (...args) => {
    captured.args = args;
    return 'scoped';
  }
});

const info = workgroupArray('float', 4);
const defaulted = workgroupArray('uint');

const byCtor = new WorkgroupInfoNode('Workgroup', 'int', 8);
const setNameSame = byCtor.setName('myArray') === byCtor;
const nameAfterSet = byCtor.name;
const setScopeSame = byCtor.setScope('Storage') === byCtor;
const inputTypeAfterScope = byCtor.getInputType();
const elementType = byCtor.getElementType();
const toAtomicSame = byCtor.toAtomic() === byCtor;
const atomicFlag = byCtor.isAtomic;
const labelSame = byCtor.label('older') === byCtor;
const nameAfterLabel = byCtor.name;

const index = {};
const element = byCtor.element(index);
const elementIndexSame = element.indexNode === index;
const elementCtor = element.constructor;

const captured = {};
const generated = info.generate(recordingBuilder(captured));

const named = new WorkgroupInfoNode('Workgroup', 'float', 4);
named.setName('named');
named.toAtomic();
const capturedNamed = {};
const generatedNamed = named.generate(recordingBuilder(capturedNamed));
const capturedOther = {};
const generatedOther = named.generate(recordingBuilder(capturedOther, false));

// The element generate needs a real array element generation, so only whether it raised is compared.
let elementGenerated = true;
try {
  element.generate({ isContextAssign: () => true }, 'float');
} catch {
  elementGenerated = false;
}

const methodLengths = {};
for (const name of [ 'isCacheable', 'setName', 'label', 'setScope', 'setAtomic', 'toAtomic',
  'getElementType', 'getInputType', 'element', 'generate' ]) {
  methodLengths[name] = info[name].length;
}

const expected = {
  name: WorkgroupInfoNode.name,
  length: WorkgroupInfoNode.length,
  elementName: elementCtor.name,
  elementLength: elementCtor.length,
  factoryLength: workgroupArray.length,
  type: info.type,
  isWorkgroupInfoNode: info.isWorkgroupInfoNode,
  scope: info.scope,
  bufferType: info.bufferType,
  bufferCount: info.bufferCount,
  elementTypeField: info.elementType,
  nameField: info.name,
  isAtomic: info.isAtomic,
  defaultedCount: defaulted.bufferCount,
  cacheable: info.isCacheable(),
  inputTypeDefault: info.getInputType(),
  setNameSame,
  nameAfterSet,
  setScopeSame,
  inputTypeAfterScope,
  elementType,
  toAtomicSame,
  atomicFlag,
  labelSame,
  nameAfterLabel,
  elementFlag: element.isWorkgroupInfoElementNode,
  elementIsNode: element.isNode,
  elementIndexSame,
  generated,
  generatedNamePrefix: String(captured.args[0]).slice(0, 15),
  generatedScope: captured.args[1],
  generatedType: captured.args[2],
  generatedCount: captured.args[3],
  generatedAtomic: captured.args[4],
  generatedNamed,
  namedName: capturedNamed.args[0],
  namedAtomic: capturedNamed.args[4],
  generatedOther,
  elementGenerated,
  methodLengths,
  source: WorkgroupInfoNode.toString(),
  elementSource: elementCtor.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('WorkgroupInfoNode parity passed');
