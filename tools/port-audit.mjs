import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const root = fileURLToPath(new URL('../', import.meta.url));
const upstream = path.resolve(process.argv[2] ?? '../three.js');
const lock = JSON.parse(fs.readFileSync(path.join(root, 'upstream.json')));
const revision = execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim();
if (revision !== lock.commit) throw new Error(`Expected upstream ${lock.commit}, found ${revision}`);
function walk(directory) {
    return fs.readdirSync(directory, { withFileTypes: true }).flatMap(entry => {
        const full = path.join(directory, entry.name);
        return entry.isDirectory() ? walk(full) : [full];
    });
}
const sourceRoot = path.join(upstream, 'src');
const modules = walk(sourceRoot).filter(f => f.endsWith('.js')).map(file => {
    const source = path.relative(sourceRoot, file).replaceAll('\\', '/');
    const header = source.replace(/\.js$/, '.h');
    const implementation = source.replace(/\.js$/, '.cpp');
    const exists = fs.existsSync(path.join(root, 'include', header));
    return {
        source: 'src/' + source,
        header: exists ? 'include/' + header : null,
        implementation: fs.existsSync(path.join(root, 'src', implementation)) ? 'src/' + implementation : null,
        status: source.startsWith('renderers/') ? 'vulkan-port-pending' : exists ? 'partial-unverified' : 'missing',
        verifiedComplete: false
    };
});
const counts = {};
for (const module of modules) counts[module.status] = (counts[module.status] ?? 0) + 1;
const report = { upstream: lock, counts, modules, note: 'A matching header only establishes port presence. It does not prove API, behavioral, numerical, serialization, or rendering parity. No module is marked complete by this inventory.' };
fs.mkdirSync(path.join(root, 'docs'), { recursive: true });
fs.writeFileSync(path.join(root, 'docs', 'port-status.json'), JSON.stringify(report, null, 2) + '\n');
console.log(`${modules.length} upstream modules: ${JSON.stringify(counts)}`);
