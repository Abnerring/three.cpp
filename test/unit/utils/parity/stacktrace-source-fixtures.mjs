export const fixtures = [];
for (const operation of ['frames','structured','capture-middle','capture-absent','lazy','default-filtered','reentry','throw-unwind','microtask']) fixtures.push({kind:'source',operation,sourceURI:new URL('./stacktrace-mapped-scenarios.mjs',import.meta.url).href});
for (const text of [
  '', 'Error', 'Error: boom\n    at load (file:///project/source.js?stamp=3:42:9)\n    at file:///project/main.js:7:11',
  'work@https://host/app.js?hmr=1:25:4\n@https://host/anonymous.js:6:2',
  'at root (http://host/StackTrace.js:1:2)\nat one (http://host/TSLCore.js:2:3)\nat two (http://host/ShaderNode.js:3:4)\nat three (http://host/three.webgpu.min.js:4:5)\nat show (http://host/NodeMaterial.js:829:4)',
  'at fn (a:b:c.js:00042:00009)\nat tab (a.js:12:4)\r\nignored line\n',
]) fixtures.push({kind:'stack',operation:'string',text});
for(const operation of ['input-split','input-split-throw','parseInt-reentry','parseInt-throw','match-protocol','global-error','empty-input-error','parseInt-getter-order'])fixtures.push({kind:'stack',operation,text:'Error\n at task (file:///project/source.js:42:9)'});
const values=[undefined,null,true,false,0,-0,17,1e-7,1e21,'','  -0x10 tail','+0XFF','0b101','0o77','08','-.1','+','-0','Infinity','NaN','123.75tail','\uFEFF\u2003-42\n','900719925474099267','f'.repeat(280),'z'.repeat(80),'1'.repeat(1050),'-'+'0'.repeat(80)+'1'.repeat(310)];
const radices=[undefined,null,0,2,3,4,8,10,16,20,32,36,1,37,-1,4294967298,1.9,'16',Infinity,NaN];
const encode=v=>v===undefined?{$tag:'undefined'}:typeof v==='number'?(Number.isNaN(v)?{$tag:'NaN'}:v===Infinity?{$tag:'Infinity'}:v===-Infinity?{$tag:'-Infinity'}:Object.is(v,-0)?{$tag:'-0'}:v):v;
for(const value of values)for(const radix of radices)fixtures.push({kind:'parseInt',value:encode(value),radix:encode(radix)});
for(const operation of ['coercion-order','string-throw','radix-throw','symbol-input','symbol-radix','metadata','alias'])fixtures.push({kind:'parseInt',operation});
