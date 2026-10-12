import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
const revision = execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim();
assert.equal(revision, lock.commit, 'Pinned upstream commit');
const T = await import(pathToFileURL(path.join(upstream, 'src/Three.Core.js')));

// Encode property order and graph identity explicitly. JSON alone would collapse
// holes/undefined, signed zero/nonfinite numbers, and all object aliases.
function trace(value) {
    const identities = new Map();
    function encode(value) {
        if (value === undefined) return { type: 'undefined' };
        if (value === null) return { type: 'null' };
        if (typeof value === 'number') return {
            type: 'number', value: Number.isNaN(value) ? 'NaN' :
                value === Infinity ? 'Infinity' : value === -Infinity ? '-Infinity' :
                    Object.is(value, -0) ? '-0' : value
        };
        if (typeof value !== 'object') return { type: typeof value, value };
        if (identities.has(value)) return { type: 'reference', id: identities.get(value) };
        const id = identities.size + 1;
        identities.set(value, id);
        const result = { type: Array.isArray(value) ? 'array' : 'object', id,
            properties: Object.keys(value).map(key => [key, encode(value[key])]) };
        if (Array.isArray(value)) result.length = value.length;
        return result;
    }
    return encode(value);
}
const expected = {};
const emit = (name, value) => expected[name] = trace(value);
emit('undefined', undefined); emit('null', null); emit('negativeZero', -0);
emit('nan', NaN); emit('positiveInfinity', Infinity); emit('negativeInfinity', -Infinity);
const object = {};
object.z = 1; object['10'] = 10; object['2'] = 2; object['01'] = 1; object.a = undefined;
const alias = object; alias.z = 8;
emit('object', object);
emit('relations', [alias === object, object === {}, Object.hasOwn(object, 'a'),
    Object.hasOwn(object, 'absent'), -0 === 0, NaN === NaN, 1 === '1']);
emit('missingProperty', object.absent);
delete object.z; object.z = 3; emit('reinserted', object);
const sparse = new Array(4); sparse[1] = undefined; sparse[3] = null; sparse.tag = 'present';
emit('sparse', sparse); sparse.length = 2; emit('truncated', sparse);
sparse[5] = 5; sparse['4294967295'] = 'string key'; emit('expanded', sparse);
const graph = {}, child = { value: 7 };
graph.first = child; graph.second = child; child.parent = graph; emit('cyclicAliases', graph);
const errorName = action => { try { action(); } catch (error) { return error.name; } return 'no error'; };
emit('errorClasses', [errorName(() => undefined.x), errorName(() => null.x),
    errorName(() => sparse.length = -1), errorName(() => sparse.length = 1.5),
    errorName(() => sparse.length = Infinity), errorName(() => JSON.stringify(graph))]);
const jsonArray = new Array(3); jsonArray[1] = undefined; jsonArray[2] = null;
emit('serialized', JSON.stringify({ z: 1, '2': 2, undefined: undefined, nan: NaN, array: jsonArray }));
const v = new T.Vector3(1, 2, 3); v.x = -0; v.y = 8;
emit('vectorProperties', { values: v.toArray(), returnSelf: v.setZ(9) === v });
const color = new T.Color(); color.r = .1; color.g = .2; color.b = .3; emit('colorProperties', color.toArray());
const quaternion = new T.Quaternion(), quaternionEvents = [];
quaternion._onChange(() => quaternionEvents.push(quaternion.toArray()));
quaternion.x = .25; quaternion.x = .25; quaternion.y = -.5;
quaternion.set(1, 2, 3, 4); quaternion.normalize();
quaternion.setFromEuler(new T.Euler(.2, .3, .4), false); quaternion.z = .8;
emit('quaternionCallbacks', quaternionEvents);
const euler = new T.Euler(), eulerEvents = [];
euler._onChange(() => eulerEvents.push(euler.toArray()));
euler.x = .1; euler.x = .1; euler.order = 'YXZ'; euler.set(.2, .3, .4, 'ZYX');
euler.setFromQuaternion(new T.Quaternion(0, 0, 0, 1), undefined, false); euler.y = .5;
emit('eulerCallbacks', eulerEvents);
const transform = new T.Object3D(); transform.rotation.x = .3; transform.rotation.y = -.2;
const synchronization = { quaternion: transform.quaternion.toArray() };
transform.quaternion.set(0, 0, 0, 1); synchronization.rotation = transform.rotation.toArray().slice(0, 3);
emit('transformSynchronization', synchronization);
const parameters = { userData: { value: 1 } }, material = new T.MeshBasicMaterial();
material.setValues(parameters);
const shared = { parameterAlias: material.userData === parameters.userData };
parameters.userData.value = 2;
const serialized = material.toJSON(); shared.serializationAlias = material.userData === serialized.userData;
const clone = material.clone(); material.userData.value = 3;
Object.assign(shared, { source: material.userData.value, serialized: serialized.userData.value, cloned: clone.userData.value });
emit('materialSharing', shared);

const actual = JSON.parse(execFileSync(path.resolve(process.argv[3]), [], { encoding: 'utf8' }));
let comparisons = 0;
function compare(a, b, location) {
    ++comparisons;
    if (typeof b === 'number' && typeof a === 'number' && !Number.isInteger(b)) {
        assert.ok(Math.abs(a - b) <= 2e-13 * Math.max(1, Math.abs(b)), `${location}: ${a} != ${b}`);
        return;
    }
    if (b !== null && typeof b === 'object') {
        assert.equal(Array.isArray(a), Array.isArray(b), location);
        assert.deepEqual(Object.keys(a), Object.keys(b), `${location}: ordered fields`);
        for (const key of Object.keys(b)) compare(a[key], b[key], `${location}.${key}`);
    } else assert.equal(a, b, location);
}
compare(actual, expected, 'runtime');
console.log(`Runtime parity: ${Object.keys(expected).length} typed traces, ${comparisons} comparisons`);
