import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync, spawnSync } from 'node:child_process';
import { registerHooks } from 'node:module';
import { pathToFileURL } from 'node:url';

if (process.argv.length !== 4) throw Error('Usage: node compare-vertex-helpers.mjs <three.js> <probe.exe>');
const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
// Load the actual addon files with their bare "three" imports bound to the
// pinned source modules, avoiding any difference in generated build artifacts.
registerHooks({ resolve(specifier, context, nextResolve) {
  if (specifier === 'three') return { url: pathToFileURL(path.join(upstream, 'src/Three.Core.js')).href, shortCircuit: true };
  return nextResolve(specifier, context);
} });
const T = await import(pathToFileURL(path.join(upstream, 'src/Three.Core.js')));
const { VertexNormalsHelper } = await import(pathToFileURL(path.join(upstream, 'examples/jsm/helpers/VertexNormalsHelper.js')));
const { VertexTangentsHelper } = await import(pathToFileURL(path.join(upstream, 'examples/jsm/helpers/VertexTangentsHelper.js')));

function number(x) {
  if (Number.isNaN(x)) return 'NaN';
  if (!Number.isFinite(x)) return x < 0 ? '-Infinity' : 'Infinity';
  if (Object.is(x, -0)) return '-0';
  return x;
}
const values = array => Array.from(array, number);
function primitive(value) {
  if (value === undefined) return { type: 'undefined' };
  if (value === null) return { type: 'null' };
  if (typeof value === 'number') return { type: 'number', value: number(value) };
  if (typeof value === 'boolean') return { type: 'boolean', value };
  if (typeof value === 'string') return { type: 'string', value };
  return { type: 'object' };
}
function attribute(v) {
  if (!v) return null;
  return { array: values(v.array), count: number(v.count), itemSize: number(v.itemSize),
    version: v.version === undefined ? 'undefined' : number(v.version),
    dataVersion: v.data === undefined ? 'undefined' : number(v.data.version) };
}
function state(h) {
  const m = h.material;
  return { type: h.type, isLine: h.isLine, isLineSegments: h.isLineSegments,
    matrixAutoUpdate: h.matrixAutoUpdate, matrixWorldNeedsUpdate: h.matrixWorldNeedsUpdate,
    matrix: values(h.matrix.elements), matrixWorld: values(h.matrixWorld.elements), size: primitive(h.size),
    position: h.geometry ? attribute(h.geometry.attributes.position) : null,
    hasBoundingSphere: !!h.geometry?.boundingSphere,
    sphere: h.geometry?.boundingSphere ? [...values(h.geometry.boundingSphere.center.toArray()),number(h.geometry.boundingSphere.radius)] : null,
    material: m ? { type: m.type, color: values(m.color.toArray()), toneMapped: m.toneMapped,
      depthTest: m.depthTest, depthWrite: m.depthWrite, vertexColors: m.vertexColors } : null,
    flag: h.isVertexNormalsHelper ?? 'undefined' };
}
function sourceGeometry(mode) {
  const g = new T.BufferGeometry();
  if (mode === 11) {
    g.setAttribute('position', new T.Float32BufferAttribute(0, 3));
    g.setAttribute('normal', new T.Float32BufferAttribute(0, 3));
    g.setAttribute('tangent', new T.Float32BufferAttribute(0, 4));
  } else if (mode === 12) {
    g.setAttribute('position', new T.Int8BufferAttribute([0,0,0,127,64,-127,-64,32,-16],3,true));
    g.setAttribute('normal', new T.Int8BufferAttribute([127,127,127,0,127,0,0,0,0],3,true));
    g.setAttribute('tangent', new T.Int8BufferAttribute([127,127,127,-127,0,127,0,127,0,0,0,-127],4,true));
  } else if (mode === 13) {
    const data = new T.InterleavedBuffer(new Float32Array([0,0,0,1,1,1,1,1,1,-1, 1,2,3,0,1,0,0,1,0,1, -2,1,-1,0,0,0,0,0,0,-1]),10);
    g.setAttribute('position',new T.InterleavedBufferAttribute(data,3,0));
    g.setAttribute('normal',new T.InterleavedBufferAttribute(data,3,3));
    g.setAttribute('tangent',new T.InterleavedBufferAttribute(data,4,6));
  } else if (mode === 14) {
    g.setAttribute('position',new T.Float16BufferAttribute([0,0,0,0x3c00,0x4000,0x4200,0xc000,0x3c00,0xbc00],3));
    g.setAttribute('normal',new T.Float16BufferAttribute([0x3c00,0x3c00,0x3c00,0,0x3c00,0,0,0,0],3));
    g.setAttribute('tangent',new T.Float16BufferAttribute([0x3c00,0x3c00,0x3c00,0xbc00,0,0x3c00,0,0x3c00,0,0,0,0xbc00],4));
  } else {
    g.setAttribute('position',new T.Float32BufferAttribute([0,0,0,1,2,3,-2,1,-1],3));
    g.setAttribute('normal',new T.Float32BufferAttribute([1,1,1,0,1,0,0,0,0],3));
    g.setAttribute('tangent',new T.Float32BufferAttribute([1,1,1,-1,0,1,0,1,0,0,0,-1],4));
  }
  return g;
}
class ProbeMesh extends T.Mesh {
  updates = 0;
  after;
  updateMatrixWorld(force) {
    this.updates++;
    super.updateMatrixWorld(force);
    this.after?.();
  }
}
function scenario(kind,mode) {
  const H = kind === 0 ? VertexNormalsHelper : VertexTangentsHelper;
  const target = kind === 0 ? 'normal' : 'tangent';
  const g = sourceGeometry(mode), original = new ProbeMesh(g);
  let source = original, parent, helper, size = 1, color = kind === 0 ? 0xff0000 : 0x00ffff;
  let colorObject, counter;
  const details = {}, events = [];
  let error = '';
  if (mode === 1 || mode === 2) {
    original.position.set(1,2,-3); original.rotation.set(.2,-.3,.4); original.scale.set(mode === 2 ? -2 : 2,.5,3);
    parent = new T.Object3D(); parent.position.set(3,-1,2); parent.rotation.set(.1,.2,-.4); parent.scale.set(1,2,3);
    parent.add(original); parent.updateMatrixWorld();
  }
  if (mode === 3) original.scale.set(0,2,0);
  if (mode === 4) {
    original.matrixAutoUpdate=false; original.matrixWorldAutoUpdate=false;
    original.matrixWorld.set(2,1,0,3,0,-2,1,4,1,0,.5,-1,.1,-.2,.3,1);
  }
  if (mode === 5) { original.matrixAutoUpdate=false; original.matrix.makeTranslation(3,4,5); }
  if (mode === 6) { parent=new T.Object3D(); parent.position.set(7,8,9); parent.add(original); original.position.set(1,2,3); }
  if (mode === 8) { g.attributes.normal.setXYZ(0,0,0,0); g.attributes.tangent.setXYZ(0,0,0,0); }
  if (mode === 9) g.attributes[target].setX(0,NaN);
  if (mode === 10) size=Infinity;
  if ([15,16,17,45,46,59].includes(mode)) g.attributes[target].count = mode===15?5:mode===16?1:mode===17?1.5:mode===45?-1:mode===59?0:NaN;
  if (mode === 26) size='2';
  if (mode === 27) size=false;
  if (mode === 29) size=NaN;
  if (mode === 38) source=new T.Line(g);
  if (mode === 39) source=new T.Points(g);
  if (mode === 40) { source=new T.Sprite(); source.geometry=g; }
  if (mode === 41) g.deleteAttribute(target);
  if (mode === 42) original.geometry=null;
  if (mode === 43) source=undefined;
  if (mode === 44) { size=undefined; color=undefined; }
  if (mode === 47 || mode === 48) {
    counter={count:0,throw:mode===48};
    size={valueOf(){ counter.count++; if(counter.throw && counter.count===2) throw new TypeError('scalar conversion'); return counter.count; }};
  }
  if (mode === 49) color='#ff8000';
  if (mode === 50) { colorObject=new T.Color(.2,.3,.4); color=colorObject; }
  if (mode === 51) color=null;
  if (mode === 52) color=0xffcc00;
  if (mode === 53) size=null;
  if (mode === 54) g.setAttribute('position',new T.BufferAttribute(new Float64Array([.123456789,0,-0,1.123456789,2,3,-2,1,-1]),3));
  if (mode === 57) { g.setIndex([2,1,0]); g.setDrawRange(1,0); }
  if (mode === 58) g.attributes.position.count=2.5;
  try {
    helper=new H(source,size,color);
    details.sameSource=helper.object===source;
    if (mode === 7) { source.position.set(-2,3,4); g.attributes[target].setXYZ(0,2,3,4); helper.size=-2; helper.update(); }
    if (mode === 18) { g.deleteAttribute('position'); helper.update(); }
    if (mode === 19) { g.deleteAttribute(target); helper.update(); }
    if (mode === 20) { original.geometry=null; helper.update(); }
    if (mode === 21) { helper.object=undefined; helper.update(); }
    if (mode === 22) { helper.geometry=null; helper.update(); }
    if (mode === 23) { helper.geometry.deleteAttribute('position'); helper.update(); }
    if (mode === 24) { helper.geometry.setAttribute('position',new T.Float32BufferAttribute(6,3)); helper.update(); }
    if (mode === 25) {
      const data=new T.InterleavedBuffer(new Float32Array(24).fill(7),4);
      helper.geometry.setAttribute('position',new T.InterleavedBufferAttribute(data,3,1)); helper.update();
    }
    if (mode === 28) { helper.size=undefined; helper.update(); }
    if (mode === 30) {
      const next=new ProbeMesh(sourceGeometry(0)); next.position.set(9,8,7);
      const other=new H(next,4);
      details.copyReturnedSelf=helper.copy(other,false)===helper;
      details.sameGeometry=helper.geometry===other.geometry;
      details.sameMaterial=helper.material===other.material;
      details.sameSourceAfterCopy=helper.object===source;
    }
    if (mode === 31 || mode === 32) helper.clone(mode===31);
    if (mode >= 33 && mode <= 36) {
      helper.addEventListener('dispose',()=>events.push('helper'));
      helper.geometry.addEventListener('dispose',()=>events.push('geometry'));
      helper.material.addEventListener('dispose',()=>events.push('material'));
      if (mode === 35) {
        const replacement=new T.LineBasicMaterial();
        replacement.addEventListener('dispose',()=>events.push('replacement'));
        helper.geometry.addEventListener('dispose',()=>helper.material=replacement);
      }
      if (mode === 36) helper.geometry.addEventListener('dispose',()=>{throw Error('dispose');});
      helper.dispose(); if (mode === 34) helper.dispose();
    }
    if (mode === 37) {
      const next=new ProbeMesh(sourceGeometry(0)); next.matrixWorld.makeTranslation(7,8,9);
      original.after=()=>helper.object=next; helper.update();
      details.replacedSource=helper.object===next; original.after=undefined;
    }
    if (mode === 50) { colorObject.setRGB(.7,.8,.9); helper.update(); }
    if (mode === 55) { g.setAttribute('position',new T.Float32BufferAttribute(0,3)); g.deleteAttribute(target); helper.update(); }
    if (mode === 56) { helper.geometry.computeBoundingSphere(); helper.size=4; helper.update(); }
  } catch (e) { error=e.name; }
  if (counter) details.conversions=counter.count;
  details.events=events;
  return { kind,mode,error,helper:helper?state(helper):null,sourceWorld:source?values(source.matrixWorld.elements):null,
    sourceUpdates:source===original?original.updates:0,details };
}
const expected=[];
for(let kind=0;kind<2;kind++) for(let mode=0;mode<60;mode++) expected.push(scenario(kind,mode));
const run=spawnSync(process.argv[3],[],{encoding:'utf8',maxBuffer:32*1024*1024});
assert.equal(run.status,0,run.stderr || String(run.error));
assert.equal(run.stderr,'');
const actual=JSON.parse(run.stdout);
assert.equal(actual.length,expected.length);
let comparisons=0;
function compare(a,b,where) {
  if(b && typeof b==='object') {
    assert.equal(Array.isArray(a),Array.isArray(b),where);
    assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),where);
    for(const k of Object.keys(b)) compare(a[k],b[k],`${where}.${k}`);
  } else {
    if(typeof a==='number' && typeof b==='number') assert.ok(Math.abs(a-b)<=1e-12*Math.max(1,Math.abs(a),Math.abs(b)),`${where}: ${a} != ${b}`);
    else assert.deepEqual(a,b,where);
    comparisons++;
  }
}
const failures=[];
for(let i=0;i<expected.length;i++) {
  try { compare(actual[i],expected[i],`${expected[i].kind}:${expected[i].mode}`); }
  catch(e) { failures.push(e.message); }
}
assert.equal(failures.length,0,failures.join('\n'));
console.log(`Vertex helpers parity: ${expected.length} scenarios / ${comparisons} comparisons; ${lock.commit}`);
