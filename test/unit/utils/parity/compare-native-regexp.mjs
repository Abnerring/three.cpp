import {spawnSync} from 'node:child_process';
import assert from 'node:assert/strict';
import fs from 'node:fs';
const hex=s=>Array.from({length:s.length},(_,i)=>s.charCodeAt(i).toString(16).padStart(4,'0')).join('');
const encode=v=>v===undefined?{undefined:true}:typeof v==='string'?{string:hex(v)}:Array.isArray(v)?v.map(encode):v;
const cases=[];
const patterns=['','a','a*','a+?','a|b','(a)?b','(a)|(b)','(a|(b))+','(z)((a+)?(b+)?(c))*','(a)?b\\1','a{1,3}','[a-z]+','[^a]','[]','[^]','[\\s\\S]+?','\\s+','\\S+','[a\\S]','[^a\\S]','[\\s\\d]+','\\d+','\\w+','\\bfoo\\b','^x','$','a$','^.*$','.(.)','(?=(a))','(?!b)a','\\0','\\x61','\\u0061','[/]','/','\\\n'];
patterns.push('(\\1a)+','(a\\1)','(\\1)+','\\1(a)' ,'\\2(a)(b)','(a)\\2(b)','(a)\\1','\\3(a)','(?=(a+))a*b\\1','\\a','\\q','a{', 'a}', 'a{,2}', 'a{2,1}', 'a{2}', '\\1', '\\8', '\\11', '\\cA', '\\c1', '[\\c1]', '[a-\\s]', '[\\w-0]', '\\uZZZZ', '\\xZZ', '(?=a)*', '[[]', '[a[:digit:]]');
const strings=['','a','aaa','abab','aba','b','abc','zaacbbbcac','x\nx','a\n','a\r','a\r\n','a\u2028x','a\u2029x','\t \n\u00a0\u1680\u2000\u200a\u2028\u2029\u202f\u205f\u3000\ufeff','\u0085\u180e\u200b','foo foo1 _foo foo','ab1239','éＡ١','😀x😀','\ud800a\udfff','a\0b','/','\n','a\nb'];
for(const p of patterns)for(const s of strings)for(const f of ['', 'g','m','gm','y','gy','s'])for(const n of [0,1,-1,s.length+1])cases.push(['E',p,f,s,'',n]);
const replacements=['','x','$$-$&-$`-$\'','$1:$2:$3:$01:$10:$99:$0:$00','$<x>','$01$02$10$20','😀$&'];
for(const p of patterns.slice(0,30))for(const s of strings.slice(0,15))for(const f of ['', 'g','gm','y'])for(const r of replacements)cases.push(['R',p,f,s,r,2]);
for(const p of patterns.slice(0,30))for(const s of strings)for(const f of ['', 'g','gm']){cases.push(['F',p,f,s,'',3]);for(const n of [0,1,2,5,4294967295])cases.push(['P',p,f,s,'',n]);}
const fixed=[[/ERROR: 0:(\d+)/,'ERROR: 0:24 error\nERROR: 0:6'],[/NUM_SUN_LIGHTS/g,'NUM_SUN_LIGHTS + NUM_SUN_LIGHTS'],[/^[ \t]*#include +<([\w\d./]+)>/gm,'  #include <foo/bar.glsl>\n#include <a>\n x'],[/#pragma unroll_loop_start\s+for\s*\(\s*int\s+i\s*=\s*(\d+)\s*;\s*i\s*<\s*(\d+)\s*;\s*i\s*\+\+\s*\)\s*{([\s\S]+?)}\s+#pragma unroll_loop_end/g,'#pragma unroll_loop_start\nfor (int i = 0; i < 4; i++) {\na[i] = i;\n}\n#pragma unroll_loop_end'],[/\[\s*i\s*\]/g,'a[i] + b[ i ]']];
for(const [re,s]of fixed)for(const mode of ['E','R','F','P'])cases.push([mode,re.source,re.flags,s,'<$1:$2:$3>',20]);
for(const s of strings)for(const separator of ['', 'a','😀','\n','undefined'])for(const n of [0,1,2,100,4294967295])cases.push(['S',separator,'',s,'',n]);
for(const s of strings)for(const needle of ['', 'a','😀','\n'])for(const r of replacements)cases.push(['L',needle,'',s,r,0]);
for(const s of strings){cases.push(['T','','',s,'',0]);cases.push(['U','','',s+'Straße ﬃ 𐐨','',0]);}
for(const p of ['(', '[', '*', '\\', '(?<x>a)','(?<=a)b'])for(const f of ['','g'])cases.push(['E',p,f,'abc','',0]);
let seed=0x5d92a371;const random=n=>{seed^=seed<<13;seed^=seed>>>17;seed^=seed<<5;return (seed>>>0)%n;};
const atoms=['a','b','c','.', '[a-b]', '[^a]', '\\s','\\S','\\d','\\w','\\b', '\\0','\\x61','\\u0062','\\8','[\\s\\S]','[a-\\s]','[a[:digit:]]'];
const chars=['a','b','c','0','_',' ', '\n','\r','\u2028','\u00a0','\ud800','\udfff','😀',']','\\'];
function pattern(depth){if(!depth||random(3)===0)return atoms[random(atoms.length)];const inner=pattern(depth-1);if(random(3)===0)return '('+inner+'|'+pattern(depth-1)+')';return '('+inner+')'+['','?','*','+','{0,2}','*?'][random(6)]+pattern(depth-1);}
for(let i=0;i<5000;i++){let s='';for(let j=0,n=random(16);j<n;j++)s+=chars[random(chars.length)];const p=pattern(2),f=['','g','gm','m','y','s'][random(6)];for(const mode of ['E','R','F','P'])cases.push([mode,p,f,s,replacements[random(replacements.length)],random(5)]);}
const expected=cases.map(([mode,p,f,s,r,n])=>{try{
 if(mode==='T'||mode==='U')return encode(mode==='T'?s.trim():s.toUpperCase());
 if(mode==='L'||mode==='S')return encode(mode==='L'?s.replace(p,r):s.split(p,n));
 const re=new RegExp(p,f);re.lastIndex=n;
 if(mode==='E'){const results=[];for(let i=0;i<3;i++){const v=re.exec(s);results.push({value:encode(v),index:v?.index??null,lastIndex:re.lastIndex});}return{results,source:encode(re.source),flags:encode(re.flags)};}
 const callback=(...args)=>'{'+args.map(v=>JSON.stringify(encode(v))+';').join('')+'}';
 const value=mode==='P'?s.split(re,n):s.replace(re,mode==='F'?callback:r);return{value:encode(value),lastIndex:re.lastIndex};
}catch(e){return{error:e.name};}});
const proc=spawnSync(process.argv[2],[],{input:cases.map(c=>[c[0],...c.slice(1,5).map(s=>hex(s)||'-'),c[5]].join(' ')).join('\n')+'\n',encoding:'utf8',maxBuffer:512*1024*1024});
assert.equal(proc.status,0,proc.stderr);const actual=proc.stdout.trimEnd().split(/\r?\n/).map(JSON.parse);assert.equal(actual.length,cases.length);
const failures=[],unsupported=[];for(let i=0;i<actual.length;i++){if(actual[i].unsupported&&['(?<x>a)','(?<=a)b','(?=a)*'].includes(cases[i][1])){unsupported.push(i);continue;}try{assert.deepEqual(actual[i],expected[i]);}catch{failures.push({case:cases[i],expected:expected[i],actual:actual[i]});}}
const categories={};for(const failure of failures){const key=failure.case[1];categories[key]=(categories[key]??0)+1;}
const summary={categories,node:process.version,cases:cases.length,unsupported:unsupported.length,failures:failures.length,examples:failures.slice(0,50)};if(process.env.REGEXP_SAVE_PREFIX)fs.writeFileSync(process.env.REGEXP_SAVE_PREFIX+'-summary.json',JSON.stringify(summary,null,2));
assert.equal(failures.length,0,JSON.stringify(failures.slice(0,8),null,2));console.log(JSON.stringify({...summary,examples:undefined}));
