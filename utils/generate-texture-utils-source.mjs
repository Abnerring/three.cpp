import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { spawnSync } from 'node:child_process';
import { createHash } from 'node:crypto';

const args = process.argv.slice(2);
const check = args.includes('--check');
const upstream = path.resolve(args.find(value => value !== '--check') ?? '../three.js');
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const commit = spawnSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' });
if (commit.status !== 0 || commit.stdout.trim() !== '11738a9d55e502fb484cefffcea3a66dc44e9fe9') throw new Error('Unexpected upstream checkout');
const source = path.join(upstream, 'src/extras/TextureUtils.js');
const module = await import(pathToFileURL(source));
const values = {};
for (const name of ['contain', 'cover', 'fill', 'getByteLength']) {
    values[name] = Function.prototype.toString.call(module[name]);
    values['static_' + name] = Function.prototype.toString.call(module.TextureUtils[name]);
}
values.TextureUtils = Function.prototype.toString.call(module.TextureUtils);
const sha = createHash('sha256').update(fs.readFileSync(source)).digest('hex');
const text = '// Generated from fixed three.js TextureUtils.js; source SHA-256: ' + sha + '\n' +
    '// Source text is observable metadata; no JavaScript is executed by the C++ library.\n' +
    'namespace TextureUtilsSource {\n' +
    Object.entries(values).map(([name, value]) => `inline constexpr const char* ${name} = ${JSON.stringify(value)};`).join('\n') + '\n}\n';
const target = path.join(root, 'src/extras/TextureUtilsSource.inc');
if (check) {
    if (fs.readFileSync(target, 'utf8') !== text) throw new Error('TextureUtils source metadata differs');
} else fs.writeFileSync(target, text);
console.log(JSON.stringify({ functions: Object.keys(values).length, upstreamSHA256: sha, check }));
