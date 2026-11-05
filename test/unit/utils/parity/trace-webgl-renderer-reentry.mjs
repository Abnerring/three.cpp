// CPU protocol oracle. Original pinned renderer/resource/state code executes;
// GL compilation/reflection and device commands are explicit recording boundaries.
// This does not execute a GLSL compiler or claim GPU image equivalence.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import {pathToFileURL} from 'node:url';
const [upstreamArg, outputArg] = process.argv.slice(2);
assert(upstreamArg, 'Usage: trace-webgl-renderer-reentry.mjs <pinned> [fresh-output-dir]');
const upstream = path.resolve(upstreamArg);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding: 'utf8'}).trim(), lock.commit);
const imports = {};
for (const file of ['constants.js', 'utils.js', 'scenes/Scene.js', 'objects/Mesh.js', 'cameras/OrthographicCamera.js', 'geometries/PlaneGeometry.js', 'core/BufferGeometry.js', 'core/BufferAttribute.js', 'textures/DataTexture.js', 'math/Matrix4.js', 'math/Vector3.js', 'math/Vector4.js', 'math/Frustum.js', 'math/FrustumArray.js', 'math/Color.js', 'math/ColorManagement.js', 'materials/MeshBasicMaterial.js', 'renderers/WebGLRenderTarget.js', 'renderers/shaders/DFGLUTData.js', ...['WebGLProperties', 'WebGLState', 'WebGLInfo', 'WebGLCapabilities', 'WebGLUtils', 'WebGLAttributes', 'WebGLGeometries', 'WebGLObjects', 'WebGLBindingStates', 'WebGLRenderLists', 'WebGLRenderStates', 'WebGLClipping', 'WebGLPrograms', 'WebGLMaterials', 'WebGLTextures', 'WebGLBackground', 'WebGLUniforms', 'WebGLBufferRenderer', 'WebGLIndexedBufferRenderer'].map(x => `renderers/webgl/${x}.js`)])
    Object.assign(imports, await import(pathToFileURL(path.join(upstream, 'src', file)).href));
const {parse} = createRequire(path.join(upstream, 'package.json'))('acorn');
const source = fs.readFileSync(path.join(upstream, 'src/renderers/WebGLRenderer.js'), 'utf8');
const body = parse(source, {sourceType: 'module', ecmaVersion: 'latest'}).body.find(n => n.type === 'ClassDeclaration' && n.id.name === 'WebGLRenderer').body.body.find(n => n.kind === 'constructor').value.body.body;
const names = ['projectObject', 'renderScene', 'renderTransmissionPass', 'renderObjects', 'renderObject', 'getTargetPixelRatio', 'getProgram', 'getProgramVariant', 'getUniformList', 'updateCommonMaterialProperties', 'findLightProbeGrid', 'setProgram', 'markUniformsLightsNeedsUpdate', 'materialNeedsLights', 'onMaterialDispose', 'deallocateMaterial', 'releaseMaterialProgramReferences'];
const functions = names.map(name => body.find(n => n.type === 'FunctionDeclaration' && n.id.name === name));
const assigned = name => body.find(n => n.type === 'ExpressionStatement' && n.expression.type === 'AssignmentExpression' && n.expression.left.type === 'MemberExpression' && n.expression.left.object.type === 'ThisExpression' && n.expression.left.property.name === name).expression.right;
const factory = new Function('services', 'imports', `"use strict";
const {Matrix4,Vector3,Vector4,Frustum,FrustumArray,Color,ColorManagement,WebGLUniforms,WebGLRenderTarget,WebGLCoordinateSystem,DoubleSide,BackSide,FrontSide,HalfFloatType,UnsignedByteType,LinearMipmapLinearFilter,NoToneMapping,error,getDFGLUT}=imports;
const {_this,_gl,properties,programCache,environments,materials,morphtargets,uniformsGroups,capabilities,clipping,textures,state,objects,renderLists,renderStates,background,shadowMap,xr,bindingStates,extensions,geometries,attributes,bufferRenderer,indexedBufferRenderer}=services;
let currentRenderList=null,currentRenderState=null,output=null,_isContextLost=false,_nodesHandler=null,_currentActiveCubeFace=0,_currentActiveMipmapLevel=0,_currentRenderTarget=null,_currentMaterialId=-1,_currentCamera=null,_currentClearAlpha=0,_opaqueSort=null,_transparentSort=null,_clippingEnabled=false,_localClippingEnabled=false,_pixelRatio=1,_height=64,_renderBackground=false;
const renderListStack=[],renderStateStack=[],_currentViewport=new Vector4(0,0,64,64),_currentClearColor=new Color(0),_frustum=new Frustum(),_frustumArray=new FrustumArray(),_projScreenMatrix=new Matrix4(),_vector3=new Vector3(),_vector4=new Vector4(),objectPosition=new Vector3(),_emptyScene={isScene:true,background:null,fog:null,environment:null,overrideMaterial:null},stencil=false;
${functions.map(n => source.slice(n.start, n.end)).join('\n')}
const render=${source.slice(assigned('render').start, assigned('render').end)};
const renderBufferDirect=${source.slice(assigned('renderBufferDirect').start, assigned('renderBufferDirect').end)};
return {render,renderBufferDirect};`);

