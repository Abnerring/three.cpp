import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { execFileSync, spawnSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import { isDeepStrictEqual } from 'node:util';
import { createMaterialXOracle } from './materialx-native-oracle.mjs';
import { parseMaterialXCallbacks, auditMaterialXCallbackExecution } from './materialx-callback-execution-audit.mjs';

function firstDifference(expected, actual, at = '$') {
    if (isDeepStrictEqual(expected, actual)) return null;
    if (expected === null || actual === null || typeof expected !== 'object' || typeof actual !== 'object')
        return { path: at, expected, actual, expectedType: typeof expected, actualType: typeof actual };
    if (Array.isArray(expected) !== Array.isArray(actual)) return { path: at, kind: 'array-kind', expected, actual };
    if (Array.isArray(expected) && expected.length !== actual.length)
        return { path: at + '.length', expected: expected.length, actual: actual.length };
    for (const key of new Set([...Object.keys(expected), ...Object.keys(actual)])) {
        if (Object.hasOwn(expected, key) !== Object.hasOwn(actual, key))
            return { path: at + '[' + JSON.stringify(key) + ']', kind: 'own-property', expected: Object.hasOwn(expected, key), actual: Object.hasOwn(actual, key) };
        const difference = firstDifference(expected[key], actual[key], at + '[' + JSON.stringify(key) + ']');
        if (difference) return difference;
    }
    return { path: at, kind: 'object-type', expected, actual };
}

export async function compareMaterialX(module) {
    const [upstreamArg, probeArg, outputArg] = process.argv.slice(2);
    assert(upstreamArg && probeArg, 'Usage: comparator upstream probe|--oracle-only [fresh-output-directory]');
    const upstream = path.resolve(upstreamArg), oracleOnly = probeArg === '--oracle-only';
    const probe = oracleOnly ? null : path.resolve(probeArg);
    const output = path.resolve(outputArg ?? path.join('D:/Temp/surui', module.toLowerCase() + '-parity-' + Date.now() + '-' + process.pid));
    assert.match(output, /^D:[\\/]/i);
    assert(!fs.existsSync(output), 'Fresh evidence directory required: ' + output);
    const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
    const commit = execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim();
    assert.equal(commit, lock.commit);
    assert.equal(execFileSync('git', ['-C', upstream, 'status', '--porcelain', '--', 'src'], { encoding: 'utf8' }).trim(), '');
    const fixtureURL = new URL('./materialx-native-fixtures.json', import.meta.url);
    const fixtures = JSON.parse(fs.readFileSync(fixtureURL));
    const cases = fixtures.cases.filter(request => request.module === module);
    assert(cases.length > 0);
    const sources = [fixtureURL, new URL('./materialx-native-oracle.mjs', import.meta.url),
        new URL('./materialx-native-compare.mjs', import.meta.url),
        new URL('./materialx-callback-execution-audit.mjs', import.meta.url),
        path.join(upstream, 'src/nodes/materialx/MaterialXNoise.js'),
        path.join(upstream, 'src/nodes/materialx/MaterialXNodes.js'),
        path.join(upstream, 'src/nodes/materialx/MaterialXColor.js'),
        path.join(upstream, 'src/nodes/materialx/MaterialXColorTransform.js')];
    const hash = file => createHash('sha256').update(fs.readFileSync(file)).digest('hex');
    const before = sources.map(file => ({ file: String(file), sha256: hash(file) }));
    const binaryBefore = probe && hash(probe);
    fs.mkdirSync(output, { recursive: true });
    const write = (name, value) => fs.writeFileSync(path.join(output, name), JSON.stringify(value, null, 2) + '\n');
    const expectedRows = [], actualRows = [], processes = [], failures = [];
    const summary = { status: 'running', sourceCommit: commit, module, caseCount: cases.length,
        exports: module === 'MaterialXNoise' ? 74 : 49,
        declarations: fixtures.declarations[module].length,
        oracleCompletedCases: 0, nativeCompletedCases: 0, oracleSuccessPaths: 0, nativeSuccessPaths: 0,
        oracleObservedThrows: 0, nativeObservedThrows: 0, nativeExecuted: false, nativeCompiledByRunner: false,
        graphExclusions: ['id', 'uuid', 'stackTrace', '_cacheKey', '_cacheKeyVersion'],
        moduleShadersRepresentedByBindingIdentity: true, realCallbackBodies: true,
        realGPUExecuted: false, failures };
    write('input.json', cases);
    const oracle = await createMaterialXOracle(upstream, fixtures);
    try {
        for (const [index, request] of cases.entries()) {
            const expected = oracle.run(request);
            expectedRows.push(expected);
            if (!expected.error && !Object.hasOwn(expected, 'thrown')) {
                ++summary.oracleCompletedCases;
                if (request.mode === 'globalOrder' && Array.isArray(expected.result) && expected.result[0] === 'throw') ++summary.oracleObservedThrows;
                else ++summary.oracleSuccessPaths;
            } else failures.push({ index, label: request.label, kind: 'oracle-exception', expected });
            if (oracleOnly) continue;
            const started = performance.now();
            const child = spawnSync(probe, [JSON.stringify(request)], { encoding: 'utf8', timeout: 180000,
                maxBuffer: 64 * 1024 * 1024, env: { ...process.env, TEMP: 'D:/Temp/surui', TMP: 'D:/Temp/surui' } });
            const executed = Number.isInteger(child.pid) && child.pid > 0;
            processes.push({ index, label: request.label, pid: child.pid ?? null, executed,
                status: child.status, signal: child.signal, elapsedMS: performance.now() - started,
                error: child.error ? String(child.error) : null });
            summary.nativeExecuted ||= executed;
            if (!executed || child.error || child.status !== 0) {
                failures.push({ index, label: request.label, kind: 'native-process', status: child.status,
                    signal: child.signal, error: child.error ? String(child.error) : null, stderr: child.stderr });
                actualRows.push(null);
                fs.writeFileSync(path.join(output, 'case-' + index + '.stderr.log'), child.stderr ?? '');
                continue;
            }
            let actual;
            try { actual = JSON.parse(child.stdout); } catch (error) {
                failures.push({ index, label: request.label, kind: 'native-json', message: error.message });
                fs.writeFileSync(path.join(output, 'case-' + index + '.stdout.txt'), child.stdout ?? '');
                actualRows.push(null);
                continue;
            }
            actualRows.push(actual);
            if (!actual.error && !Object.hasOwn(actual, 'thrown')) {
                ++summary.nativeCompletedCases;
                if (request.mode === 'globalOrder' && Array.isArray(actual.result) && actual.result[0] === 'throw') ++summary.nativeObservedThrows;
                else ++summary.nativeSuccessPaths;
            }
            if (!isDeepStrictEqual(expected, actual)) {
                failures.push({ index, label: request.label, kind: 'strict-difference', first: firstDifference(expected, actual) });
                write('case-' + index + '.expected.json', expected);
                write('case-' + index + '.actual.json', actual);
            }
            if ((index + 1) % 20 === 0) process.stderr.write(module + ': ' + (index + 1) + '/' + cases.length + '\n');
        }
        // Parse the pinned source rather than trusting the port's source table.
        // Repeated arrow text occurs at distinct MaterialXNoise source locations.
        function coverage(callbacks, rows) {
            const strings = new Set();
            function collect(value) {
                if (typeof value === 'string') strings.add(value);
                else if (value && typeof value === 'object') Object.values(value).forEach(collect);
            }
            collect(rows);
            const missing = callbacks.map((source, index) => ({ index, source })).filter(row => !strings.has(row.source));
            return { sourceLocations: callbacks.length, uniqueSources: new Set(callbacks).size,
                sourceLocationsCovered: callbacks.length - missing.length, missing };
        }
        summary.callbackCoverage = {};
        summary.callbackExecutionCoverage = {};
        for (const name of module === 'MaterialXNodes' ? [module, 'MaterialXColor', 'MaterialXColorTransform'] : [module]) {
            const manifest = parseMaterialXCallbacks(upstream, name, path.join(output, 'node-cache'));
            const callbacks = manifest.callbacks;
            const row = { source: coverage(callbacks, expectedRows), native: oracleOnly ? null : coverage(callbacks, actualRows) };
            summary.callbackCoverage[name] = row;
            if (row.source.missing.length || (!oracleOnly && row.native.missing.length))
                failures.push({ kind: 'missing-real-callback-source', module: name, coverage: row });
            const execution = { source: auditMaterialXCallbackExecution(manifest, cases, expectedRows),
                native: oracleOnly ? null : auditMaterialXCallbackExecution(manifest, cases, actualRows) };
            summary.callbackExecutionCoverage[name] = execution;
            if (!execution.source.exact || (!oracleOnly && !execution.native.exact))
                failures.push({ kind: 'missing-real-callback-execution', module: name, coverage: execution });
        }
        write('callback-coverage.json', summary.callbackCoverage);
        write('callback-execution-coverage.json', summary.callbackExecutionCoverage);
        summary.status = failures.length ? 'failed' : oracleOnly ? 'oracle-only; native pending' : 'native-passed';
    } finally {
        write('expected.json', expectedRows);
        if (!oracleOnly) write('actual.json', actualRows);
        write('processes.json', processes);
        const after = sources.map(file => ({ file: String(file), sha256: hash(file) }));
        const binaryAfter = probe && hash(probe);
        write('guards.json', { before, after, binaryBefore, binaryAfter });
        if (!isDeepStrictEqual(before, after) || binaryBefore !== binaryAfter) {
            failures.push({ kind: 'files-changed-during-validation' });
            summary.status = 'failed';
        }
        write('summary.json', summary);
    }
    assert.equal(summary.oracleCompletedCases, cases.length, 'Every requested source path must actually execute; evidence: ' + output);
    assert.equal(failures.length, 0, JSON.stringify(failures[0]) + '; evidence: ' + output);
    if (!oracleOnly) assert.equal(summary.nativeCompletedCases, cases.length);
    process.stdout.write(JSON.stringify({ module, cases: cases.length, status: summary.status, output }) + '\n');
}
