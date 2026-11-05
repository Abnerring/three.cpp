import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { createHash } from 'node:crypto';

const sha256 = source => createHash('sha256').update(source).digest('hex');

// Parse the pinned source, independently of the native metadata table. Retain
// every AST occurrence: several Noise callbacks have identical source text.
export function parseMaterialXCallbacks(upstream, name, cacheDirectory) {
    const sourceFile = path.join(upstream, 'src/nodes/materialx', name + '.js');
    const source = fs.readFileSync(sourceFile, 'utf8');
    const parser = spawnSync(process.execPath, ['--expose-internals', '-e', `
        const fs = require('node:fs'), acorn = require('internal/deps/acorn/acorn/dist/acorn');
        const source = fs.readFileSync(0, 'utf8');
        const ast = acorn.parse(source, { sourceType: 'module', ecmaVersion: 'latest', locations: true });
        function arrows(node, out = []) {
            if (!node || typeof node !== 'object') return out;
            if (node.type === 'ArrowFunctionExpression') out.push(node);
            for (const [key, value] of Object.entries(node)) {
                if (['start', 'end', 'loc', 'type'].includes(key)) continue;
                if (Array.isArray(value)) value.forEach(child => arrows(child, out));
                else arrows(value, out);
            }
            return out;
        }
        const describe = node => ({ line: node.loc.start.line, source: source.slice(node.start, node.end) });
        const definitions = [];
        for (const statement of ast.body) {
            const declaration = statement.type === 'ExportNamedDeclaration' ? statement.declaration : statement;
            if (declaration?.type !== 'VariableDeclaration') continue;
            for (const value of declaration.declarations) {
                let body, kind;
                if (value.init?.type === 'ArrowFunctionExpression') { body = value.init; kind = 'function'; }
                else if (value.init?.type === 'CallExpression' && value.init.callee.type === 'Identifier' && value.init.callee.name === 'Fn') {
                    body = value.init.arguments[0]; kind = 'Fn';
                }
                if (body?.type !== 'ArrowFunctionExpression') continue;
                definitions.push({ name: value.id.name, kind, ...describe(body), nested: arrows(body).slice(1).map(describe) });
            }
        }
        process.stdout.write(JSON.stringify({ callbacks: arrows(ast).map(node => source.slice(node.start, node.end)), definitions }));
    `], { input: source, encoding: 'utf8', timeout: 30000, maxBuffer: 8 * 1024 * 1024,
        env: { ...process.env, TEMP: 'D:/Temp/surui', TMP: 'D:/Temp/surui', NODE_COMPILE_CACHE: cacheDirectory } });
    assert.equal(parser.error, undefined);
    assert.equal(parser.status, 0, parser.stderr);
    return { module: name, sourceFile, sourceSHA256: sha256(source), ...JSON.parse(parser.stdout) };
}

function graphRecord(graph, reference) {
    assert(Array.isArray(reference) && reference.length === 2 && reference[0] === 'ref', 'Expected graph reference');
    assert(Number.isSafeInteger(reference[1]) && reference[1] >= 0, 'Invalid graph reference index');
    const record = graph.objects?.[reference[1]];
    assert(record && Array.isArray(record.properties), 'Missing graph record');
    return record;
}

function property(graph, reference, name) {
    const matches = graphRecord(graph, reference).properties.filter(row => row[0] === name);
    assert.equal(matches.length, 1, 'Expected one graph property: ' + name);
    return matches[0][1];
}

function executedControls(graph) {
    // Only runtime control records count. Searching all strings would also count
    // callback metadata, which does not establish that the body was called.
    property(graph, graph.root, 'result');
    property(graph, graph.root, 'inputs');
    property(graph, graph.root, 'stack');
    const reference = property(graph, graph.root, 'controls'), controls = graphRecord(graph, reference);
    assert.equal(controls.kind, 'array');
    assert(Number.isSafeInteger(controls.length) && controls.length >= 0, 'Invalid controls length');
    const records = [];
    for (let index = 0; index < controls.length; ++index) {
        const record = property(graph, reference, String(index));
        const kind = property(graph, record, 'kind'), source = property(graph, record, 'source');
        assert(kind === 'Loop' || kind === 'Shader', 'Unknown executed control kind');
        assert.equal(typeof source, 'string');
        property(graph, record, 'inputs');
        property(graph, record, 'result');
        property(graph, record, 'stack');
        records.push({ kind, source });
    }
    return records;
}

function counts(values) {
    const result = new Map();
    for (const value of values) result.set(value, (result.get(value) ?? 0) + 1);
    return result;
}

