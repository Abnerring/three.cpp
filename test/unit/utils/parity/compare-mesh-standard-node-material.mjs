import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

const [upstreamArgument, binaryArgument, outputArgument] = process.argv.slice(2);
const upstream = path.resolve(upstreamArgument);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const load = p => import(pathToFileURL(path.join(upstream, 'src', p + '.js')).href);
const { default: Material } = await load('materials/nodes/MeshStandardNodeMaterial');
const { default: NodeMaterial } = await load('materials/nodes/NodeMaterial');
const { default: StackNode } = await load('nodes/core/StackNode');
const properties = await load('nodes/core/PropertyNode');
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
    const object = { kind: Array.isArray(value) ? 'array' : 'object', properties: [] };
    objects.push(object);
    if (Array.isArray(value)) object.length = value.length;
    if (value.isNode === true) object.nodeType = value.type;
    for (const key of Object.keys(value)) {
      if (['id', 'uuid', '_uuid', 'stackTrace', '_cacheKey', '_cacheKeyVersion'].includes(key)) continue;
      object.properties.push([key, encode(value[key])]);
    }
    return ['ref', id];
  }
  return { root: encode(root), objects };
}
function input(value) {
  if (value === '@undefined') return undefined;
  if (value === '@node') return tsl.float(0.37);
  if (value === '@vec3') return tsl.vec3(0.2, 0.4, 0.6);
  return value;
}
function snapshot(material) {
  const names = Object.getOwnPropertyNames(material).filter(name => name !== 'uuid').sort();
  const kinds = {}, scalars = {};
  for (const name of names) {
    const value = material[name], kind = value === null ? 'null' : typeof value;
    kinds[name] = kind;
    if (name !== 'id' && ['number', 'boolean', 'string'].includes(kind)) scalars[name] = value;
  }
  return { names, kinds, scalars };
}
function isConstructor(fn) { try { Reflect.construct(Object, [], fn); return true; } catch { return false; } }
function metadata() {
  const methods = ['setupEnvironment', 'setupLightingModel', 'setupSpecular', 'setupVariants'].map(key => {
    const d = Object.getOwnPropertyDescriptor(Material.prototype, key), fn = d.value;
    return { key, name: fn.name, length: fn.length, constructible: isConstructor(fn), writable: d.writable,
      enumerable: d.enumerable, configurable: d.configurable, source: fn.toString() };
  });
  const d = Object.getOwnPropertyDescriptor(Material, 'type'), getter = d.get;
  return { name: Material.name, length: Material.length, type: Material.type, source: Material.toString(),
    base: Object.getPrototypeOf(Material) === NodeMaterial,
    basePrototype: Object.getPrototypeOf(Material.prototype) === NodeMaterial.prototype,
    prototypeKeys: Object.getOwnPropertyNames(Material.prototype), typeGetter: {
      name: getter.name, length: getter.length, source: getter.toString(), enumerable: d.enumerable,
      configurable: d.configurable, setter: d.set !== undefined, constructible: isConstructor(getter)
    }, methods };
}
function constructorCase(request) {
  const params = { roughness: 0.23, metalness: 0.81, color: 0x2468ac, emissive: 0x102030,
    emissiveIntensity: 2.5, emissiveNode: input('@vec3'), metalnessNode: input('@node') };
  const material = request.parameters ? new Material(params) : new Material(), other = new Material();
  return { snapshot: snapshot(material), color: [material.color.r, material.color.g, material.color.b],
    emissive: [material.emissive.r, material.emissive.g, material.emissive.b],
    independentColor: material.color !== other.color, independentNormalScale: material.normalScale !== other.normalScale,
    independentEmissive: material.emissive !== other.emissive, independentRotation: material.envMapRotation !== other.envMapRotation,
    metalnessInput: material.metalnessNode === params.metalnessNode, emissiveInput: material.emissiveNode === params.emissiveNode };
}
function checkpoint(state, label) {
  state.events.push(label);
  if (state.request.throwAt === label) throw new TypeError(label);
}
function constructorHooks(request) {
  const state = { request, events: [], defaults: null, nested: false };
  class Derived extends Material {}
  for (const name of ['setDefaultValues', 'setValues']) {
    Derived.prototype[name] = function (...args) {
      checkpoint(state, name + (state.nested ? ':nested' : ':outer'));
      state.events.push({ flag: this.isMeshStandardNodeMaterial, lights: this.lights,
        emissiveNull: this.emissiveNode === null, metalnessNull: this.metalnessNode === null, roughnessNull: this.roughnessNode === null });
      if (name === 'setDefaultValues') {
        if (state.defaults === null) state.defaults = args[0];
        state.events.push(args[0] === state.defaults);
        if (request.reenter && !state.nested) {
          state.nested = true;
          Reflect.construct(Material, ['nested'], Derived);
          state.nested = false;
        }
      } else state.events.push(args[0]);
      return 99;
    };
  }
  let error = null, result;
  try { result = Reflect.construct(Material, ['outer'], Derived); } catch (e) { error = e.name; }
  return { error, events: state.events, derived: result !== undefined && Object.getPrototypeOf(result) === Derived.prototype };
}
function environmentCase(request) {
  const proto = Material.prototype, homeBase = Object.getPrototypeOf(proto), material = new Material();
  const state = { request, events: [], reads: 0, nested: false }, builder = {}, fallback = input(request.fallback);
  Object.defineProperty(builder, 'environmentNode', { enumerable: true, configurable: true, get() {
    checkpoint(state, 'environment:' + ++state.reads);
    state.events.push(this === builder);
    if (state.reads === 2 && Object.hasOwn(request, 'second')) return input(request.second);
    return fallback;
  } });
  let result, error = null;
  if (request.realBase) {
    material.envNode = input(request.base);
    if (request.map) material.envMap = { isCubeTexture: request.cube };
    try { result = material.setupEnvironment(builder); } catch (e) { error = e.name; }
  } else {
    const method = proto.setupEnvironment, bridge = Object.create(homeBase);
    Object.defineProperty(bridge, 'setupEnvironment', { enumerable: true, configurable: true, get() {
      checkpoint(state, 'super:get'); state.events.push(this === material);
      return function (...args) {
        checkpoint(state, 'super:call'); state.events.push([this === material, args.length, args[0] === builder]);
        if (request.reenter && !state.nested) {
          state.nested = true;
          const value = method.call(material, builder);
          state.events.push(value === null ? 'nested:null' : 'nested:environment');
          state.nested = false;
        }
        return input(request.base);
      };
    } });
    Object.setPrototypeOf(proto, bridge);
    try { result = material.setupEnvironment(builder); } catch (e) { error = e.name; }
    finally { Object.setPrototypeOf(proto, homeBase); }
  }
  const out = { error, events: state.events, isNull: result === null };
  if (error === null && result !== null) Object.assign(out, {
    type: result.type, envIsFallback: result.envNode === fallback, env: graph(result.envNode)
  });
  return out;
}
function sequenceProperty(state, material, key, values) {
  let at = 0;
  Object.defineProperty(material, key, { configurable: true, enumerable: true, get() {
    checkpoint(state, 'get:' + key + ':' + (at + 1));
    return values[Math.min(at++, values.length - 1)];
  } });
}
function variantsCase(request) {
  const material = new Material(), stack = new StackNode(), previous = tsl.getCurrentStack();
  const state = { request, events: [], nested: false };
  for (const key of ['metalnessNode', 'roughnessNode']) sequenceProperty(state, material, key,
    request[key] === undefined ? [null] : request[key].map(input));
  const originalSpecular = material.setupSpecular;
  Object.defineProperty(material, 'setupSpecular', { configurable: true, enumerable: true, get() {
    checkpoint(state, 'get:setupSpecular'); state.events.push(this === material);
    return function (...args) {
      checkpoint(state, 'call:setupSpecular'); state.events.push([this === material, args.length]);
      if (request.reenter && !state.nested) {
        state.nested = true; new Material().setupVariants(); state.nested = false;
      }
      if (!request.overrideSpecular) return originalSpecular.call(this);
    };
  } });
  const restores = [];
  if (request.observeAssign) for (const key of ['metalness', 'roughness', 'specularColor', 'specularColorBlended', 'specularF90', 'diffuseContribution']) {
    const property = properties[key], original = property.assign, old = Object.getOwnPropertyDescriptor(property, 'assign');
    restores.push(() => { if (old) Object.defineProperty(property, 'assign', old); else delete property.assign; });
    Object.defineProperty(property, 'assign', { configurable: true, get() {
      checkpoint(state, 'get:assign:' + key); state.events.push(this === property);
      return function (...args) {
        checkpoint(state, 'call:assign:' + key); state.events.push([this === property, args.length]);
        return original.apply(this, args);
      };
    } });
  }
  tsl.setCurrentStack(stack);
  let result, error = null, roughnessBody;
  try { result = material[request.specularOnly ? 'setupSpecular' : 'setupVariants'](); } catch (e) { error = e.name; }
  if (request.roughnessBody && error === null) {
    try {
      const assignment = stack.nodes.find(n => n.targetNode?.name === 'Roughness');
      if (assignment) {
        let source = assignment.sourceNode;
        while (source.shaderNode === undefined) source = source.node;
        roughnessBody = source.shaderNode.jsFunc(source.rawInputs[0]);
      }
    } catch (e) { error = e.name; }
  }
  restores.forEach(restore => restore());
  tsl.setCurrentStack(previous);
  return { error, returnsUndefined: result === undefined, events: state.events,
    graph: graph({ nodes: stack.nodes, roughnessBody }) };
}
function modelCase() {
  const material = new Material(), a = material.setupLightingModel(), b = material.setupLightingModel();
  return { distinct: a !== b, prototypeName: a.constructor.name, snapshot: graph(a) };
}
function run(request) {
  return ({ metadata, constructor: constructorCase, constructorHooks, environment: environmentCase,
    variants: variantsCase, model: modelCase })[request.kind](request);
}

