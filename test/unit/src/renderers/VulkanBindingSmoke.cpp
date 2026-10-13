#include "Three.h"
#include "core/InterleavedBuffer.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/InstancedBufferGeometry.h"
#include "core/InstancedInterleavedBuffer.h"
#include "core/JSJobs.h"
#include <iostream>
#include <stdexcept>
int main() {
    try {
        VulkanRenderer renderer({64,64,false});renderer.srgbOutput=false;
        OrthographicCamera camera(-1,1,1,-1,.1,20);camera.position.z=5;
        Scene scene;auto geometry=std::make_shared<BufferGeometry>();
        auto values=Float32Array{-.8f,-.8f,0,1,0,0, .8f,-.8f,0,1,0,0, 0,.8f,0,1,0,0};
        InterleavedBuffer shared(values,6);
        auto position=std::make_shared<InterleavedBufferAttribute>(shared.object(),3,0);
        auto color=std::make_shared<InterleavedBufferAttribute>(shared.object(),3,3);
        geometry->setAttribute("position",position).setAttribute("color",color);
        auto material=std::make_shared<MeshBasicMaterial>(Color(1,1,1));material->vertexColors=true;
        auto mesh=std::make_shared<Mesh>(geometry,material);mesh->frustumCulled=false;scene.add(mesh);
        renderer.render(scene,camera);
        auto pixels=renderer.readPixels();auto at=(32*64+32)*4;
        if(pixels[at]!=255 || pixels[at+1]!=0)throw std::runtime_error("Interleaved position/color initial red pixel failed");
        if(renderer.getInfo().attributeUploadBuffers!=1)throw std::runtime_error("Interleaved views did not share one actual upload buffer");
        for(unsigned i=0;i<3;++i){values[i*6+3]=0;values[i*6+4]=1;}
        renderer.render(scene,camera);pixels=renderer.readPixels();
        if(pixels[at]!=255)throw std::runtime_error("Unversioned interleaved writes changed GPU bytes");
        shared.addUpdateRange(3,3);shared.addUpdateRange(9,3);shared.addUpdateRange(15,3);shared.needsUpdate=true;
        renderer.render(scene,camera);pixels=renderer.readPixels();
        if(pixels[at]!=0 || pixels[at+1]!=255)throw std::runtime_error("Shared partial interleaved update did not reach actual green pixel");
        geometry->dispose();
        if(renderer.getInfo().attributeUploadBuffers!=0)throw std::runtime_error("Interleaved shared disposal did not retire raw Vulkan buffer");
        renderer.render(scene,camera);pixels=renderer.readPixels();
        if(pixels[at+1]!=255)throw std::runtime_error("Disposed interleaved geometry did not rebind and render");
        // Offset changes alone are not a binding cache invalidation in three.js.
        color->offset=0;
        renderer.render(scene,camera);pixels=renderer.readPixels();
        if(pixels[at+1]!=255)throw std::runtime_error("Same-identity offset unexpectedly changed cached pointer");
        color->offset=3;
        auto replacement=Float32Array{-.8f,-.8f,0,0,0,1, .8f,-.8f,0,0,0,1, 0,.8f,0,0,0,1};
        InterleavedBuffer next(replacement,6);position->data=next.object();color->data=next.object();
        renderer.render(scene,camera);pixels=renderer.readPixels();
        if(pixels[at+2]!=255 || pixels[at+1]!=0)throw std::runtime_error("Changing shared data identity did not rebind both views");
        scene.clear();geometry->dispose();
        auto instancedGeometry=std::make_shared<InstancedBufferGeometry>();
        instancedGeometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(
            std::vector<float>{-.8f,-.8f,0,.8f,-.8f,0,0,.8f,0},3));
        InstancedInterleavedBuffer repeats(Uint8Array{255,0,0,9,0,255,0,9},4,2);
        auto repeatedColors=std::make_shared<InterleavedBufferAttribute>(repeats.object(),3,0,true);
        instancedGeometry->setAttribute("color",repeatedColors);instancedGeometry->instanceCount=3;
        auto repeatedMesh=std::make_shared<Mesh>(instancedGeometry,material);repeatedMesh->frustumCulled=false;scene.add(repeatedMesh);
        renderer.render(scene,camera);pixels=renderer.readPixels();
        if(pixels[at+1]!=255 || pixels[at]!=0)throw std::runtime_error("Divisor two normalized interleaved colors did not select instance row one");
        if(renderer.getInfo().triangles!=3 || renderer.getInfo().calls!=1)throw std::runtime_error("InstancedBufferGeometry count was not submitted");
        if(!nativeGeometry(instancedGeometry).get("_maxInstanceCount").strictEquals(4))throw std::runtime_error("BindingStates did not set native geometry max instance count");
        instancedGeometry->instanceCount=2;renderer.render(scene,camera);pixels=renderer.readPixels();
        if(pixels[at]!=255 || pixels[at+1]!=0)throw std::runtime_error("Divisor two normalized interleaved colors did not repeat first row");
        instancedGeometry->dispose();
        if(nativeGeometry(instancedGeometry).hasOwn("_maxInstanceCount"))throw std::runtime_error("Instanced geometry dispose retained max instance count");
        scene.clear();
        auto defaultsGeometry=std::make_shared<BufferGeometry>();
        defaultsGeometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(
            std::vector<float>{-.8f,-.8f,0,.8f,-.8f,0,0,.8f,0},3));
        auto texture=std::make_shared<DataTexture>(Uint8Array{255,0,0,255,0,255,0,255},2,1);
        texture->needsUpdate=true;texture->magFilter=NearestFilter;texture->minFilter=NearestFilter;
        auto textured=std::make_shared<MeshBasicMaterial>(Color(1,1,1));textured->map=texture;
        auto defaults=JSValue::object();defaults.set("uv",JSValue::fromJSON(JsonValue::Array{.75,.5}));
        nativeMaterial(textured).set("defaultAttributeValues",defaults);
        auto defaultMesh=std::make_shared<Mesh>(defaultsGeometry,textured);defaultMesh->frustumCulled=false;scene.add(defaultMesh);
        renderer.render(scene,camera);pixels=renderer.readPixels();
        if(pixels[at+1]!=255 || pixels[at]!=0)throw std::runtime_error("Material default UV did not drive actual texture fetch");
        defaults.set("uv",JSValue::fromJSON(JsonValue::Array{.25,.5}));
        renderer.render(scene,camera);pixels=renderer.readPixels();
        if(pixels[at+1]!=255)throw std::runtime_error("Default UV mutation invalidated unchanged binding cache");
        defaultsGeometry->dispose();renderer.render(scene,camera);pixels=renderer.readPixels();
        if(pixels[at]!=255 || pixels[at+1]!=0)throw std::runtime_error("Default UV was not refreshed after disposal/rebinding");
        defaultsGeometry->dispose();scene.clear();
        for(bool disposeCurrent:{false,true}) {
            VulkanRenderer local({64,64,false});local.srgbOutput=false;Scene pair;
            auto p=std::make_shared<Float32BufferAttribute>(std::vector<float>{-.3f,-.8f,0,.3f,-.8f,0,0,.8f,0},3);
            auto c=std::make_shared<Float32BufferAttribute>(std::vector<float>{0,1,0,0,1,0,0,1,0},3);
            auto i=std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0,1,2},1);
            auto a=std::make_shared<BufferGeometry>(),b=std::make_shared<BufferGeometry>();
            a->setAttribute("position",p).setAttribute("color",c).setIndex(i);
            b->setAttribute("position",p).setAttribute("color",c).setIndex(i);
            auto ma=std::make_shared<Mesh>(a,material),mb=std::make_shared<Mesh>(b,material);
            ma->frustumCulled=mb->frustumCulled=false;ma->position.x=-.5;mb->position.x=.5;ma->renderOrder=0;mb->renderOrder=1;
            pair.add(ma);pair.add(mb);local.render(pair,camera);
            if(local.getInfo().attributeUploadBuffers!=3)throw std::runtime_error("Shared VAOs duplicated upload storage");
            if(disposeCurrent){pair.remove(mb);b->dispose();}else{pair.remove(ma);a->dispose();}
            if(local.getInfo().attributeUploadBuffers!=(disposeCurrent?3u:0u))throw std::runtime_error("Delete did not distinguish current VAO detach from unbound retention");
            for(unsigned n=0;n<3;++n)c->setXYZ(n,1,0,0);
            local.render(pair,camera);const auto image=local.readPixels();const auto x=disposeCurrent?16:48;const auto sample=(32*64+x)*4;
            if(image[sample]!=0 || image[sample+1]!=(disposeCurrent?255:0))throw std::runtime_error("Deleted buffer VAO pixel lifetime disagrees with current/unbound rule");
            if(local.getInfo().attributeUploadBuffers!=(disposeCurrent?5u:3u))throw std::runtime_error("Reupload did not preserve only referenced deleted vertex storage");
            if(disposeCurrent)a->dispose();else b->dispose();
            if(local.getInfo().attributeUploadBuffers!=0)throw std::runtime_error("Final VAO disposal leaked deleted buffer storage");
        }
        {
            VulkanRenderer local({64,64,false});local.srgbOutput=false;Scene integers;
            auto g=std::make_shared<BufferGeometry>();
            g->setAttribute("position",std::make_shared<Int32BufferAttribute>(std::vector<std::int32_t>{-1,-1,0,1,-1,0,0,1,0},3));
            auto m=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
            auto mesh=std::make_shared<Mesh>(g,m);mesh->frustumCulled=false;int hooks=0;
            mesh->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++hooks;};
            mesh->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++hooks;};integers.add(mesh);
            local.render(integers,camera);auto image=local.readPixels();
            if(image[at]!=0 || image[at+1]!=0 || local.getInfo().calls!=1 || hooks!=2)throw std::runtime_error("Integer pointer/float shader mismatch did not preserve draw-error callback flow");
            g->setAttribute("position",std::make_shared<Float32BufferAttribute>(std::vector<float>{-1,-1,0,1,-1,0,0,1,0},3));
            local.render(integers,camera);image=local.readPixels();
            if(image[at]!=255)throw std::runtime_error("Compatible position replacement did not recover integer mismatch");
            auto c=std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0,65535,0,0,65535,0,0,65535,0},3,true);
            c->gpuType=IntType;g->setAttribute("color",c);m->vertexColors=true;
            local.render(integers,camera);image=local.readPixels();
            if(image[at+1]!=0)throw std::runtime_error("Explicit integer GPU type was silently converted to float color");
            c->gpuType=FloatType;local.render(integers,camera);image=local.readPixels();
            if(image[at+1]!=0)throw std::runtime_error("Same-identity GPU type mutation bypassed binding cache");
            g->dispose();local.render(integers,camera);image=local.readPixels();
            if(image[at]!=0 || image[at+1]!=255)throw std::runtime_error("Rebound normalized float color did not recover");
        }
        {
            VulkanRenderer local({64,64,false});local.srgbOutput=false;Scene defaults;
            auto g=std::make_shared<PlaneGeometry>(1.6,1.6);auto m=std::make_shared<MeshBasicMaterial>(Color(1,1,1));m->vertexColors=true;
            auto mesh=std::make_shared<Mesh>(g,m);mesh->frustumCulled=false;defaults.add(mesh);
            local.render(defaults,camera);auto image=local.readPixels();
            if(image[at]!=0 || image[at+1]!=0)throw std::runtime_error("Missing active color must use WebGL generic black");
            auto values=JSValue::object();values.set("color",JSValue::fromJSON(JsonValue::Array{0,1,0,.25}));
            nativeMaterial(m).set("defaultAttributeValues",values);g->dispose();local.render(defaults,camera);image=local.readPixels();
            if(image[at]!=0 || image[at+1]!=255 || image[at+3]!=255)throw std::runtime_error("Default RGB color did not preserve USE_COLOR without vertex alpha");
            values.set("color",JSValue::fromJSON(JsonValue::Array{1,0,0}));local.render(defaults,camera);image=local.readPixels();
            if(image[at+1]!=255)throw std::runtime_error("Unchanged binding cache refreshed default color unexpectedly");
            g->dispose();local.render(defaults,camera);image=local.readPixels();
            if(image[at]!=255 || image[at+1]!=0)throw std::runtime_error("Rebound default color did not update global generic attribute");
        }
        {
            unsigned failed=0;
            const auto check=[&](bool passed,const char* message){
                if(!passed){++failed;std::cerr<<message<<'\n';}
            };
            for(const double requested:{3.0,0.0,9.0}) {
                VulkanRenderer local({64,64,false});local.srgbOutput=false;Scene indexed;
                auto g=std::make_shared<InstancedBufferGeometry>();
                g->setAttribute("position",std::make_shared<Float32BufferAttribute>(
                    std::vector<float>{-.8f,-.8f,0,.8f,-.8f,0,0,.8f,0},3));
                g->setAttribute("color",std::make_shared<InstancedBufferAttribute>(
                    std::vector<float>{0,1,0,0,1,0,0,1,0,0,1,0},3));
                auto index=std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0,1,2},1);
                unsigned uploads=0;
                // Upload three actual indices, then expose a larger logical range.
                // The next frame must follow renderInstances even on a draw error.
                index->onUpload([&]{++uploads;index->count=6;});
                g->setIndex(index);g->instanceCount=3;
                auto m=std::make_shared<MeshBasicMaterial>(Color(1,1,1));m->vertexColors=true;
                auto mesh=std::make_shared<Mesh>(g,m);mesh->frustumCulled=false;
                unsigned before=0,after=0;
                mesh->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++before;};
                mesh->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++after;};
                indexed.add(mesh);local.render(indexed,camera);auto image=local.readPixels();
                check(image[at]==0 && image[at+1]==255,"IBG index callback initial draw lost its captured range");
                check(local.getInfo().calls==1 && local.getInfo().triangles==3,"IBG initial instance statistics differ");
                check(uploads==1 && index->count==6 && nativeGeometry(g).get("_maxInstanceCount").strictEquals(4),
                      "IBG index callback or instance capacity was not established");
                g->instanceCount=requested;local.render(indexed,camera);image=local.readPixels();
                const double expectedCalls=requested==0?0:1;
                const double expectedTriangles=2*std::min(requested,4.0);
                std::cout<<"index-oob requested="<<requested<<" calls="<<local.getInfo().calls
                         <<" triangles="<<local.getInfo().triangles<<" expected="<<expectedCalls<<','<<expectedTriangles<<'\n';
                check(local.getInfo().calls==expectedCalls,"IBG indexed bounds failure changed renderInstances zero-count handling");
                check(local.getInfo().triangles==expectedTriangles,"IBG indexed bounds failure lost logical instance count");
                check(image[at]==0 && image[at+1]==0 && image[at+2]==0,"IBG invalid index range submitted pixels");
                check(before==2 && after==2 && uploads==1,"IBG invalid index range changed render/upload callback order");
                // Restoring a valid logical range recovers the existing uploaded storage.
                index->count=3;g->instanceCount=3;local.render(indexed,camera);image=local.readPixels();
                check(image[at]==0 && image[at+1]==255 && local.getInfo().calls==1 &&
                      local.getInfo().triangles==3 && uploads==1,"IBG valid-range recovery failed or reuploaded unchanged storage");
            }
            if(failed)throw std::runtime_error("Instanced indexed bounds regression checks failed: "+std::to_string(failed));
        }
        {
            unsigned failed=0;
            const auto check=[&](bool passed,const char* message){
                if(!passed){++failed;std::cerr<<message<<'\n';}
            };
            for(const double requested:{3.0,0.0}) {
                VulkanRenderer local({64,64,false});local.srgbOutput=false;Scene ordinary;
                auto g=std::make_shared<InstancedBufferGeometry>();
                g->setAttribute("position",std::make_shared<Float32BufferAttribute>(
                    std::vector<float>{-.8f,-.8f,0,.8f,-.8f,0,0,.8f,0},3));
                g->instanceCount=requested;
                auto m=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
                auto mesh=std::make_shared<Mesh>(g,m);mesh->frustumCulled=false;
                unsigned hooks=0;
                mesh->onBeforeRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++hooks;};
                mesh->onAfterRender=[&](auto&,auto&,auto&,auto*,auto*,auto*){++hooks;};
                ordinary.add(mesh);local.render(ordinary,camera);const auto image=local.readPixels();
                std::cout<<"missing-maximum requested="<<requested<<" calls="<<local.getInfo().calls
                         <<" triangles="<<local.getInfo().triangles<<'\n';
                check(local.getInfo().calls==(requested==0?0:1) && local.getInfo().triangles==requested,
                      "IBG without instanced attributes did not use infinite maximum");
                check(image[at]==(requested==0?0:255) && image[at+1]==(requested==0?0:255),
                      "IBG ordinary position produced incorrect pixels");
                check(nativeGeometry(g).get("_maxInstanceCount").isUndefined(),"IBG fallback introduced a synthetic capacity field");
                check(hooks==2,"IBG ordinary position changed zero-instance callbacks");
            }
            for(unsigned path=0;path<3;++path) {
                VulkanRenderer local({64,64,false});local.srgbOutput=false;Scene dynamic;
                auto g=std::make_shared<InstancedBufferGeometry>();
                if(path==2)g->setAttribute("position",std::make_shared<Int32BufferAttribute>(
                    std::vector<std::int32_t>{-1,-1,0,1,-1,0,0,1,0},3));
                else g->setAttribute("position",std::make_shared<Float32BufferAttribute>(
                    std::vector<float>{-.8f,-.8f,0,.8f,-.8f,0,0,.8f,0},3));
                auto m=std::make_shared<MeshBasicMaterial>(Color(1,1,1));
                auto mesh=std::make_shared<Mesh>(g,m);mesh->frustumCulled=false;dynamic.add(mesh);
                if(path==1) {
                    auto index=std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0,1,2},1);
                    index->onUpload([weak=std::weak_ptr<BufferAttribute>(index)]{if(auto p=weak.lock())p->count=6;});
                    g->setIndex(index);g->instanceCount=1;local.render(dynamic,camera);
                }
                auto trace=JSValue::object();trace.set("log","");
                const JSValue::NativeFunction event=[](const JSValue&,const auto&,const auto& captures)->JSValue {
                    auto trace=captures[0];trace.set("log",trace.get("log").toString()+captures[1].toString());return captures[2];
                };
                auto maximum=JSValue::object(),requested=JSValue::object();
                maximum.set("valueOf",JSValue::function(event,{trace,"maximum.number;",4}));
                requested.set("valueOf",JSValue::function(event,{trace,"requested.number;",3}));
                JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.enumerable=true;
                auto wrapped=nativeGeometry(g);
                descriptor.getter=JSValue::function(event,{trace,"maximum.get;",maximum});
                wrapped.defineProperty("_maxInstanceCount",descriptor);
                descriptor.getter=JSValue::function(event,{trace,"requested.get;",requested});
                wrapped.defineProperty("instanceCount",descriptor);
                local.render(dynamic,camera);const auto image=local.readPixels();
                std::cout<<"instance-read-order path="<<path<<" "<<trace.get("log").toString()<<'\n';
                check(trace.get("log").strictEquals("maximum.get;maximum.get;requested.get;requested.number;maximum.number;"),
                      "IBG maximum getter/read/conversion order differs from WebGLRenderer");
                check(local.getInfo().calls==1 && local.getInfo().triangles==(path==1?6:3),
                      "IBG dynamic count did not reach the selected draw path");
                check(image[at]==(path==0?255:0),"IBG dynamic count changed valid/invalid draw pixels");
            }
            if(failed)throw std::runtime_error("Instanced count fallback/read-order checks failed: "+std::to_string(failed));
        }
        std::cout<<"76 native binding GPU checks passed\n";
    } catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
