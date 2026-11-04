import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const {textureBicubicLevel, textureBicubic} = await import(pathToFileURL(path.join(upstream, 'src/nodes/accessors/TextureBicubic.js')));
const {Texture} = await import(pathToFileURL(path.join(upstream, 'src/textures/Texture.js')));
const {vec2, float} = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLCore.js')));
const textureNode = (await import(pathToFileURL(path.join(upstream, 'src/nodes/accessors/TextureNode.js')))).texture(new Texture(), vec2(.25, .5));
const lod = float(.5);
const describe = (fn, result) => ({name: fn.name, isFn: fn.isFn, source: fn.shaderNode.jsFunc.toString(), type: result.type, nodeType: result.node.type});
const expected = {
    textureBicubicLevel: describe(textureBicubicLevel, textureBicubicLevel(textureNode, lod)),
    textureBicubic: describe(textureBicubic, textureBicubic(textureNode, lod))
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('TextureBicubic native/module parity passed');