function multisetDifferences(expected, actual) {
    const a = counts(expected), b = counts(actual), differences = [];
    for (const source of new Set([...a.keys(), ...b.keys()])) {
        const expectedCount = a.get(source) ?? 0, actualCount = b.get(source) ?? 0;
        if (expectedCount !== actualCount) differences.push({ source, sourceSHA256: sha256(source), expectedCount, actualCount });
    }
    return differences;
}

export function auditMaterialXCallbackExecution(manifest, requests, results) {
    const report = { module: manifest.module, sourceFile: manifest.sourceFile, sourceSHA256: manifest.sourceSHA256,
        metadataExcluded: true, representativeMode: 'body', sourceArrowOccurrences: manifest.callbacks.length,
        topLevelFnBodiesExecuted: 0, topLevelPlainBodiesExecuted: 0,
        nestedCallbackOccurrencesExpected: manifest.definitions.reduce((sum, definition) => sum + definition.nested.length, 0),
        nestedCallbackOccurrencesExecuted: 0, sourceCallbackOccurrencesExecuted: 0,
        controlsKinds: { Loop: 0, Shader: 0 }, topLevelBodyNamesExpected: manifest.definitions.map(row => row.name),
        topLevelBodyNamesExecuted: [], representativeCases: [], unmapped: [] };
    const reexport = manifest.module === 'MaterialXColor' || manifest.module === 'MaterialXColorTransform';
    const requestModule = reexport ? 'MaterialXNodes' : manifest.module;
    for (const definition of manifest.definitions) {
        const label = (reexport ? 'reexport:' : 'body:') + definition.name;
        const indices = requests.flatMap((request, index) => request.module === requestModule && request.mode === 'body'
            && request.fn === definition.name && request.label === label ? [index] : []);
        const row = { name: definition.name, line: definition.line, kind: definition.kind, label,
            caseIndex: indices.length === 1 ? indices[0] : null, requestKind: null,
            requestKindMatches: false, bodyCompleted: false, topLevelSourceSHA256: sha256(definition.source),
            nestedExpected: definition.nested.length, nestedExecuted: 0, controlKinds: [], nested: [], differences: [] };
        report.representativeCases.push(row);
        if (indices.length !== 1) {
            report.unmapped.push({ name: definition.name, label, reason: 'Expected one representative body request', matches: indices.length });
            continue;
        }
        const request = requests[row.caseIndex], result = results[row.caseIndex];
        row.requestKind = request.kind;
        row.requestKindMatches = request.kind === definition.kind;
        if (!row.requestKindMatches || !result || result.error || Object.hasOwn(result, 'thrown')) {
            report.unmapped.push({ name: definition.name, reason: 'Body failed or request used the wrong dispatch' });
            continue;
        }
        let controls;
        try { controls = executedControls(result); } catch (error) {
            report.unmapped.push({ name: definition.name, reason: 'Invalid runtime execution graph', message: error.message });
            continue;
        }
        row.bodyCompleted = true;
        row.nestedExecuted = controls.length;
        row.controlKinds = controls.map(record => record.kind);
        row.differences = multisetDifferences(definition.nested.map(record => record.source), controls.map(record => record.source));
        const actualCounts = counts(controls.map(record => record.source)), occurrences = new Map();
        for (const callback of definition.nested) {
            const occurrence = (occurrences.get(callback.source) ?? 0) + 1;
            occurrences.set(callback.source, occurrence);
            row.nested.push({ line: callback.line, sourceSHA256: sha256(callback.source), occurrence,
                executed: (actualCounts.get(callback.source) ?? 0) >= occurrence });
        }
        if (row.differences.length) report.unmapped.push({ name: definition.name, reason: 'Nested execution source multiset differs', differences: row.differences });
        if (definition.kind === 'Fn') ++report.topLevelFnBodiesExecuted;
        else ++report.topLevelPlainBodiesExecuted;
        report.topLevelBodyNamesExecuted.push(definition.name);
        report.nestedCallbackOccurrencesExecuted += controls.length;
        for (const control of controls) ++report.controlsKinds[control.kind];
    }
    report.topLevelBodyNameCoverageExact = JSON.stringify(report.topLevelBodyNamesExpected) === JSON.stringify(report.topLevelBodyNamesExecuted);
    report.sourceCallbackOccurrencesExecuted = report.topLevelFnBodiesExecuted + report.topLevelPlainBodiesExecuted + report.nestedCallbackOccurrencesExecuted;
    report.exact = report.unmapped.length === 0 && report.topLevelBodyNameCoverageExact
        && report.sourceCallbackOccurrencesExecuted === report.sourceArrowOccurrences
        && report.representativeCases.every(row => row.bodyCompleted && row.requestKindMatches && row.nestedExpected === row.nestedExecuted && row.differences.length === 0);
    return report;
}
