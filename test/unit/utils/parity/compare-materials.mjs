import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const T=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const names=['Material','LineBasicMaterial','LineDashedMaterial','MeshBasicMaterial','MeshDepthMaterial','MeshDistanceMaterial','MeshLambertMaterial','MeshMatcapMaterial','MeshNormalMaterial','MeshPhongMaterial','MeshStandardMaterial','MeshPhysicalMaterial','MeshToonMaterial','PointsMaterial','ShadowMaterial','SpriteMaterial','ShaderMaterial','RawShaderMaterial'];
function encode(value){
    if(value===undefined)return '$undefined';
    if(typeof value==='number')return Number.isNaN(value)?'$NaN':value===Infinity?'$Infinity':value===-Infinity?'$-Infinity':Object.is(value,-0)?'$-0':value;
    if(value===null||typeof value!=='object')return value;
    if(value.isTexture)return {texture:value.uuid};
    for(const flag of ['isColor','isVector2','isVector3','isVector4','isMatrix3','isMatrix4','isQuaternion'])if(value[flag])return {kind:flag,components:value.toArray()};
    if(value.isEuler)return value.toArray();
    if(value.isPlane)return value.toJSON();
    if(Array.isArray(value))return Array.from(value,encode);
    return Object.fromEntries(Object.keys(value).filter(k=>typeof value[k]!=='function').map(k=>[k,encode(value[k])]));
}
function snapshot(material){
    const fields={};
    for(const key of Object.keys(material))if(key!=='uuid'&&key!=='_listeners')fields[key]=encode(material[key]);
    fields.alphaTest=encode(material.alphaTest);
    if(material.isMeshPhysicalMaterial)for(const key of ['anisotropy','clearcoat','diffuseRoughness','dispersion','iridescence','retroreflectivity','sheen','transmission','reflectivity'])fields[key]=encode(material[key]);
    const saved=material.uuid;material.uuid='material';const json=material.toJSON();material.uuid=saved;
    return {fields,json:JSON.parse(JSON.stringify(json))};
}
function run(r){
    if(r.runtime==='physical'){
        const m=new T.MeshPhysicalMaterial(),versions=[];
        for(const k of ['anisotropy','clearcoat','diffuseRoughness','dispersion','iridescence','retroreflectivity','sheen','transmission','alphaTest'])for(const v of [0,.4,.6,0,-1,1]){m[k]=v;versions.push(m.version);}
        m.reflectivity=.8;const d=Object.getOwnPropertyDescriptor(m,'reflectivity');
        return {versions,ior:m.ior,reflectivity:m.reflectivity,own:Object.hasOwn(m,'reflectivity'),enumerable:d.enumerable,configurable:d.configurable,alphaOwn:Object.hasOwn(m,'alphaTest'),backingOwn:Object.hasOwn(m,'_alphaTest'),same:true};
    }
    if(r.runtime==='setValues'){
        const m=new T.MeshPhysicalMaterial(),color=m.color,normal=m.normalScale,rotation=m.envMapRotation;
        m.setValues({color:0x123456,normalScale:new T.Vector2(3,4),envMapRotation:new T.Euler(.2,.3,.4),needsUpdate:true,clearcoat:.5,opacity:.2});
        const before=m.version;m.needsUpdate=true;m.needsUpdate=1;m.onBeforeCompile=()=>{};m.onBeforeCompile.toString=()=>'custom program';
        return {colorSame:color===m.color,vectorSame:normal===m.normalScale,eulerSame:rotation===m.envMapRotation,color:m.color.getHex(),normal:m.normalScale.toArray(),rotation:m.envMapRotation.toArray().slice(0,3),versionBeforeDirect:before,version:m.version,key:m.customProgramCacheKey()};
    }
    if(r.runtime==='extra'){
        const m=new T.Material().fromJSON({roughness:.3},{}),shared={value:4},trace=[];m.custom=shared;m.roughness=.7;
        Object.defineProperty(m,'getter',{enumerable:true,configurable:true,get(){trace.push(this.roughness);return this.roughness;}});const got=m.getter,removed=delete m.roughness;
        return {same:m.custom===shared,got,trace,removed,gone:!('roughness'in m),descriptor:Object.getOwnPropertyDescriptor(m,'getter').enumerable};
    }
    if(r.runtime==='uniforms'){
        const m=new T.ShaderMaterial(),value={x:2},uniforms={value:{value}};m.uniforms=uniforms;const clone=m.clone();
        return {same:m.uniforms===uniforms,nativeSame:true,cloneDifferent:clone.uniforms!==uniforms,ordinaryShared:clone.uniforms.value.value===value};
    }
    const m=new T[r.type]();if(r.parameters)m.setValues(r.parameters);
    const texture=new T.Texture();texture.uuid='texture';texture.source.uuid='image';const textures={texture};
    if(r.textures)for(const key of Object.keys(m))if(key==='map'||key==='matcap'||key.endsWith('Map'))m[key]=texture;
    if(r.reentrantTexture)texture.toJSON=function(meta){m.opacity=.3;m.clearcoat=.9;m.dispersion=.4;m.sheen=.2;m.clearcoatNormalScale.set(2,3);m.envMapIntensity=7;m.userData.callback=true;m.fog=false;return T.Texture.prototype.toJSON.call(this,meta);};
    if(r.load)m.fromJSON(r.load,textures);
    const versions=[];for(const step of r.toggles??[]){m.setValues(step);versions.push(encode(m.version));}
    if(r.uniformTypes){
        const add=(name,value)=>m.uniforms[name]={value};
        add('texture',texture);add('color',new T.Color(0x123456));add('vector',new T.Vector2(3,4));
        const nested={a:2};add('object',nested);const a=new Array(3);a[1]=nested;add('array',a);
        add('vectors',[new T.Vector2(7,8),new T.Vector2(9,10)]);
    }
    const out={source:snapshot(m),versions};const clone=m.clone(),target=new T[r.type]();
    target.name='target';target.version=100;target.alphaTest=.7;target.copy(m);
    if(r.uniformTypes)for(const c of [clone,target])c.uniforms.texture.value.uuid='texture-clone';
    out.clone=snapshot(clone);out.copy=snapshot(target);
    const identity={uuidDifferent:clone.uuid!==m.uuid,idDifferent:clone.id!==m.id};
    for(const key of Object.keys(m))if(key==='map'||key==='matcap'||key.endsWith('Map'))identity[key]=clone[key]===m[key];
    if(m.isShaderMaterial){
        identity.uniformsDifferent=m.uniforms!==clone.uniforms;identity.definesDifferent=m.defines!==clone.defines;
        identity.attributeArrayShared=m.defaultAttributeValues.color===clone.defaultAttributeValues.color;
        if(r.uniformTypes)for(const key of ['texture','color','vector','object','array','vectors'])identity['uniform:'+key]=m.uniforms[key].value===clone.uniforms[key].value;
    }
    if(m.isMeshPhysicalMaterial)identity.iridescenceRangeDifferent=m.iridescenceThicknessRange!==clone.iridescenceThicknessRange;
    out.identity=identity;return out;
}
const cases=[];
for(const runtime of ['physical','setValues','extra','uniforms'])cases.push({runtime});
for(const type of names){
    cases.push({type},{type,textures:true});
    const defaults=new T[type]();
    for(let seed=0;seed<8;++seed){
        const parameters={};
        for(const [key,value]of Object.entries(defaults)){
            if(key==='uuid'||key==='id'||key==='version'||key[0]==='_'||key.startsWith('is'))continue;
            if(typeof value==='number')parameters[key]=(seed+key.length)%7-2;
            else if(typeof value==='boolean')parameters[key]=(seed+key.length)%2===0;
            else if(typeof value==='string'&&key!=='type')parameters[key]='value-'+seed;
            else if(value?.isColor)parameters[key]=seed%2?'#12ab34':0x437ea1;
        }
        parameters.name=type+'-'+seed;parameters.alphaTest=(seed%3)*.2;
        parameters.userData={seed,nested:{array:[1,null,true,'x']}};
        if('defines'in defaults)parameters.defines={CUSTOM:seed};
        if(defaults.isShaderMaterial){parameters.extensions={multiDraw:seed%2===0,custom:true,falseValue:1};parameters.uniforms={scalar:{value:seed},array:{value:[1,2,3]}};}
        cases.push({type,parameters,textures:seed%2===0});
    }
    cases.push({type,load:{name:'loaded',color:0x123456,blending:5,blendSrcAlpha:201,blendColor:0xaabbcc,opacity:.2,transparent:true,alphaTest:.4,vertexColors:2,clipIntersection:true,clippingPlanes:[{normal:[1,2,3],constant:4}],userData:{loaded:true}}});
}
for(const type of ['MeshLambertMaterial','MeshPhongMaterial','MeshStandardMaterial','MeshPhysicalMaterial','MeshToonMaterial','MeshNormalMaterial','MeshMatcapMaterial'])
    cases.push({type,load:{normalMap:'texture',normalScale:.25,normalMapType:1,bumpMap:'texture',bumpScale:2,displacementMap:'texture',displacementScale:3,displacementBias:-2,aoMap:'missing'}});
