import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

if (process.argv.length !== 4) throw Error('Usage: node compare-webgl-animation.mjs <three.js> <probe>');
const upstream = path.resolve(process.argv[2]);
const revision = '11738a9d55e502fb484cefffcea3a66dc44e9fe9';
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), revision);
const file = 'src/renderers/webgl/WebGLAnimation.js';
const source = execFileSync('git', ['-C', upstream, 'show', `HEAD:${file}`]);
assert.equal(createHash('sha256').update(source).digest('hex'), 'ad86bf7da307078fc5ea85573ff3db457b0f53a3df4196a05b2b1810a4afb5b7');
assert.equal(fs.readFileSync(path.join(upstream, file), 'utf8').replaceAll('\r\n', '\n'), source.toString());
const { WebGLAnimation } = await import(pathToFileURL(path.join(upstream, file)));
const encode = v => v === undefined ? 'undefined' : v;
function scenario(mode, failAt) {
    const trace = [], results = [], frame = {};
    let events = 0, requests = 0, acted = false, pending;
    const animation = WebGLAnimation();
    function event(...values) {
        trace.push(values);
        if (++events === failAt) throw new RangeError('event');
    }
    function loop(time, receivedFrame) {
        event('loop', this === undefined, encode(time), receivedFrame === frame);
        if (mode === 10) animation.stop();
        if (mode === 11) {
            animation.setContext(second);
            animation.setAnimationLoop(function(time) { event('replacement-loop', this === undefined, encode(time)); });
        }
        if (mode === 14) throw new RangeError('loop');
    }
    function context(id) {
        const result = { id };
        for (const cancel of [false, true]) Object.defineProperty(result, cancel ? 'cancelAnimationFrame' : 'requestAnimationFrame', {
            enumerable: true, configurable: true,
            get() {
                event(cancel ? 'get-cancel' : 'get-request', id);
                if (!cancel && !acted) {
                    acted = true;
                    if (mode === 6) animation.setContext(second);
                    if (mode === 16) throw new RangeError('request-getter');
                }
                if (cancel && mode === 12) animation.start();
                return function(arg) {
                    if (cancel) {
                        event('cancel', id, this.id, encode(arg));
                        if (mode === 13) throw new RangeError('cancel');
                        return;
                    }
                    event('request', id, this.id, arg.name, arg.length);
                    const n = ++requests;
                    pending = arg;
                    if (n === 1) {
                        if (mode === 7) animation.stop();
                        if (mode === 8) animation.setAnimationLoop(null);
                        if (mode === 9) animation.setContext(second);
                        if (mode === 15) throw new RangeError('request');
                    }
                    return n + 100;
                };
            }
        });
        return result;
    }
    const second = context(2), first = context(1);
    function invoke(fn) { try { return fn() === undefined ? 'undefined' : 'value'; } catch (e) { return e.name; } }
    function run(name, ...args) { results.push([name, invoke(() => animation[name](...args))]); }
    run('start'); run('stop');
    if (mode === 1) run('setAnimationLoop', loop);
    run('setContext', mode === 4 ? undefined : mode === 5 ? null : mode === 18 ? {} : first);
    run('start');
    run('setAnimationLoop', mode === 2 ? null : mode === 3 ? undefined : mode === 17 ? 23 : loop);
    run('start'); run('start');
    for (let tick = 0; tick < 2; ++tick) results.push(['tick', pending === undefined ? 'absent' : invoke(() => pending(tick === 0 ? 12 : undefined, frame))]);
    run('stop'); run('start'); run('stop'); run('setContext', null); run('stop');
    run('setAnimationLoop', null); run('start');
    return { mode, failAt, results, trace, events };
}
function flags(object, name) {
    const d = Object.getOwnPropertyDescriptor(object, name);
    return [d.writable ?? false, d.enumerable ?? false, d.configurable ?? false];
}
function metadata() {
    const animation = new WebGLAnimation();
    const methods = ['start', 'stop', 'setAnimationLoop', 'setContext'].map(name => {
        const method = animation[name];
        return { name: method.name, length: method.length, flags: flags(animation, name),
            prototypeFlags: flags(method, 'prototype'), constructor: method.prototype.constructor === method };
    });
    return { kind: 'metadata', name: WebGLAnimation.name, length: WebGLAnimation.length, methods };
}
const expected = [];
for (let mode = 0; mode < 19; ++mode) for (let failAt = 0; failAt <= 32; ++failAt) expected.push(scenario(mode, failAt));
expected.push(metadata());
const actual = execFileSync(process.argv[3], [], { encoding: 'utf8', maxBuffer: 12 * 1024 * 1024 }).trim().split(/\r?\n/).map(JSON.parse);
assert.equal(actual.length, expected.length);
for (let i = 0; i < expected.length; ++i) {
    try { assert.deepEqual(actual[i], expected[i]); }
    catch (error) {
        const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'threecpp-animation-frame-'));
        fs.writeFileSync(path.join(directory, 'difference.json'), JSON.stringify({ i, actual: actual[i], expected: expected[i] }, null, 2));
        console.error(`Diagnostics: ${directory}`); throw error;
    }
}
console.log(`WebGLAnimation: ${expected.length} actual pinned-source scenarios passed; scheduling, callbacks, throws, reentry and metadata.`);
