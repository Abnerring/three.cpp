import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const [upstream, probe] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const load = file => import(pathToFileURL(path.join(upstream, 'src', file)));
const lightingModule = await load('nodes/core/LightingModel.js');
const {default: LightingModel} = lightingModule;
const {default: BasicLightingModel} = await load('nodes/functions/BasicLightingModel.js');
const {default: PhongLightingModel} = await load('nodes/functions/PhongLightingModel.js');
const object = () => Object.create(null);
const modes = ['baseline', 'throw:get lightsNode:1', 'throw:get setupLights', 'throw:get lightsNode:2',
  'throw:get getLightNodes', 'throw:call getLightNodes', 'throw:call setupLights', 'throw:get indirect',
  'throw:call indirect', 'badSetup', 'badGetNodes', 'badIndirect', 'nullFirst', 'nullSecond',
  'reenter:call getLightNodes', 'reenter:call setupLights', 'reenter:call indirect', 'replaceSetup'];

const descriptor = d => ({writable: Object.hasOwn(d, 'writable') ? d.writable : '@absent',
  enumerable: d.enumerable, configurable: d.configurable,
  getter: d.get !== undefined, setter: d.set !== undefined});
function metadata() {
  const ctor = LightingModel, prototype = ctor.prototype;
  return {
    exports: Object.keys(lightingModule), name: ctor.name, length: ctor.length,
    constructible: true, source: ctor.toString(), staticNames: Object.getOwnPropertyNames(ctor),
    prototypeNames: Object.getOwnPropertyNames(prototype),
    statics: Object.getOwnPropertyNames(ctor).map(key => ({...descriptor(Object.getOwnPropertyDescriptor(ctor, key)),
      key, ...(key !== 'prototype' ? {value: ctor[key]} : {})})),
    methods: Object.getOwnPropertyNames(prototype).map(key => ({...descriptor(Object.getOwnPropertyDescriptor(prototype, key)),
      key, name: prototype[key].name, length: prototype[key].length,
      constructible: key === 'constructor', source: prototype[key].toString()}))
  };
}
function constructionAndHooks() {
  const ctor = LightingModel, prototype = ctor.prototype;
  const first = new ctor(17), second = new ctor();
  function Derived() {}
  const derivedInstance = Reflect.construct(ctor, [], Derived);
  const out = {instanceOwnNames: Object.getOwnPropertyNames(first), distinct: first !== second,
    notConstructor: first !== ctor, notCallable: typeof first !== 'function',
    prototype: Object.getPrototypeOf(first) === prototype, constructor: first.constructor === ctor,
    instanceof: first instanceof ctor, objectPrototype: Object.getPrototypeOf(prototype) === Object.prototype,
    functionPrototype: Object.getPrototypeOf(ctor) === Function.prototype,
    newTargetPrototype: Object.getPrototypeOf(derivedInstance) === Derived.prototype,
    newTargetConstructor: derivedInstance.constructor === Derived};
  Derived.prototype = 7;
  out.primitivePrototypeFallback = Object.getPrototypeOf(Reflect.construct(ctor, [], Derived)) === Object.prototype;
  try { ctor.call(undefined); out.bareCall = 'value'; }
  catch (error) { if (!(error instanceof TypeError)) throw error; out.bareCall = 'TypeError'; }
  out.emptyHooks = ['finish', 'direct', 'directRectArea', 'indirect', 'ambientOcclusion'].map(key => {
    const fn = prototype[key], row = {key, undefined: fn.call(null, null, 17, false) === undefined};
    try { new fn(); row.new = 'value'; }
    catch (error) { if (!(error instanceof TypeError)) throw error; row.new = 'TypeError'; }
    return row;
  });
  Object.assign(out, {
    basicBase: Object.getPrototypeOf(BasicLightingModel) === ctor,
    basicPrototypeBase: Object.getPrototypeOf(BasicLightingModel.prototype) === prototype,
    phongBase: Object.getPrototypeOf(PhongLightingModel) === BasicLightingModel,
    phongCanonicalStart: PhongLightingModel.prototype.start === prototype.start,
    basicInstancePrototype: Object.getPrototypeOf(new BasicLightingModel()) === BasicLightingModel.prototype,
    phongInstancePrototype: Object.getPrototypeOf(new PhongLightingModel()) === PhongLightingModel.prototype
  });
  return out;
}
function startCase(mode) {
  const builder = object(), self = object(), first = object(), second = object();
  const events = [], checks = [], nodes = ['direct-light-list'];
  const start = LightingModel.prototype.start;
  let reads = 0, nested = false;
  function tick(label) {
    events.push(label);
    if (mode === `throw:${label}`) throw new TypeError(label);
  }
  function reenter(label) {
    if (mode === `reenter:${label}` && !nested) {
      nested = true;
      start.call(self, builder);
    }
  }
  Object.defineProperty(builder, 'lightsNode', {configurable: true, enumerable: true, get() {
    ++reads; tick(`get lightsNode:${reads}`); checks.push(this === builder);
    if ((reads === 1 && mode === 'nullFirst') || (reads === 2 && mode === 'nullSecond')) return null;
    return reads % 2 === 1 ? first : second;
  }});
  function setupLights(...args) {
    tick('call setupLights'); checks.push(this === first && args.length === 2 && args[0] === builder && args[1] === nodes);
    reenter('call setupLights'); return 'ignored';
  }
  Object.defineProperty(first, 'setupLights', {configurable: true, enumerable: true, get() {
    tick('get setupLights'); checks.push(this === first); return mode === 'badSetup' ? 17 : setupLights;
  }});
  function getLightNodes(...args) {
    tick('call getLightNodes'); checks.push(this === second && args.length === 1 && args[0] === builder);
    if (mode === 'replaceSetup') Object.defineProperty(first, 'setupLights', {
      writable: true, configurable: true, enumerable: true,
      value() { tick('wrong replacement'); }
    });
    reenter('call getLightNodes'); return nodes;
  }
  Object.defineProperty(second, 'getLightNodes', {configurable: true, enumerable: true, get() {
    tick('get getLightNodes'); checks.push(this === second); return mode === 'badGetNodes' ? 17 : getLightNodes;
  }});
  function indirect(...args) {
    tick('call indirect'); checks.push(this === self && args.length === 1 && args[0] === builder);
    reenter('call indirect'); return 'ignored';
  }
  Object.defineProperty(self, 'indirect', {configurable: true, enumerable: true, get() {
    tick('get indirect'); checks.push(this === self); return mode === 'badIndirect' ? 17 : indirect;
  }});
  const out = {mode};
  try { out.result = start.call(self, builder) === undefined ? 'undefined' : 'value'; }
  catch (error) { if (!(error instanceof TypeError)) throw error; out.result = 'TypeError'; }
  Object.assign(out, {events, checks, reads});
  return out;
}

const expected = {metadata: metadata(), constructionAndHooks: constructionAndHooks(), startCases: modes.map(startCase)};
if (probe === '--oracle') {
  console.log(JSON.stringify(expected));
} else {
  const actual = JSON.parse(execFileSync(path.resolve(probe), {encoding: 'utf8'}));
  assert.deepEqual(actual, expected);
  console.log('LightingModel metadata, construction, shared inheritance and 18 start protocol scenarios passed');
}
