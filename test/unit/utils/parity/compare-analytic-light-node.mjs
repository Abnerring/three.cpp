import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [upstream, probe] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const {default: AnalyticLightNode} = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/AnalyticLightNode.js')));
const {float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')));

const object = () => Object.create(null);
const tagged = tag => Object.assign(object(), {tag});
const tag = value => value === null ? 'null' : value === undefined ? 'undefined'
  : typeof value !== 'object' && typeof value !== 'function' ? value : value.tag ?? (value.isNode ? 'node' : 'object');
const invoke = (name, self, args = []) => AnalyticLightNode.prototype[name].apply(self, args);
const snapshot = self => Object.fromEntries(['colorNode', 'baseColorNode', 'shadowNode', 'shadowColorNode'].map(key => [key, tag(self[key])]));
const attempt = (method, self, args) => {
  try { return invoke(method, self, args) === undefined ? 'undefined' : 'value'; }
  catch (error) { return error instanceof TypeError ? 'TypeError' : tag(error); }
};
const baseState = () => Object.assign(object(), {colorNode: tagged('color'), baseColorNode: null, shadowNode: null, shadowColorNode: null});

function metadata() {
  const methods = {};
  for (const name of ['dispose', 'disposeShadow', 'getHash', 'getLightVector', 'setupDirect', 'setupDirectRectArea', 'setupShadowNode', 'setupShadow', 'setup', 'update']) {
    const fn = AnalyticLightNode.prototype[name];
    methods[name] = {name: fn.name, length: fn.length, source: fn.toString()};
  }
  return {name: AnalyticLightNode.name, length: AnalyticLightNode.length, type: AnalyticLightNode.type,
    source: AnalyticLightNode.toString(), prototypeKeys: Object.getOwnPropertyNames(AnalyticLightNode.prototype),
    typeSource: Object.getOwnPropertyDescriptor(AnalyticLightNode, 'type').get.toString(), methods};
}

function constructorCases() {
  const out = [];
  for (let variant = 0; variant < 5; variant++) {
    const light = Object.assign(tagged('light'), {uuid: 'uuid'});
    if (variant === 1) light.colorNode = tagged('custom');
    if (variant === 2) light.colorNode = 0;
    if (variant === 3) light.colorNode = null;
    const node = new AnalyticLightNode(variant === 4 ? null : light);
    out.push({type: node.type, lightSame: node.light === (variant === 4 ? null : light),
      colorTag: tag(node.colorNode), uniform: node.colorNode.type === 'UniformNode',
      group: variant === 1 ? null : node.colorNode.groupNode.name,
      flags: node.isAnalyticLightNode === true && node.isLightingNode === true,
      baseNull: node.baseColorNode === null, shadowNull: node.shadowNode === null,
      shadowColorNull: node.shadowColorNode === null, updateType: node.updateType,
      hasListener: Object.hasOwn(node, '_shadowDisposeListener')});
  }
  return out;
}

function listenerLifecycle() {
  const light = Object.assign(tagged('light'), {shadow: object()});
  const events = [];
  let listener;
  light.addEventListener = function(type, callback) { listener = callback; events.push(this === light && type === 'dispose' ? 'add:dispose' : 'bad-add'); };
  light.removeEventListener = function(type, callback) { events.push(this === light && type === 'dispose' && callback === listener ? 'remove:dispose:same' : 'bad-remove'); };
  const node = new AnalyticLightNode(light);
  node.disposeShadow = function() { events.push(this === node ? 'shadow:lexical-self' : 'bad-self'); };
  node.dispatchEvent = function(event) { events.push(this === node && event.type === 'dispose' ? 'node:dispose' : 'bad-event'); };
  listener.call(object());
  const result = attempt('dispose', node);
  return {events, result, listenerSame: node._shadowDisposeListener === listener, source: listener.toString()};
}

function disposalCases() {
  const rows = [];
  for (let variant = 0; variant < 4; variant++) {
    const self = baseState(), events = [], shadow = tagged('shadow'), base = tagged('base');
    Object.assign(self, {colorNode: tagged('shaded'), shadowColorNode: tagged('shaded'), shadowNode: shadow, baseColorNode: base});
    shadow.dispose = function() {
      events.push(this === shadow ? 'dispose:self' : 'dispose:bad-self', snapshot(self));
      if (variant === 1) throw tagged('dispose-error');
      if (variant === 2) Object.assign(self, {baseColorNode: tagged('reentrant-base'), shadowColorNode: tagged('reentrant-color')});
    };
    if (variant === 3) Object.assign(self, {shadowNode: null, baseColorNode: null});
    const result = attempt('disposeShadow', self);
    rows.push({result, state: snapshot(self), events});
  }
  return rows;
}

function shadowCases() {
  const rows = [];
  for (let variant = 0; variant < 11; variant++) {
    const self = baseState(), light = object(), lightShadow = object(), builder = object(), renderer = object(), map = object(), context = object(), events = [];
    const shadow = tagged('shadow'), shaded = tagged('shaded'), color = self.colorNode, custom = float(0.5);
    if (variant === 1) lightShadow.shadowNode = custom;
    if (variant === 9) lightShadow.shadowNode = null;
    if (variant === 10) lightShadow.shadowNode = 0;
    light.shadow = lightShadow; self.light = light; map.enabled = variant === 0 ? false : variant === 8 ? 0 : true;
    renderer.shadowMap = map; Object.assign(builder, {renderer, context});
    color.mul = function(input) {
      events.push(this === color ? 'mul:self' : 'mul:bad-self', tag(input));
      if (variant === 4) throw tagged('mul-error');
      return shaded;
    };
    self.setupShadowNode = function() { events.push(this === self ? 'default:self' : 'default:bad-self'); return shadow; };
    shadow.dispose = () => { events.push('shadow:dispose'); };
    self.disposeShadow = AnalyticLightNode.prototype.disposeShadow;
    if (variant === 2) {
      Object.assign(self, {shadowColorNode: tagged('cached'), shadowNode: shadow, baseColorNode: tagged('base')});
      delete self.light;
    }
    if (variant === 3 || variant === 5 || variant === 6) context.getShadow = function(node, passedBuilder) {
      events.push(this === context && node === self && passedBuilder === builder ? 'getShadow:args' : 'getShadow:bad-args', snapshot(self));
      if (variant === 5) throw tagged('getShadow-error');
      if (variant === 6) invoke('disposeShadow', self);
      return tagged('override');
    };
    if (variant === 7) { map.enabled = false; context.getShadow = true; }
    const result = attempt('setupShadow', self, [builder]);
    rows.push({result, state: snapshot(self), events, customSame: variant === 1 ? self.shadowNode === custom : false});
  }
  return rows;
}

function setupCases() {
  const rows = [];
  for (let variant = 0; variant < 7; variant++) {
    const self = baseState(), light = object(), builder = object(), object3d = object(), lights = object(), events = [];
    light.castShadow = variant === 1 || variant === 2; object3d.receiveShadow = variant === 1;
    self.light = light; Object.assign(builder, {object: object3d, lightsNode: lights});
    if (variant === 3) {
      const shadow = tagged('shadow'); shadow.dispose = () => { events.push('shadow:dispose'); };
      Object.assign(self, {baseColorNode: tagged('base'), shadowNode: shadow, shadowColorNode: tagged('shaded')});
    }
    const direct = tagged('direct'), rect = tagged('rect');
    for (const key of ['setupDirect', 'setupDirectRectArea', 'setupShadow']) self[key] = function(passedBuilder) {
      events.push(key, this === self && passedBuilder === builder, tag(this.colorNode));
      if (variant === 4 && key === 'setupDirectRectArea') throw tagged('rect-error');
      if (key === 'setupShadow') { this.colorNode = tagged('shadowed'); return; }
      if (variant === 5) return 0;
      return key === 'setupDirect' ? direct : rect;
    };
    for (const key of ['setupDirectLight', 'setupDirectRectAreaLight']) lights[key] = function(passedBuilder, node, data) {
      events.push(key, this === lights && passedBuilder === builder && node === self, tag(data));
      if (variant === 6 && key === 'setupDirectLight') throw tagged('dispatch-error');
    };
    const result = attempt('setup', self, [builder]); rows.push({result, state: snapshot(self), events});
  }
  return rows;
}

function defaultFactory() {
  const light = Object.assign(object(), {shadow: object()}), self = Object.assign(object(), {light});
  const shadow = invoke('setupShadowNode', self);
  return {type: shadow.type, isShadowNode: shadow.isShadowNode, lightSame: shadow.light === light, shadowSame: shadow.shadow === light.shadow};
}

function updateCases() {
  const rows = [];
  for (let variant = 0; variant < 7; variant++) {
    const self = object(), light = object(), source = tagged('light-color'), color = tagged('color'), returned = tagged('copy-result'), events = [];
    light.color = source; Object.assign(self, {color, light: variant === 3 ? null : light});
    Object.defineProperty(light, 'intensity', {configurable: true, get() { events.push('get:intensity'); if (variant === 5) throw tagged('intensity-error'); return 3; }});
    color.copy = function(value) {
      events.push(this === color && value === source ? 'copy:args' : 'copy:bad-args');
      if (variant === 2) throw tagged('copy-error');
      if (variant === 4) self.color = tagged('replaced-color');
      return variant === 1 ? 0 : returned;
    };
    returned.multiplyScalar = function(scalar) { events.push(this === returned ? 'multiply:return-value' : 'multiply:bad-self', scalar); return this; };
    if (variant === 6) Object.defineProperty(returned, 'multiplyScalar', {configurable: true, get() { events.push('get:multiplyScalar'); throw tagged('multiply-error'); }});
    const result = attempt('update', self); rows.push({result, events, color: tag(self.color)});
  }
  return rows;
}

const expected = {metadata: metadata(), constructor: constructorCases(), listener: listenerLifecycle(), disposal: disposalCases(), shadow: shadowCases(), setup: setupCases(), defaultFactory: defaultFactory(), update: updateCases()};
if (probe === '--oracle-only') {
  console.log(JSON.stringify(expected, null, 2));
} else {
  const actual = JSON.parse(execFileSync(path.resolve(probe), {encoding: 'utf8'}));
  assert.deepEqual(actual, expected);
  console.log('AnalyticLightNode: full source metadata and 36 construction, lifecycle, shadow, dispatch, update scenarios passed');
}
