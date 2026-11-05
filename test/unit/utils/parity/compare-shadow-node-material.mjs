import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: ShadowMaskModel } = await import(pathToFileURL(path.join(upstream, 'src/nodes/functions/ShadowMaskModel.js')).href);
const { default: ShadowNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/ShadowNodeMaterial.js')).href);

// The same snapshot the probe takes: own property names, the kind of every value, and every scalar,
// skipping the random uuid and the per process id value.
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
    if (name === 'id') continue;
    if (kind === 'number' || kind === 'boolean' || kind === 'string') scalars[name] = value;
  }
  return { names, kinds, scalars };
};

const model = new ShadowMaskModel();
model.direct({ lightNode: { shadowNode: null } });
model.direct({ lightNode: { shadowNode: model.shadowNode } });
const outgoing = { count: 0 };
outgoing.rgb = { assign: () => { outgoing.count += 1; } };
model.finish({ context: { outgoingLight: outgoing } });

const material = new ShadowNodeMaterial();
const builtModel = material.setupLightingModel({});
const secondModel = material.setupLightingModel({});

const expected = {
  maskName: ShadowMaskModel.name,
  maskLength: ShadowMaskModel.length,
  shadowNodeType: model.shadowNode.type,
  shadowNodeIsNode: model.shadowNode.isNode === true,
  hook_start: model.start.length,
  hook_finish: model.finish.length,
  hook_direct: model.direct.length,
  hook_directRectArea: model.directRectArea.length,
  hook_indirect: model.indirect.length,
  hook_ambientOcclusion: model.ambientOcclusion.length,
  finishAssigns: outgoing.count,
  materialName: ShadowNodeMaterial.name,
  materialLength: ShadowNodeMaterial.length,
  materialStaticType: ShadowNodeMaterial.type,
  materialType: material.type,
  materialFlag: material.isShadowNodeMaterial,
  materialIsNodeMaterial: material.isNodeMaterial,
  materialLights: material.lights,
  materialTransparent: material.transparent,
  materialSnapshot: snapshot(material),
  builtShadowNodeType: builtModel.shadowNode.type,
  builtDirectLength: builtModel.direct.length,
  builtFinishLength: builtModel.finish.length,
  distinctModels: secondModel !== builtModel
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('ShadowMaskModel and ShadowNodeMaterial parity passed');
