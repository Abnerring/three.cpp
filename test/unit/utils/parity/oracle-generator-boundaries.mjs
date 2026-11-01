import fs from 'node:fs';
import path from 'node:path';
const output=process.argv[2];if(!output||process.argv.length!==3)throw new Error('Usage: oracle-generator-boundaries.mjs new-output.json');if(fs.existsSync(output))throw new Error('Evidence exists: '+output);
const GF=Object.getPrototypeOf(function*(){}).constructor;
const results=[];
for(const [name,Ctor] of [['Function',Function],['GeneratorFunction',GF]])for(const args of [[],[''],['return 17'],['a','return a'],['yield 3'],[')']]){
 const events=[],converted=args.map((source,index)=>({toString(){events.push(['toString',index]);return source;}}));
 let result;try{const f=Reflect.apply(Ctor,undefined,converted);const value=f(19);result={name:f.name,length:f.length,source:Function.prototype.toString.call(f),first:name==='GeneratorFunction'?value.next():value};}catch(e){result={error:e.name};}
 results.push({constructor:name,args,events,result,nativeCandidate:args.length===0?'implemented-native-empty-body':'pending-native-dynamic-compiler'});
}
fs.mkdirSync(path.dirname(path.resolve(output)),{recursive:true});fs.writeFileSync(output,JSON.stringify({node:process.version,v8:process.versions.v8,scope:'Node-only boundaries. Pending cases are not native passes.',results},null,2)+'\n');console.log(JSON.stringify({cases:results.length,output}));
