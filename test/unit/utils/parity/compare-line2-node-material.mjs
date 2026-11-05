import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import {isDeepStrictEqual} from 'node:util';

const [upstreamArg, binary, output] = process.argv.slice(2), upstream = path.resolve(upstreamArg);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const load = name => import(pathToFileURL(path.join(upstream, 'src', name + '.js')));
const [tsl, {default: StackNode}, {default: NodeMaterial}] = await Promise.all([
  load('nodes/tsl/TSLBase'), load('nodes/core/StackNode'), load('materials/nodes/NodeMaterial')]);
const sourcePath = path.join(upstream, 'src/materials/nodes/Line2NodeMaterial.js');
let source = fs.readFileSync(sourcePath, 'utf8').replace(/from '([^']+)'/g,
  (_, name) => `from '${pathToFileURL(path.resolve(path.dirname(sourcePath), name)).href}'`);
const privateNames = ['trimSegmentAlpha', 'closestLineToLine', 'mvpLine', 'alphaLine', 'worldStart', 'worldEnd', 'lineDistance', 'worldPos', 'positionLocal'];
source += '\nexport { ' + privateNames.join(', ') + ' };\n';
const mod = await import('data:text/javascript;base64,' + Buffer.from(source).toString('base64')), Material = mod.default;
const value = v => v === '@undefined' ? undefined : v === '@NaN' ? NaN : v === '@-0' ? -0 :
  v === '@node' ? tsl.float(.37) : v === '@vec2' ? tsl.vec2(.2, .4) : v === '@vec3' ? tsl.vec3(.2, .4, .6) : v;
const attempt = body => { try { return body(); } catch (error) { return error instanceof Error ? {error: error.name} : {throw: error}; } };
function graph(root) {
  const seen = new Map(), objects = [];
  function encode(v) {
    if (v === undefined) return ['undefined'];
    if (typeof v === 'function') return ['function'];
    if (typeof v === 'symbol') return ['symbol'];
    if (typeof v === 'number') {
      if (Number.isNaN(v)) return ['number', 'NaN'];
      if (!Number.isFinite(v)) return ['number', v > 0 ? 'Infinity' : '-Infinity'];
      if (Object.is(v, -0)) return ['number', '-0'];
    }
    if (v === null || typeof v !== 'object') return v;
    if (seen.has(v)) return ['ref', seen.get(v)];
    const id = objects.length; seen.set(v, id);
    const row = {kind: Array.isArray(v) ? 'array' : 'object', properties: []}; objects.push(row);
    if (Array.isArray(v)) row.length = v.length;
    if (v.isNode === true) row.nodeType = v.type;
    for (const key of Object.keys(v).sort()) {
      if (['id', 'uuid', '_uuid', 'stackTrace', '_cacheKey', '_cacheKeyVersion'].includes(key)) continue;
      row.properties.push([key, key === 'name' && 'inputs' in v && 'type' in v ? '@fn' : encode(v[key])]);
    }
    return ['ref', id];
  }
  const encoded = encode(root); return {root: encoded, objects};
}
function isConstructor(fn) { try { Reflect.construct(Object, [], fn); return true; } catch { return false; } }
const functionMetadata = fn => ({name: fn.name, length: fn.length, source: fn.toString(), constructible: isConstructor(fn)});
function shaderBody(name) {
  let h = mod[name];
  if (!h.isFn) while (h.shaderNode === undefined) h = h.node;
  return h.shaderNode.jsFunc;
}
function metadata() {
  const descriptors = Object.getOwnPropertyNames(Material.prototype).filter(key => key !== 'constructor').map(key => {
    const d = Object.getOwnPropertyDescriptor(Material.prototype, key);
    return {key, enumerable: d.enumerable, configurable: d.configurable,
      value: d.value ? functionMetadata(d.value) : null, get: d.get ? functionMetadata(d.get) : null, set: d.set ? functionMetadata(d.set) : null};
  });
  const helpers = {};
  for (const key of ['trimSegmentAlpha', 'closestLineToLine', 'mvpLine', 'alphaLine']) {
    let h = mod[key]; if (!h.isFn) while (h.shaderNode === undefined) h = h.node;
    helpers[key] = {callback: functionMetadata(h.shaderNode.jsFunc), nodeType: h.shaderNode.nodeType,
      layout: h.shaderNode.layout === null ? null : h.shaderNode.layout.inputs};
  }
  return {class: functionMetadata(Material), type: Material.type, typeGetter: functionMetadata(Object.getOwnPropertyDescriptor(Material, 'type').get),
    base: Object.getPrototypeOf(Material) === NodeMaterial, prototypeBase: Object.getPrototypeOf(Material.prototype) === NodeMaterial.prototype,
    descriptors, helpers};
}
function snapshot(material) {
  const names = Object.getOwnPropertyNames(material).filter(name => name !== 'uuid').sort(), kinds = {}, scalars = {};
  for (const name of names) { const v = material[name]; kinds[name] = v === null ? 'null' : typeof v;
    if (name !== 'id' && ['number', 'boolean', 'string'].includes(typeof v)) scalars[name] = v; }
  return {names, kinds, scalars};
}
function construct(request) {
  const args = []; if ('parameter' in request) args.push(value(request.parameter)); if ('values' in request) args.push(request.values);
  let material; const result = attempt(() => { material = new Material(...args); });
  const out = {result: result === undefined ? ['undefined'] : result};
  if (material) { out.snapshot = snapshot(material); out.fields = graph(Object.fromEntries(
    ['vertexColors', 'dashed', 'worldUnits', 'alphaToCoverage', 'offsetNode', 'dashScaleNode', 'dashSizeNode', 'gapSizeNode', 'blending', 'dashOffset', 'version'].map(key => [key, material[key]]))); }
  return out;
}
function accessor(request) {
  const key = request.key, privateKey = key === 'worldUnits' ? '_useWorldUnits' : key === 'dashed' ? '_useDash' : key === 'alphaToCoverage' ? '_useAlphaToCoverage' : 'colorNode';
  const self = {}, events = []; let stored = value(request.initial);
  const d = Object.getOwnPropertyDescriptor(Material.prototype, key);
  Object.defineProperty(self, privateKey, {configurable: true, get() {
    events.push('get:' + privateKey); if (request.throwAt === 'get') throw 'get'; return stored;
  }, set(next) { events.push(['set:' + privateKey, graph(next)]); if (request.throwAt === privateKey) throw privateKey; stored = next; }});
  Object.defineProperty(self, 'needsUpdate', {configurable: true, set(next) {
    events.push(['needsUpdate', next]); if (request.throwAt === 'needsUpdate') throw 'needsUpdate'; }});
  const oldWarn = console.warn; console.warn = () => {};
  try { const before = attempt(() => d.get.call(self)), next = value(request.next), setResult = attempt(() => d.set.call(self, next));
    return graph({before, setResult, after: stored, events}); } finally { console.warn = oldWarn; }
}
function warningCase() {
  const events = [], self = {}; let stored = 0;
  const previous = console.warn; console.warn = (...args) => { events.push(['console', 'warn', ...args]); throw 'console'; };
  Object.defineProperty(self, 'colorNode', {set(next) { events.push(['color', next]); stored = next; }});
  const setter = Object.getOwnPropertyDescriptor(Material.prototype, 'lineColorNode').set;
  try { const first = attempt(() => setter.call(self, 1)), second = attempt(() => setter.call(self, 2));
    return graph({first, second, value: stored, events}); } finally { console.warn = previous; }
}
function withStack(body) { const old = tsl.getCurrentStack(), stack = new StackNode(); tsl.setCurrentStack(stack);
  try { return body(stack); } finally { tsl.setCurrentStack(old); } }
