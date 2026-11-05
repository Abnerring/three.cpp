import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const sampleModule = await import(pathToFileURL(path.join(upstream, 'src/nodes/utils/SampleNode.js')).href);
const { default: SampleNode, sample } = sampleModule;
const sharedModule = await import(pathToFileURL(path.join(upstream, 'src/nodes/display/ViewportSharedTextureNode.js')).href);
const { default: ViewportSharedTextureNode, viewportSharedTexture } = sharedModule;
const { float } = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')).href);

// The same counting callback the probe builds: it records its calls and the uv it was handed, and
// answers a node so the delegation can be checked.
const countingCallback = (state) => (uv) => {
  state.count += 1;
  state.lastArg = uv;
  return float(2);
};

const state = { count: 0 };
const sampleNode = new SampleNode(countingCallback(state));
const uvArgument = float(0.5);
const sampled = sampleNode.sample(uvArgument);
const argumentWasTheUv = state.lastArg === uvArgument;
const afterFirstSample = state.count;
const built = sampleNode.setup();
const setupArgumentIsNode = state.lastArg.isNode === true;

const factoryState = { count: 0 };
const factoryCallback = countingCallback(factoryState);
const factoryNode = sample(factoryCallback);

const first = new ViewportSharedTextureNode();
const second = new ViewportSharedTextureNode();
const produced = viewportSharedTexture();

const expected = {
  sampleName: SampleNode.name,
  sampleLength: SampleNode.length,
  sampleStaticType: SampleNode.type,
  sampleType: sampleNode.type,
  sampleIsSampleNode: sampleNode.isSampleNode,
  sampleUvNodeIsNull: sampleNode.uvNode === null,
  sampleIsCacheable: sampleNode.isCacheable(),
  sampleCallsAfterFirst: afterFirstSample,
  sampleArgumentWasTheUv: argumentWasTheUv,
  sampleResultIsNode: sampled.isNode === true,
  sampleCallsAfterSetup: state.count,
  sampleSetupArgumentIsNode: setupArgumentIsNode,
  sampleSetupResultIsNode: built.isNode === true,
  factoryName: sample.name,
  factoryLength: sample.length,
  factoryIsSampleNode: factoryNode.isSampleNode,
  factoryCallbackSame: factoryNode.callback === factoryCallback,
  factoryUvNodeIsNull: factoryNode.uvNode === null,
  sharedName: ViewportSharedTextureNode.name,
  sharedLength: ViewportSharedTextureNode.length,
  sharedStaticType: ViewportSharedTextureNode.type,
  sharedType: first.type,
  sharedTextureSame: first.getTextureForReference() === second.getTextureForReference(),
  updateReferenceReturnsSelf: first.updateReference() === first,
  proxyIsCallable: typeof viewportSharedTexture === 'function',
  proxyLength: viewportSharedTexture.length,
  proxyProducesCallable: typeof produced.getTextureForReference === 'function',
  proxyTextureIsNotNull: produced.getTextureForReference() !== null,
  sampleSource: SampleNode.toString(),
  sharedSource: ViewportSharedTextureNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('SampleNode and ViewportSharedTextureNode parity passed');
