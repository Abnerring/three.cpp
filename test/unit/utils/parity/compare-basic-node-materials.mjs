import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: LineBasicNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/LineBasicNodeMaterial.js')).href);
const { default: MeshNormalNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/MeshNormalNodeMaterial.js')).href);

// The same structural snapshot the probe takes.
//
// The enumeration has to be getOwnPropertyNames rather than Object.keys. An earlier run used Object.keys
// here while the probe used own property names, which made the material id look like a deviation: it is
// defined with a value only descriptor, so it is an own property that Object.keys hides on both sides.
const snapshot = (material) => {
  const names = Object.getOwnPropertyNames(material).filter((name) => name !== 'uuid').sort();
  const kinds = {};
  const scalars = {};
  for (const name of names) {
    const value = material[name];
    let kind = 'object';
    if (value === null) kind = 'null';
    else if (typeof value === 'number') kind = 'number';
    else if (typeof value === 'boolean') kind = 'boolean';
    else if (typeof value === 'string') kind = 'string';
    else if (typeof value === 'function') kind = 'function';
    else if (value === undefined) kind = 'undefined';
    kinds[name] = kind;
    // The id counter is per process, so only its name and kind are comparable.
    if (name === 'id') continue;
    if (kind === 'number' || kind === 'boolean' || kind === 'string') scalars[name] = value;
  }
  return { names, kinds, scalars };
};

const descriptorOf = (material) => {
  const descriptor = Object.getOwnPropertyDescriptor(material, 'id');
  return {
    present: descriptor !== undefined,
    writable: descriptor ? descriptor.writable === true : false,
    enumerable: descriptor ? descriptor.enumerable === true : false,
    configurable: descriptor ? descriptor.configurable === true : false
  };
};

const line = new LineBasicNodeMaterial();
const normal = new MeshNormalNodeMaterial();

const expected = {
  lineName: LineBasicNodeMaterial.name,
  lineLength: LineBasicNodeMaterial.length,
  lineStaticType: LineBasicNodeMaterial.type,
  lineType: line.type,
  lineFlag: line.isLineBasicNodeMaterial,
  lineIsNodeMaterial: line.isNodeMaterial,
  lineIdDescriptor: descriptorOf(line),
  lineFog: line.fog,
  lineLinewidth: line.linewidth,
  lineSnapshot: snapshot(line),
  normalName: MeshNormalNodeMaterial.name,
  normalLength: MeshNormalNodeMaterial.length,
  normalStaticType: MeshNormalNodeMaterial.type,
  normalType: normal.type,
  normalFlag: normal.isMeshNormalNodeMaterial,
  normalIsNodeMaterial: normal.isNodeMaterial,
  normalIdDescriptor: descriptorOf(normal),
  normalFlatShading: normal.flatShading,
  normalSnapshot: snapshot(normal)
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('LineBasicNodeMaterial and MeshNormalNodeMaterial parity passed');