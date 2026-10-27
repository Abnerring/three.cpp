import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
import {execFileSync} from 'node:child_process';

const args=process.argv.slice(2),check=args.includes('--check');
const upstream = path.resolve(args.find(value=>value!=='--check') ?? '../three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw new Error('Unexpected upstream checkout');
const constants = await import(pathToFileURL(path.join(upstream, 'src/constants.js')));
const literal = value => typeof value === 'string' ? JSON.stringify(value) : String(value);
const lines = ['#pragma once', '// Generated from the pinned three.js src/constants.js. Do not renumber constants.'];
for (const [name, value] of Object.entries(constants)) {
    if (typeof value === 'object') {
        lines.push(`namespace ${name} {`);
        for (const [key, member] of Object.entries(value)) {
            if (!['string', 'number', 'boolean'].includes(typeof member)) throw new Error(`${name}.${key}: unsupported constant`);
            lines.push(`inline constexpr auto ${key} = ${literal(member)};`);
        }
        lines.push('}');
    } else {
        lines.push(`inline constexpr auto ${name} = ${literal(value)};`);
    }
}
lines.push('// Native managed module bindings; object constants retain shared mutable identity.', '#include <string>', 'class JSValue;', 'JSValue nativeConstantsModule();', 'JSValue nativeConstantsBinding(const std::string& name);');
const runtime=['// Generated from fixed three.js constants.js; native data, no JS execution.'];
for(const [name,value] of Object.entries(constants)){
    if(value!==null&&typeof value==='object'){
        runtime.push('{ auto value=JSValue::object();');
        for(const [key,member] of Object.entries(value))runtime.push(`own(value,${JSON.stringify(key)},${literal(member)});`);
        runtime.push(`own(bindings,${JSON.stringify(name)},value); }`);
    }else runtime.push(`own(bindings,${JSON.stringify(name)},${literal(value)});`);
}
for(const [file,text] of [['constants.h',lines.join('\n')+'\n'],['ConstantsRuntimeData.inc',runtime.join('\n')+'\n']]){
    const target=new URL('../src/'+file,import.meta.url);
    if(check){if(fs.readFileSync(target,'utf8').replaceAll('\r\n','\n')!==text)throw new Error('Stale '+file);}else fs.writeFileSync(target,text);
}
console.log(`Constants: ${Object.keys(constants).length} exports verified`);
