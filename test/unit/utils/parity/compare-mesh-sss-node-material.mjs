import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const [upstream, probe] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const load = file => import(pathToFileURL(path.join(upstream, 'src', file)));
const {default: Material} = await load('materials/nodes/MeshSSSNodeMaterial.js');
const {float, vec3} = await load('nodes/tsl/TSLBase.js');
const {normalView} = await load('nodes/accessors/Normal.js');
const {positionViewDirection} = await load('nodes/accessors/Position.js');
const object = () => Object.create(null);
const fields = ['thicknessColorNode', 'thicknessDistortionNode', 'thicknessAmbientNode',
  'thicknessAttenuationNode', 'thicknessPowerNode', 'thicknessScaleNode'];

function graph(value) {
  if (value === normalView) return '@normalView';
  if (value === positionViewDirection) return '@positionViewDirection';
  if (value === undefined) return '@undefined';
  if (value === null || typeof value !== 'object') return value;
  if (Array.isArray(value)) return value.map(graph);
  const out = object();
  if (value.isNode === true) {
    out.type = value.type;
    for (const key of ['nodeType', 'value', 'method', 'op', 'aNode', 'bNode', 'cNode',
      'node', 'nodes', 'convertTo', 'components', 'property', 'global']) {
      if (Object.hasOwn(value, key)) out[key] = graph(value[key]);
    }
  } else {
    for (const key of Object.keys(value)) out[key] = graph(value[key]);
  }
  return out;
}
const defaults = material => Object.fromEntries(fields.map(key => [key, graph(material[key])]));
function metadata() {
  const material = new Material(), prototype = Material.prototype;
  const model = material.setupLightingModel(), modelCtor = model.constructor;
  return {
    name: Material.name, length: Material.length, type: Material.type,
    parent: Object.getPrototypeOf(Material).name, materialType: material.type,
    ownFlag: Object.hasOwn(material, 'isMeshSSSNodeMaterial'),
    prototypeNames: Object.getOwnPropertyNames(prototype), source: Material.toString(),
    typeSource: Object.getOwnPropertyDescriptor(Material, 'type').get.toString(),
    useSSSSource: Object.getOwnPropertyDescriptor(prototype, 'useSSS').get.toString(),
    setupSource: prototype.setupLightingModel.toString(),
    modelName: modelCtor.name, modelLength: modelCtor.length,
    modelParent: Object.getPrototypeOf(modelCtor).name,
    modelPrototypeNames: Object.getOwnPropertyNames(modelCtor.prototype),
    modelSource: modelCtor.toString(), directSource: modelCtor.prototype.direct.toString(),
    directLength: modelCtor.prototype.direct.length, defaults: defaults(material)
  };
}
function constructorAndCopy() {
  const parameters = {roughness: 0.25, thickness: 2};
  for (const key of ['clearcoat', 'sheen', 'iridescence', 'anisotropy', 'transmission', 'dispersion',
    'retroreflectivity', 'diffuseRoughness']) parameters[key] = 1;
  const material = new Material(parameters);
  const out = {roughness: material.roughness, thickness: material.thickness,
    defaults: defaults(material), initialSSS: material.useSSS};
  const color = vec3(0.8, 0.4, 0.2), power = float(7);
  out.enabled = [null, false, 0, undefined, color].map(value => {
    material.thicknessColorNode = value;
    return material.useSSS;
  });
  material.thicknessPowerNode = power;
  const model = material.setupLightingModel();
  out.flags = Object.fromEntries(['clearcoat', 'sheen', 'iridescence', 'anisotropy', 'transmission',
    'dispersion', 'retroreflection', 'diffuseRoughness', 'useSSS'].map(key => [key, model[key]]));
  out.distinctModel = material.setupLightingModel() !== model;
  const clone = material.clone();
  out.cloneType = clone.type;
  out.cloneColorSame = clone.thicknessColorNode === color;
  out.clonePowerSame = clone.thicknessPowerNode === power;
  const destination = new Material(), customScale = object(), copyState = object();
  customScale.copy = function (value) {
    copyState.receiver = this === customScale;
    copyState.source = value === material.thicknessScaleNode;
    return this;
  };
  destination.thicknessScaleNode = customScale;
  out.copyReturnsSelf = destination.copy(material) === destination;
  out.copyColorSame = destination.thicknessColorNode === color;
  out.customScaleRetained = destination.thicknessScaleNode === customScale;
  out.copyState = copyState;
  return out;
}
function observed(target, key, value, events, label) {
  Object.defineProperty(target, key, {configurable: true, enumerable: true,
    get() { events.push(label); return value; }});
}
function directCase(gate, fail = false, reenter = false) {
  const material = new Material();
  material.thicknessColorNode = vec3(0.8, 0.4, 0.2);
  const model = material.setupLightingModel(), method = model.direct;
  const events = [], graphs = [], forwards = [];
  let nested = false;
  const input = object(), builder = object(), reflected = object(), diffuse = object();
  const direction = vec3(0, 0, -1), color = vec3(2, 3, 4);
  reflected.directDiffuse = diffuse;
  observed(input, 'lightDirection', direction, events, 'input.lightDirection');
  observed(input, 'lightColor', color, events, 'input.lightColor');
  observed(input, 'reflectedLight', reflected, events, 'input.reflectedLight');
  input.extra = 'must-not-forward';
  observed(model, 'useSSS', gate, events, 'self.useSSS');
  observed(builder, 'material', material, events, 'builder.material');
  for (const key of fields) observed(material, key, material[key], events, `material.${key}`);
  diffuse.addAssign = value => {
    events.push('addAssign'); graphs.push(graph(value));
    if (fail) throw new TypeError('diffuse-failure');
    if (reenter && !nested) { nested = true; method.call(model, input, builder); }
    return 'ignored';
  };
  const prototype = model.constructor.prototype, originalBase = Object.getPrototypeOf(prototype);
  const fixtureBase = Object.create(originalBase);
  fixtureBase.direct = function (value, receivedBuilder) {
    events.push('super.direct');
    forwards.push({keys: Object.keys(value), receiver: this === model, builder: receivedBuilder === builder,
      distinctInput: value !== input, direction: value.lightDirection === direction,
      color: value.lightColor === color, reflected: value.reflectedLight === reflected});
    return 'ignored';
  };
  Object.setPrototypeOf(prototype, fixtureBase);
  const out = object();
  try { out.result = method.call(model, input, builder) === undefined ? 'undefined' : 'value'; }
  catch (error) { if (!(error instanceof TypeError)) throw error; out.result = 'TypeError'; }
  finally { Object.setPrototypeOf(prototype, originalBase); }
  Object.assign(out, {events, graphs, forwards});
  return out;
}

const expected = {metadata: metadata(), constructorAndCopy: constructorAndCopy(),
  direct: [directCase(false), directCase(1), directCase(true), directCase(true, true), directCase(true, false, true)]};
if (probe === '--oracle') {
  console.log(JSON.stringify(expected));
} else {
  const actual = JSON.parse(execFileSync(path.resolve(probe), {encoding: 'utf8'}));
  assert.deepEqual(actual, JSON.parse(JSON.stringify(expected)));
  console.log('MeshSSSNodeMaterial constructor, inherited copy, full SSS graph and direct protocol parity passed');
}
