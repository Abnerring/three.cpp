import fs from 'node:fs';
import path from 'node:path';
import { createHash } from 'node:crypto';
import { createRequire } from 'node:module';
import { execFileSync } from 'node:child_process';
import { API_LIMITATIONS, extractApi } from './api-inventory.mjs';

const slash = value => value.split(path.sep).join('/');
const sha256 = value => createHash('sha256').update(value).digest('hex');
export const AUDIT_SCOPES = ['src', 'examples/jsm'];

export function walk(directory) {
    return fs.readdirSync(directory, { withFileTypes: true }).sort((a, b) => a.name < b.name ? -1 : a.name > b.name ? 1 : 0).flatMap(entry => {
        const full = path.join(directory, entry.name);
        if (entry.isSymbolicLink()) throw new Error(`Audit does not follow symlinks: ${full}`);
        return entry.isDirectory() ? walk(full) : [full];
    });
}

function resolveDependency(source, owner) {
    if (source.startsWith('.')) return path.posix.normalize(path.posix.join(path.posix.dirname(owner), source));
    if (source === 'three') return 'src/Three.js';
    if (source === 'three/webgpu') return 'src/Three.WebGPU.js';
    if (source === 'three/tsl') return 'src/Three.TSL.js';
    if (source === 'three/addons') return 'examples/jsm/Addons.js';
    if (source.startsWith('three/addons/')) return `examples/jsm/${source.slice('three/addons/'.length)}`;
    return null;
}

function counts(items, key) {
    const result = {};
    for (const item of items) result[item[key]] = (result[item[key]] ?? 0) + 1;
    return result;
}

function safeExists(root, file) {
    if (!file) return false;
    const resolved = path.resolve(root, file);
    const relative = path.relative(root, resolved);
    if (relative.startsWith('..') || path.isAbsolute(relative)) throw new Error(`Evidence path is outside the checkout: ${file}`);
    return fs.existsSync(resolved) && fs.statSync(resolved).isFile();
}

