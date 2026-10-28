// Included in NativeObject.cpp's existing attribute authority namespace.
#include "core/BufferAttributeSource.inc"
JSValue attributeNormalizedValue(const JSValue& value,const JSValue& array,bool write){
    const auto ctor=array.get("constructor");
    for(const auto& [name,scale,signedType]:std::initializer_list<std::tuple<const char*,double,bool>>{{"Float32Array",1,false},{"Uint32Array",4294967295.,false},{"Uint16Array",65535.,false},{"Uint8Array",255.,false},{"Uint8ClampedArray",255.,false},{"Int32Array",2147483647.,true},{"Int16Array",32767.,true},{"Int8Array",127.,true}}){
        if(!ctor.strictEquals(nativeGlobalBinding(name)))continue;if(std::string(name)=="Float32Array")return value;
        if(write){const auto math=nativeGlobalBinding("Math"),round=math.get("round");return round.call(math,{mathProduct(value,scale)});}
        if(signedType){const auto math=nativeGlobalBinding("Math"),max=math.get("max");return max.call(math,{value.toNumber()/scale,-1.});}return value.toNumber()/scale;
    }
    throw std::runtime_error("THREE.MathUtils: Invalid component type.");
}
JSValue attributeMethod(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0].string();const auto source=argument(args,0);
    if(name=="onUploadCallback")return {};
    if(name=="needsUpdate"){if(source.strictEquals(true))self.set("version",self.get("version").toNumber()+1);return {};}
    if(name=="setUsage"||name=="onUpload"){self.set(name=="setUsage"?"usage":"onUploadCallback",source);return self;}
    if(name=="addUpdateRange"){const auto ranges=self.get("updateRanges"),push=ranges.get("push");auto range=JSValue::object();range.defineProperty("start",dataDescriptor(source));range.defineProperty("count",dataDescriptor(argument(args,1)));push.call(ranges,{range});return {};}
    if(name=="clearUpdateRanges"){auto ranges=self.get("updateRanges");ranges.set("length",0);return {};}
    if(name=="copy"){
        self.set("name",source.get("name"));const auto ctor=source.get("array").get("constructor");self.set("array",ctor.construct({source.get("array")}));for(const auto* key:{"itemSize","count","normalized","usage","gpuType"})self.set(key,source.get(key));return self;
    }
    if(name=="copyAt"){
        const auto left=mathProduct(source,self.get("itemSize"));const auto attribute=argument(args,1);const auto right=mathProduct(argument(args,2),attribute.get("itemSize"));const auto count=self.get("itemSize");
        for(double i=0;i<count.toNumber();++i){auto array=self.get("array");const auto key=JSValue(left+i).toPropertyKey();const auto input=attribute.get("array");array.set(key,input.get(JSValue(right+i)));}return self;
    }
    if(name=="copyArray"||name=="set"){const auto array=self.get("array"),set=array.get("set");if(name=="copyArray")set.call(array,{source});else set.call(array,{source,argument(args,1).isUndefined()?JSValue(0):argument(args,1)});return self;}
    if(name=="clone"){const auto ctor=self.get("constructor"),array=self.get("array"),size=self.get("itemSize");const auto clone=ctor.construct({array,size}),copy=clone.get("copy");return copy.call(clone,{self});}
    if(name=="toJSON"){
        auto data=JSValue::object();data.defineProperty("itemSize",dataDescriptor(self.get("itemSize")));data.defineProperty("type",dataDescriptor(self.get("array").get("constructor").get("name")));const auto array=nativeGlobalBinding("Array"),from=array.get("from");data.defineProperty("array",dataDescriptor(from.call(array,{self.get("array")})));data.defineProperty("normalized",dataDescriptor(self.get("normalized")));for(const auto* key:{"name","usage","gpuType"})data.set(key,self.get(key));return data;
    }
    if(name=="dispose"){const auto dispatch=self.get("dispatchEvent");auto event=JSValue::object();event.defineProperty("type",dataDescriptor("dispose"));dispatch.call(self,{event});return {};}
    if(name=="applyMatrix3"||name=="applyMatrix4"||name=="applyNormalMatrix"||name=="transformDirection"){
        unsigned count=3;if(name=="applyMatrix3"){if(self.get("itemSize").strictEquals(2))count=2;else if(!self.get("itemSize").strictEquals(3))return self;}
        const auto vector=captures[count==2?2:1],limit=self.get("count");for(double i=0;i<limit.toNumber();++i){mathCall(vector,"fromBufferAttribute",{self,i});mathCall(vector,name.c_str(),{source});const auto set=self.get(count==2?"setXY":"setXYZ");Args values{i,vector.get("x"),vector.get("y")};if(count==3)values.push_back(vector.get("z"));set.call(self,values);}return self;
    }
    const bool read=name.rfind("get",0)==0,component=name=="getComponent"||name=="setComponent";
    if(read){const auto array=self.get("array"),size=self.get("itemSize");const auto index=mathProduct(source,size);JSValue key=index;if(component)key=mathAdd(index,argument(args,1));else if(name!="getX")key=index+(name=="getY"?1:name=="getZ"?2:3);auto value=array.get(key);if(self.get("normalized").toBoolean())value=attributeNormalizedValue(value,self.get("array"),false);return value;}
    if(name=="setComponent"||name=="setX"||name=="setY"||name=="setZ"||name=="setW"){
        auto value=argument(args,component?2:1);if(self.get("normalized").toBoolean())value=attributeNormalizedValue(value,self.get("array"),true);auto array=self.get("array");const auto size=self.get("itemSize");const auto index=mathProduct(source,size);JSValue key=index;if(component)key=mathAdd(index,argument(args,1));else if(name!="setX")key=index+(name=="setY"?1:name=="setZ"?2:3);array.set(key,value);return self;
    }
    if(name=="setXY"||name=="setXYZ"||name=="setXYZW"){
        const auto index=mathProduct(source,self.get("itemSize"));const auto count=name.size()-3;Args values;for(std::size_t i=0;i<count;++i)values.push_back(argument(args,i+1));if(self.get("normalized").toBoolean())for(auto& value:values)value=attributeNormalizedValue(value,self.get("array"),true);for(std::size_t i=0;i<count;++i){auto array=self.get("array");array.set(JSValue(index+double(i)),values[i]);}return self;
    }
    throw JSTypeError("Unknown BufferAttribute method");
}
JSValue attributeModule(){
    static thread_local JSWeakValue cachedModule;return cachedModuleValue(cachedModule,[]{
        auto module=JSValue::object(nullptr);const auto vector=nativeVector3Constructor().construct(),vector2=nativeVector2Constructor().construct(),base=EventDispatcher::runtimeConstructor();module.set("nextId",0);
        auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& a,const Args& c)->JSValue{
            const auto array=argument(a,0),itemSize=argument(a,1),normalized=argument(a,2).isUndefined()?JSValue(false):argument(a,2);auto self=c[0].get("constructor").getPrototype().construct({},target);
            const auto arrayCtor=nativeGlobalBinding("Array"),isArray=arrayCtor.get("isArray");if(isArray.call(arrayCtor,{array}).toBoolean())throw JSTypeError("THREE.BufferAttribute: array should be a Typed Array.");self.set("isBufferAttribute",true);
            const auto object=nativeGlobalBinding("Object"),define=object.get("defineProperty");auto descriptor=JSValue::object();const auto id=c[0].get("nextId");auto module=c[0];module.set("nextId",id.toNumber()+1);descriptor.defineProperty("value",dataDescriptor(id));define.call(object,{self,"id",descriptor});
            self.set("name","");self.set("array",array);self.set("itemSize",itemSize);double count=0;if(!array.isUndefined()){const auto length=array.get("length");const auto numerator=length.toNumber();count=numerator/itemSize.toNumber();}self.set("count",count);self.set("normalized",normalized);self.set("usage",StaticDrawUsage);self.set("updateRanges",JSValue::array());self.set("gpuType",FloatType);self.set("version",0);return self;
        },{module},"BufferAttribute",2);ctor.setPrototype(base);ctor.setFunctionSourceText(BufferAttributeSource::classSource);auto prototype=ctor.get("prototype");prototype.setPrototype(base.get("prototype"));module.set("constructor",ctor);module.set("prototype",prototype);
        for(const auto& spec:BufferAttributeSource::methods){auto fn=JSValue::function(attributeMethod,{spec.name,vector,vector2},spec.setter?"set needsUpdate":spec.name,spec.arity);fn.setFunctionSourceText(spec.source);JSPropertyDescriptor d;d.configurable=true;if(spec.setter)d.setter=fn;else{d.value=fn;d.writable=true;}prototype.defineProperty(spec.name,d);}return module;
    });
}