const cases = [{ kind: 'metadata' }, { kind: 'constructor' }, { kind: 'constructor', parameters: true }, { kind: 'model' }];
for (const reenter of [false, true]) {
  cases.push({ kind: 'constructorHooks', reenter });
  for (const throwAt of ['setDefaultValues:outer', 'setValues:outer', 'setDefaultValues:nested', 'setValues:nested'])
    cases.push({ kind: 'constructorHooks', reenter, throwAt });
}
for (const base of [null, false, 0, '', '@undefined', '@node', 2, 'texture', {}])
  for (const fallback of [null, false, 0, '@undefined', '@node', 3])
    cases.push({ kind: 'environment', base, fallback });
for (const second of [false, null, '@undefined', '@node', 4])
  cases.push({ kind: 'environment', base: null, fallback: '@node', second });
for (const realBase of [false, true]) {
  cases.push({ kind: 'environment', realBase, base: null, fallback: '@node', reenter: !realBase });
  for (const throwAt of ['super:get', 'super:call', 'environment:1', 'environment:2'])
    cases.push({ kind: 'environment', realBase, base: null, fallback: '@node', throwAt });
}
for (const base of [null, '@node', false]) for (const fallback of [null, '@node'])
  cases.push({ kind: 'environment', realBase: true, base, fallback });
