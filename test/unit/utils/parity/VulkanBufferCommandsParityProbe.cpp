#include "renderers/vulkan/VulkanBufferCommands.h"
#include "core/NativeArray.h"
#include "renderers/webgl/WebGLAttributes.h"
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;using A=JsonValue::Array;using O=JsonValue::Object;
struct Memory final:VulkanBufferCommands::Storage{
    std::vector<std::byte> bytes;
    void setData(std::span<const std::byte> data)override{bytes.assign(data.begin(),data.end());}
    void setSubData(std::size_t offset,std::span<const std::byte> data)override{
        if(offset>bytes.size()||data.size()>bytes.size()-offset)throw std::runtime_error("Adapter passed invalid native bounds");
        std::copy(data.begin(),data.end(),bytes.begin()+offset);
    }
    std::vector<std::byte> read()const override{return bytes;}
    std::size_t size()const override{return bytes.size();}
};
JSValue call(JSValue object,const char* name,const Args& args={}){return object.get(name).call(object,args);}
Args numbers(){return {0,-0.0,.25,-.25,1,4,5,-1,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),4294967296.0,4294967297.0,0x1p64,-0x1p64,"2",nullptr,JSValue()};}
JsonValue run(unsigned kind,unsigned mode,unsigned a,unsigned b){
    VulkanBufferCommands commands([]{return std::make_shared<Memory>();});auto gl=commands.object(),buffer=call(gl,"createBuffer");
    call(gl,"bindBuffer",{34962,buffer});Uint8Array initial(32);for(std::size_t i=0;i<32;++i)initial[i]=160;
    call(gl,"bufferData",{34962,NativeArray_detail::wrap(initial),35044});
    const std::size_t width=kind==0||kind==1||kind==8?1:kind==2||kind==3?2:kind==7?8:4;
    Uint8Array backing(8*width);for(std::size_t i=0;i<backing.size();++i)backing[i]=double(i*3+1);
    auto source=NativeArray_detail::constructor(kind).construct({nativeArrayBuffer(backing.buffer),double(2*width),4});
    const auto values=numbers();JSValue destination=0,offset=0,length=1;
    if(mode==0)destination=values[a];
    if(mode==1)offset=values[a];
    if(mode==2){offset=1;length=values[a];}
    if(mode==3){destination=values[a];offset=values[b];length=0;}
    call(gl,"bufferSubData",{34962,destination,source,offset,length});A bytes;
    for(const auto value:commands.read(buffer))bytes.emplace_back(std::to_integer<int>(value));
    const auto error=call(gl,"getError").number();const auto revision=commands.revision(buffer);
    commands.shutdown();return O{{"kind",kind},{"mode",mode},{"a",a},{"b",b},{"error",error},{"revision",double(revision)},{"bytes",bytes}};
}
JSValue range(JSValue start,JSValue count){auto v=JSValue::object();v.set("start",start);v.set("count",count);return v;}
JSValue array(const Args& values){auto v=JSValue::array();v.get("push").call(v,values);return v;}
JsonValue integration(unsigned mode){
    VulkanBufferCommands commands([]{return std::make_shared<Memory>();});auto gl=commands.object();WebGLAttributes manager(gl);
    Uint8Array values(12);auto a=JSValue::object();a.set("array",NativeArray_detail::wrap(values));a.set("usage",35044);a.set("version",0);a.set("uploads",0);a.set("updateRanges",JSValue::array());
    a.set("onUploadCallback",JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{auto a=self;a.set("uploads",a.get("uploads").number()+1);return {};}));
    a.set("clearUpdateRanges",JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{auto ranges=self.get("updateRanges");ranges.set("length",0);return {};}));
    manager.update(a,34962);auto buffer=manager.get(a).get("buffer");for(std::size_t i=0;i<12;++i)values[i]=double(i+1);
    a.set("updateRanges",array({range(0,1),range(4,-1),range(8,1)}));a.set("version",1);manager.update(a,34962);
    const auto initialError=call(gl,"getError").number();std::string error="none";
    if(mode){
        auto second=range(4,1),counter=JSValue::object(),start=JSValue::object();counter.set("n",0);
        start.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("source offset");}));
        JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{auto counter=c[0];auto n=counter.get("n").number()+1;counter.set("n",n);return n>=3?c[1]:JSValue(4);},{counter,start});second.defineProperty("start",d);
        auto ranges=array({range(0,1),second});ranges.set("sort",JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{return self;}));
        values[0]=11;values[4]=22;a.set("updateRanges",ranges);a.set("version",2);try{manager.update(a,34962);}catch(const JSRangeError&){error="RangeError";}
    }
    A bytes;for(auto v:commands.read(buffer))bytes.emplace_back(std::to_integer<int>(v));
    return O{{"integration",mode},{"firstError",initialError},{"exception",error},{"error",call(gl,"getError").number()},
        {"revision",double(commands.revision(buffer))},{"version",manager.get(a).get("version").number()},
        {"uploads",a.get("uploads").number()},{"ranges",double(a.get("updateRanges").length())},{"bytes",bytes}};
}
}
int main(){try{auto witness=JSValue::object();for(unsigned kind=0;kind<9;++kind){
    for(unsigned mode=0;mode<3;++mode)for(unsigned a=0;a<18;++a){std::cout<<run(kind,mode,a,0).dump()<<'\n';witness.collectGarbage();}
    for(unsigned a=0;a<18;++a)for(unsigned b=0;b<18;++b){std::cout<<run(kind,3,a,b).dump()<<'\n';witness.collectGarbage();}
}for(unsigned mode=0;mode<2;++mode){std::cout<<integration(mode).dump()<<'\n';witness.collectGarbage();}
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