export async function auditCheckout({ root, upstream }) {
    root = path.resolve(root);
    upstream = path.resolve(upstream);
    const lock = JSON.parse(fs.readFileSync(path.join(root, 'upstream.json'), 'utf8'));
    const revision = execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim();
    if (revision !== lock.commit) throw new Error(`Expected upstream ${lock.commit}, found ${revision}`);
    const dirty = execFileSync('git', ['-C', upstream, 'status', '--porcelain', '--untracked-files=all', '--', ...AUDIT_SCOPES, 'package.json'], { encoding: 'utf8' }).trim();
    if (dirty) throw new Error(`Pinned upstream has local source changes; cannot certify its inventory:\n${dirty}`);
    const upstreamPackage = JSON.parse(fs.readFileSync(path.join(upstream, 'package.json'), 'utf8'));
    if (upstreamPackage.version !== lock.version) throw new Error(`Expected package version ${lock.version}, found ${upstreamPackage.version}`);
    const require = createRequire(path.join(upstream, 'package.json'));
    let acorn;
    let parserPackage;
    try {
        acorn = require('acorn');
        parserPackage = require('acorn/package.json');
    } catch (error) {
        throw new Error('A local Acorn parser is required from the upstream checkout; no regex API fallback is used.', { cause: error });
    }
    const constantsAst = acorn.parse(fs.readFileSync(path.join(upstream, 'src/constants.js'), 'utf8'), { ecmaVersion: 'latest', sourceType: 'module' });
    const revisionDeclaration = constantsAst.body.find(statement => statement.type === 'ExportNamedDeclaration' && statement.declaration?.type === 'VariableDeclaration' && statement.declaration.declarations.some(declaration => declaration.id.type === 'Identifier' && declaration.id.name === 'REVISION'));
    const revisionValue = revisionDeclaration?.declaration.declarations.find(declaration => declaration.id.name === 'REVISION')?.init;
    if (revisionValue?.type !== 'Literal' || revisionValue.value !== lock.revision) throw new Error(`Expected source REVISION ${lock.revision}, found ${revisionValue?.value ?? 'unrecognized expression'}`);
    const allFiles = AUDIT_SCOPES.flatMap(scope => walk(path.join(upstream, scope)).map(file => ({ file, source: slash(path.relative(upstream, file)), scope })));
    const jsFiles = allFiles.filter(item => item.source.endsWith('.js'));
    const knownModules = new Set(jsFiles.map(item => item.source));
    const apiModules = [];
    const modules = [];
    const upstreamUnitRoots = ['test/unit/src', 'test/unit/addons'];
    for (const { file, source, scope } of jsFiles) {
        const content = fs.readFileSync(file, 'utf8');
        const inventory = extractApi(content, acorn.parse);
        for (const imported of inventory.imports) {
            imported.resolvedSource = resolveDependency(imported.source, source);
            imported.resolution = imported.resolvedSource === null ? 'external-or-unresolved' : knownModules.has(imported.resolvedSource) ? 'in-scope-module' : 'outside-module-inventory';
        }
        for (const asset of inventory.assets) {
            asset.resolvedSource = typeof asset.source === 'string' ? resolveDependency(asset.source, source) : null;
            asset.exists = asset.resolvedSource ? safeExists(upstream, asset.resolvedSource) : false;
        }
        const kind = source.startsWith('examples/jsm/libs/') ? 'third-party-dependency' : scope === 'src' ? 'core' : 'addon';
        const headerCandidate = source.replace(/\.js$/, '.h');
        const implementationCandidate = source.replace(/\.js$/, '.cpp');
        const header = safeExists(root, headerCandidate) ? headerCandidate : null;
        const implementation = safeExists(root, implementationCandidate) ? implementationCandidate : null;
        const relative = scope === 'src' ? source.slice(4) : source.slice('examples/jsm/'.length);
        const upstreamUnit = `${upstreamUnitRoots[scope === 'src' ? 0 : 1]}/${relative.replace(/\.js$/, '.tests.js')}`;
        const cppUnit = upstreamUnit.replace(/\.js$/, '.cpp');
        const index = apiModules.length;
        apiModules.push({ source, kind, upstreamSha256: sha256(content), ...inventory });
        modules.push({
            source, scope, kind, upstreamSha256: sha256(content), header, implementation,
            expectedHeader: kind === 'third-party-dependency' ? null : headerCandidate,
            expectedImplementation: kind === 'third-party-dependency' ? null : implementationCandidate,
            status: kind === 'third-party-dependency' ? 'dependency-unverified' : source.startsWith('src/renderers/') ? 'vulkan-port-pending' : header || implementation ? 'partial-unverified' : 'missing',
            verifiedComplete: false,
            apiExtraction: inventory.extraction.status,
            verification: {
                apiInventory: { path: 'docs/api-inventory.json', pointer: `/modules/${index}` },
                upstreamUnitTests: safeExists(upstream, upstreamUnit) ? [upstreamUnit] : [],
                cppUnitTests: safeExists(root, cppUnit) ? [cppUnit] : [],
                reviewedImplementation: [], differentialResults: [], renderingResults: [], dependencyResults: [],
                latestPassingRun: null,
                note: 'Test file presence is a candidate verification location, not passing evidence or complete coverage.'
            }
        });
    }
    const directories = [...new Set(allFiles.map(item => path.posix.dirname(item.source)))].sort().map(directory => ({
        path: directory,
        upstreamFiles: allFiles.filter(item => path.posix.dirname(item.source) === directory).length,
        moduleCount: modules.filter(module => path.posix.dirname(module.source) === directory).length,
        modulesWithCppFiles: modules.filter(module => path.posix.dirname(module.source) === directory && (module.header || module.implementation)).length,
        verifiedComplete: 0
    }));
    const assets = allFiles.filter(item => !item.source.endsWith('.js')).map(item => ({
        source: item.source,
        upstreamSha256: sha256(fs.readFileSync(item.file)),
        kind: item.source.endsWith('.wasm') ? 'runtime-dependency' : 'license-or-document',
        status: 'dependency-unverified', verifiedComplete: false,
        verification: { dependencyResults: [], latestPassingRun: null }
    }));
    const upstreamInfo = { ...lock, auditedCommit: revision, auditedRevision: revisionValue.value, auditedPackageVersion: upstreamPackage.version, sourceTreeClean: true };
    const scopeCounts = Object.fromEntries(AUDIT_SCOPES.map(scope => [scope, { modules: modules.filter(module => module.scope === scope).length, counts: counts(modules.filter(module => module.scope === scope), 'status') }]));
    const api = {
        schemaVersion: 1, upstream: upstreamInfo,
        parser: { name: 'acorn', version: parserPackage.version, ecmaVersion: 'latest', sourceType: 'module-with-script-fallback' },
        limitations: API_LIMITATIONS,
        counts: { modules: apiModules.length, parseErrors: apiModules.filter(module => module.extraction.status === 'parse-error').length, extraction: counts(apiModules.map(module => ({ status: module.extraction.status })), 'status'), exports: apiModules.reduce((total, module) => total + module.exports.length, 0), classes: apiModules.reduce((total, module) => total + module.classes.length, 0), classMethods: apiModules.reduce((total, module) => total + module.classes.reduce((sum, klass) => sum + klass.methods.length, 0), 0), classProperties: apiModules.reduce((total, module) => total + module.classes.reduce((sum, klass) => sum + klass.properties.length, 0), 0) },
        modules: apiModules
    };
    const report = {
        schemaVersion: 2, upstream: upstreamInfo,
        scope: { included: AUDIT_SCOPES, excludedFromThisPhase: ['editor', 'manual', 'docs', 'devtools', 'tsl application', 'examples outside examples/jsm'], note: 'Accepted port scope is src and examples/jsm, including tracked third-party dependencies. Editor/manual web applications are excluded from native application rewrites. Native host behavior, packaging and all three platforms require separate implementation and verification evidence.' },
        counts: counts(modules, 'status'), scopeCounts,
        directoryInventory: directories, assets, modules,
        evidence: { apiInventory: 'docs/api-inventory.json', historicalValidation: safeExists(root, 'docs/VALIDATION.md') ? 'docs/VALIDATION.md' : null, note: 'The historical validation summary is not automatically assigned as a passing result to individual modules. Empty result lists mean no recorded completion evidence.' },
        note: 'Headers, implementations, static API candidates and test files establish presence only. No module is marked complete. Dynamic/generated interfaces, inherited members, behavior and GPU output require separate review and execution.'
    };
    return { api, report };
}

export function writeAudit(root, audit, { checkOnly = false } = {}) {
    for (const [file, content] of [['docs/port-status.json', audit.report], ['docs/api-inventory.json', audit.api]]) {
        const target = path.join(root, file);
        const serialized = JSON.stringify(content, null, 2) + '\n';
        if (checkOnly) {
            if (!fs.existsSync(target) || fs.readFileSync(target, 'utf8').replaceAll('\r\n', '\n') !== serialized) throw new Error(`Generated inventory is stale: ${file}. Run utils/port-audit.mjs without --check.`);
        } else fs.writeFileSync(target, serialized);
    }
}
