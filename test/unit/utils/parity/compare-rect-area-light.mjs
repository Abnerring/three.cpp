// Differential check for the native examples/jsm/lights port
// (RectAreaLightTexturesLib + RectAreaLightUniformsLib).
//
//   node compare-rect-area-light.mjs <three.js checkout> <probe executable>
//
// Loading choice: every examples/jsm module imports the bare specifier 'three', which the
// checkout's package.json maps to the prebuilt build/three.cjs — a bundle whose tables can
// differ from src/. Both pinned modules are therefore loaded from base64 `data:` URLs with
// 'three' rewritten to <checkout>/src/Three.js (the technique of compare-jsm-shaders.mjs),
// and the relative './RectAreaLightTexturesLib.js' specifier of the uniforms module is
// rewritten to that same data URL. The comparison is thus against the pinned source
// semantics, sharing one UniformsLib instance with the imported src/Three.js.
//
// The description is compared with assert.deepEqual and no tolerances: the class-object
// shapes, every data-texture field, the FULL 16384-element pixel arrays of all four
// textures, and the four writes RectAreaLightUniformsLib.init() performs on UniformsLib.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';

if (process.argv.length !== 4) throw new Error('Usage: node compare-rect-area-light.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version, 'v26.5.0', 'The differential oracle is pinned to the Node release recorded in upstream.json');
const upstream = path.resolve(process.argv[2]);
const probePath = path.resolve(process.argv[3]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const threeUrl = pathToFileURL(path.join(upstream, 'src/Three.js')).href;
const three = await import(threeUrl);
const lightsDir = path.join(upstream, 'examples/jsm/lights');
// Same rewrites the C++ port documents: the bare 'three' specifier and the relative import
// between the two ported modules.
function moduleDataUrl(file, replacements) {
    let source = fs.readFileSync(path.join(lightsDir, file), 'utf8');
    for (const [from, to] of replacements) {
        assert.ok(source.includes(from), `${file}: missing ${from}`);
        source = source.split(from).join(to);
    }
    return 'data:text/javascript;base64,' + Buffer.from(source).toString('base64');
}
const texturesUrl = moduleDataUrl('RectAreaLightTexturesLib.js', [["'three'", JSON.stringify(threeUrl)]]);
const texturesModule = await import(texturesUrl);
const uniformsUrl = moduleDataUrl('RectAreaLightUniformsLib.js',
    [["'three'", JSON.stringify(threeUrl)], ["'./RectAreaLightTexturesLib.js'", JSON.stringify(texturesUrl)]]);
const uniformsModule = await import(uniformsUrl);

const PIXELS = 64 * 64 * 4;
// The static fields of the class in upstream source order and, separately, the order in
// which RectAreaLightUniformsLib.init() writes them into UniformsLib.
const FIELDS = ['LTC_HALF_1', 'LTC_HALF_2', 'LTC_FLOAT_1', 'LTC_FLOAT_2'];
const UNIFORM_FIELDS = ['LTC_FLOAT_1', 'LTC_FLOAT_2', 'LTC_HALF_1', 'LTC_HALF_2'];
const number = value => Number.isNaN(value) ? 'NaN' : value === Infinity ? 'Infinity'
    : value === -Infinity ? '-Infinity' : Object.is(value, -0) ? '-0' : value;
const numbers = array => Array.from(array, number);
const hasOwnFields = (object, fields) => Object.fromEntries(fields.map(field => [field, Object.hasOwn(object, field)]));
const identityFields = (left, right, fields) => Object.fromEntries(fields.map(field => [field, left[field] === right[field]]));
const ordinaryCall = callable => { try { callable(); return 'none'; } catch (error) { return error.name; } };
function textureNode(texture) {
    return {
        isDataTexture: texture.isDataTexture,
        width: number(texture.image.width), height: number(texture.image.height),
        format: number(texture.format), type: number(texture.type),
        mapping: number(texture.mapping), wrapS: number(texture.wrapS), wrapT: number(texture.wrapT),
        magFilter: number(texture.magFilter), minFilter: number(texture.minFilter),
        anisotropy: number(texture.anisotropy),
        flipY: texture.flipY, generateMipmaps: texture.generateMipmaps,
        premultiplyAlpha: texture.premultiplyAlpha, matrixAutoUpdate: texture.matrixAutoUpdate,
        unpackAlignment: number(texture.unpackAlignment),
        colorSpace: texture.colorSpace,
        version: number(texture.version), sourceVersion: number(texture.source.version),
        mipmapCount: number(texture.mipmaps.length),
        dataType: texture.image.data.constructor.name,
        data: numbers(texture.image.data)
    };
}
function sameData(left, right) {
    if (left.constructor !== right.constructor || left.length !== right.length) return false;
    for (let index = 0; index < left.length; ++index) if (!Object.is(left[index], right[index])) return false;
    return true;
}

const expected = {};
const texturesClass = texturesModule.RectAreaLightTexturesLib;
expected.texturesModuleKeys = Object.keys(texturesModule);
expected.texturesModuleExportIsClassObject = texturesModule.RectAreaLightTexturesLib === texturesClass;

expected.classType = typeof texturesClass;
expected.className = texturesClass.name;
expected.classLength = number(texturesClass.length);
expected.classOwnKeys = Object.getOwnPropertyNames(texturesClass);
expected.classEnumerableKeys = Object.keys(texturesClass);
expected.classPrototypeConstructorIsClass = texturesClass.prototype.constructor === texturesClass;
expected.ordinaryClassCall = ordinaryCall(texturesClass);
expected.instanceKeys = Object.keys(new texturesClass());
expected.instancePrototypeIsClassPrototype = Object.getPrototypeOf(new texturesClass()) === texturesClass.prototype;

const init = texturesClass.init;
expected.initType = typeof init;
expected.initName = init.name;
expected.initLength = number(init.length);
expected.initOwnOnClass = Object.hasOwn(texturesClass, 'init');
expected.staticFieldsBeforeInit = Object.fromEntries(FIELDS.map(field =>
    [field, texturesClass[field] === null ? 'null' : 'not-null']));

expected.initReturnsClassObject = texturesClass.init() === texturesClass;
expected.textures = Object.fromEntries(FIELDS.map(field => [field, textureNode(texturesClass[field])]));
for (const field of FIELDS) assert.equal(expected.textures[field].data.length, PIXELS, `${field}: pixel count`);
expected.classEnumerableKeysAfterInit = Object.keys(texturesClass);
expected.classStableAcrossInit = texturesModule.RectAreaLightTexturesLib === texturesClass;

const uniformsClass = uniformsModule.RectAreaLightUniformsLib;
expected.uniformsModuleKeys = Object.keys(uniformsModule);
expected.uniformsModuleExportIsClassObject = uniformsModule.RectAreaLightUniformsLib === uniformsClass;
expected.uniformsClassName = uniformsClass.name;
expected.uniformsClassType = typeof uniformsClass;
expected.uniformsClassOwnKeys = Object.getOwnPropertyNames(uniformsClass);
expected.uniformsClassEnumerableKeys = Object.keys(uniformsClass);
expected.uniformsOrdinaryClassCall = ordinaryCall(uniformsClass);

const uniformsLib = three.UniformsLib;
const keysBefore = Object.keys(uniformsLib);
expected.uniformsHasOwnBeforeInit = hasOwnFields(uniformsLib, UNIFORM_FIELDS);

const uniformsInit = uniformsClass.init;
expected.uniformsInitType = typeof uniformsInit;
expected.uniformsInitName = uniformsInit.name;
expected.uniformsInitLength = number(uniformsInit.length);
expected.uniformsInitReturnsUndefined = uniformsInit.call(uniformsClass) === undefined;

const keysAfter = Object.keys(uniformsLib);
expected.uniformsHasOwnAfterInit = hasOwnFields(uniformsLib, UNIFORM_FIELDS);
expected.uniformsKeysBeforeArePrefix = keysBefore.every((key, index) => keysAfter[index] === key);
expected.uniformsKeysAdded = keysAfter.length - keysBefore.length;
expected.uniformsLastKeys = keysAfter.slice(-UNIFORM_FIELDS.length);
expected.uniformsIdentity = identityFields(uniformsLib, texturesClass, UNIFORM_FIELDS);

const previous = FIELDS.map(field => texturesClass[field]);
const previousData = previous.map(texture => texture.image.data);
uniformsInit.call(uniformsClass);
expected.secondInit = {
    fieldsReplaced: Object.fromEntries(FIELDS.map((field, index) => [field, texturesClass[field] !== previous[index]])),
    uniformsFollowNewTextures: UNIFORM_FIELDS.every(field => uniformsLib[field] === texturesClass[field]),
    rebuiltTablesIdentical: FIELDS.every((field, index) => sameData(previousData[index], texturesClass[field].image.data))
};
expected.nativeInitReturnsClassObject = texturesClass.init() === texturesClass;

const run = execFileSync(probePath, [], {encoding: 'utf8', maxBuffer: 256 * 1024 * 1024});
const actual = JSON.parse(run);
for (const field of FIELDS) {
    assert.equal(actual.textures[field].data.length, PIXELS, `${field}: native pixel count`);
    assert.equal(actual.textures[field].dataType, expected.textures[field].dataType, `${field}: storage type`);
}

let comparisons = 0;
function compare(node, reference, where) {
    comparisons++;
    if (reference !== null && typeof reference === 'object') {
        assert.equal(typeof node, 'object', `${where}: type`);
        assert.ok(node !== null, `${where}: null`);
        assert.deepEqual(Object.keys(node).sort(), Object.keys(reference).sort(), `${where}: keys`);
        for (const key of Object.keys(reference)) compare(node[key], reference[key], `${where}.${key}`);
    } else {
        assert.deepEqual(node, reference, where);
    }
}
compare(actual, expected, 'root');

const pixels = FIELDS.length * PIXELS;
console.log(`RectAreaLight parity: ${Object.keys(expected).length} description fields / ${pixels} pixel values `
    + `(4 textures x ${PIXELS}) / ${comparisons} comparisons against ${lock.commit}; ${process.version}`);