function graphCase(request) {
  const material = {}, renderer = {}, events = [];
  for (const key of ['_useDash', '_useWorldUnits', '_useAlphaToCoverage']) material[key] = request[key];
  for (const key of ['dashScaleNode', 'offsetNode', 'dashSizeNode', 'gapSizeNode']) {
    const raw = value(request[key]); Object.defineProperty(material, key, {configurable: true, get() {
      events.push(key); if (request.throwAt === key) throw key; return raw; }});
  }
  renderer.currentSamples = 'samples' in request ? value(request.samples) : 4;
  let input = {material, renderer};
  if (request.helper === 'trimSegmentAlpha') input = {start: tsl.vec4(1, 2, -3, 1), end: tsl.vec4(2, 3, 4, 1)};
  if (request.helper === 'closestLineToLine') input = Object.fromEntries(['p1', 'p2', 'p3', 'p4'].map(key => [key, tsl.vec3(.2, .4, .8)]));
  return withStack(stack => {
    const result = attempt(() => shaderBody(request.helper)(input)), branches = [], seen = new Set();
    function walk(node) {
      if (node === null || typeof node !== 'object' || seen.has(node)) return; seen.add(node);
      if (node.type === 'ConditionalNode') for (const key of ['ifNode', 'elseNode']) {
        const shader = node[key]; if (typeof shader?.jsFunc !== 'function') continue;
        const branchStack = new StackNode(); tsl.setCurrentStack(branchStack);
        const result = attempt(() => shader.jsFunc()); branches.push({source: shader.jsFunc.toString(), result, nodes: branchStack.nodes});
        walk(branchStack.nodes); tsl.setCurrentStack(stack);
      }
      for (const key of Object.keys(node)) if (key !== 'stackTrace') walk(node[key]);
    }
    if (!request.skipBranches) walk(stack.nodes);
    return graph({result, nodes: stack.nodes, branches, events});
  });
}
function setupCase(request) {
  const material = new Material(), events = [], base = Object.getPrototypeOf(Material.prototype), saved = new Map();
  const geometry = {}, builder = {geometry};
  for (const key of ['vertexColors', 'transparent']) material[key] = value(request[key]);
  geometry.hasAttribute = key => { events.push(['hasAttribute', key]); if (request.throwAt === 'hasAttribute') throw 'hasAttribute'; return request.attribute; };
  builder.needsPreviousData = () => { events.push('needsPreviousData'); if (request.throwAt === 'needsPreviousData') throw 'needsPreviousData'; return request.previous; };
  for (const key of ['setupDiffuseColor', 'setupPosition']) { saved.set(key, Object.getOwnPropertyDescriptor(base, key));
    base[key] = function(...args) { events.push([key, this === material, args.length, args[0] === builder]); if (request.throwAt === 'super') throw 'super'; return mod.positionLocal; }; }
  try { return withStack(stack => graph({result: attempt(() => material[request.method](builder)), nodes: stack.nodes, events})); }
  finally { for (const [key, d] of saved) if (d) Object.defineProperty(base, key, d); else delete base[key]; }
}
function run(request) {
  if (request.kind === 'metadata') return metadata();
  if (request.kind === 'constructor') return construct(request);
  if (request.kind === 'accessor') return accessor(request);
  if (request.kind === 'warning') return warningCase();
  if (request.kind === 'graph') return graphCase(request);
  if (request.kind === 'setup') return setupCase(request);
  throw new Error('Unknown Line2 scenario');
}
const requests = [{kind: 'metadata'}, {kind: 'warning'}, ...[undefined, '@undefined', null, 0, false, 'raw'].map(parameter => ({kind: 'constructor', ...(parameter === undefined ? {} : {parameter})})),
  {kind: 'constructor', values: {vertexColors: true, dashed: true, alphaToCoverage: false, worldUnits: true, linewidth: 5, dashOffset: .4, color: 0x224466}}];
