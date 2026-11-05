#include "renderers/VulkanRenderer.h"
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "core/InstancedBufferAttribute.h"
#include "core/InstancedBufferGeometry.h"
#include "core/InstancedInterleavedBuffer.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "core/JSGlobal.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "geometries/PlaneGeometry.h"
#include "materials/RawShaderMaterial.h"
#include "objects/Mesh.h"
#include "objects/InstancedMesh.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using Args=std::vector<JSValue>;
int checks=0,images=0;
void require(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
JSValue call(JSValue self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSValue list(const Args& values){auto result=JSValue::array();for(const auto& value:values)result.set(JSValue(result.length()),value);return result;}
JSValue paint(const std::string& input,const std::string& vertexBody="",
              const std::string& expression="code==16777217u") {
    auto parameters=JSValue::object();parameters.set("glslVersion","300 es");
    parameters.set("vertexShader","precision highp float;precision highp int;in vec3 position;in "+input+
        " code;uniform mat4 projectionMatrix;uniform mat4 modelViewMatrix;flat out int correct;void main(){correct=("+
        expression+")?1:0;vec3 p=position;"+vertexBody+"gl_Position=projectionMatrix*modelViewMatrix*vec4(p,1.);}");
    parameters.set("fragmentShader","precision highp float;precision highp int;flat in int correct;out vec4 result;void main(){result=correct==1?vec4(0.,1.,0.,1.):vec4(1.,0.,0.,1.);}");
    return nativeRawShaderMaterialConstructor().construct({parameters});
}
struct World {
    JSValue scene=nativeSceneConstructor().construct(),view=nativeOrthographicCameraConstructor().construct({-1,1,1,-1,.1,10});
    JSValue shape=nativePlaneGeometryConstructor().construct({1.6,1.6}),material=paint("uint"),object;
    World(){view.get("position").set("z",3);object=nativeMeshConstructor().construct({shape,material});object.set("frustumCulled",false);call(scene,"add",{object});}
};
void expect(VulkanRenderer& renderer,std::array<int,3> expected,const char* message,unsigned x=32,unsigned y=32) {
    ++images;const auto bytes=renderer.readPixels();const auto at=(y*64+x)*4;
    require(bytes.size()==64*64*4,"actual Vulkan integer-input readback");
    if(std::abs(int(bytes[at])-expected[0])>2||std::abs(int(bytes[at+1])-expected[1])>2||std::abs(int(bytes[at+2])-expected[2])>2) {
        std::cerr<<message<<": actual="<<int(bytes[at])<<','<<int(bytes[at+1])<<','<<int(bytes[at+2])<<'\n';require(false,message);
    }require(true,message);
}
void configure(VulkanRenderer& renderer){renderer.srgbOutput=false;auto value=renderer.object();value.set("outputColorSpace",LinearSRGBColorSpace);}
void validate(VulkanRenderer& renderer){require(renderer.getValidationMessages().empty(),"no collected integer-input Vulkan validation messages");}

void unsignedExactAndVersions(bool validation) {
    VulkanRenderer renderer({64,64,validation});configure(renderer);World world;
    auto values=nativeUint32BufferAttributeConstructor().construct({list({16777217,16777217,16777217,16777217}),1});
    call(world.shape,"setAttribute",{"code",values});renderer.render(world.scene,world.view);
    expect(renderer,{0,255,0},"uint32 vertex values above float precision retain their exact bits");
    auto array=values.get("array");for(unsigned i=0;i<4;++i)array.set(JSValue(i),16777216);
    renderer.render(world.scene,world.view);expect(renderer,{0,255,0},"unchanged attribute version retains actual previously uploaded integers");
    values.set("needsUpdate",true);renderer.render(world.scene,world.view);
    expect(renderer,{255,0,0},"integer attribute version updates the actual GPU input");
    auto signedValues=nativeBufferAttributeConstructor().construct({nativeGlobalBinding("Int32Array").construct({list({16777217,16777217,16777217,16777217})}),1});
    call(world.shape,"setAttribute",{"code",signedValues});renderer.render(world.scene,world.view);
    expect(renderer,{0,0,0},"signed integer pointer cannot bind an unsigned shader input");
    auto gl=call(renderer.object(),"getContext");require(call(gl,"getError").strictEquals(0x0502),"signedness mismatch queues native INVALID_OPERATION");
    call(world.shape,"setAttribute",{"code",values});renderer.render(world.scene,world.view);
    expect(renderer,{255,0,0},"valid integer geometry recovers after a mismatched draw");validate(renderer);
}
void signedWidthsAndPadding(bool validation) {
    VulkanRenderer renderer({64,64,validation});configure(renderer);World world;
    world.material=paint("ivec4","","code==ivec4(-32768,32767,0,1)");world.object.set("material",world.material);
    auto values=nativeBufferAttributeConstructor().construct({nativeGlobalBinding("Int16Array").construct({list({-32768,32767,-32768,32767,-32768,32767,-32768,32767})}),2,true});
    values.set("gpuType",IntType);call(world.shape,"setAttribute",{"code",values});
    renderer.render(world.scene,world.view);expect(renderer,{0,255,0},"signed short integer fetch ignores normalized and pads missing z/w with zero/one");
    auto bytes=nativeBufferAttributeConstructor().construct({nativeGlobalBinding("Int8Array").construct({list({-128,127,-128,127,-128,127,-128,127})}),2});bytes.set("gpuType",IntType);
    world.object.set("material",paint("ivec4","","code==ivec4(-128,127,0,1)"));call(world.shape,"setAttribute",{"code",bytes});
    renderer.render(world.scene,world.view);expect(renderer,{0,255,0},"signed byte vertex values sign-extend into actual ivec4 inputs");validate(renderer);
}
void genericConstantsAndErrors(bool validation) {
    VulkanRenderer renderer({64,64,validation});configure(renderer);World world;
    auto values=nativeUint32BufferAttributeConstructor().construct({list({16777217,16777217,16777217,16777217}),1});
    call(world.shape,"setAttribute",{"code",values});renderer.render(world.scene,world.view);
    auto facade=renderer.object(),gl=call(facade,"getContext");
    const auto program=facade.get("info").get("programs").get("0").get("program");
    const auto location=call(gl,"getAttribLocation",{program,"code"});require(location.toNumber()>=0,"actual source integer attribute location");
    call(world.shape,"deleteAttribute",{"code"});call(gl,"vertexAttribI4ui",{location,16777217,0,0,1});
    renderer.render(world.scene,world.view);expect(renderer,{0,255,0},"disabled integer input reads the actual global unsigned generic constant");
    call(gl,"vertexAttribI4uiv",{location,list({4311744513.75,0,0,1})});
    renderer.render(world.scene,world.view);expect(renderer,{0,255,0},"unsigned generic list values truncate and wrap with no float conversion");
    call(gl,"vertexAttribI4i",{location,16777217,0,0,1});renderer.render(world.scene,world.view);
    expect(renderer,{0,0,0},"signed generic constant cannot bind an unsigned shader input");require(call(gl,"getError").strictEquals(0x0502),"generic constant category mismatch reports INVALID_OPERATION");
    call(gl,"vertexAttribI4uiv",{location,list({16777217,0,0})});require(call(gl,"getError").strictEquals(0x0501),"short integer generic list reports INVALID_VALUE");
    call(gl,"vertexAttribI4ui",{location,16777217,0,0,1});renderer.render(world.scene,world.view);
    expect(renderer,{0,255,0},"a valid generic command restores native integer input");validate(renderer);
}
void integerInstanceDivisor(bool validation) {
    VulkanRenderer renderer({64,64,validation});configure(renderer);World world;
    auto shape=nativePlaneGeometryConstructor().construct({.35,1.2});
    auto values=nativeInstancedBufferAttributeConstructor().construct({
        nativeGlobalBinding("Uint32Array").construct({list({16777217,16777216})}),1,false,2});
    call(shape,"setAttribute",{"code",values});
    auto material=paint("uint","p.x+=float(gl_InstanceID)*.5-.75;");
    auto object=nativeInstancedMeshConstructor().construct({shape,material,4});object.set("frustumCulled",false);
    call(world.scene,"remove",{world.object});call(world.scene,"add",{object});renderer.render(world.scene,world.view);
    expect(renderer,{0,255,0},"integer divisor two selects the exact first row for instance zero",8);
    expect(renderer,{0,255,0},"integer divisor two repeats the first row for instance one",24);
    expect(renderer,{255,0,0},"integer divisor two advances to the second row for instance two",40);
    expect(renderer,{255,0,0},"integer divisor two repeats the second row for instance three",56);
    auto gl=call(renderer.object(),"getContext");require(call(gl,"getError").strictEquals(0),"integer instance divisor produces no native GL error");validate(renderer);
}
void integerInterleavedLayout(bool validation) {
    VulkanRenderer renderer({64,64,validation});configure(renderer);World world;
    auto array=nativeGlobalBinding("Uint32Array").construct({list({
        1,2,16777217,3,4,5,6,16777217,7,8,9,10,16777217,11,12,13,14,16777217,15,16})});
    auto data=InterleavedBuffer::constructor().construct({array,5});
    auto attribute=InterleavedBufferAttribute::constructor().construct({data,1,2,true});
    call(world.shape,"setAttribute",{"code",attribute});renderer.render(world.scene,world.view);
    expect(renderer,{0,255,0},"integer interleaved fetch honors twenty-byte stride and eight-byte offset without float normalization");
    for(const auto index:{2,7,12,17})array.set(JSValue(index),16777216);
    renderer.render(world.scene,world.view);expect(renderer,{0,255,0},"unversioned interleaved edits keep the previously uploaded exact integer rows");
    for(const auto index:{2,7,12,17})call(data,"addUpdateRange",{index,1});
    const auto ranges=data.get("updateRanges");attribute.set("needsUpdate",true);
    require(data.get("version").strictEquals(1),"interleaved attribute needsUpdate versions its original shared data owner");
    renderer.render(world.scene,world.view);expect(renderer,{255,0,0},"versioned disjoint interleaved ranges update the selected integer components");
    require(data.get("updateRanges").sameValue(ranges)&&ranges.length()==0,"source upload clears the same interleaved updateRanges array");
    require(array.get("0").strictEquals(1)&&array.get("19").strictEquals(16),"interleaved padding remains the original application data");validate(renderer);
}
void integerInstancedInterleavedLayout(bool validation) {
    VulkanRenderer renderer({64,64,validation});configure(renderer);World world;
    auto array=nativeGlobalBinding("Uint32Array").construct({list({41,16777217,16777219,42,43,51,16777216,16777219,52,53})});
    auto data=nativeInstancedInterleavedBufferConstructor().construct({array,5,3});
    auto attribute=InterleavedBufferAttribute::constructor().construct({data,2,1,true});
    auto shape=nativePlaneGeometryConstructor().construct({.25,1.2});call(shape,"setAttribute",{"code",attribute});
    auto material=paint("uvec2","p.x+=float(gl_InstanceID)/3.-5./6.;","code==uvec2(16777217u,16777219u)");
    auto object=nativeInstancedMeshConstructor().construct({shape,material,6});object.set("frustumCulled",false);
    call(world.scene,"remove",{world.object});call(world.scene,"add",{object});renderer.render(world.scene,world.view);
    for(const auto x:{5u,16u,27u})expect(renderer,{0,255,0},"instanced integer interleaved divisor three repeats its exact two-component first row",x);
    for(const auto x:{37u,48u,59u})expect(renderer,{255,0,0},"instanced integer interleaved divisor three advances after three instances",x);
    array.set("6",16777217);call(data,"addUpdateRange",{6,1});data.set("needsUpdate",true);renderer.render(world.scene,world.view);
    for(const auto x:{37u,48u,59u})expect(renderer,{0,255,0},"a partial version update preserves interleaved stride while refreshing the repeated second integer row",x);
    auto gl=call(renderer.object(),"getContext");require(call(gl,"getError").strictEquals(0),"instanced interleaved integer layout produces no native GL error");validate(renderer);
}
void integerRecordedBufferVersions(bool validation) {
    VulkanRenderer renderer({64,64,validation});configure(renderer);renderer.autoClear=false;
    auto facade=renderer.object();facade.set("autoClear",false);World world;
    auto shape=nativePlaneGeometryConstructor().construct({.7,1.4});
    auto values=nativeUint32BufferAttributeConstructor().construct({list({16777217,16777217,16777217,16777217}),1});call(shape,"setAttribute",{"code",values});
    auto left=nativeMeshConstructor().construct({shape,world.material}),right=nativeMeshConstructor().construct({shape,world.material});
    left.set("frustumCulled",false);right.set("frustumCulled",false);left.get("position").set("x",-.5);right.get("position").set("x",.5);
    call(world.scene,"remove",{world.object});call(world.scene,"add",{left});
    auto inner=nativeSceneConstructor().construct();call(inner,"add",{right});auto state=JSValue::object();state.set("uploads",0);state.set("nested",0);
    call(values,"onUpload",{JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        auto state=c[0];state.set("uploads",state.get("uploads").number()+1);return {};
    },{state})});
    left.set("onAfterRender",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        auto attribute=c[0],array=attribute.get("array"),state=c[1];
        require(attribute.get("version").strictEquals(0),"first integer draw records source version zero");
        for(unsigned i=0;i<4;++i)array.set(JSValue(i),16777216);
        call(attribute,"addUpdateRange",{0,4});attribute.set("needsUpdate",true);state.set("nested",state.get("nested").number()+1);
        // The nested original render advances source frame bookkeeping, so
        // WebGLObjects performs its normal versioned upload between GPU draws.
        call(c[2],"render",{c[3],c[4]});return {};
    },{values,state,facade,inner,world.view}));
    renderer.clear();renderer.render(world.scene,world.view);
    expect(renderer,{0,255,0},"earlier integer draw retains its old 16777217 buffer until the shared frame fence",16);
    expect(renderer,{255,0,0},"later draw in the same recording reads the new 16777216 buffer revision",48);
    require(state.get("nested").strictEquals(1)&&state.get("uploads").strictEquals(2),"the source callback records exactly two real version uploads and one nested draw");
    require(values.get("version").strictEquals(1)&&values.get("updateRanges").length()==0,"source commits and clears the real integer upload ranges");
    auto gl=call(facade,"getContext");require(call(gl,"getError").strictEquals(0),"same-frame integer buffer replacement produces no native GL error");validate(renderer);
}
void integerPublicContextDispatch(bool validation) {
    VulkanRenderer renderer({64,64,validation});configure(renderer);World world;
    auto facade=renderer.object(),gl=call(facade,"getContext"),state=JSValue::object();
    require(call(facade,"getContext").sameValue(gl),"getContext returns the same public context before source rendering");
    auto trace=JSValue::array();state.set("trace",trace);state.set("codeUploads",0);state.set("pointerReads",0);state.set("pointerCalls",0);
    state.set("arrayBuffer",nullptr);state.set("codeBuffer",nullptr);state.set("uploadCompleted",false);
    const auto nativeBind=gl.get("bindBuffer"),nativeData=gl.get("bufferData"),nativePointer=gl.get("vertexAttribIPointer");
    gl.set("bindBuffer",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        require(self.sameValue(c[0]),"source bindBuffer wrapper receives the actual public context");
        const auto result=c[1].call(self,args);auto state=c[2],trace=state.get("trace");
        if(args.at(0).strictEquals(0x8892))state.set("arrayBuffer",args.at(1));
        trace.set(JSValue(trace.length()),"bindBuffer");return result;
    },{gl,nativeBind,state}));
    auto dataWrapper=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        require(self.sameValue(c[0]),"source bufferData wrapper receives the actual public context");
        auto state=c[2],trace=state.get("trace");
        if(args.at(1).sameValue(state.get("codeArray"))) {
            require(trace.length()>=2&&trace.get(JSValue(trace.length()-2)).strictEquals("bindBuffer")&&
                trace.get(JSValue(trace.length()-1)).strictEquals("get bufferData"),"source reads bufferData after bindBuffer and immediately calls the returned method");
            const auto handle=call(self,"getParameter",{0x8894});
            require(handle.isObject()&&handle.sameValue(state.get("arrayBuffer")),"public upload wrapper observes the real source-selected code buffer handle");
            state.set("codeBuffer",handle);state.set("codeUploads",state.get("codeUploads").number()+1);
        }
        trace.set(JSValue(trace.length()),"call bufferData");return c[1].call(self,args);
    },{gl,nativeData,state});
    JSPropertyDescriptor dataGetter;dataGetter.configurable=true;
    dataGetter.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        require(self.sameValue(c[0]),"source bufferData getter receives the actual public context");
        auto trace=c[1].get("trace");trace.set(JSValue(trace.length()),"get bufferData");return c[2];
    },{gl,state,dataWrapper});gl.defineProperty("bufferData",dataGetter);
    auto makePointer=[&](int generation){return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        require(self.sameValue(c[0]),"source integer pointer wrapper receives the actual public context");
        auto state=c[2],trace=state.get("trace");
        require(trace.length()>0&&trace.get(JSValue(trace.length()-1)).strictEquals("get vertexAttribIPointer"),"source calls the integer pointer implementation returned by its current getter");
        require(state.get("uploadCompleted").strictEquals(true),"source reads and calls its integer pointer after the real attribute onUpload callback");
        require(args.size()==5&&args[2].strictEquals(0x1405),"source integer pointer preserves the unsigned type and five original arguments");
        require(call(self,"getParameter",{0x8894}).sameValue(state.get("codeBuffer")),"source pointer wrapper observes the same real code buffer uploaded through the public context");
        state.set("codeLocation",args[0]);state.set("pointerCalls",state.get("pointerCalls").number()+1);state.set("pointerGeneration",c[3]);
        trace.set(JSValue(trace.length()),"call vertexAttribIPointer");return c[1].call(self,args);
    },{gl,nativePointer,state,generation});};
    state.set("pointerMethod",makePointer(1));JSPropertyDescriptor pointerGetter;pointerGetter.configurable=true;
    pointerGetter.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        require(self.sameValue(c[0]),"source integer pointer getter receives the actual public context");
        auto state=c[1],trace=state.get("trace");state.set("pointerReads",state.get("pointerReads").number()+1);
        trace.set(JSValue(trace.length()),"get vertexAttribIPointer");return state.get("pointerMethod");
    },{gl,state});gl.defineProperty("vertexAttribIPointer",pointerGetter);
    auto installCode=[&](int value){
        auto attribute=nativeUint32BufferAttributeConstructor().construct({list({value,value,value,value}),1});
        state.set("codeArray",attribute.get("array"));state.set("uploadCompleted",false);
        call(attribute,"onUpload",{JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
            require(self.get("array").sameValue(c[0].get("codeArray")),"public-context observation preserves the original source attribute onUpload receiver");
            auto state=c[0];state.set("uploadCompleted",true);return {};
        },{state})});call(world.shape,"setAttribute",{"code",attribute});
    };
    installCode(16777217);renderer.render(world.scene,world.view);
    expect(renderer,{0,255,0},"delegating public context wrappers preserve exact integer GPU drawing");
    require(state.get("codeUploads").strictEquals(1)&&state.get("pointerReads").strictEquals(1)&&state.get("pointerCalls").strictEquals(1),"original source upload and binding each observe the installed public methods exactly once");
    const auto program=facade.get("info").get("programs").get("0").get("program");
    require(call(gl,"getAttribLocation",{program,"code"}).sameValue(state.get("codeLocation")),"the public integer pointer wrapper records the real reflected code location");
    renderer.render(world.scene,world.view);expect(renderer,{0,255,0},"cached source bindings remain valid through public context wrappers");
    require(state.get("codeUploads").strictEquals(1)&&state.get("pointerReads").strictEquals(1),"unchanged source attribute identity skips upload and pointer method getters");
    state.set("pointerMethod",makePointer(2));installCode(16777216);renderer.render(world.scene,world.view);
    expect(renderer,{255,0,0},"new source attribute identity uploads its actual integer bytes through the same public context");
    require(state.get("codeUploads").strictEquals(2)&&state.get("pointerReads").strictEquals(2)&&state.get("pointerCalls").strictEquals(2)&&
        state.get("pointerGeneration").strictEquals(2),"source binding dynamically reads the replacement public method implementation on its next setup");
    require(call(facade,"getContext").sameValue(gl),"getContext preserves strict public context identity after upload and source binding updates");
    require(call(gl,"getError").strictEquals(0),"delegating public context wrappers produce no native GL error");validate(renderer);
}
void integerPublicBufferGrowth(bool validation) {
    VulkanRenderer renderer({64,64,validation});configure(renderer);World world;
    auto shape=nativeBufferGeometryConstructor().construct();
    call(shape,"setAttribute",{"position",nativeFloat32BufferAttributeConstructor().construct({list({
        -.9,-.7,0,-.1,-.7,0,-.5,.7,0,.1,-.7,0,.9,-.7,0,.5,.7,0}),3})});
    auto values=nativeUint32BufferAttributeConstructor().construct({list({16777217,16777217,16777217}),1});
    call(shape,"setAttribute",{"code",values});call(shape,"setDrawRange",{0,3});world.object.set("geometry",shape);
    // One active vertex input makes the public ARRAY_BUFFER_BINDING query
    // unambiguous. The actual source binding setup must select the code buffer;
    // the unused geometry position attribute supplies its normal draw count.
    auto parameters=JSValue::object();parameters.set("glslVersion","300 es");
    parameters.set("vertexShader","precision highp float;precision highp int;in uint code;uniform mat4 projectionMatrix;uniform mat4 modelViewMatrix;flat out int correct;const vec2 vertices[6]=vec2[6](vec2(-.9,-.7),vec2(-.1,-.7),vec2(-.5,.7),vec2(.1,-.7),vec2(.9,-.7),vec2(.5,.7));void main(){correct=code==16777217u?1:0;gl_Position=projectionMatrix*modelViewMatrix*vec4(vertices[gl_VertexID],0.,1.);}");
    parameters.set("fragmentShader","precision highp float;precision highp int;flat in int correct;out vec4 result;void main(){result=correct==1?vec4(0.,1.,0.,1.):vec4(1.,0.,0.,1.);}");
    world.object.set("material",nativeRawShaderMaterialConstructor().construct({parameters}));
    auto gl=call(renderer.object(),"getContext");
    renderer.render(world.scene,world.view);expect(renderer,{0,255,0},"initial three-row integer buffer renders the first triangle",16);expect(renderer,{0,0,0},"source drawRange initially omits the second triangle",48);
    const auto program=renderer.object().get("info").get("programs").get("0").get("program");
    require(call(gl,"getAttribLocation",{program,"position"}).strictEquals(-1)&&call(gl,"getAttribLocation",{program,"code"}).number()>=0,"code is the sole active vertex array selected by actual source binding setup");
    const auto codeBuffer=call(gl,"getParameter",{0x8894});
    require(codeBuffer.isObject(),"public ARRAY_BUFFER_BINDING returns the actual source-selected integer buffer handle");
    call(gl,"bindBuffer",{0x8892,codeBuffer});
    call(gl,"bufferData",{0x8892,nativeGlobalBinding("Uint32Array").construct({list({16777216,16777216,16777216,16777217,16777217,16777217})}),0x88e8});
    require(call(gl,"getError").strictEquals(0),"public native GL bufferData accepts the larger exact integer storage");
    require(values.get("version").strictEquals(0)&&values.get("count").strictEquals(3),"direct GL storage growth leaves the original source attribute version and array length intact");
    call(shape,"setDrawRange",{0,6});renderer.render(world.scene,world.view);
    expect(renderer,{255,0,0},"the new native buffer revision replaces the first triangle's integer bytes",16);
    expect(renderer,{0,255,0},"grown integer cache reaches the new final row and preserves 16777217 exactly",48);
    require(call(gl,"getError").strictEquals(0),"draw after public integer buffer growth has no native GL error");validate(renderer);
}
void integerNullPointerDraw(bool validation) {
    VulkanRenderer renderer({64,64,validation});configure(renderer);World world;
    auto values=nativeUint32BufferAttributeConstructor().construct({list({16777217,16777217,16777217,16777217}),1});
    call(world.shape,"setAttribute",{"code",values});renderer.render(world.scene,world.view);
    expect(renderer,{0,255,0},"original integer source pointer renders before a public null-buffer redefinition");
    auto gl=call(renderer.object(),"getContext");const auto program=renderer.object().get("info").get("programs").get("0").get("program");
    const auto location=call(gl,"getAttribLocation",{program,"code"});require(location.number()>=0,"null pointer test uses the actual reflected code location");
    call(gl,"bindBuffer",{0x8892,nullptr});call(gl,"vertexAttribIPointer",{location,1,0x1405,0,0});
    require(call(gl,"getError").strictEquals(0),"zero-offset integer pointer redefinition with no ARRAY_BUFFER queues no API error");
    renderer.render(world.scene,world.view);expect(renderer,{0,0,0},"a cached enabled integer array with no storage rejects the actual GPU draw");
    require(call(gl,"getError").strictEquals(0x0502),"draw-time null integer storage queues INVALID_OPERATION");
    auto replacement=nativeUint32BufferAttributeConstructor().construct({list({16777217,16777217,16777217,16777217}),1});
    call(world.shape,"setAttribute",{"code",replacement});renderer.render(world.scene,world.view);
    expect(renderer,{0,255,0},"new canonical attribute identity makes source binding setup restore the exact integer pointer");
    require(call(gl,"getError").strictEquals(0),"native integer drawing recovers after null pointer rejection");validate(renderer);
}
}
int main(int argc,char** argv) {
    try {bool validation=false;for(int i=1;i<argc;++i)if(std::string(argv[i])=="--validation")validation=true;
        unsignedExactAndVersions(validation);signedWidthsAndPadding(validation);genericConstantsAndErrors(validation);
        integerInstanceDivisor(validation);integerInterleavedLayout(validation);integerInstancedInterleavedLayout(validation);integerRecordedBufferVersions(validation);
        integerPublicContextDispatch(validation);integerPublicBufferGrowth(validation);integerNullPointerDraw(validation);
        std::cout<<"native integer inputs assertions="<<checks<<" images="<<images<<" validation_requested="<<validation<<'\n';return 0;
    }catch(const std::exception& error){std::cerr<<"Native integer inputs failed: "<<error.what()<<'\n';return 1;}
}
