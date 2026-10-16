// Differential check for the native examples/jsm/math/ColorSpaces.js port.
//
// The pinned module and the pinned ColorManagement are imported from the checkout, the same
// description is built there, and it is compared with the probe output using
// assert.deepEqual. The description deliberately carries the identities the module
// establishes (which transfer function is used, which objects Extended sRGB shares with the
// sRGB space), not only the numeric tables.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-color-spaces.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const ColorSpaces=await import(url('examples/jsm/math/ColorSpaces.js'));
// The pinned module imports 	hree, so the comparison must read the same module instance
// (the checkout's bundle) instead of the source ColorManagement, which has its own spaces.
const THREE=await import(url('build/three.module.js'));
const {ColorManagement,SRGBColorSpace,LinearSRGBColorSpace}=THREE;

const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);
const configObject=config=>Object.fromEntries(Object.keys(config).map(key=>[key,String(config[key])]));
function spaceJSON(space,srgbSpace,linearSpace){
    const hasWorking=Object.hasOwn(space,'workingColorSpaceConfig');
    return {
        primaries:encodeArray(space.primaries),
        whitePoint:encodeArray(space.whitePoint),
        luminanceCoefficients:encodeArray(space.luminanceCoefficients),
        transferIsSRGB:space.transfer===srgbSpace.transfer,
        transferIsLinear:space.transfer===linearSpace.transfer,
        toXYZ:encodeArray(space.toXYZ.elements),
        fromXYZ:encodeArray(space.fromXYZ.elements),
        outputColorSpaceConfig:Object.hasOwn(space,'outputColorSpaceConfig')?configObject(space.outputColorSpaceConfig):null,
        hasOutputColorSpaceConfig:Object.hasOwn(space,'outputColorSpaceConfig'),
        hasWorkingColorSpaceConfig:hasWorking,
        workingColorSpaceConfig:hasWorking?configObject(space.workingColorSpaceConfig):null,
        sharesToXYZWithSRGB:space.toXYZ===srgbSpace.toXYZ,
        sharesPrimariesWithSRGB:space.primaries===srgbSpace.primaries
    };
}
const srgbSpace=ColorManagement.spaces[SRGBColorSpace];
const linearSpace=ColorManagement.spaces[LinearSRGBColorSpace];
const extended=ColorSpaces.ExtendedSRGBColorSpaceImpl;
const expected={
    // An ES module namespace lists its exports in lexicographic order, so the export set is
    // compared without order; the probe keeps the upstream source order.
    exportKeys:Object.keys(ColorSpaces).sort(),
    names:[ColorSpaces.DisplayP3ColorSpace,ColorSpaces.LinearDisplayP3ColorSpace,
        ColorSpaces.LinearRec2020ColorSpace,ColorSpaces.ExtendedSRGBColorSpace],
    displayP3:spaceJSON(ColorSpaces.DisplayP3ColorSpaceImpl,srgbSpace,linearSpace),
    linearDisplayP3:spaceJSON(ColorSpaces.LinearDisplayP3ColorSpaceImpl,srgbSpace,linearSpace),
    linearRec2020:spaceJSON(ColorSpaces.LinearRec2020ColorSpaceImpl,srgbSpace,linearSpace),
    extendedSRGB:{
        ownKeys:Object.keys(extended),
        srgbOwnKeys:Object.keys(srgbSpace),
        sharesEverySpreadField:['primaries','whitePoint','transfer','toXYZ','fromXYZ','luminanceCoefficients']
            .every(key=>extended[key]===srgbSpace[key]),
        outputColorSpaceConfig:configObject(extended.outputColorSpaceConfig),
        sharesOutputConfig:extended.outputColorSpaceConfig===srgbSpace.outputColorSpaceConfig,
        workingColorSpaceConfig:Object.hasOwn(extended,'workingColorSpaceConfig')
            ?configObject(extended.workingColorSpaceConfig):null
    }
};
const raw=execFileSync(path.resolve(probePath),[],{encoding:'utf8',maxBuffer:16*1024*1024}).trim();
const actual=JSON.parse(raw);
if(Array.isArray(actual.exportKeys))actual.exportKeys=[...actual.exportKeys].sort();
assert.deepEqual(actual,expected,'ColorSpaces module description');
console.log('ColorSpaces parity: module exports, the four space implementations, the transfer identities and the '
    +`Extended sRGB spread identities match the pinned checkout; ${process.version}`);