for (const cube of [false, true]) cases.push({ kind: 'environment', realBase: true, base: null, fallback: '@node', map: true, cube });
for (const metalnessNode of [[null], [false], [0], ['@undefined'], [0.7], [true], ['@node'], ['@vec3'], [true, 0.27], ['@node', null]])
  for (const roughnessNode of [[null], [0.23], ['@node'], [true, 0.63]])
    cases.push({ kind: 'variants', metalnessNode, roughnessNode, observeAssign: true, roughnessBody: true });
for (const specularOnly of [false, true]) for (const overrideSpecular of [false, true]) for (const reenter of [false, true])
  cases.push({ kind: 'variants', specularOnly, overrideSpecular, reenter, observeAssign: true });
for (const throwAt of ['get:metalnessNode:1', 'get:metalnessNode:2', 'get:roughnessNode:1', 'get:roughnessNode:2',
  'get:setupSpecular', 'call:setupSpecular', ...['metalness', 'roughness', 'specularColor', 'specularColorBlended', 'specularF90', 'diffuseContribution']
    .flatMap(key => ['get:assign:' + key, 'call:assign:' + key])])
  cases.push({ kind: 'variants', metalnessNode: [true, 0.43], roughnessNode: [true, 0.67], observeAssign: true, throwAt });

const expected = cases.map(run);
const directory = outputArgument && path.resolve(outputArgument);
if (directory) {
  fs.mkdirSync(directory, { recursive: true });
  fs.writeFileSync(path.join(directory, 'input.json'), JSON.stringify(cases, null, 2));
  fs.writeFileSync(path.join(directory, 'expected.json'), JSON.stringify(expected, null, 2));
}
if (!binaryArgument || binaryArgument === '--oracle') {
  console.log(`MeshStandardNodeMaterial oracle: ${cases.length} scenarios`);
  process.exit(0);
}
const actual = JSON.parse(execFileSync(path.resolve(binaryArgument), [], {
  encoding: 'utf8', input: JSON.stringify(cases), maxBuffer: 128 * 1024 * 1024
}));
assert.equal(actual.length, cases.length);
const differences = [];
for (let i = 0; i < cases.length; ++i) {
  try { assert.deepEqual(actual[i], expected[i]); }
  catch { differences.push({ index: i, input: cases[i], expected: expected[i], actual: actual[i] }); }
}
if (directory) {
  fs.writeFileSync(path.join(directory, 'actual.json'), JSON.stringify(actual, null, 2));
  fs.writeFileSync(path.join(directory, 'differences.json'), JSON.stringify(differences, null, 2));
}
assert.equal(differences.length, 0, differences.length ? JSON.stringify(differences[0], null, 2).slice(0, 6000) : undefined);
console.log(`MeshStandardNodeMaterial: ${cases.length} pinned scenarios passed`);
