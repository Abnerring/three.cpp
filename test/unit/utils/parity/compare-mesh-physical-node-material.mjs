import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import {isDeepStrictEqual} from 'node:util';

const [upstreamArgument, binary, output] = process.argv.slice(2);
const upstream = path.resolve(upstreamArgument);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const load = p => import(pathToFileURL(path.join(upstream, 'src', p + '.js')).href);
const {default: Material} = await load('materials/nodes/MeshPhysicalNodeMaterial');
const {default: Base} = await load('materials/nodes/MeshStandardNodeMaterial');
const {default: StackNode} = await load('nodes/core/StackNode');
const tsl = await load('nodes/tsl/TSLBase');

function graph(root) {
  const seen = new Map(), objects = [];
  function encode(value) {
    if (value === undefined) return ['undefined'];
    if (typeof value === 'function') return ['function'];
    if (typeof value === 'symbol') return ['symbol'];
    if (typeof value === 'number') {
      if (Number.isNaN(value)) return ['number', 'NaN'];
      if (!Number.isFinite(value)) return ['number', value > 0 ? 'Infinity' : '-Infinity'];
      if (Object.is(value, -0)) return ['number', '-0'];
    }
    if (value === null || typeof value !== 'object') return value;
    if (seen.has(value)) return ['ref', seen.get(value)];
    const id = objects.length;
    seen.set(value, id);
    const object = {kind: Array.isArray(value) ? 'array' : 'object', properties: []};
    objects.push(object);
    if (Array.isArray(value)) object.length = value.length;
    if (value.isNode === true) object.nodeType = value.type;
    for (const key of Object.keys(value)) {
      if (['id', 'uuid', '_uuid', 'stackTrace', '_cacheKey', '_cacheKeyVersion'].includes(key)) continue;
      object.properties.push([key, encode(value[key])]);
    }
    return ['ref', id];
  }
  return {root: encode(root), objects};
}
function input(v) {
  if (v === '@undefined') return undefined;
  if (v === '@node') return tsl.float(0.37);
  if (v === '@vec3') return tsl.vec3(0.2, 0.4, 0.6);
  return v;
}
function snapshot(material) {
  const names = Object.getOwnPropertyNames(material).filter(n => n !== 'uuid').sort(), kinds = {}, scalars = {};
  for (const name of names) {
    const value = material[name], kind = value === null ? 'null' : typeof value;
    kinds[name] = kind;
    if (name !== 'id' && ['number', 'boolean', 'string'].includes(kind)) scalars[name] =
      kind === 'number' && !Number.isFinite(value) ? ['number', Number.isNaN(value) ? 'NaN' : value > 0 ? 'Infinity' : '-Infinity'] : value;
  }
  return {names, kinds, scalars};
}
function isConstructor(fn) { try { Reflect.construct(Object, [], fn); return true; } catch { return false; } }
function metadata() {
  const descriptors = [];
  for (const key of Object.getOwnPropertyNames(Material.prototype)) {
    if (key === 'constructor') continue;
    const d = Object.getOwnPropertyDescriptor(Material.prototype, key), fn = d.get || d.value;
    descriptors.push({key, getter: !!d.get, name: fn.name, length: fn.length, source: fn.toString(),
      constructible: isConstructor(fn), writable: Object.hasOwn(d, 'writable') ? d.writable : ['undefined'], enumerable: d.enumerable, configurable: d.configurable});
  }
  return {name: Material.name, length: Material.length, source: Material.toString(), type: Material.type,
    typeSource: Object.getOwnPropertyDescriptor(Material, 'type').get.toString(),
    base: Object.getPrototypeOf(Material) === Base, basePrototype: Object.getPrototypeOf(Material.prototype) === Base.prototype, descriptors};
}
function construct(request) {
  const params = request.configured ? {roughness: 0.23, clearcoat: 0.7, transmission: 0.5, sheen: 0.8,
    clearcoatNode: input('@node'), attenuationColor: 0x123456} : {};
  const value = new Material(params), clone = value.clone();
  return {snapshot: snapshot(value), features: ['useClearcoat', 'useDiffuseRoughness', 'useIridescence', 'useSheen',
    'useAnisotropy', 'useTransmission', 'useDispersion', 'useRetroreflection'].map(k => value[k]),
    clone: snapshot(clone), nodeShared: clone.clearcoatNode === value.clearcoatNode};
}
function features(request) {
  return Object.entries({useClearcoat: 'clearcoat', useDiffuseRoughness: 'diffuseRoughness', useIridescence: 'iridescence',
    useSheen: 'sheen', useAnisotropy: 'anisotropy', useTransmission: 'transmission', useDispersion: 'dispersion', useRetroreflection: 'retroreflectivity'}).map(([key, field]) => {
    const events = [], self = {};
    Object.defineProperty(self, field, {get() { events.push('value'); return request.coerce ? {valueOf() {
      events.push('coerce'); if (request.throws) throw new TypeError('coerce'); return input(request.value);
    }} : input(request.value); }});
    Object.defineProperty(self, field + 'Node', {get() { events.push('node'); return input(request.node); }});
    let result, error = null;
    try { result = Object.getOwnPropertyDescriptor(Material.prototype, key).get.call(self); } catch (e) {error = e.constructor.name;}
    return {key, result: result === undefined ? ['undefined'] : result, error, events};
  });
}
function variants(request) {
  const material = new Material(), stack = new StackNode(), events = [], oldStack = tsl.getCurrentStack();
  for (const [key, v] of Object.entries(request.values || {})) material[key] = input(v);
  if (request.observe) for (const key of ['useDiffuseRoughness', 'useClearcoat', 'useSheen', 'useRetroreflection',
    'useIridescence', 'useAnisotropy', 'useTransmission', 'useDispersion', 'clearcoatNode', 'clearcoatRoughnessNode',
    'sheenNode', 'sheenRoughnessNode', 'iridescenceNode', 'iridescenceIORNode', 'iridescenceThicknessNode', 'anisotropyNode',
    'transmissionNode', 'thicknessNode', 'attenuationDistanceNode', 'attenuationColorNode', 'dispersionNode',
    'retroreflectivityNode', 'diffuseRoughnessNode', 'iorNode', 'specularColorNode', 'specularIntensityNode']) {
    const v = material[key];
    Object.defineProperty(material, key, {configurable: true, get() {
      events.push(key); if (request.throwAt === key) throw new TypeError('getter'); return v;
    }});
  }
  tsl.setCurrentStack(stack);
  let error = null, result;
  const branches = [];
  try {
    try { result = material[request.specularOnly ? 'setupSpecular' : 'setupVariants']({}); } catch (e) {error = e.constructor.name;}
    if (request.branches && error === null) try {
      for (const n of stack.nodes) if (n.isConditionalNode === true) for (const key of ['ifNode', 'elseNode']) {
        const shader = n[key], branchStack = new StackNode();
        tsl.setCurrentStack(branchStack);
        shader.jsFunc();
        branches.push({source: shader.jsFunc.toString(), nodes: branchStack.nodes});
      }
    } catch (e) {error = e.constructor.name;}
    return {error, returnsUndefined: result === undefined, events, graph: graph({nodes: stack.nodes, branches})};
  } finally {tsl.setCurrentStack(oldStack);}
}
function model(request) {
  const material = new Material(), events = [];
  for (const key of ['useClearcoat', 'useSheen', 'useIridescence', 'useAnisotropy', 'useTransmission', 'useDispersion', 'useRetroreflection', 'useDiffuseRoughness'])
    Object.defineProperty(material, key, {get() {events.push(key); if (request.throwAt === key) throw new TypeError('flag'); return request.value;}});
  let result, error = null;
  try {result = material.setupLightingModel();} catch (e) {error = e.constructor.name;}
  return {events, error, graph: graph(result)};
}
function setupCase(request) {
  const material = new Material(), base = Object.getPrototypeOf(Material.prototype), old = Object.getOwnPropertyDescriptor(base, 'setup');
  const context = {}, builder = {context}, events = [], sentinel = tsl.vec3(0.1, 0.2, 0.3);
  material.setupClearcoatNormal = function normal(...args) {
    events.push(['normal', this === material, args.length, args[0] === builder]);
    if (request.normalThrows) throw new TypeError('normal'); return sentinel;
  };
  base.setup = function superSetup(...args) {
    events.push(['super', this === material, args.length, args[0] === builder, typeof builder.context.setupClearcoatNormal === 'function']);
    if (request.superThrows) throw new TypeError('super'); return 'ignored';
  };
  try {
    let result, error = null, callbackResult, callbackError = null;
    try {result = material.setup(builder);} catch(e) {error = e.constructor.name;}
    const callback = context.setupClearcoatNormal;
    try {callbackResult = callback.call({});} catch(e) {callbackError = e.constructor.name;}
    return {events, error, callbackError, returnsUndefined: result === undefined, callbackLength: callback.length,
      callbackSource: callback.toString(), graph: graph(callbackResult)};
  } finally {if (old) Object.defineProperty(base, 'setup', old); else delete base.setup;}
}
function run(request) {
  return ({metadata, constructor: construct, features, variants, model, setup: setupCase})[request.kind](request);
}
const all = {diffuseRoughness: 0.2, clearcoat: 0.3, sheen: 0.4, retroreflectivity: 0.5, iridescence: 0.6,
  anisotropy: 0.7, transmission: 0.8, dispersion: 0.9};
