import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
import { fileURLToPath } from 'node:url';

const fixtureURL=new URL('./object-loader-fixtures.json',import.meta.url);
export const fixtureBundle=JSON.parse(fs.readFileSync(fixtureURL,'utf8'));
export function withSchema(request){return {...request,_schema:fixtureBundle.schema,_brands:fixtureBundle.brands.map(row=>row[1]),_structuralBrands:fixtureBundle.structuralBrands};}
export async function createOracle(upstream){
    const load=relative=>import(pathToFileURL(path.join(upstream,'src',relative)).href);
    const [{ObjectLoader},{FileLoader},{Loader},{DefaultLoadingManager},{setConsoleFunction,getConsoleFunction}]=await Promise.all([
        load('loaders/ObjectLoader.js'),load('loaders/FileLoader.js'),load('loaders/Loader.js'),load('loaders/LoadingManager.js'),load('utils.js')]);
    const {MaterialLoader}=await load('loaders/MaterialLoader.js');
    // Inline-image cube handling uses the source's actual instanceof branch.
    // A host constructor is sufficient here: these fixtures use typed pixels,
    // and never manufacture an HTML image or replace ImageLoader's algorithm.
    const oldImage=Object.getOwnPropertyDescriptor(globalThis,'HTMLImageElement');
    if(!oldImage)globalThis.HTMLImageElement=class HTMLImageElement {};
    function metadata(){
        const ctor=ObjectLoader,p=ctor.prototype,methods={};
        for(const name of Object.getOwnPropertyNames(p)){
            if(name==='constructor')continue;
            const d=Object.getOwnPropertyDescriptor(p,name),f=d.value;
            methods[name]={name:f.name,length:f.length,source:f.toString(),flags:[d.writable,d.enumerable,d.configurable],async:Object.getPrototypeOf(f)===Object.getPrototypeOf(async function(){}),constructible:isConstructor(f)};
        }
        const f=ctor.registerGeometry;methods['static:registerGeometry']={name:f.name,length:f.length,source:f.toString()};
        const loader=new ctor(),defaults={};for(const name of ['crossOrigin','withCredentials','path','resourcePath','requestHeader'])defaults[name]=loader[name];
        return {name:ctor.name,length:ctor.length,source:ctor.toString(),prototypeNames:Object.getOwnPropertyNames(p),methods,defaults};
    }
    const isConstructor=fn=>{try{Reflect.construct(function(){},[],fn);return true;}catch{return false;}};
    class Snapshot{
        constructor(request){this.schema=request._schema;this.brands=request._brands;this.structural=request._structuralBrands;this.seen=new Map();this.uuids=new Set();this.collect(request);}
        collect(value){if(value===null||typeof value!=='object')return;for(const key of Object.keys(value)){if(['_schema','_brands','_structuralBrands'].includes(key))continue;const child=value[key];if(key==='uuid'&&typeof child==='string')this.uuids.add(child);this.collect(child);}}
        value(source){
            if(source===undefined)return {$undefined:true};
            if(typeof source==='number'&&!Number.isFinite(source))return {$number:Number.isNaN(source)?'NaN':source<0?'-Infinity':'Infinity'};
            if(source===null||!['object','function'].includes(typeof source))return source;
            if(this.seen.has(source))return {$ref:this.seen.get(source)};
            const result={$id:this.seen.size+1};this.seen.set(source,result.$id);
            if(Array.isArray(source)){result.$type='Array';result.items=Array.from({length:source.length},(_,i)=>this.value(source[i]));return result;}
            for(const name of ['Int8Array','Uint8Array','Uint8ClampedArray','Int16Array','Uint16Array','Int32Array','Uint32Array','Float32Array','Float64Array']){
                if(source instanceof globalThis[name]){result.$type=name;result.items=Array.from(source,item=>this.value(item));return result;}
            }
            let brand='Object';for(const candidate of this.brands)if(source['is'+candidate]){brand=candidate;break;}
            if(brand==='Object')for(const [a,b,candidate] of this.structural)if(a in source&&b in source){brand=candidate;break;}
            result.$type=brand;
            const fields=brand==='Object'?Object.keys(source).sort():this.schema[brand];
            for(const key of fields){
                if(key==='id'||!(key in source))continue;
                const child=source[key];
                if(key==='uuid'&&(typeof child!=='string'||!this.uuids.has(child)))continue;
                if(brand==='Object'&&typeof child==='function')continue;
                result[key]=this.value(child);
            }
            return result;
        }
    }
    function dispatch(loader,request,events,onLoad){
        const names=['parseAnimations','parseShapes','parseGeometries','parseImages','parseTextures','parseMaterials','parseObject','parseSkeletons','bindSkeletons','bindLightTargets'];
        const label=value=>value===undefined?'undefined':typeof value==='function'?'callback':value.tag??'undefined';
        const makeJSON=owner=>Object.fromEntries([]);
        const outer=makeJSON('outer'),inner=makeJSON('inner');
        for(const [json,owner] of [[outer,'outer'],[inner,'inner']])for(const key of ['animations','shapes','geometries','images','textures','materials','object','skeletons'])Object.defineProperty(json,key,{configurable:true,enumerable:true,get(){events.push(['get',owner,key]);if(request.variant==='getterThrow'&&owner==='outer'&&key==='geometries')throw 'geometries';return {tag:owner+':'+key};}});
        for(const name of names)Object.defineProperty(loader,name,{configurable:true,get(){events.push(['method',name]);return function(...args){
            events.push(['call',name,args.map(label)]);
            if(name==='parseImages'&&request.variant==='syncImages')args[1]();
            if(name==='parseObject'){
                const owner=args[0].tag.split(':')[0];
                if(owner==='outer'&&request.variant==='reenter'){const nested=loader.parse(inner);events.push(['reenter',nested.tag]);}
                return {tag:owner};
            }
            if(name.startsWith('bind'))return undefined;
            return {tag:name};
        };}});
        return loader.parse(outer,request.onLoad?onLoad:undefined);
    }
    function geometryDispatch(loader,request,events){
        const variant=request.variant,type=variant==='unknown'?'ObjectLoaderParityUnknown':'ObjectLoaderParityCustom',shared={payload:17},shapes={tag:'shapes'};
        const makeData=owner=>{const data={};for(const key of ['type','uuid','name','userData'])Object.defineProperty(data,key,{configurable:true,enumerable:true,get(){events.push(['get',owner,key,this===data]);return key==='type'?type:key==='uuid'?owner:key==='name'?owner+'-geometry':shared;}});return data;};
        const outer=makeData('outer'),inner=makeData('inner'),geometryClass={};
        Object.defineProperty(geometryClass,'fromJSON',{configurable:true,enumerable:true,get(){events.push(['get:fromJSON',this===geometryClass]);return function(data,receivedShapes){const owner=data===outer?'outer':'inner';events.push(['fromJSON',owner,this===geometryClass,receivedShapes===shapes]);if(variant==='throw')throw 'custom geometry';if(owner==='outer'&&variant==='reenter'){const nested=loader.parseGeometries([inner],shapes);events.push(['reenter',nested.inner.marker]);}return {marker:owner};};}});
        if(variant!=='unknown')ObjectLoader.registerGeometry(type,geometryClass);
        return loader.parseGeometries([outer],shapes);
    }
    function materialDispatch(loader,request,events){
        const old=Object.getOwnPropertyDescriptor(MaterialLoader.prototype,'parse');let coercions=0;const key={toString(){events.push(['key',++coercions]);if(request.variant==='keyThrow'&&coercions===2)throw 'material key';return 'material';}},data={};
        Object.defineProperty(data,'uuid',{configurable:true,enumerable:true,get(){events.push('uuid');return key;}});
        Object.defineProperty(MaterialLoader.prototype,'parse',{configurable:true,enumerable:true,get(){events.push('get:parse');return function(input){events.push(['parse',input===data]);if(request.variant==='parseThrow')throw 'material parse';return {marker:17};};}});
        try{return loader.parseMaterials([data],{});}finally{Object.defineProperty(MaterialLoader.prototype,'parse',old);}
    }
    async function run(input){
        const request=withSchema(input);
        if(request.op==='metadata')return metadata();
        const events=[],result={},snapshot=new Snapshot(request),loader=new ObjectLoader();
        if(request.options)Object.assign(loader,request.options);
        let callbackCount=0,callbackValue,receiverUndefined=true;
        const onLoad=function(value){callbackCount++;callbackValue=value;receiverUndefined&&=this===undefined;events.push('onLoad');};
        const onError=error=>events.push(['onError',error.name]);
        const oldConsole=getConsoleFunction();setConsoleFunction((level,message,...params)=>events.push(['console',level,message,...request.op.startsWith('load')?params.map(()=>'argument'):params]));
        let restoreLoad;
        try{
            let value;
            switch(request.op){
                case 'parse':value=loader.parse(request.json,request.onLoad?onLoad:undefined);break;
                case 'parseAsync':value=await loader.parseAsync(request.json);break;
                case 'parseObject':value=loader.parseObject(request.json,request.geometries,request.materials,request.textures,request.animations);break;
                case 'parseShapes':case 'parseAnimations':value=loader[request.op](request.json);break;
                case 'parseImages':value=loader.parseImages(request.json,request.onLoad?onLoad:undefined);break;
                case 'parseImagesAsync':value=await loader.parseImagesAsync(request.json);break;
                case 'parseGeometries':value=loader.parseGeometries(request.json,request.shapes);break;
                case 'parseMaterials':value=loader.parseMaterials(request.json,request.textures);break;
                case 'parseTextures':value=loader.parseTextures(request.json,loader.parseImages(request.images));break;
                case 'parseSkeletons':{const root=loader.parseObject(request.object,{},{},{},{});value={root,skeletons:loader.parseSkeletons(request.json,root)};break;}
                case 'dispatch':value=dispatch(loader,request,events,onLoad);break;
                case 'geometryDispatch':value=geometryDispatch(loader,request,events);break;
                case 'materialDispatch':value=materialDispatch(loader,request,events);break;
                case 'load':case 'loadAsync':{
                    const old=Object.getOwnPropertyDescriptor(FileLoader.prototype,'load');
                    restoreLoad=()=>Object.defineProperty(FileLoader.prototype,'load',old);
                    FileLoader.prototype.load=function(url,onLoad,onProgress,onError){events.push(['file.load',url,this.path,this.withCredentials,this.requestHeader===loader.requestHeader]);onLoad(request.variant==='badJSON'?'{':JSON.stringify(request.json));};
                    if(request.op==='load'){loader.load('asset.json',request.onLoad?onLoad:undefined,undefined,onError);value=callbackValue;}
                    else value=await loader.loadAsync('asset.json');
                    break;
                }
                default:throw new Error('Unknown ObjectLoader fixture operation: '+request.op);
            }
            result.value=snapshot.value(value);
            if(request.onLoad){result.onLoadCount=callbackCount;result.onLoadSame=callbackValue===value;result.receiverUndefined=receiverUndefined;}
        }catch(error){result.error=error!==null&&(typeof error==='object'||typeof error==='function')?{name:error.name}:{name:'ThrownValue',raw:error};}
        finally{restoreLoad?.();setConsoleFunction(oldConsole);}
        result.events=events;return result;
    }
    return {run,metadata,dispose(){if(oldImage)Object.defineProperty(globalThis,'HTMLImageElement',oldImage);else delete globalThis.HTMLImageElement;}};
}

if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
    const [upstream,arg]=process.argv.slice(2),oracle=await createOracle(upstream);
    try{const input=JSON.parse(arg??fs.readFileSync(0,'utf8'));let result;if(Array.isArray(input)){result=[];for(const request of input)result.push(await oracle.run(request));}else result=await oracle.run(input);process.stdout.write(JSON.stringify(result)+'\n');}finally{oracle.dispose();}
}
