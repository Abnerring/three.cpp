import {pathToFileURL} from 'node:url';
import path from 'node:path';
for(const kind of ['Quaternion','Euler']){
 const {[kind]:Type}=await import(pathToFileURL(path.join(process.argv[2],'src/math/'+kind+'.js')));
 let late=0,old=0,next=0;const a=new Type();a._onChange(()=>{late++;});const own=Object.hasOwn(a,'_onChangeCallback');a._onChangeCallback();
 const b=new Type();b._onChange(()=>{old++;});const saved=b._onChangeCallback;b._onChange(()=>{next++;});saved.call(b);
 console.log(JSON.stringify({kind,late:{own,calls:late},snapshot:{old,new:next}}));
}
