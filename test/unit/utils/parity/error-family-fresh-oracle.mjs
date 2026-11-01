import assert from 'node:assert/strict';
assert.equal(process.version,'v26.5.0');assert.equal(process.argv.length,3);
const name=process.argv[2];assert(['Error','EvalError','RangeError','ReferenceError','SyntaxError','TypeError','URIError'].includes(name));
const ctor=globalThis[name],prototype=ctor.prototype;prototype.constructor='sentinel';Object.preventExtensions(prototype);
const value=ctor('fixture');console.log(JSON.stringify({name,globalIdentity:globalThis[name]===ctor,constructorAlias:prototype.constructor,prototypeExtensible:Object.isExtensible(prototype),instancePrototype:Object.getPrototypeOf(value)===prototype,brand:Error.isError(value),nameValue:value.name}));
