// Differential check for the native src/extras/Controls.js port.
//
// The upstream class is imported from the pinned three.js checkout, the same observable
// state is produced there, and the probe output is compared field by field: constructor
// defaults, the input maps, the connect() sequence with a counting disconnect override,
// and the inherited EventDispatcher base.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-controls.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Controls}=await import(pathToFileURL(path.join(sourceDir,'src/extras/Controls.js')).href);
const {EventDispatcher}=await import(pathToFileURL(path.join(sourceDir,'src/core/EventDispatcher.js')).href);
const {Object3D}=await import(pathToFileURL(path.join(sourceDir,'src/core/Object3D.js')).href);

class ProbeControls extends Controls {
    constructor(object, domElement){
        super(object, domElement);
        this.disconnects=0;
    }
    disconnect(){this.disconnects+=1;}
}
const tag=controls=>controls.domElement===null?'null':controls.domElement===undefined?'undefined':String(controls.domElement);
const mapValues=map=>Object.fromEntries(Object.keys(map).map(key=>[key,map[key]===null?null:String(map[key])]));
const step=(controls,name)=>({step:name,disconnectCalls:controls.disconnects,domElement:tag(controls)});

const object=new Object3D();
const defaults=new Controls(object);
const sequence=new ProbeControls(object);
const preset=new ProbeControls(object,'initial');

const expected={
    defaults:{
        objectIsObject3D:defaults.object instanceof Object3D,
        domElement:tag(defaults),
        enabled:defaults.enabled,
        state:defaults.state,
        keys:mapValues(defaults.keys),
        mouseButtons:mapValues(defaults.mouseButtons),
        touches:mapValues(defaults.touches),
        isEventDispatcher:defaults instanceof EventDispatcher
    },
    sequence:[step(sequence,'construct')],
    preset:[step(preset,'construct with element')],
    updateReturnsUndefined:true
};
sequence.connect('first');expected.sequence.push(step(sequence,'connect first'));
sequence.connect('second');expected.sequence.push(step(sequence,'connect second'));
sequence.connect(undefined);expected.sequence.push(step(sequence,'connect undefined'));
sequence.connect(null);expected.sequence.push(step(sequence,'connect null'));
sequence.connect('third');expected.sequence.push(step(sequence,'connect third'));
preset.connect('next');expected.preset.push(step(preset,'connect next'));
const plain=new Controls(object);
assert.equal(plain.update(0.25),undefined,'update() returns undefined upstream');
assert.equal(plain.dispose(),undefined,'dispose() returns undefined upstream');
assert.equal(defaults.connect('element'),undefined,'connect() returns undefined upstream');

const output=execFileSync(path.resolve(probePath),[],{encoding:'utf8'}).trim();
assert.deepEqual(JSON.parse(output),expected,'Controls state parity');
console.log(`Controls parity: constructor defaults, ${expected.sequence.length + expected.preset.length} connect steps `
    +`and the EventDispatcher base match the pinned checkout; ${process.version}`);
