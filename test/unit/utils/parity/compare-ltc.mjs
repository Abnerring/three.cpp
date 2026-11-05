import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/functions/BSDF/LTC.js')).href);
const { vec3, float, mat3, getCurrentStack, setCurrentStack } = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')).href);
const { default: StackNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/core/StackNode.js')).href);

const names = ['LTC_Uv', 'LTC_Evaluate', 'LTC_Evaluate_Volume'];
const kinds = {};
for (const name of names) kinds[name] = typeof mod[name] === 'function' ? 'function' : 'other';
const shape = shader => ({ callbackLength: shader.jsFunc.length, nodeType: shader.nodeType,
  layoutType: shader.layout.type, inputs: shader.layout.inputs });
const shapes = Object.fromEntries(names.map(name => [name, shape(mod[name].shaderNode)]));
const namedInputs = () => ({ N: vec3(0, 0, 1), V: vec3(.2, .1, 1), P: vec3(0, 0, 2), mInv: mat3(),
  p0: vec3(-1, -1, 0), p1: vec3(1, -1, 0), p2: vec3(1, 1, 0), p3: vec3(-1, 1, 0), roughness: float(.5) });
function withStack(fn) {
  const previous = getCurrentStack(), stack = new StackNode();
  setCurrentStack(stack);
  try { return fn(stack); } finally { setCurrentStack(previous); }
}
function uvBody() {
  const inputs = namedInputs();
  return withStack(stack => {
    const result = mod.LTC_Uv.shaderNode.jsFunc(inputs);
    return { isNode: result.isNode, type: result.type, assignments: stack.nodes.length };
  });
}
function internalShapes() {
  const inputs = namedInputs();
  return withStack(stack => {
    mod.LTC_Evaluate_Volume.shaderNode.jsFunc(inputs);
    stack._currentCond.ifNode.jsFunc();
    const shapes = {}, seen = new Set();
    function walk(value) {
      if (value === null || typeof value !== 'object' || seen.has(value)) return;
      seen.add(value);
      const shader = value.shaderNode;
      if (shader?.layout) {
        const first = shader.layout.inputs[0].name;
        if (first === 'f') shapes.LTC_ClippedSphereFormFactor = shape(shader);
        if (first === 'v1') shapes.LTC_EdgeVectorFormFactor = shape(shader);
      }
      for (const key of Object.keys(value)) if (key !== 'stackTrace') walk(value[key]);
    }
    walk(stack.nodes);
    return shapes;
  });
}

const uv = mod.LTC_Uv(namedInputs());
const expected = {
  names,
  kinds,
  shapes,
  uvIsNode: uv.isNode === true,
  uvType: uv.type === undefined ? 'undefined' : String(uv.type),
  uvBody: uvBody(),
  internalShapes: internalShapes()
};
if (probe === '--source-only') { console.log(JSON.stringify(expected)); process.exit(0); }
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('LTC five Fn layouts and named-input body parity passed');