cases.push({type:'MeshPhysicalMaterial',parameters:{anisotropy:.4,clearcoat:.9,sheen:.3,transmission:.7,diffuseRoughness:1,dispersion:.1,iridescence:.2,retroreflectivity:.6,reflectivity:.8},textures:true});
cases.push({type:'MeshPhysicalMaterial',toggles:[...['anisotropy','clearcoat','diffuseRoughness','dispersion','iridescence','retroreflectivity','sheen','transmission','alphaTest'].flatMap(k=>[0,.2,.3,0,-1,0,1].map(v=>({[k]:v}))),{needsUpdate:1},{needsUpdate:false},{needsUpdate:true},{needsUpdate:'true'}]});
cases.push({type:'Material',parameters:{version:9007199254740992},toggles:[{needsUpdate:true},{alphaTest:1},{alphaTest:0}]});
cases.push({type:'MeshPhysicalMaterial',textures:true,reentrantTexture:true});
for(const type of ['Material','MeshBasicMaterial','MeshPhysicalMaterial'])cases.push({type,load:{color:'red',blendColor:'255',sheenColor:23,roughness:.5,map:123},textures:true});
for(const type of ['ShaderMaterial','RawShaderMaterial']){
    cases.push({type,uniformTypes:true});
    cases.push({type,parameters:{linewidth:4},load:{uniforms:{scalar:{value:4},color:{type:'c',value:0xff00ff},vector:{type:'v2',value:[2,3]},v3:{type:'v3',value:[4,5,6]},v4:{type:'v4',value:[7,8,9,10]},m3:{type:'m3',value:[1,2,3,4,5,6,7,8,9]},m4:{type:'m4',value:[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]},missing:{type:'t',value:'missing'}},extensions:{newExtension:true},lights:true,clipping:true,glslVersion:'300 es'}});
    for(const value of ['ab',true,42])cases.push({type,parameters:{uniforms:value,defines:value,extensions:value,defaultAttributeValues:value,index0AttributeName:'position'}});
}
const input=JSON.stringify(cases),expected=cases.map(run);
const actual=JSON.parse(execFileSync(binary,[],{input,encoding:'utf8',maxBuffer:64*1024*1024}));
assert.equal(actual.length,expected.length);
let comparisons=0;
function compare(a,b,p){
    if(b!==null&&typeof b==='object'){
        assert.equal(Array.isArray(a),Array.isArray(b),p);assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),p+' keys');
        for(const k of Object.keys(b))compare(a[k],b[k],p+'.'+k);
    }else{assert.deepEqual(a,b,p);++comparisons;}
}
for(let i=0;i<cases.length;++i)compare(actual[i],expected[i],JSON.stringify(cases[i]));
console.log(`Material parity passed: ${cases.length} scenarios, ${comparisons} exact comparisons`);