for (const key of ['worldUnits', 'dashed', 'alphaToCoverage', 'lineColorNode']) {
  for (const initial of [false, 0, null, '@undefined', '@NaN']) for (const next of [false, 0, true, null, '@undefined', '@NaN', '@-0']) requests.push({kind: 'accessor', key, initial, next});
  for (const throwAt of ['get', key === 'worldUnits' ? '_useWorldUnits' : key === 'dashed' ? '_useDash' : key === 'alphaToCoverage' ? '_useAlphaToCoverage' : 'colorNode', 'needsUpdate']) requests.push({kind: 'accessor', key, initial: false, next: true, throwAt});
}
for (const helper of ['trimSegmentAlpha', 'closestLineToLine']) requests.push({kind: 'graph', helper});
for (const _useDash of [false, true]) for (const _useWorldUnits of [false, true]) {
  requests.push({kind: 'graph', helper: 'mvpLine', _useDash, _useWorldUnits});
  requests.push({kind: 'graph', helper: 'mvpLine', _useDash, _useWorldUnits, dashScaleNode: '@node', offsetNode: '@node'});
  for (const _useAlphaToCoverage of [false, true]) for (const samples of [0, 4]) requests.push({kind: 'graph', helper: 'alphaLine', _useDash, _useWorldUnits, _useAlphaToCoverage, samples, dashSizeNode: '@node', gapSizeNode: '@node'});
}
for (const [helper, throwAt] of [['mvpLine', 'dashScaleNode'], ['mvpLine', 'offsetNode'], ['alphaLine', 'dashSizeNode'], ['alphaLine', 'gapSizeNode']]) requests.push({kind: 'graph', helper, _useDash: true, throwAt});
for (const previous of [false, true]) requests.push({kind: 'setup', method: 'setupPosition', previous});
for (const throwAt of ['super', 'needsPreviousData']) requests.push({kind: 'setup', method: 'setupPosition', previous: true, throwAt});
for (const vertexColors of [false, true, 1]) for (const attribute of [false, true]) for (const transparent of [false, true]) requests.push({kind: 'setup', method: 'setupDiffuseColor', vertexColors, attribute, transparent});
for (const throwAt of ['super', 'hasAttribute']) requests.push({kind: 'setup', method: 'setupDiffuseColor', vertexColors: true, attribute: true, transparent: true, throwAt});
const expected = requests.map(run);
if (output) fs.writeFileSync(output, JSON.stringify({requests, expected}, null, 2));
if (binary === '--oracle-only') console.log(`Line2NodeMaterial: ${requests.length} source scenarios generated`);
else {
  const actual = JSON.parse(execFileSync(path.resolve(binary), {input: JSON.stringify(requests), encoding: 'utf8', maxBuffer: 128 * 1024 * 1024}));
  if (output) fs.writeFileSync(output + '.actual.json', JSON.stringify(actual, null, 2));
  assert.equal(actual.length, expected.length);
  function difference(a, b, at = '$') { if (isDeepStrictEqual(a, b)) return null;
    if (a === null || b === null || typeof a !== 'object' || typeof b !== 'object') return {at, actual: a, expected: b};
    if (!isDeepStrictEqual(Object.keys(a).sort(), Object.keys(b).sort())) return {at, actualKeys: Object.keys(a), expectedKeys: Object.keys(b)};
    for (const key of Object.keys(b)) { const d = difference(a[key], b[key], at + '.' + key); if (d) return d; } return {at, actual: a, expected: b}; }
  for (let i = 0; i < requests.length; ++i) { const d = difference(actual[i], expected[i]); assert.equal(d, null, JSON.stringify({request: requests[i], difference: d})); }
  console.log(`Line2NodeMaterial: ${requests.length} metadata, defaults, alias, exception and real TSL graph scenarios passed`);
}
