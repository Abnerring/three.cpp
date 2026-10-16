// Differential check for the native examples/jsm/utils/SortUtils.js port.
//
// The upstream module is imported from the pinned three.js checkout and the same scenarios
// are run through radixSort there. Getter functions, the reversed flag and a caller-owned
// aux array are exercised, and both the sorted array and the scratch array are compared
// after the call, so placement of the closing pass is covered as well.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-sort-utils.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {radixSort}=await import(pathToFileURL(path.join(sourceDir,'examples/jsm/utils/SortUtils.js')).href);

const getters=[
    el=>el,
    el=>el*3+7,
    el=>Math.floor(el)%3,
    el=>el/7,
    el=>el*4294967296,
    el=>~el,
    el=>el*1e20
];

let state=0x9e3779b9;
function next(){state^=state<<13;state^=state>>>17;state^=state<<5;return state>>>0;}
const special=[0,-0,1,-1,255,256,255.5,65535,65536,16777216,2147483647,2147483648,4294967295,4294967296,
    NaN,Infinity,-Infinity,1e20,-1e20,1e-7,-0.5,0.5,1/3];
function patternValues(kind,count){
    return Array.from({length:count},(_,i)=>{
        if(kind===0)return next()%1024-512;
        if(kind===1)return next()%7;
        if(kind===2)return i;
        if(kind===3)return count-i;
        if(kind===4)return i%2?count-i:i;
        if(kind===5)return next();
        if(kind===6)return special[next()%special.length];
        if(kind===7)return next()%2?(next()%3)*4294967296:next();
        return i%256;
    });
}
const scenarios=[];
for(const count of [0,1,2,3,4,7,8,31,32,33,63,64,65,127,128,255,256,257,300,1000,2048]){
    for(let getMode=0;getMode<getters.length;++getMode){
        for(const reversed of [0,1]){
            for(const auxMode of [0,1]){
                scenarios.push({values:patternValues((count+getMode+reversed)%9,count),getMode,reversed,auxMode});
            }
        }
    }
}
for(let i=0;i<160;++i)scenarios.push({
    values:patternValues(next()%9,next()%600),getMode:next()%getters.length,
    reversed:next()%2,auxMode:next()%2});

const encodeNumber=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const encodeArray=values=>values.map(value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value);
const input=scenarios.map(({values,getMode,reversed,auxMode})=>{
    const aux=values.map((_,i)=>auxMode?i*3+1:0);
    const tokens=[values.length,getMode,reversed,auxMode,...values.map(encodeNumber)];
    if(auxMode)for(const value of aux)tokens.push(encodeNumber(value));
    return tokens.join(' ');
}).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:256*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');

let comparisons=0;
for(let index=0;index<scenarios.length;++index){
    const {values,getMode,reversed,auxMode}=scenarios[index];
    const get=getters[getMode];
    const array=values.slice(0);
    const aux=auxMode?values.map((_,i)=>i*3+1):null;
    radixSort(array,auxMode?{get,reversed:Boolean(reversed),aux}:{get,reversed:Boolean(reversed)});
    const expected={arr:encodeArray(array),aux:auxMode?encodeArray(aux):null,length:values.length};
    comparisons+=values.length;
    assert.deepEqual(JSON.parse(output[index]),expected,
        `scenario ${index}: length ${values.length}, getter ${getMode}, reversed ${reversed}, aux ${auxMode}`);
}
console.log(`SortUtils radixSort parity: ${scenarios.length} scenarios, ${comparisons} elements, `
    +`getters ${getters.length}, upstream module from the pinned checkout; ${process.version}`);
