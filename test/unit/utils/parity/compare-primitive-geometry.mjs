// Differential for the primitive geometries: the same parameters through the port probe and through the
// pinned upstream module, compared field for field. Attribute values travel as float32 bit patterns so the
// comparison is exact.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const sourceDir = process.argv[2];
const probePath = process.argv[3];

const {BoxGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/BoxGeometry.js')).href);
const {PlaneGeometry} = await import(pathToFileURL(path.join(sourceDir, 'src/geometries/PlaneGeometry.js')).href);

const bitArray = (values) => Array.from(new Uint32Array(new Float32Array(values).buffer));

const describeAttributes = (geometry) => {
    const out = {};
    for (const [name, itemSize] of [['position', 3], ['normal', 3], ['uv', 2]]) {
        const attribute = geometry.attributes[name];
        if (!attribute) {
            out[name] = null;
            continue;
        }
        const array = attribute.array;
        const limit = Math.min(attribute.count, 6);
        const values = [];
        for (let i = 0; i < limit * itemSize; i += 1) values.push(array[i]);
        out[name] = {count: attribute.count, itemSize, bits: bitArray(values)};
    }
    return out;
};

const describeIndex = (geometry) => {
    if (!geometry.index) return null;
    const array = geometry.index.array;
    return {count: geometry.index.count, first: Array.from(array.slice(0, 8)).map(Number)};
};

const scenarios = [
    ['box 2 3 4 1 1 1', () => new BoxGeometry(2, 3, 4, 1, 1, 1)],
    ['box 1 1 1 2 2 2', () => new BoxGeometry(1, 1, 1, 2, 2, 2)],
    ['box 0.5 7 3 3 1 2', () => new BoxGeometry(0.5, 7, 3, 3, 1, 2)],
    ['plane 2 4 1 1', () => new PlaneGeometry(2, 4, 1, 1)],
    ['plane 2 4 2 3', () => new PlaneGeometry(2, 4, 2, 3)],
    ['plane 1.5 0.25 4 1', () => new PlaneGeometry(1.5, 0.25, 4, 1)],
];

const raw = execFileSync(probePath, [], {input: scenarios.map(([line]) => line).join('\n'), encoding: 'utf8', maxBuffer: 64 * 1024 * 1024});
const lines = raw.trim().split('\n').map((line) => JSON.parse(line));
assert.equal(lines.length, scenarios.length);

for (let i = 0; i < scenarios.length; i += 1) {
    const [line, build] = scenarios[i];
    const geometry = build();
    const expected = {
        kind: line.split(' ')[0],
        ...describeAttributes(geometry),
        index: describeIndex(geometry),
        groups: geometry.groups.length,
    };
    assert.deepEqual(lines[i], expected, `scenario ${line}`);
}

console.log(`primitive geometry parity: ${scenarios.length} scenarios`);