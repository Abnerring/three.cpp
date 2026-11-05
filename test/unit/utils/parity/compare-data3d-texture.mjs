import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {spawnSync} from 'node:child_process';
const upstream=path.resolve(process.argv[2]??'D:/Temp/surui/threejs-pinned-11738a-20261104');
const binary=process.argv[3],directory=process.argv[4];
const {Data3DTexture:C}=await import(pathToFileURL(path.join(upstream,'src/textures/Data3DTexture.js')).href);
const base=Object.getPrototypeOf(C),result={source:String(C),copySource:String(C.prototype.copy),name:C.name,length:C.length,copyLength:C.prototype.copy.length,inherits:Object.getPrototypeOf(C)===base&&Object.getPrototypeOf(C.prototype)===base.prototype};
const t=new C(),initial={};for(const key of ['isData3DTexture','magFilter','minFilter','wrapR','generateMipmaps','flipY','unpackAlignment','version'])initial[key]=t[key];
Object.assign(initial,{image:t.image,alias:t.source.data===t.image,sourceVersion:t.source.version});result.initial=initial;
const data=new Uint8Array([1,2,3,4]),odd=new C(data,'2',null,false);result.uncoerced=[odd.image.data===data,odd.image.width,odd.image.height,odd.image.depth];
odd.wrapR=1000;const cloned=odd.clone();result.clone=[Object.getPrototypeOf(cloned)===C.prototype,cloned.source===odd.source,cloned.image===odd.image,cloned.wrapR,cloned.version];
result.construction=[];
for(const fail of ['','isData3DTexture','image','magFilter','minFilter','wrapR','generateMipmaps','flipY','unpackAlignment']){
    const state={events:[],fail};function Parent(...args){state.newTarget=new.target===C;state.superArgs=args;const self={};
        for(const name of ['isData3DTexture','image','magFilter','minFilter','wrapR','generateMipmaps','flipY','unpackAlignment'])Object.defineProperty(self,name,{configurable:true,set(value){state.events.push(name);if(fail===name)throw 'field sentinel';if(name==='image')state.image=value;}});
        return self;
    }
    Object.setPrototypeOf(C,Parent);let threw=false;try{new C(undefined,'2',null,false);}catch(e){assert.equal(e,'field sentinel');threw=true;}finally{Object.setPrototypeOf(C,base);}
    const row={fail,events:state.events,newTarget:state.newTarget,superArgs:state.superArgs,threw};if(Object.hasOwn(state,'image'))row.image=state.image;result.construction.push(row);
}
const target=new C(),prototype=C.prototype,saved=Object.getPrototypeOf(prototype),events=[],source={},parent={};
Object.defineProperty(parent,'copy',{get(){events.push(['get super.copy',this===target]);return function(value){events.push(['call super.copy',this===target,value===source]);return 17;};}});
Object.defineProperty(source,'wrapR',{get(){events.push('get source.wrapR');return 'wrap';}});
Object.setPrototypeOf(prototype,parent);let copied;try{copied=prototype.copy.call(target,source);}finally{Object.setPrototypeOf(prototype,saved);}
result.copy={events,returnsSelf:copied===target,wrapR:target.wrapR};
const expected=JSON.parse(JSON.stringify(result));
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));}
if(!binary){console.log('Generated unchanged pinned Data3DTexture constructor, clone, dynamic super and setter-throw oracle');process.exit(0);}
const run=spawnSync(binary,['--data3d-oracle'],{encoding:'utf8',env:process.env});assert.equal(run.status,0,run.stderr??run.error?.message);
const actual=JSON.parse(run.stdout);if(directory)fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));assert.deepEqual(actual,expected);
console.log('Data3DTexture: exact source metadata, defaults, aliases, clone, 9 dynamic constructor cases and dynamic super.copy match pinned JS');
