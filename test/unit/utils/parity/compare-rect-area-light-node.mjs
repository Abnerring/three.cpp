import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: RectAreaLightNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/RectAreaLightNode.js')).href);
const { Matrix4 } = await import(pathToFileURL(path.join(upstream, 'src/math/Matrix4.js')).href);
const { Color } = await import(pathToFileURL(path.join(upstream, 'src/math/Color.js')).href);
const { Texture } = await import(pathToFileURL(path.join(upstream, 'src/textures/Texture.js')).href);

// The same ninety degree turn about y the probe builds, so the half extents can be compared as numbers.
const matrixWorld = new Matrix4().set(
  0, 0, 1, 0,
  0, 1, 0, 0,
  -1, 0, 0, 0,
  0, 0, 0, 1
);
const light = { color: new Color(1, 1, 1), intensity: 1, matrixWorld, width: 4, height: 2 };
const node = new RectAreaLightNode(light);
const camera = { matrixWorldInverse: new Matrix4().identity() };
node.update({ camera });
const halfWidth = node.halfWidth.value;
const halfHeight = node.halfHeight.value;


const library = { LTC_FLOAT_1: new Texture(), LTC_FLOAT_2: new Texture(), LTC_HALF_1: new Texture(), LTC_HALF_2: new Texture() };
RectAreaLightNode.setLTC(library);
const result = node.setupDirectRectArea({ isAvailable: (name) => name === 'float32Filterable' });
const halfResult = node.setupDirectRectArea({ isAvailable: () => false });

const keys = ['lightColor', 'lightPosition', 'halfWidth', 'halfHeight', 'ltc_1', 'ltc_2'];
const expected = {
  name: RectAreaLightNode.name,
  length: RectAreaLightNode.length,
  staticType: RectAreaLightNode.type,
  instanceType: node.type,
  isAnalyticLightNode: node.isAnalyticLightNode,
  updateType: node.updateType,
  halfWidthX: halfWidth.x,
  halfWidthY: halfWidth.y,
  halfWidthZ: halfWidth.z,
  halfHeightX: halfHeight.x,
  halfHeightY: halfHeight.y,
  halfHeightZ: halfHeight.z,
  keys,
  ltc1IsNode: result.ltc_1.isNode === true,
  ltc2IsNode: result.ltc_2.isNode === true,
  ltcDistinct: result.ltc_1 !== result.ltc_2,
  halfWidthIsUniform: result.halfWidth === node.halfWidth,
  lightColorIsColorNode: result.lightColor === node.colorNode,
  ltc1WrapsFloat: result.ltc_1.value === library.LTC_FLOAT_1,
  ltc2WrapsFloat: result.ltc_2.value === library.LTC_FLOAT_2,
  halfLtc1WrapsHalf: halfResult.ltc_1.value === library.LTC_HALF_1,
  halfLtc2WrapsHalf: halfResult.ltc_2.value === library.LTC_HALF_2,
  source: RectAreaLightNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('RectAreaLightNode native/module parity passed');
