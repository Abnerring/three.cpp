import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream = path.resolve(process.argv[2]);
const binary = path.resolve(process.argv[3]);
const lock = (await import('../../../../upstream.json', {with: {type: 'json'}})).default;
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);

const {default: UserDataNode, userData} = await import(pathToFileURL(path.join(upstream, 'src/nodes/accessors/UserDataNode.js')));
const explicitData = {rotation: 1.5};
const fallbackData = {rotation: 2.5};
const state = {object: {userData: fallbackData}};
const explicitNode = new UserDataNode('rotation', 'float', explicitData);
const fallbackNode = new UserDataNode('rotation', 'float');
const explicitReference = explicitNode.updateReference(state);
const fallbackReference = fallbackNode.updateReference(state);
const moduleKeys = [UserDataNode.name, userData.name];

const expected = {
    moduleKeys,
    constructorName: UserDataNode.name,
    constructorLength: UserDataNode.length,
    type: UserDataNode.type,
    explicit: {
        property: explicitNode.property,
        uniformType: explicitNode.uniformType,
        userData: explicitNode.userData,
        reference: explicitReference
    },
    fallback: {
        property: fallbackNode.property,
        uniformType: fallbackNode.uniformType,
        userData: fallbackNode.userData,
        reference: fallbackReference
    }
};
const actual = JSON.parse(execFileSync(binary, {encoding: 'utf8'}));
assert.deepEqual(actual, expected);
console.log('UserDataNode native/module parity passed');
