import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
const probe=process.argv[2];
assert.equal(process.versions.unicode,'17.0','This oracle requires the pinned Unicode 17 runtime');
const encode=text=>Array.from({length:text.length},(_,i)=>text.charCodeAt(i).toString(16)).join(',');
const full=execFileSync(probe,['--all'],{encoding:'utf8',maxBuffer:64*1024*1024}).trimEnd().split(/\r?\n/);
assert.equal(full.length,0x110000);
for(let p=0;p<=0x10ffff;p++)assert.equal(full[p],encode(String.fromCodePoint(p).toUpperCase()),'codepoint '+p.toString(16));
let random=0x9147c25d;
function next(){random^=random<<13;random^=random>>>17;random^=random<<5;return random>>>0;}
const input=['','straße','\u00df\ufb03\u0149','\u03b1\u0345\u0301','\u039f\u03a3\u03c2','iI\u0131\u0130','\ud800','\udc00','\ud800a\udc00','\ud801\udc28','\ud801\ud800\udc28\udfff','\u0000a\u0000'];
for(let i=0;i<10000;i++){let value='';const length=next()%65;for(let j=0;j<length;j++){value+=next()%2?String.fromCharCode(next()&0xffff):String.fromCodePoint(next()%0x110000);}input.push(value);}
const raw=execFileSync(probe,[],{input:input.map(s=>encode(s).replaceAll(',',' ')).join('\n')+'\n',encoding:'utf8',maxBuffer:64*1024*1024});
const lines=raw.replace(/\r\n/g,'\n').split('\n');assert.equal(lines.pop(),'');assert.equal(lines.length,input.length);
for(let i=0;i<input.length;i++)assert.equal(lines[i],encode(input[i].toUpperCase()),'string '+i);
console.log(`Unicode upper: ${full.length} codepoints and ${input.length} UTF-16 strings match Node ${process.version} / Unicode ${process.versions.unicode}`);
