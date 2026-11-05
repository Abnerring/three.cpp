import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/utils/RTTNode.js')).href);
const RTTNode = mod.default;
const { rtt, convertToTexture } = mod;


const source = { isNode: true };
const node = new RTTNode(source);

const widthNull = node.width === null;
const heightNull = node.height === null;
const valueIsTexture = node.value === node.renderTarget.texture;
const autoResizeDefault = node.autoResize === true;
const updateBeforeTypeMatches = node.updateBeforeType === 'frame';
const quadMaterialIsNodeMaterial = node._quadMesh.material.isNodeMaterial === true;

const sizedNode = new RTTNode(source, 4, 4);
const autoResizeExplicit = sizedNode.autoResize === false;
const explicitWidth = sizedNode.renderTarget.width;

const scaled = new RTTNode(source, 10, 5, { resolutionScale: 0.5 });
scaled.setSize(10, 5);
const scaledWidth = scaled.renderTarget.width;
const scaledHeight = scaled.renderTarget.height;

const autoNode = new RTTNode(source);
const scaleReturnsSelf = autoNode.setResolutionScale(0.25) === autoNode;
const storedScale = autoNode.getResolutionScale();
const explicitScaled = new RTTNode(source, 8, 8);
explicitScaled.setResolutionScale(0.5);
const explicitScaledWidth = explicitScaled.renderTarget.width;

const copy = node.clone();
const cloneIsTextureNode = copy.isTextureNode === true;
const cloneRefersBack = copy.referenceNode === node;
const cloneSamplerSame = copy.sampler === node.sampler;
let disposeRan = true;
try {
  node.dispose();
} catch {
  disposeRan = false;
}

const sample = { isSampleNode: true };
const samplePassThrough = convertToTexture(sample) === sample;
const texture = { isTextureNode: true };
const texturePassThrough = convertToTexture(texture) === texture;
const produced = { isTextureNode: true };
const pass = { isPassNode: true, getTextureNode: () => produced };
const passUsesGetter = convertToTexture(pass) === produced;
const plain = { isNode: true };
const wrapped = convertToTexture(plain);
const wrappedIsRTT = wrapped.isRTTNode === true;
const wrappedKeepsNode = wrapped.node === plain;

const expected = {
  name: RTTNode.name,
  length: RTTNode.length,
  staticType: RTTNode.type,
  type: node.type,
  isRTTNode: node.isRTTNode,
  widthNull,
  heightNull,
  valueIsTexture,
  textureNeedsUpdate: node.textureNeedsUpdate,
  autoUpdate: node.autoUpdate,
  resolutionScale: node._resolutionScale,
  autoResizeDefault,
  autoResizeExplicit,
  explicitWidth,
  updateBeforeTypeMatches,
  quadMaterialIsNodeMaterial,
  scaledWidth,
  scaledHeight,
  scaleReturnsSelf,
  storedScale,
  explicitScaledWidth,
  cloneIsTextureNode,
  cloneRefersBack,
  cloneSamplerSame,
  disposeRan,
  samplePassThrough,
  texturePassThrough,
  passUsesGetter,
  wrappedIsRTT,
  wrappedKeepsNode,
  rttLength: rtt.length,
  convertLength: convertToTexture.length,
  source: RTTNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('RTTNode parity passed');
