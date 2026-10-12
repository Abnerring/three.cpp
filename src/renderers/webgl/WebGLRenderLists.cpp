#include "renderers/webgl/WebGLRenderLists.h"
#include "core/JSWeakMap.h"
#include <algorithm>

namespace {
using Args=std::vector<JSValue>;
JSValue ordinaryFunction(JSValue::NativeFunction callback,const Args& captures,const char* name,std::size_t arity){
    return JSValue::constructor(callback,captures,name,arity,JSValue::ConstructorKind::Callable);
}
JSValue argument(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue();}
void literal(JSValue object,const char* key,JSValue value){
    JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);
}
double subtract(const JSValue& left,const JSValue& right){const auto a=left.toNumber();const auto b=right.toNumber();return a-b;}
JSValue painterSort(const JSValue&,const Args& args,const Args& captures){
    const auto a=argument(args,0),b=argument(args,1);const bool reverse=captures[0].toBoolean();
    for(const auto* key:{"groupOrder","renderOrder"}){
        const auto left=a.get(key);const auto right=b.get(key);
        if(!left.strictEquals(right)){const auto x=a.get(key);const auto y=b.get(key);return subtract(x,y);}
    }
    if(!reverse){
        const auto left=a.get("material").get("id");const auto right=b.get("material").get("id");
        if(!left.strictEquals(right)){const auto x=a.get("material").get("id");const auto y=b.get("material").get("id");return subtract(x,y);}
        const auto x=a.get("materialVariant");const auto y=b.get("materialVariant");
        if(!x.strictEquals(y)){const auto variantA=a.get("materialVariant");const auto variantB=b.get("materialVariant");return subtract(variantA,variantB);}
    }
    const auto left=a.get("z");const auto right=b.get("z");
    if(!left.strictEquals(right)){
        const auto x=(reverse?b:a).get("z");const auto y=(reverse?a:b).get("z");return subtract(x,y);
    }
    const auto x=a.get("id");const auto y=b.get("id");return subtract(x,y);
}
JSValue stableSortFunction(bool reverse){
    static thread_local JSWeakValue forwardCache,reverseCache;
    auto& cache=reverse?reverseCache:forwardCache;
    if(auto value=cache.lock();value&&value->isCallable())return *value;
    auto value=ordinaryFunction(painterSort,{reverse},reverse?"reversePainterSortStable":"painterSortStable",2);
    value.retainForHeapLifetime();cache=JSWeakValue(value);return value;
}
int materialVariant(const JSValue& object){
    int value=0;if(object.get("isInstancedMesh").toBoolean())value+=2;
    if(object.get("isSkinnedMesh").toBoolean())value+=1;return value;
}
JSValue nextItem(JSValue state,const JSValue& object,const JSValue& geometry,const JSValue& material,
                 const JSValue& groupOrder,const JSValue& z,const JSValue& group){
    auto items=state.get("items");auto item=items.get(state.get("index").toString());
    if(item.isUndefined()){
        item=JSValue::object();
        literal(item,"id",object.get("id"));literal(item,"object",object);literal(item,"geometry",geometry);
        literal(item,"material",material);literal(item,"materialVariant",materialVariant(object));
        literal(item,"groupOrder",groupOrder);literal(item,"renderOrder",object.get("renderOrder"));
        literal(item,"z",z);literal(item,"group",group);
        items.set(state.get("index").toString(),item);
    }else{
        item.set("id",object.get("id"));item.set("object",object);item.set("geometry",geometry);item.set("material",material);
        item.set("materialVariant",materialVariant(object));item.set("groupOrder",groupOrder);
        item.set("renderOrder",object.get("renderOrder"));item.set("z",z);item.set("group",group);
    }
    state.set("index",state.get("index").toNumber()+1);return item;
}
JSValue listInit(const JSValue&,const Args&,const Args& c){
    auto state=c[0];state.set("index",0);for(std::size_t i=1;i<=3;++i){auto array=c[i];array.set("length",0);}return {};
}
JSValue insertItem(const JSValue&,const Args& args,const Args& c){
    const auto object=argument(args,0),geometry=argument(args,1),material=argument(args,2);
    const auto groupOrder=argument(args,3),group=argument(args,5);auto z=argument(args,4);
    const bool front=c[4].toBoolean();
    if(!front&&argument(args,6).get("reversedDepth").strictEquals(JSValue(true)))z=-z.toNumber();
    const auto item=nextItem(c[0],object,geometry,material,groupOrder,z,group);
    JSValue target;
    if(material.get("transmission").toNumber()>0)target=c[2];
    else if(material.get("transparent").strictEquals(JSValue(true)))target=c[3];
    else target=c[1];
    target.get(front?"unshift":"push").call(target,{item});return {};
}
JSValue listSort(const JSValue&,const Args& args,const Args& c){
    for(std::size_t i=0;i<3;++i){
        const auto array=c[i];
        if(array.get("length").toNumber()>1){
            const auto method=array.get("sort");const auto custom=argument(args,i==0?0:1);
            method.call(array,{custom.toBoolean()?custom:c[i==0?3:4]});
        }
    }return {};
}
JSValue listFinish(const JSValue&,const Args&,const Args& c){
    const auto state=c[0],items=state.get("items");
    double i=state.get("index").toNumber();const auto length=items.get("length").toNumber();
    for(;i<length;++i){auto item=items.get(JSValue(i).toString());if(item.get("id").isNull())break;
        for(const auto* key:{"id","object","geometry","material","group"})item.set(key,nullptr);
    }return {};
}
std::vector<std::uint16_t> units(const std::string& text){
    std::vector<std::uint16_t> out;for(std::size_t i=0;i<text.size();){auto c=static_cast<unsigned char>(text[i++]);
        unsigned count=c<0x80?0:c<0xe0?1:c<0xf0?2:3;std::uint32_t value=count?c&(0x7f>>count):c;
        while(count--)value=(value<<6)|(static_cast<unsigned char>(text[i++])&63);
        if(value<=0xffff)out.push_back(static_cast<std::uint16_t>(value));else{value-=0x10000;out.push_back(static_cast<std::uint16_t>(0xd800+(value>>10)));out.push_back(static_cast<std::uint16_t>(0xdc00+(value&1023)));}
    }return out;
}
bool greaterEqual(const JSValue& left,const JSValue& right){
    const auto a=left.toPrimitive(JSValue::PreferredType::Number);const auto b=right.toPrimitive(JSValue::PreferredType::Number);
    if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=units(a.string()),y=units(b.string());return !std::lexicographical_compare(x.begin(),x.end(),y.begin(),y.end());}
    const auto x=a.toNumber();const auto y=b.toNumber();return x>=y;
}
JSValue listsGet(const JSValue&,const Args& args,const Args& c){
    const auto state=c[0],scene=argument(args,0),depth=argument(args,1);
    const auto map=state.get("lists");const auto listArray=map.get("get").call(map,{scene});JSValue list;
    if(listArray.isUndefined()){
        list=WebGLRenderList().object();auto array=JSValue::array();literal(array,"0",list);
        const auto live=state.get("lists");live.get("set").call(live,{scene,array});
    }else{
        const auto length=listArray.get("length");
        if(greaterEqual(depth,length)){
            list=WebGLRenderList().object();listArray.get("push").call(listArray,{list});
        }else list=listArray.get(depth.toPrimitive(JSValue::PreferredType::String).toString());
    }return list;
}
JSValue listsDispose(const JSValue&,const Args&,const Args& c){auto state=c[0];state.set("lists",createNativeWeakMap());return {};}
}
WebGLRenderList::WebGLRenderList():object_(JSValue::object()){
    auto state=JSValue::object(nullptr);literal(state,"items",JSValue::array());literal(state,"index",0);
    const auto opaqueItems=JSValue::array(),transmissiveItems=JSValue::array(),transparentItems=JSValue::array();
    literal(object_,"opaque",opaqueItems);literal(object_,"transmissive",transmissiveItems);literal(object_,"transparent",transparentItems);
    literal(object_,"init",ordinaryFunction(listInit,{state,opaqueItems,transmissiveItems,transparentItems},"init",0));
    literal(object_,"push",ordinaryFunction(insertItem,{state,opaqueItems,transmissiveItems,transparentItems,false},"push",7));
    literal(object_,"unshift",ordinaryFunction(insertItem,{state,opaqueItems,transmissiveItems,transparentItems,true},"unshift",6));
    literal(object_,"finish",ordinaryFunction(listFinish,{state},"finish",0));
    literal(object_,"sort",ordinaryFunction(listSort,{opaqueItems,transmissiveItems,transparentItems,stableSortFunction(false),stableSortFunction(true)},"sort",2));
}
JSValue WebGLRenderList::init(){return object_.get("init").call(object_);}
JSValue WebGLRenderList::push(JSValue object,JSValue geometry,JSValue material,JSValue order,JSValue z,JSValue group,JSValue camera){return object_.get("push").call(object_,{object,geometry,material,order,z,group,camera});}
JSValue WebGLRenderList::unshift(JSValue object,JSValue geometry,JSValue material,JSValue order,JSValue z,JSValue group){return object_.get("unshift").call(object_,{object,geometry,material,order,z,group});}
JSValue WebGLRenderList::sort(JSValue opaqueSort,JSValue transparentSort){return object_.get("sort").call(object_,{opaqueSort,transparentSort});}
JSValue WebGLRenderList::finish(){return object_.get("finish").call(object_);}
WebGLRenderLists::WebGLRenderLists():object_(JSValue::object()){
    auto state=JSValue::object(nullptr);literal(state,"lists",createNativeWeakMap());
    literal(object_,"get",ordinaryFunction(listsGet,{state},"get",2));literal(object_,"dispose",ordinaryFunction(listsDispose,{state},"dispose",0));
}
JSValue WebGLRenderLists::get(JSValue scene,JSValue depth){return object_.get("get").call(object_,{scene,depth});}
JSValue WebGLRenderLists::dispose(){return object_.get("dispose").call(object_);}
