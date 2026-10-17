// Differential check for the native image loading family: ImageLoader, TextureLoader and
// CubeTextureLoader. The oracle runs the pinned modules with a stubbed document and a fake image, the
// way the file loader comparison stubs ProgressEvent; both sides synthesise the same pixels from the
// url, so what is compared is the wrapper each loader builds.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-texture-loader.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const hashOf=url=>{let hash=0;for(const code of Buffer.from(url,'utf8'))hash=(hash*31+code)&0xff;return hash;};
const widthOf=url=>2+(hashOf(url)&3);
const heightOf=url=>1+((hashOf(url)>>2)&3);
// A fake image element: setting src fires the load listener synchronously, which is how the pinned
// loader is driven without a browser.
function makeImage(){
    const listeners={load:[],error:[]};
    const image={
        width:undefined,height:undefined,complete:false,
        addEventListener:name=>{if(listeners[name])listeners[name].push(true);},
        removeEventListener:()=>{},
        set src(value){
            this._src=value;
            this.width=widthOf(value);
            this.height=heightOf(value);
            this.complete=true;
            this.__onSrc&&this.__onSrc();
        },
        get src(){return this._src;}
    };
    return image;
}
// createElementNS must return an image whose load listener fires when src is set; the loader registers
// the listener before assigning src, so the stub records the callback and replays it from the setter.
let pendingLoad=null;
globalThis.document={createElementNS:(ns,name)=>{
    const image=makeImage();
    image.addEventListener=(event,fn)=>{if(event==='load')pendingLoad=fn;};
    image.__onSrc=()=>{const fn=pendingLoad;pendingLoad=null;if(fn)fn.call(image);};
    return image;
}};
const {TextureLoader}=await import(pathToFileURL(path.join(sourceDir,'src/loaders/TextureLoader.js')).href);
const {CubeTextureLoader}=await import(pathToFileURL(path.join(sourceDir,'src/loaders/CubeTextureLoader.js')).href);
const scenarios=[
    'tex|tex-a.png',
    'tex|tex-longer-name.png',
    'cube|6|px.png|nx.png|py.png|ny.png|pz.png|nz.png',
    'cube|1|solo.png',
    'cube|4|a.png|b.png|c.png|d.png'
];
const describeTexture=texture=>({
    width:texture.image===undefined?undefined:texture.image.width,
    height:texture.image===undefined?undefined:texture.image.height,
    hasImage:texture.image!==undefined&&texture.image!==null,
    version:texture.version,
    colorSpace:texture.colorSpace
});
const lines=scenarios.map(entry=>{
    const [mode,countOrUrl,...rest]=entry.split('|');
    return mode==='tex'?`tex ${countOrUrl}`:`cube ${countOrUrl} ${rest.join(' ')}`;
});
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
for(let index=0;index<scenarios.length;++index){
    const parts=scenarios[index].split('|');
    let expected;
    if(parts[0]==='tex'){
        const texture=new TextureLoader().load(parts[1]);
        expected=describeTexture(texture);
    }else{
        const count=Number(parts[1]);
        const urls=parts.slice(2,2+count);
        const texture=new CubeTextureLoader().load(urls);
        expected={version:texture.version,colorSpace:texture.colorSpace};
    }
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH scenario',index,scenarios[index]);
        console.log('  probe :',JSON.stringify(actual).slice(0,240));
        console.log('  oracle:',JSON.stringify(expected).slice(0,240));
    }
    assert.deepEqual(actual,expected,`scenario ${index}`);
}
console.log(`image loader parity: ${scenarios.length} scenarios`);