function recorder() {
    let serial = 0, token = 0x100000, current = null;
    const events = [], draws = [], errors = [], values = new Map();
    const gl = new Proxy({
        FLOAT: 0x1406, INT: 0x1404, UNSIGNED_INT: 0x1405, UNSIGNED_SHORT: 0x1403, UNSIGNED_BYTE: 0x1401,
        FLOAT_VEC3: 0x8b51, FLOAT_MAT4: 0x8b5c, FLOAT_MAT3: 0x8b5b, BOOL: 0x8b56, SAMPLER_2D: 0x8b5e,
        VERTEX_SHADER: 0x8b31, FRAGMENT_SHADER: 0x8b30, TRIANGLES: 4, LINES: 1, POINTS: 0,
        createShader(type) { return {id: ++serial, type}; }, shaderSource(shader, text) { shader.source = text; },
        compileShader(shader) { events.push(['compile-boundary', shader.id]); }, getShaderParameter() { return true; }, getShaderInfoLog() { return ''; },
        createProgram() { const p = {id: ++serial, shaders: [], uniforms: {}, infos: []}; values.set(p.id, p); return p; },
        attachShader(p, s) { p.shaders.push(s); }, detachShader() {}, deleteShader() {}, deleteProgram() {}, bindAttribLocation() {},
        linkProgram(p) {
            const mapped = p.shaders.some(s => /^#define USE_MAP$/m.test(s.source));
            p.infos = [['diffuse', gl.FLOAT_VEC3], ['opacity', gl.FLOAT], ['projectionMatrix', gl.FLOAT_MAT4], ['viewMatrix', gl.FLOAT_MAT4], ['modelViewMatrix', gl.FLOAT_MAT4], ['normalMatrix', gl.FLOAT_MAT3], ['modelMatrix', gl.FLOAT_MAT4], ['isOrthographic', gl.BOOL]];
            if (mapped) p.infos.push(['map', gl.SAMPLER_2D], ['mapTransform', gl.FLOAT_MAT3]);
            events.push(['link-reflection-boundary', p.id, mapped, p.infos.map(x => x[0])]);
        },
        getProgramParameter(p, name) {
            if (name === gl.ACTIVE_UNIFORMS) return p.infos.length;
            if (name === gl.ACTIVE_ATTRIBUTES) return 1;
            if (name === gl.LINK_STATUS || name === gl.VALIDATE_STATUS) return true;
            throw new Error('Unknown program query ' + name);
        },
        getProgramInfoLog() { return ''; }, getActiveUniform(p, i) { const [name, type] = p.infos[i]; return {name, type, size: 1}; },
        getUniformLocation(p, name) { return {program: p, name}; }, getActiveAttrib() { return {name: 'position', type: gl.FLOAT_VEC3, size: 1}; }, getAttribLocation() { return 0; },
        useProgram(p) { current = p; events.push(['useProgram', p?.id ?? null]); },
        getShaderPrecisionFormat() { return {rangeMin: 127, rangeMax: 127, precision: 23}; }, getExtension() { return null; },
        getParameter(name) {
            if (name === gl.VERSION) return 'WebGL 2.0 pinned CPU protocol recorder';
            if (name === gl.SCISSOR_BOX || name === gl.VIEWPORT) return [0, 0, 64, 64];
            if (name === gl.MAX_TEXTURE_SIZE || name === gl.MAX_CUBE_MAP_TEXTURE_SIZE) return 4096;
            if (name === gl.MAX_SAMPLES) return 0;
            if (name === gl.MAX_TEXTURE_IMAGE_UNITS || name === gl.MAX_COMBINED_TEXTURE_IMAGE_UNITS || name === gl.MAX_VERTEX_ATTRIBS) return 16;
            return 1024;
        },
        getError() { return errors.shift()?.code ?? 0; },
        createTexture() { return {id: ++serial}; }, createBuffer() { return {id: ++serial}; }, createVertexArray() { return {id: ++serial}; },
    }, {get(target, key) {
        if (key in target) return target[key];
        if (typeof key === 'string' && /^[A-Z][A-Z_0-9]*$/.test(key)) return target[key] = token++;
        if (typeof key === 'string' && /^uniform/.test(key)) return target[key] = (location, ...args) => {
            if (!location) return;
            if (location.program !== current) { const error = {code: 0x502, location: location.name, requested: location.program.id, current: current?.id}; errors.push(error); events.push(['uniform-invalid-operation', error]); return; }
            const value = args.at(-1); current.uniforms[location.name] = ArrayBuffer.isView(value) || Array.isArray(value) ? Array.from(value) : args;
            events.push([key, current.id, location.name, current.uniforms[location.name]]);
        };
        if (typeof key === 'string' && /^draw(?:Arrays|Elements)/.test(key)) return target[key] = (...args) => {
            const draw = {command: key, program: current?.id, diffuse: current?.uniforms.diffuse ?? null, args}; draws.push(draw); events.push(['draw', draw]);
        };
        if (['enable','disable','colorMask','depthMask','depthFunc','stencilMask','stencilFunc','stencilOp','clearColor','clearDepth','clearStencil','clear','frontFace','cullFace','polygonOffset','lineWidth','scissor','viewport','activeTexture','bindTexture','deleteTexture','texParameteri','texParameterf','texImage2D','texImage3D','texStorage2D','texStorage3D','texSubImage2D','texSubImage3D','generateMipmap','pixelStorei','bindBuffer','bufferData','bufferSubData','deleteBuffer','bindVertexArray','deleteVertexArray','enableVertexAttribArray','disableVertexAttribArray','vertexAttribPointer','vertexAttribIPointer','vertexAttribDivisor','vertexAttrib1fv','vertexAttrib2fv','vertexAttrib3fv','vertexAttrib4fv','blendEquation','blendEquationSeparate','blendFunc','blendFuncSeparate','blendColor'].includes(key))
            return target[key] = () => {};
        throw new Error('Unimplemented GL command boundary: ' + String(key));
    }});
    return {gl, events, draws, errors};
}
function harness() {
    const h = recorder(), gl = h.gl;
    const {WebGLProperties,WebGLState,WebGLInfo,WebGLUtils,WebGLCapabilities,WebGLAttributes,WebGLBindingStates,WebGLGeometries,WebGLObjects,WebGLClipping,WebGLPrograms,WebGLMaterials,WebGLTextures,WebGLRenderLists,WebGLRenderStates,WebGLBackground,WebGLBufferRenderer,WebGLIndexedBufferRenderer} = imports;
    const extensions = {has() { return false; }, get() { return null; }};
    const properties = new WebGLProperties(), state = new WebGLState(gl, extensions), info = new WebGLInfo(gl), utils = new WebGLUtils(gl, extensions);
    const capabilities = new WebGLCapabilities(gl, extensions, {}, utils), attributes = new WebGLAttributes(gl), bindingStates = new WebGLBindingStates(gl, attributes);
    const geometries = new WebGLGeometries(gl, attributes, info, bindingStates), objects = new WebGLObjects(gl, geometries, attributes, bindingStates, info), clipping = new WebGLClipping(properties);
    const environments = {get(texture) { assert.equal(texture, null); return null; }};
    const renderer = {state, info, debug:{checkShaderErrors:false}, sortObjects:true, autoClear:true, localClippingEnabled:false, clippingPlanes:[], outputColorSpace:imports.LinearSRGBColorSpace, toneMapping:imports.NoToneMapping, toneMappingExposure:1, getContext(){return gl;}, getRenderTarget(){return null;}, getPixelRatio(){return 1;}, clear(){gl.clear(gl.COLOR_BUFFER_BIT|gl.DEPTH_BUFFER_BIT);}};
    const textures = new WebGLTextures(gl, extensions, state, properties, capabilities, utils, info), programCache = new WebGLPrograms(renderer, environments, extensions, capabilities, bindingStates, clipping), materials = new WebGLMaterials(renderer, properties);
    renderer.shadowMap = {enabled:false,type:imports.PCFShadowMap,render(shadows){assert.equal(shadows.length,0);}};
    const xr = renderer.xr = {enabled:false,isPresenting:false,getEnvironmentBlendMode(){return 'opaque';}};
    const services = {_this:renderer,_gl:gl,properties,state,info,capabilities,attributes,bindingStates,geometries,objects,clipping,environments,textures,programCache,materials,extensions,xr,shadowMap:renderer.shadowMap,renderLists:new WebGLRenderLists(),renderStates:new WebGLRenderStates(extensions),bufferRenderer:new WebGLBufferRenderer(gl,extensions,info),indexedBufferRenderer:new WebGLIndexedBufferRenderer(gl,extensions,info),morphtargets:{update(){throw new Error('Morphs outside oracle');}},uniformsGroups:{}};
    services.background = new WebGLBackground(renderer,environments,state,objects,false,true);
    Object.assign(renderer, factory(services,imports)); h.renderer=renderer; h.services=services; return h;
}
function world(color, indexed=false, mapped=false) {
    const {Scene,OrthographicCamera,BufferGeometry,Float32BufferAttribute,MeshBasicMaterial,Mesh,DataTexture} = imports;
    const scene=new Scene(),camera=new OrthographicCamera(-1,1,1,-1,.1,10);camera.position.z=3;
    const geometry=new BufferGeometry();geometry.setAttribute('position',new Float32BufferAttribute([-.8,-.8,0,.8,-.8,0,0,.8,0],3));
    if(indexed)geometry.setIndex([0,1,2]);
    const material=new MeshBasicMaterial({color}),mesh=new Mesh(geometry,material);mesh.frustumCulled=false;scene.add(mesh);
    if(mapped){geometry.setAttribute('uv',new Float32BufferAttribute([0,0,1,0,.5,1],2));material.map=new DataTexture(new Uint8Array([255,0,0,255]),1,1);material.map.needsUpdate=true;}
    return {scene,camera,geometry,material,mesh};
}
function run(kind) {
    const h=harness(),r=h.renderer,outer=world(0xff0000),inner=world(0x0000ff);
    r.render(outer.scene,outer.camera);r.autoClear=false;h.draws.length=0;h.events.length=0;
    if(kind==='legacy-standalone-direct') {
        let caught;
        try { r.renderBufferDirect(outer.camera,outer.scene,outer.geometry,outer.material,outer.mesh,null); } catch(error) { caught={name:error.name,message:error.message}; }
        assert.equal(caught?.name,'TypeError');assert.equal(h.draws.length,0);
        return {kind,sourceException:caught,draws:h.draws,errors:h.errors,events:h.events};
    } else if(kind==='info-update') {
        const update=r.info.update;let active=false;
        r.info.update=function(...args){update.apply(this,args);if(!active){active=true;r.render(inner.scene,inner.camera);active=false;}};
        r.render(outer.scene,outer.camera);
        assert.deepEqual(h.draws.map(d=>d.diffuse),[[1,0,0],[0,0,1]]);
    } else if(kind==='index-upload') {
        outer.geometry.setIndex([0,1,2]);let callbacks=0;
        outer.geometry.index.onUpload(()=>{callbacks++;h.events.push(['index.onUpload.before']);r.render(inner.scene,inner.camera);h.events.push(['index.onUpload.after']);});
        r.render(outer.scene,outer.camera);
        assert.equal(callbacks,1);assert.deepEqual(h.draws.map(d=>d.diffuse),[[0,0,1],[0,0,1]]);
    } else {
        outer.geometry.setAttribute('uv',new imports.Float32BufferAttribute([0,0,1,0,.5,1],2));
        const t=outer.material.map=new imports.DataTexture(new Uint8Array([255,0,0,255]),1,1);t.needsUpdate=true;outer.material.needsUpdate=true;let callbacks=0;
        t.onUpdate=function(value){assert.equal(this,t);assert.equal(value,t);callbacks++;h.events.push(['texture.onUpdate.before']);r.render(inner.scene,inner.camera);h.events.push(['texture.onUpdate.after']);};
        r.render(outer.scene,outer.camera);assert.equal(callbacks,1);assert.deepEqual(h.draws.map(d=>d.diffuse),[[0,0,1],[0,0,1]]);
        outer.mesh.position.x=.1;r.render(outer.scene,outer.camera);assert.equal(callbacks,1);assert.deepEqual(h.draws.at(-1).diffuse,[1,0,0]);
    }
    return {kind,draws:h.draws,errors:h.errors,events:h.events};
}
const results=['legacy-standalone-direct','info-update','index-upload','texture-upload'].map(run);
const output=outputArg??path.join('D:/Temp/surui','renderer-reentry-'+Date.now());assert(!fs.existsSync(output),'Output must be fresh');fs.mkdirSync(output,{recursive:true});
fs.writeFileSync(path.join(output,'trace.json'),JSON.stringify({pin:lock.commit,boundary:'CPU GL recorder; explicit compilation/reflection; no GPU claim',results},null,2));
console.log(JSON.stringify({output,results:results.map(({kind,sourceException,draws,errors})=>({kind,sourceException,draws:draws.map(d=>({program:d.program,diffuse:d.diffuse})),errors:errors.length}))}));
