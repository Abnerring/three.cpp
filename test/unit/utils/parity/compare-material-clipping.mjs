import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url), 'utf8'));
assert.equal(spawnSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).stdout.trim(), lock.commit);
const source = name => import(pathToFileURL(path.join(upstream, 'src', name)));
const { MeshBasicMaterial } = await source('materials/MeshBasicMaterial.js');
const { Plane } = await source('math/Plane.js');
const { Vector3 } = await source('math/Vector3.js');
const { ClippingGroup } = await source('objects/ClippingGroup.js');
const makePlane = n => new Plane(n % 2 ? new Vector3(0, 1, 0) : new Vector3(1, 0, 0), n + 2);
const planes = value => value === null ? null : value.map(p => p === null ? null : {
    normal: [p.normal.x, p.normal.y, p.normal.z], constant: p.constant
});

class Fixture {
    constructor() {
        this.source = new MeshBasicMaterial();
        this.target = new MeshBasicMaterial();
        this.p = makePlane(0); this.q = makePlane(1); this.r = makePlane(2);
        this.source.name = 'source'; this.target.name = 'target';
        this.source.clippingPlanes = [this.p, this.q];
        this.target.clippingPlanes = [this.r];
        this.original = this.source.clippingPlanes;
        this.targetOriginal = this.target.clippingPlanes;
    }
    snapshot() {
        const current = this.source.clippingPlanes, other = this.target.clippingPlanes;
        return {
            source: planes(current), target: planes(other), old: planes(this.original),
            targetOld: planes(this.targetOriginal), sameOld: current === this.original,
            sameTargetOld: other === this.targetOriginal, sameFields: current === other,
            names: [this.source.name, this.target.name]
        };
    }
}
function scenario(mode) {
    const f = new Fixture();
    let cloned, json = null, result = 'undefined';
    try {
        switch (mode) {
        case 0: break;
        case 1:
            f.target.clippingPlanes = f.source.clippingPlanes;
            f.target.clippingPlanes[0].constant = 12;
            break;
        case 2:
            f.source.clippingPlanes = f.targetOriginal;
            f.source.clippingPlanes.push(f.q);
            break;
        case 3: f.source.clippingPlanes.push(f.r); break;
        case 4: f.original.push(f.r); break;
        // Keep shift as an independent regression: splice must not replace it.
        case 5: f.original.shift(); break;
        case 6:
            f.source.clippingPlanes = [f.r];
            f.original[0].constant = 8;
            break;
        case 7: f.source.clippingPlanes = null; f.original.push(f.r); break;
        case 8: {
            const alias = f.source.clippingPlanes;
            f.source.clippingPlanes = [f.r];
            alias.push(f.r);
            break;
        }
        case 9: cloned = f.source.clone(); break;
        case 10: f.target.copy(f.source); break;
        case 11: f.source.copy(f.source); break;
        case 12: json = f.source.toJSON().clippingPlanes; break;
        case 13: f.source.fromJSON({ clippingPlanes: [f.r.toJSON()] }); break;
        case 14: f.source.clippingPlanes = null; cloned = f.source.clone(); break;
        case 15: f.source.clippingPlanes = []; cloned = f.source.clone(); break;
        case 16: f.source.clippingPlanes[1] = null; f.target.copy(f.source); break;
        case 17: {
            const group = new ClippingGroup();
            group.clippingPlanes = f.source.clippingPlanes;
            f.target.clippingPlanes = group.clippingPlanes;
            group.clippingPlanes.push(f.r);
            break;
        }
        case 18: f.source.clippingPlanes = null; break;
        case 19: f.source.setValues({ clippingPlanes: f.targetOriginal }); break;
        case 20:
            Object.defineProperty(f.source, 'clippingPlanes', {
                value: f.targetOriginal, writable: true, enumerable: true, configurable: true
            });
            break;
        case 21: f.original.pop(); f.original.unshift(f.r); break;
        case 22: f.source.clippingPlanes.length = 0; break;
        case 23: cloned = f.source.clone(); cloned.clippingPlanes[0].constant = 27; break;
        case 24: f.original.splice(0, 1); break;
        }
    } catch (e) { result = { error: e.name }; }
    let clone = null;
    if (cloned) {
        const entries = cloned.clippingPlanes;
        clone = {
            values: planes(entries), sameArray: entries === f.source.clippingPlanes,
            sameFirst: entries !== null && entries.length > 0 && entries[0] === f.original[0]
        };
    }
    return {
        mode, result, snapshot: f.snapshot(), clone, json,
        repeatIdentity: f.source.clippingPlanes === f.source.clippingPlanes
    };
}
const expected = Array.from({ length: 25 }, (_, i) => scenario(i));
if (process.argv[3] === '--oracle-only') {
    console.log(`${expected.length} pinned Material clipping scenarios executed`);
    process.exit(0);
}
const native = spawnSync(process.argv[3], [], { encoding: 'utf8', maxBuffer: 16 * 1024 * 1024 });
assert.equal(native.status, 0, native.stderr);
const actual = native.stdout.trim().split(/\r?\n/).map(JSON.parse);
assert.equal(actual.length, expected.length);
let comparisons = 0;
function compare(a, b, key) {
    comparisons++;
    if (a && b && typeof a === 'object' && typeof b === 'object') {
        assert.equal(Array.isArray(a), Array.isArray(b), key);
        assert.deepEqual(Object.keys(a).sort(), Object.keys(b).sort(), key + ' keys');
        for (const k of Object.keys(b)) compare(a[k], b[k], key + '.' + k);
    } else assert.deepEqual(a, b, key);
}
const failures = [];
for (let i = 0; i < expected.length; ++i) {
    try { compare(actual[i], expected[i], `case[${i}]`); }
    catch (e) { failures.push(e); console.error(e.message); }
}
assert.equal(failures.length, 0, `${failures.length} Material clipping scenarios diverged`);
console.log(`Native Material clipping parity: ${expected.length} scenarios / ${comparisons} comparisons passed against ${lock.commit}`);
