import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const upstream = path.resolve(process.argv[2] ?? '../three.js');
const constants = await import(pathToFileURL(path.join(upstream, 'src/constants.js')));
const literal = value => typeof value === 'string' ? JSON.stringify(value) : String(value);
const lines = ['#pragma once', '// Generated from the pinned three.js src/constants.js. Do not renumber constants.'];
for (const [name, value] of Object.entries(constants)) {
    if (typeof value === 'object') {
        lines.push(`namespace ${name} {`);
        for (const [key, member] of Object.entries(value)) {
            if (!['string', 'number', 'boolean'].includes(typeof member)) throw new Error(`${name}.${key}: unsupported constant`);
            lines.push(`inline constexpr auto ${key} = ${literal(member)};`);
        }
        lines.push('}');
    } else {
        lines.push(`inline constexpr auto ${name} = ${literal(value)};`);
    }
}
fs.writeFileSync(new URL('../src/constants.h', import.meta.url), lines.join('\n') + '\n');
