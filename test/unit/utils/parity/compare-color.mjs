// Differential check for the native src/math/Color.js port.
//
// One call per scenario through the pinned module. The cases lean on the parsing and conversion paths a
// hand port drifts on: hex with and without a colour space, CSS functional notation for rgb and hsl, hue
// wrapping past one, the two sRGB conversions, and the hex round trip.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-color.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Color}=await import(pathToFileURL(path.join(sourceDir,'src/math/Color.js')).href);
const scenarios=[
    ['setRGB',0.1,0.2,0.3],
    ['setHex',16711680],
    ['setHex',255],
    ['setStyle','#ff0000'],
    ['setStyle','#0f0'],
    ['setStyle','rgb(255,0,0)'],
    ['setStyle','hsl(120,100%,50%)'],
    ['setHSL',0.5,0.5,0.5],
    ['setHSL',1.5,1,0.5],
    ['setHSL',-0.25,0.4,0.6],
    ['getHSL',0.2,0.4,0.6],
    ['getHSL',1,1,1],
    ['srgbToLinear',0.5,0.25,1],
    ['linearToSrgb',0.5,0.25,1],
    ['lerp',0,0,0,1,1,1,0.25],
    ['getHex',0.2,0.4,0.6],
    ['getHexString',1,0,0]
];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const lines=scenarios.map(parts=>parts.join(' '));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
const call=(op,args)=>{
    const color=new Color(args[0],args[1],args[2]);
    switch(op){
        case 'setRGB': return new Color().setRGB(args[0],args[1],args[2]);
        case 'setHex': return new Color().setHex(args[0]);
        case 'setStyle': return new Color().setStyle(args[0]);
        case 'setHSL': return new Color().setHSL(args[0],args[1],args[2]);
        case 'getHSL': { const out={}; color.getHSL(out); return out; }
        case 'srgbToLinear': return color.convertSRGBToLinear();
        case 'linearToSrgb': return color.convertLinearToSRGB();
        case 'lerp': return color.lerp(new Color(args[3],args[4],args[5]),args[6]);
        case 'getHex': return {value:color.getHex()};
        case 'getHexString': return {value:color.getHexString()};
        default: throw new Error('unknown op '+op);
    }
};
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    const value=call(op,args);
    const expected=value.value!==undefined?{value:value.value}
        :value.h!==undefined?{h:encode(value.h),s:encode(value.s),l:encode(value.l)}
        :{r:encode(value.r),g:encode(value.g),b:encode(value.b)};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,args.join(' '));
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,`${op}(${args.join(', ')})`);
}
console.log(`Color parity: ${scenarios.length} scenarios`);