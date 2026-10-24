import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
const binary = path.resolve(process.argv[2]), outputDirectory = process.argv[3] && path.resolve(process.argv[3]);
const hex = s => Array.from({ length: s.length }, (_, i) => s.charCodeAt(i).toString(16).padStart(4, '0')).join('');
const escaped = s => Array.from({ length: s.length }, (_, i) => '\\u' + s.charCodeAt(i).toString(16).padStart(4, '0')).join('');
const encode = v => v === undefined ? { undefined: true } : typeof v === 'string' ? { string: hex(v) } : Array.isArray(v) ? v.map(encode) : v;
const cases = [];
// Every BMP code unit whose full-uppercase mapping changes, in both directions.
// This includes length expansions and the non-Unicode non-ASCII -> ASCII rule.
for (let i = 0; i <= 0xffff; ++i) {
    const s = String.fromCharCode(i), upper = s.toUpperCase(); if (s === upper) continue;
    cases.push(['E', escaped(s), 'i', upper, '', 0], ['E', escaped(upper), 'i', s, '', 0]);
}
const characters = ['a', 'A', 'z', 'Z', '-', '^', 'é', 'É', 'ß', 'ẞ', 'µ', 'Μ', 'μ', 'ς', 'σ', 'Σ', 'ſ', 's', 'K', 'k', 'ı', 'İ', 'Å', 'Å', '\u0000', '\u00a0', '\ud800', '\udfff', '𐐨', '𐐀'];
const classes = ['[a-z]', '[A-Z]', '[a-ß]', '[^a-ß]', '[µ-ÿ]', '[Α-ω]', '[^Α-ω]', '[^^a]', '[\\u0000-\\uffff]', '[^\\u0000-\\uffff]', '[\\w]', '[^\\w]', '[\\s\\S]', '[^]', '[]', '[\\c1]', '[a-\\s]', '[\\w-0]'];
for (const pattern of classes) for (const s of characters) cases.push(['E', '^' + pattern + '$', 'i', s, '', 0]);
const patterns = ['a+', '(a)\\1', '([a-z])\\1', '(µ)\\1', '(ς)\\1', '(é)(ß)?', '\\bfoo\\b', '\\a\\q', '\\x61\\u0062\\143', '\\uZZZZ', '\\xZZ', '\\c1', '\\cA', '(\\1a)+', '(a\\1)', '\\1(a)', '(?=(a+))a*b\\1', '.[a-z]', '^a$', '[[]', '[a[:digit:]]'];
const strings = ['aA', 'Aa', 'AA', 'abC', 'AAB', 'µΜμ', 'ςΣσ', 'éÉßẞ', 'ſSKK', 'FOO _foo foo1', 'ABCAaA', '\\C1', '\u0001', 'uZZZZ', 'xZZ', '\nA', 'A\n', '[a', '😀A'];
for (const pattern of patterns) for (const s of strings) for (const flags of ['i', 'gi', 'im', 'gim', 'iy', 'isy']) cases.push(['E', pattern, flags, s, '', 0]);
for (const pattern of ['([a-z]+)', '(é|µ|ς)', '[^a-ß]', '(a)?b']) for (const s of ['aA Bb', 'éÉ µΜμ ςΣσ', 'ſSKK', 'bB Ab aB', '😀A']) for (const flags of ['i', 'gi', 'gim']) {
    cases.push(['R', pattern, flags, s, '<$&:$1>', 1], ['F', pattern, flags, s, '', 2], ['P', pattern, flags, s, '', 10]);
}
for (const pattern of ['[', '(', '*', '\\', '[z-a]']) cases.push(['E', pattern, 'i', 'aA', '', 0]);
// Append empty-class quantifiers without changing the original 5,665 requests.
const emptyClassPatterns = ['[]*', '[]?', '[]+', '[]{0}', '[]{1}', '[]|a', '[^]*', '[^]?', '[^]+', '[^]{0}', '[^]{1}', '[^]|a',
    '([])*', '([])?', '([])+', '([]){0}', '([]){1}', '(?:[]|a)*', '[]*?', '[]??', '[]+?', '[]{0,2}?', '[^]*?', '[^]+?',
    '^[]*$', '^[]?$', '^[]{0}$', '^[^]*$', '^[^]?$', '^([^])*?$', 'a[]*b', 'a[]?b', 'a[]+b', 'a[^]*b', 'a[^]?b', 'a[^]+b'];
for (const pattern of emptyClassPatterns) for (const s of ['', 'a', 'b', 'ab', '\n', '😀', '\ud800', '\udfff', 'A\nb']) for (const flags of ['i', 'gi', 'iy']) cases.push(['E', pattern, flags, s, '', 0]);
for (const pattern of emptyClassPatterns.slice(0, 12)) for (const s of ['', 'a', 'ab', '\n', '😀']) for (const flags of ['i', 'gi']) for (const mode of ['R', 'F', 'P']) cases.push([mode, pattern, flags, s, '<$&:$1>', 3]);
const expected = cases.map(([mode, pattern, flags, s, replacement, n]) => {
    try {
        const re = new RegExp(pattern, flags); re.lastIndex = n;
        if (mode === 'E') { const results = []; for (let i = 0; i < 3; ++i) { const value = re.exec(s); results.push({ value: encode(value), index: value?.index ?? null, lastIndex: re.lastIndex }); } return { results, source: encode(re.source), flags: encode(re.flags) }; }
        const callback = (...args) => '{' + args.map(v => JSON.stringify(encode(v)) + ';').join('') + '}';
        const value = mode === 'P' ? s.split(re, n) : s.replace(re, mode === 'F' ? callback : replacement); return { value: encode(value), lastIndex: re.lastIndex };
    } catch (e) { return { error: e.name }; }
});
const unsupported = ['id', 'iu', 'iv'].map(flags => ['E', 'a', flags, 'A', '', 0]);
const input = [...cases, ...unsupported].map(c => [c[0], ...c.slice(1, 5).map(s => hex(s) || '-'), c[5]].join(' ')).join('\n') + '\n';
const processResult = spawnSync(binary, [], { input, encoding: 'utf8', maxBuffer: 256 * 1024 * 1024 });
assert.equal(processResult.status, 0, processResult.stderr);
const actualAll = processResult.stdout.trimEnd().split(/\r?\n/).map(JSON.parse); assert.equal(actualAll.length, cases.length + unsupported.length);
const actual = actualAll.slice(0, cases.length), failures = [];
for (let i = 0; i < cases.length; ++i) try { assert.deepEqual(actual[i], expected[i]); } catch { failures.push({ index: i, request: cases[i], expected: expected[i], actual: actual[i] }); }
for (const result of actualAll.slice(cases.length)) assert.equal(typeof result.unsupported, 'string', 'd/u/v must remain explicitly unsupported');
if (outputDirectory) { fs.mkdirSync(outputDirectory, { recursive: true }); for (const [name, value] of [['input', cases], ['expected', expected], ['actual', actual], ['differences', failures], ['unsupported', { input: unsupported, actual: actualAll.slice(cases.length) }]]) fs.writeFileSync(path.join(outputDirectory, 'regexp-ignore-case-' + name + '.json'), JSON.stringify(value, null, 2)); }
assert.equal(failures.length, 0, `${failures.length} ignoreCase differences; first ${JSON.stringify(failures[0])}`);
console.log(`RegExp non-Unicode ignoreCase parity: ${cases.length} scenarios; d/u/v explicitly unsupported in ${unsupported.length} additional checks`);
