import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {mkdirSync, writeFileSync} from 'node:fs';
import {isDeepStrictEqual} from 'node:util';

const [upstream, probe] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const load = file => import(pathToFileURL(path.join(upstream, 'src', file)));
const module = await load('nodes/functions/BSDF/BRDF_EON.js');
const {default: brdf, EON_DirectionalAlbedo: directional} = module;
const {float, vec3} = await load('nodes/tsl/TSLBase.js');
const {normalView} = await load('nodes/accessors/Normal.js');
const {positionViewDirection} = await load('nodes/accessors/Position.js');
const fields = ['nodeType', 'value', 'method', 'op', 'aNode', 'bNode', 'cNode', 'node', 'nodes',
  'condNode', 'ifNode', 'elseNode', 'convertTo', 'components', 'global', 'shaderNode', 'rawInputs', 'jsFunc', 'layout', 'once'];

function graph(root) {
  const seen = new Map(), objects = [];
  function encode(value) {
    if (value === normalView) return '@normalView';
    if (value === positionViewDirection) return '@positionViewDirection';
    if (value === undefined) return '@undefined';
    if (typeof value === 'function') return value.toString();
    if (typeof value === 'string' && /^fn\d+$/.test(value)) return '@function';
    if (value === null || typeof value !== 'object') return value;
    if (seen.has(value)) return {ref: seen.get(value)};
    const ref = {ref: seen.size}; seen.set(value, seen.size);
    const row = {properties: []}; objects.push(row);
    if (Array.isArray(value)) { row.kind = 'array'; row.properties = value.map(encode); }
    else {
      row.kind = value.isNode === true ? value.type : 'object';
      const keys = value.isNode === true ? fields : Object.keys(value);
      for (const key of keys) if (Object.hasOwn(value, key)) row.properties.push([key, encode(value[key])]);
    }
    return ref;
  }
  return {root: encode(root), objects};
}
function inputs(defaults = false) {
  return {lightDirection: vec3(.2, .7, .6), diffuseColor: vec3(.5, .4, .3), roughness: float(.35),
    ...(!defaults ? {normalView: vec3(0, 0, 1), viewDirection: vec3(.1, .2, .97)} : {})};
}
const directionalInputs = (roughness = .35, dotNV = .6) => ({diffuseColor: vec3(1.2, -.2, .5), roughness: float(roughness), dotNV: float(dotNV)});
function findFON(value, seen = new Set()) {
  if (value === null || typeof value !== 'object' || seen.has(value)) return;
  seen.add(value);
  if (Object.hasOwn(value, 'jsFunc') && value.jsFunc.toString().includes('const muComp = mu.oneMinus()')) return value.jsFunc;
  const keys = value.isNode === true ? fields : Object.keys(value);
  for (const key of keys) if (Object.hasOwn(value, key)) {
    const found = findFON(value[key], seen); if (found) return found;
  }
}
function numeric(value) {
  return {value, oneMinus() { return numeric(1 - this.value); },
    mul(rhs) { return numeric(this.value * (typeof rhs === 'object' ? rhs.value : rhs)); },
    add(rhs) { return numeric(this.value + (typeof rhs === 'object' ? rhs.value : rhs)); },
    sub(rhs) { return numeric(this.value - (typeof rhs === 'object' ? rhs.value : rhs)); }};
}
function fonValues() {
  const fon = findFON(directional.shaderNode.jsFunc(directionalInputs()));
  assert.equal(typeof fon, 'function');
  return [0, .25, .5, .75, 1].map(mu => fon({mu: numeric(mu), roughness: numeric(.65), A: numeric(.7)}).value);
}
function metadata(fn) {
  const {shaderNode: shader} = fn;
  const chainLength = fn.length;
  const length = typeof chainLength === 'function' ? {kind: 'function', name: chainLength.name,
    length: chainLength.length, source: chainLength.toString()} : {kind: 'value', value: chainLength};
  return {name: fn.name, length, isFn: fn.isFn, source: shader.jsFunc.toString(),
    callbackLength: shader.jsFunc.length, layout: shader.layout ? {type: shader.layout.type, inputs: shader.layout.inputs} : null};
}
const expected = {exports: Object.keys(module), default: metadata(brdf), directional: metadata(directional),
  graphs: [graph(brdf.shaderNode.jsFunc(inputs())), graph(brdf.shaderNode.jsFunc(inputs(true))),
    ...[[0, -.3], [1e-8, 0], [.35, .6], [1, 1.4]].map(([r, n]) => graph(directional.shaderNode.jsFunc(directionalInputs(r, n))))],
  fonValues: fonValues()};
if (probe === '--oracle') console.log(JSON.stringify(expected));
else {
  const actual = JSON.parse(execFileSync(path.resolve(probe), {encoding: 'utf8'}));
  if (!isDeepStrictEqual(actual, expected)) {
    const directory = 'D:/Temp/surui/brdf-eon-parity';
    mkdirSync(directory, {recursive: true});
    writeFileSync(path.join(directory, 'actual.json'), JSON.stringify(actual, null, 2));
    writeFileSync(path.join(directory, 'expected.json'), JSON.stringify(expected, null, 2));
    function firstDifference(a, b, location = '$') {
      if (isDeepStrictEqual(a, b)) return;
      if (a === null || b === null || typeof a !== 'object' || typeof b !== 'object')
        return {path: location, actual: a, expected: b};
      const aKeys = Object.keys(a), bKeys = Object.keys(b);
      const missing = bKeys.filter(key => !Object.hasOwn(a, key)), extra = aKeys.filter(key => !Object.hasOwn(b, key));
      if (missing.length || extra.length) return {path: location, missing, extra};
      for (const key of bKeys) {
        const result = firstDifference(a[key], b[key], `${location}.${key}`);
        if (result) return result;
      }
      return {path: location, actual: a, expected: b};
    }
    assert.fail(`BRDF_EON first difference: ${JSON.stringify(firstDifference(actual, expected))}; snapshots: ${directory}`);
  }
  console.log('BRDF_EON exports, layouts, callback metadata, six complete graphs and quartic numeric parity passed');
}
