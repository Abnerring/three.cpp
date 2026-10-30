import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
const destination=process.argv[2]??'D:/Temp/surui/threecpp-batched-mesh-20261028/boolean-boundaries01';fs.mkdirSync(destination,{recursive:true});
const context=vm.createContext({});
const foreign=vm.runInContext('({Boolean, prototype:Boolean.prototype, boxed:new Boolean(false), Derived:function Derived(){}})',context);
foreign.Derived.prototype=null;
const output=[];
for(const mode of ['ordinary','bound','proxy','boundProxy']){
 let target=foreign.Derived;
 if(mode==='proxy'||mode==='boundProxy')target=new Proxy(target,{});
 if(mode==='bound'||mode==='boundProxy')target=target.bind(null);
 const result=Reflect.construct(Boolean,[false],target);
 output.push({case:'foreign newTarget fallback',mode,foreignPrototype:Object.getPrototypeOf(result)===foreign.prototype,currentRealmPrototype:Object.getPrototypeOf(result)===Boolean.prototype,value:Boolean.prototype.valueOf.call(result)});
}
output.push({case:'foreign BooleanData receiver',value:Boolean.prototype.valueOf.call(foreign.boxed),string:Boolean.prototype.toString.call(foreign.boxed)});
for(const value of [0n,1n,-1n,2n**128n])output.push({case:'BigInt ToBoolean',input:value.toString()+'n',value:Boolean(value),boxedValue:Boolean.prototype.valueOf.call(new Boolean(value))});
output.push({case:'Symbol Object wrapper prerequisite',boxed:Object(Symbol('x')).valueOf().description,prototype:Object.getPrototypeOf(Object(Symbol()))===Symbol.prototype,truthy:Boolean(Symbol())});
fs.writeFileSync(path.join(destination,'oracle.json'),JSON.stringify(output,null,2)+'\n');
fs.writeFileSync(path.join(destination,'scope.json'),JSON.stringify({node:process.version,status:'oracle-only, explicitly outside candidate C++ runtime domain',reason:'Current JSHeap has one intrinsic realm per active thread heap and rejects foreign heap edges. JSValue has no BigInt type. Existing Symbol Object wrapper intrinsic is not implemented. No fake cross-realm identities or Symbol/BigInt adapters are introduced.',cases:output.length},null,2)+'\n');
console.log(JSON.stringify({cases:output.length,destination}));
