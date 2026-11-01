import path from 'node:path';
import {pathToFileURL} from 'node:url';
const root=process.argv[2],load=p=>import(pathToFileURL(path.join(root,'src',p+'.js')));
const {Object3D}=await load('core/Object3D');
const reason={marker:'actual-import-constructor-setter'},old=Object.getOwnPropertyDescriptor(Object3D.prototype,'type');let calls=0;
Object.defineProperty(Object3D.prototype,'type',{configurable:true,set(){calls++;throw reason;}});
let first,second,third;
try{await load('renderers/common/QuadMesh');}catch(e){first=e;}
try{await load('renderers/common/QuadMesh');}catch(e){second=e;}
if(old)Object.defineProperty(Object3D.prototype,'type',old);else delete Object3D.prototype.type;
try{await load('renderers/common/QuadMesh');}catch(e){third=e;}
process.stdout.write(JSON.stringify({setterCalls:calls,firstIsReason:first===reason,secondSame:second===first,thirdSame:third===first}));