const requests = [{kind: 'metadata'}, {kind: 'constructor'}, {kind: 'constructor', configured: true}];
for (const value of [-1, 0, 0.5, '1', null, '@undefined']) for (const node of [null, 0, false, '@undefined', '@node'])
  requests.push({kind: 'features', value, node});
requests.push({kind: 'features', value: 0, node: null, coerce: true}, {kind: 'features', value: 1, node: null, coerce: true},
  {kind: 'features', value: 1, node: null, coerce: true, throws: true}, {kind: 'variants'},
  {kind: 'variants', values: all, observe: true, branches: true}, {kind: 'variants', specularOnly: true},
  {kind: 'variants', specularOnly: true, observe: true, values: {iorNode: '@node', specularColorNode: '@vec3', specularIntensityNode: '@node'}});
for (const key of Object.keys(all)) requests.push({kind: 'variants', observe: true, values: {[key]: 0.5}, branches: true});
for (const key of ['useDiffuseRoughness', 'useClearcoat', 'clearcoatRoughnessNode', 'useSheen', 'useAnisotropy', 'useTransmission', 'attenuationColorNode', 'useDispersion'])
  requests.push({kind: 'variants', values: all, observe: true, throwAt: key});
for (const value of [null, false, 0, 'raw']) requests.push({kind: 'model', value});
requests.push({kind: 'model', value: true, throwAt: 'useTransmission'}, {kind: 'setup'}, {kind: 'setup', superThrows: true}, {kind: 'setup', normalThrows: true});
const expected = requests.map(run);
if (output) fs.writeFileSync(output, JSON.stringify({requests, expected}, null, 2));
if (binary !== '--oracle-only') {
  const actual = JSON.parse(execFileSync(path.resolve(binary), {input: JSON.stringify(requests), encoding: 'utf8', maxBuffer: 32 * 1024 * 1024}));
  if (output) fs.writeFileSync(output + '.actual.json', JSON.stringify(actual, null, 2));
  function firstDifference(a, b, at = '$') {
    if (isDeepStrictEqual(a, b)) return null;
    if (a === null || b === null || typeof a !== 'object' || typeof b !== 'object') return {at, actual: a, expected: b};
    const ak = Object.keys(a), bk = Object.keys(b);
    if (!isDeepStrictEqual([...ak].sort(), [...bk].sort())) return {at, actualKeys: ak, expectedKeys: bk};
    for (const key of bk) { const d = firstDifference(a[key], b[key], at + '.' + key); if (d) return d; }
    return {at, actual: a, expected: b};
  }
  assert.equal(actual.length, expected.length);
  for (let i = 0; i < requests.length; ++i) {
    const difference = firstDifference(actual[i], expected[i]);
    assert.equal(difference, null, JSON.stringify({request: requests[i], difference}));
  }
  console.log(`MeshPhysicalNodeMaterial: ${requests.length} source, feature, graph, exception and callback scenarios passed`);
}
