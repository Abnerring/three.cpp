import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: LineBasicNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/LineBasicNodeMaterial.js')).href);
const { default: MeshNormalNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/MeshNormalNodeMaterial.js')).href);

// The same structural snapshot the probe takes: own property names, the kind of every value, and every
// scalar. Object values are compared by structure rather than by content, which is what a material can
// be checked on without a renderer.
const snapshot = (material) => {
  // uuid is random per instance. id is excluded because this port's material base carries one while
  // the pinned class does not, a deviation recorded in the handoff and not hidden here.
  const names = Object.keys(material).filter((name) => name !== 'uuid' && name !== 'id').sort();
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
    if (kind === 'number' || kind === 'boolean' || kind === 'string') scalars[name] = value;
  }
  return { names, kinds, scalars };
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
  lineFog: line.fog,
  lineLinewidth: line.linewidth,
  lineSnapshot: snapshot(line),
  normalName: MeshNormalNodeMaterial.name,
  normalLength: MeshNormalNodeMaterial.length,
  normalStaticType: MeshNormalNodeMaterial.type,
  normalType: normal.type,
  normalFlag: normal.isMeshNormalNodeMaterial,
  normalIsNodeMaterial: normal.isNodeMaterial,
  normalFlatShading: normal.flatShading,
  normalSnapshot: snapshot(normal)
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('LineBasicNodeMaterial and MeshNormalNodeMaterial parity passed');
