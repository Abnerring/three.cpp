#include "animation/AnimationRuntime.h"
#include "animation/AnimationClip.h"
#include "animation/PropertyBinding.h"
#include "core/NativeObject.h"
#include "math/interpolants/BezierInterpolant.h"
#include "math/interpolants/CubicInterpolant.h"
#include "math/interpolants/QuaternionLinearInterpolant.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace {
using Args = std::vector<JSValue>;
constexpr auto bindingTag = "three.PropertyBinding";
constexpr auto compositeTag = "three.PropertyBinding.Composite";
constexpr auto mixerTag = "three.PropertyMixer";
constexpr auto interpolantTag = "three.Interpolant";
constexpr auto clipTag = "three.AnimationClip";
constexpr auto trackTag = "three.KeyframeTrack";
constexpr auto tracksTag = "three.KeyframeTrackArray";
constexpr auto clipsTag = "three.AnimationClipArray";
constexpr auto settingsTag = "three.AnimationSettings";
constexpr auto gettersTag = "three.PropertyBinding.GetterTable";
constexpr auto settersTag = "three.PropertyBinding.SetterTable";
template<class T> std::shared_ptr<T> payload(const JSValue& value, const char* tag) {
    return std::static_pointer_cast<T>(value.hostPayload(tag));
}
JSValue arg(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
JSPropertyDescriptor descriptor(JSValue value, bool enumerable = true) {
    JSPropertyDescriptor d; d.value = std::move(value); d.writable = true; d.enumerable = enumerable; d.configurable = true; return d;
}
std::optional<std::size_t> arrayIndex(const std::string& key) {
    if (key.empty() || (key.size() > 1 && key[0] == '0')) return {};
    std::uint64_t n = 0;
    for (char c : key) { if (c < '0' || c > '9') return {}; n = n * 10 + c - '0'; if (n >= UINT32_MAX) return {}; }
    return std::size_t(n);
}
std::ptrdiff_t indexValue(JSValue value) {
    const double n = value.toNumber();
    if (!std::isfinite(n) || std::trunc(n) != n || n < double(PTRDIFF_MIN) || n >= -double(PTRDIFF_MIN))
        throw JSTypeError("Native interpolant cache indices require finite integers");
    return std::ptrdiff_t(n);
}
JSValue prototype(const std::string& tag);
JSValue read(const JSValue& self, const std::string& key);
bool write(const JSValue& self, const std::string& key, JSValue value);
std::vector<std::string> fields(const JSValue& self);
std::vector<JSNativeOwnerNode> ownership(const JSValue& self);
std::vector<JSValue> trace(const JSValue& self);
JSValue invoke(const JSValue& self, const Args& args, const Args& captures);

// Native members are the authoritative data. Extra properties and callbacks
// live in the managed graph; getters never make a detached JSON representation.
JSHostPropertyResult own(const JSValue& self, const std::string& key) {
    const auto names = fields(self);
    if (std::find(names.begin(), names.end(), key) == names.end()) return {};
    return {true, descriptor(read(self, key))};
}
std::optional<bool> define(const JSValue& self, const std::string& key, const JSPropertyDescriptor& d) {
    const auto names = fields(self);
    const bool present = std::find(names.begin(), names.end(), key) != names.end();
    if (!present) {
        // Optional native members become own properties when first assigned.
        if(d.getter||d.setter)return {};
        if(d.value && write(self,key,*d.value)) {
            auto retained=self;retained.setHostSlot("present."+key,true);return true;
        }
        return {};
    }
    if (d.getter || d.setter || (d.writable && !*d.writable) ||
        (d.enumerable && !*d.enumerable) || (d.configurable && !*d.configurable))
        throw JSTypeError("Native animation fields require writable enumerable configurable data descriptors");
    if (d.value) write(self, key, *d.value);
    return true;
}
std::optional<bool> erase(const JSValue& self, const std::string& key) {
    const auto names = fields(self);
    if (std::find(names.begin(), names.end(), key) == names.end()) return {};
    throw JSTypeError("Deleting authoritative native animation fields is not supported");
}
std::shared_ptr<const JSHostObjectHooks> hooks() {
    static const auto value = [] { auto h = std::make_shared<JSHostObjectHooks>();
        h->getOwnProperty=own;h->defineOwnProperty=define;h->erase=erase;h->ownPropertyNames=fields;h->nativeOwnership=ownership;h->trace=trace;
        return h; }();
    return value;
}
template<class T> JSValue wrap(const std::shared_ptr<T>& value, const char* tag) {
    if (!value) return nullptr;
    auto result=JSValue::hostObject(value,value.get(),tag);
    if (result.getHostSlot("animation.initialized").isUndefined()) {
        result.setHostSlot("animation.initialized",true);result.setHostHooks(hooks());result.setPrototype(prototype(tag));
    }
    return result;
}
void arrayOwnership(std::vector<JSNativeOwnerNode>& nodes, JSNativeOwnerNode& node, const AnimationArray& array) {
    std::visit([&](const auto& value) {
        if constexpr(std::is_same_v<std::decay_t<decltype(value)>,JSValue>) node.roots.push_back(&value);
        else {
            auto ownership=nativeArrayOwnership(value);
            node.edges.insert(node.edges.end(),ownership.edges.begin(),ownership.edges.end());
            nodes.insert(nodes.end(),std::make_move_iterator(ownership.nodes.begin()),std::make_move_iterator(ownership.nodes.end()));
        }
    },array.storage());
}
template<class T> void nativeEdge(JSNativeOwnerNode& node, const void* slot, const std::shared_ptr<T>& value) {
    if (value) node.edges.push_back({slot,value});
}
template<class Callback> void callbackOwner(std::vector<JSNativeOwnerNode>& result,
                                          JSNativeOwnerNode& parent, const Callback& callback) {
    const auto& target=callback.runtimeTarget();
    if (!target) return;
    nativeEdge(parent,&callback,target);
    result.push_back({target,{&target->runtimeFunction},{}});
}

template<class Callback> JSValue nativeCallback(const Callback& callback, const std::string& name, const char* tag) {
    const auto& target=callback.runtimeTarget();
    if (!target) return {};
    if (!target->runtimeFunction.isUndefined()) return target->runtimeFunction;
    if (!name.empty()) return prototype(tag).get(name);
    // Custom C++ callback closures remain an explicit native boundary. The
    // supported JS callback path uses managed(), whose roots are enumerated.
    throw JSTypeError("Expose custom native animation callbacks with PropertyMixerCallback::managed");
}
JSValue bindingCallback(const PropertyBinding::Function& callback) {
    return nativeCallback(callback,PropertyBinding::runtimeFunctionName(callback),bindingTag);
}
template<class Callback> JSValue mixerCallback(const Callback& callback) {
    const auto& target=callback.runtimeTarget();
    return nativeCallback(callback,target?target->runtimeName:std::string{},mixerTag);
}

PropertyBinding::Function readBindingFunction(JSValue value) {
    return PropertyBinding::Function::managed(std::move(value),[](const JSValue& fn,PropertyBinding& binding,AnimationArray array,JSValue offset){
        fn.call(nativePropertyBinding(binding.shared_from_this()),{nativeArray(array),offset});
    });
}
using GetterTable = SharedArray<PropertyBinding::Function>;
using SetterTable = SharedArray<GetterTable>;
JSValue nativeGetterTable(const GetterTable& table);
JSValue nativeSetterTable(const SetterTable& table);
template<class Table,bool Outer> JSValue nativeBindingTable(const Table& table,const char* tag) {
    const auto storage=table.storage();auto value=JSValue::hostObject(storage,storage.get(),tag,JSValue::Type::Array);
    if(!value.getHostSlot("animation.initialized").isUndefined())return value;
    value.setHostSlot("animation.initialized",true);
    static const auto h=[] {auto result=std::make_shared<JSHostObjectHooks>();
        result->getOwnProperty=[](const JSValue& self,const std::string& key)->JSHostPropertyResult{
            const auto s=payload<typename Table::Storage>(self,Outer?settersTag:gettersTag);
            if(key=="length"){auto d=descriptor(double(s->values.size()),false);d.configurable=false;return {true,d};}
            const auto i=arrayIndex(key);if(!i)return {};if(*i>=s->values.size()||s->holes.contains(*i))return {true,{}};
            if constexpr(Outer)return {true,descriptor(nativeGetterTable(s->values[*i]))};
            else return {true,descriptor(bindingCallback(s->values[*i]))};
        };
        result->defineOwnProperty=[](const JSValue& self,const std::string& key,const JSPropertyDescriptor& d)->std::optional<bool>{
            const auto i=arrayIndex(key);if(key!="length"&&!i)return {};
            if(d.getter||d.setter||(d.writable&&!*d.writable)||(d.enumerable&&*d.enumerable!=(key!="length"))||
               (d.configurable&&*d.configurable!=(key!="length")))throw JSTypeError("Native binding tables require ordinary data descriptors");
            if(!d.value)return true;auto s=payload<typename Table::Storage>(self,Outer?settersTag:gettersTag);
            if(key=="length"){const auto n=d.value->toNumber();if(!std::isfinite(n)||n<0||n>UINT32_MAX||std::trunc(n)!=n)throw JSRangeError("Invalid array length");
                for(std::size_t j=s->values.size();j<std::size_t(n);++j)s->holes.insert(j);s->values.resize(std::size_t(n));}
            else {if(*i>=s->values.size()){for(auto j=s->values.size();j<*i;++j)s->holes.insert(j);s->values.resize(*i+1);}
                if constexpr(Outer)s->values[*i]=GetterTable(payload<GetterTable::Storage>(*d.value,gettersTag));
                else s->values[*i]=readBindingFunction(*d.value);s->holes.erase(*i);
            }return true;
        };
        result->ownPropertyNames=[](const JSValue& self){const auto s=payload<typename Table::Storage>(self,Outer?settersTag:gettersTag);
            std::vector<std::string> names;for(std::size_t i=0;i<s->values.size();++i)if(!s->holes.contains(i))names.push_back(std::to_string(i));names.push_back("length");return names;};
        result->nativeOwnership=ownership;return result;
    }();value.setHostHooks(h);return value;
}
JSValue nativeGetterTable(const GetterTable& table){return nativeBindingTable<GetterTable,false>(table,gettersTag);}
JSValue nativeSetterTable(const SetterTable& table){return nativeBindingTable<SetterTable,true>(table,settersTag);}

std::optional<bool> nativeSequenceErase(const JSValue& self,const std::string& key) {
    if(key=="length")return false;
    if(!arrayIndex(key))return {};
    if(self.getOwnPropertyDescriptor(key))
        throw JSTypeError("Deleting native clip/track array elements requires sparse runtime array storage");
    return true;
}
JSValue nativeTracks(const AnimationClip::TracksPtr& tracks) {
    if (!tracks) return nullptr;
    auto value=JSValue::hostObject(tracks,tracks.get(),tracksTag,JSValue::Type::Array);
    if (value.getHostSlot("animation.initialized").isUndefined()) {
        value.setHostSlot("animation.initialized",true);
        static const auto h=[] { auto result=std::make_shared<JSHostObjectHooks>();
            result->getOwnProperty=[](const JSValue& self,const std::string& key)->JSHostPropertyResult {
                const auto tracks=payload<AnimationClip::Tracks>(self,tracksTag);
                if(key=="length") { auto d=descriptor(double(tracks->size()),false);d.configurable=false;return {true,d}; }
                const auto index=arrayIndex(key);if(!index)return {};
                if(*index>=tracks->size())return {true,{}};
                return {true,descriptor(nativeKeyframeTrack((*tracks)[*index]))};
            };
            result->defineOwnProperty=[](const JSValue& self,const std::string& key,const JSPropertyDescriptor& d)->std::optional<bool>{
                const auto index=arrayIndex(key);if(key!="length"&&!index)return {};
                if(d.getter||d.setter||(d.writable&&!*d.writable)||(d.enumerable&&*d.enumerable!=(key!="length"))||
                   (d.configurable&&*d.configurable!=(key!="length")))throw JSTypeError("Native track arrays require ordinary data descriptors");
                if(!d.value)return true;
                auto tracks=payload<AnimationClip::Tracks>(self,tracksTag);
                if(key=="length") { const auto n=d.value->toNumber();if(!std::isfinite(n)||n<0||n>UINT32_MAX||std::trunc(n)!=n)throw JSRangeError("Invalid array length");tracks->resize(std::size_t(n)); }
                else { auto track=keyframeTrackFromNative(*d.value);if(*index>=tracks->size())tracks->resize(*index+1);(*tracks)[*index]=std::move(track); }
                return true;
            };
            result->ownPropertyNames=[](const JSValue& self){auto tracks=payload<AnimationClip::Tracks>(self,tracksTag);std::vector<std::string> names;for(std::size_t i=0;i<tracks->size();++i)names.push_back(std::to_string(i));names.push_back("length");return names;};
            result->erase=nativeSequenceErase;result->nativeOwnership=ownership;result->trace=trace;return result;
        }();value.setHostHooks(h);
    }
    return value;
}
AnimationClip::TracksPtr readTracks(const JSValue& value) {
    if(value.isHostObject(tracksTag))return payload<AnimationClip::Tracks>(value,tracksTag);
    if(value.isNull())return {};
    throw JSTypeError("Native clip.tracks assignment requires a native track-array handle");
}
JSValue nativeSettings(const AnimationSettings& settings) {
    if(const auto raw=std::get_if<JSValue>(&settings.storage()))return *raw;
    const auto& tangents=std::get<std::shared_ptr<AnimationSettings::Tangents>>(settings.storage());
    return wrap(tangents,settingsTag);
}
AnimationSettings readSettings(JSValue value) {
    if(!value.isHostObject(settingsTag))return AnimationSettings(std::move(value));
    // Preserve the native pair's identity, including both typed-array views.
    return AnimationSettings(payload<AnimationSettings::Tangents>(value,settingsTag));
}

std::vector<std::string> fields(const JSValue& self) {
    if(self.isHostObject(bindingTag)) {
        const auto b=payload<PropertyBinding>(self,bindingTag);
        std::vector<std::string> names{"path","parsedPath","node","rootNode","getValue","setValue"};
        for(const auto& [key,value]:std::initializer_list<std::pair<const char*,const JSValue*>>{
            {"targetObject",&b->targetObject},{"resolvedProperty",&b->resolvedProperty},{"propertyName",&b->propertyName},{"propertyIndex",&b->propertyIndex}})
            if(!value->isUndefined()||self.getHostSlot(std::string("present.")+key).toBoolean())names.push_back(key);
        for(const char* key:{"BindingType","Versioning","GetterByBindingType","SetterByBindingTypeAndVersioning"})
            if(self.getHostSlot(std::string("present.")+key).toBoolean())names.push_back(key);
        return names;
    }
    if(self.isHostObject(compositeTag))return {"_targetGroup","_bindings"};
    if(self.isHostObject(mixerTag)) {
        const auto m=payload<PropertyMixer>(self,mixerTag);
        std::vector<std::string> names{"binding","valueSize","buffer"};
        if(!m->_workIndex.isUndefined())names.push_back("_workIndex");
        for(const char* key:{"_mixBufferRegion","_mixBufferRegionAdditive","_setIdentity","_origIndex","_addIndex","cumulativeWeight","cumulativeWeightAdditive","useCount","referenceCount"})names.push_back(key);
        if(!m->_cacheIndex.isUndefined()||self.getHostSlot("present._cacheIndex").toBoolean())names.push_back("_cacheIndex");
        return names;
    }
    if(self.isHostObject(interpolantTag)) {
        const auto value=payload<Interpolant>(self,interpolantTag);
        std::vector<std::string> names{"parameterPositions","_cachedIndex","resultBuffer","sampleValues","valueSize","settings","DefaultSettings_"};
        if(!value->__cacheIndex.isUndefined()||self.getHostSlot("present.__cacheIndex").toBoolean())names.push_back("__cacheIndex");
        if(dynamic_cast<CubicInterpolant*>(value.get()))for(const char* n:{"_weightPrev","_offsetPrev","_weightNext","_offsetNext"})names.push_back(n);
        if(dynamic_cast<BezierInterpolant*>(value.get()))for(const char* n:{"inTangents","outTangents"})names.push_back(n);
        return names;
    }
    if(self.isHostObject(clipTag))return {"name","tracks","duration","blendMode","uuid","userData"};
    if(self.isHostObject(trackTag)) {
        const auto track=payload<KeyframeTrack>(self,trackTag);
        std::vector<std::string> names{"name","times","values","createInterpolant"};
        if(!track->settings.isUndefined())names.push_back("settings");
        return names;
    }
    if(self.isHostObject(settingsTag))return {"inTangents","outTangents"};
    return {};
}

JSValue read(const JSValue& self, const std::string& key) {
    if(self.isHostObject(bindingTag)) {
        const auto b=payload<PropertyBinding>(self,bindingTag);
#define FIELD(N) if(key==#N)return b->N;
        FIELD(path) FIELD(parsedPath) FIELD(node) FIELD(rootNode) FIELD(targetObject) FIELD(resolvedProperty) FIELD(propertyName) FIELD(propertyIndex)
        FIELD(BindingType) FIELD(Versioning)
#undef FIELD
        if(key=="getValue")return bindingCallback(b->getValueFunction);
        if(key=="setValue")return bindingCallback(b->setValueFunction);
        if(key=="GetterByBindingType")return nativeGetterTable(b->GetterByBindingType);
        if(key=="SetterByBindingTypeAndVersioning")return nativeSetterTable(b->SetterByBindingTypeAndVersioning);
    } else if(self.isHostObject(compositeTag)) {
        const auto b=payload<PropertyBindingComposite>(self,compositeTag);
        if(key=="_targetGroup")return b->_targetGroup;if(key=="_bindings")return b->_bindings;
    } else if(self.isHostObject(mixerTag)) {
        const auto m=payload<PropertyMixer>(self,mixerTag);
#define FIELD(N) if(key==#N)return m->N;
        FIELD(valueSize) FIELD(cumulativeWeight) FIELD(cumulativeWeightAdditive) FIELD(useCount) FIELD(referenceCount)
        FIELD(_origIndex) FIELD(_addIndex) FIELD(_workIndex) FIELD(_cacheIndex)
#undef FIELD
        if(key=="binding")return nativePropertyBinding(m->binding);
        if(key=="buffer")return nativeArray(m->buffer);
        if(key=="_mixBufferRegion")return mixerCallback(m->_mixBufferRegion);
        if(key=="_mixBufferRegionAdditive")return mixerCallback(m->_mixBufferRegionAdditive);
        if(key=="_setIdentity")return mixerCallback(m->_setIdentity);
    } else if(self.isHostObject(interpolantTag)) {
        const auto v=payload<Interpolant>(self,interpolantTag);
#define FIELD(N) if(key==#N)return v->N;
        FIELD(valueSize) FIELD(_cachedIndex) FIELD(settings) FIELD(DefaultSettings_) FIELD(__cacheIndex)
#undef FIELD
        if(key=="parameterPositions")return nativeArray(v->parameterPositions);
        if(key=="sampleValues")return nativeArray(v->sampleValues);
        if(key=="resultBuffer")return nativeArray(v->resultBuffer);
        if(auto c=dynamic_cast<CubicInterpolant*>(v.get())) {
#define FIELD(N) if(key==#N)return c->N;
            FIELD(_weightPrev) FIELD(_offsetPrev) FIELD(_weightNext) FIELD(_offsetNext)
#undef FIELD
        }
        if(auto b=dynamic_cast<BezierInterpolant*>(v.get())) {
            if(key=="inTangents")return nativeArray(b->inTangents);if(key=="outTangents")return nativeArray(b->outTangents);
        }
    } else if(self.isHostObject(clipTag)) {
        const auto c=payload<AnimationClip>(self,clipTag);
#define FIELD(N) if(key==#N)return c->N;
        FIELD(name) FIELD(uuid) FIELD(duration) FIELD(blendMode) FIELD(userData)
#undef FIELD
        if(key=="tracks")return nativeTracks(c->tracks);
    } else if(self.isHostObject(trackTag)) {
        const auto t=payload<KeyframeTrack>(self,trackTag);
        if(key=="name")return t->name;if(key=="times")return nativeArray(t->times);if(key=="values")return nativeArray(t->values);
        if(key=="settings")return nativeSettings(t->settings);
        if(key=="ValueTypeName")return t->ValueTypeName;if(key=="DefaultInterpolation")return t->DefaultInterpolation;
        if(key=="createInterpolant") {
            const auto fn=t->createInterpolant.function();
            if(!fn)return {};
            if(!fn->runtimeFunction.isUndefined())return fn->runtimeFunction;
            const auto interpolation=t->getInterpolation();
            const char* name=interpolation.strictEquals(InterpolateDiscrete)?"InterpolantFactoryMethodDiscrete":
                interpolation.strictEquals(InterpolateLinear)?"InterpolantFactoryMethodLinear":interpolation.strictEquals(InterpolateSmooth)?"InterpolantFactoryMethodSmooth":
                interpolation.strictEquals(InterpolateBezier)?"InterpolantFactoryMethodBezier":nullptr;
            if(name)return prototype(std::string(trackTag)+":"+t->ValueTypeName).get(name);
            throw JSTypeError("Custom native interpolant factories require a managed runtimeFunction");
        }
    } else if(self.isHostObject(settingsTag)) {
        const auto t=payload<AnimationSettings::Tangents>(self,settingsTag);
        return nativeArray(key=="inTangents"?t->in:t->out);
    }
    return {};
}

bool write(const JSValue& self,const std::string& key,JSValue value) {
    if(self.isHostObject(bindingTag)) {
        const auto b=payload<PropertyBinding>(self,bindingTag);
#define FIELD(N) if(key==#N){b->N=std::move(value);return true;}
        FIELD(path) FIELD(parsedPath) FIELD(node) FIELD(rootNode) FIELD(targetObject) FIELD(resolvedProperty) FIELD(propertyName) FIELD(propertyIndex)
        FIELD(BindingType) FIELD(Versioning)
#undef FIELD
        if(key=="getValue"||key=="setValue") {
            auto callback=PropertyBinding::Function::managed(value,[](const JSValue& fn,PropertyBinding& target,AnimationArray array,JSValue offset){
                // Alias the already-owned target only for the duration of this call.
                const auto self=nativePropertyBinding(target.shared_from_this());fn.call(self,{nativeArray(array),offset});
            });
            (key=="getValue"?b->getValueFunction:b->setValueFunction)=std::move(callback);return true;
        }
        if(key=="GetterByBindingType"){b->GetterByBindingType=GetterTable(payload<GetterTable::Storage>(value,gettersTag));return true;}
        if(key=="SetterByBindingTypeAndVersioning"){b->SetterByBindingTypeAndVersioning=SetterTable(payload<SetterTable::Storage>(value,settersTag));return true;}
    } else if(self.isHostObject(compositeTag)) {
        auto b=payload<PropertyBindingComposite>(self,compositeTag);if(key=="_targetGroup")b->_targetGroup=value;else if(key=="_bindings")b->_bindings=value;else return false;return true;
    } else if(self.isHostObject(mixerTag)) {
        auto m=payload<PropertyMixer>(self,mixerTag);
#define NUMBER(N) if(key==#N){m->N=value.toNumber();return true;}
        NUMBER(valueSize) NUMBER(useCount) NUMBER(referenceCount) NUMBER(_origIndex) NUMBER(_addIndex)
#undef NUMBER
#define FIELD(N) if(key==#N){m->N=std::move(value);return true;}
        FIELD(cumulativeWeight) FIELD(cumulativeWeightAdditive) FIELD(_workIndex) FIELD(_cacheIndex)
#undef FIELD
        if(key=="binding"){m->binding=propertyBindingFromNative(value);return true;}
        if(key=="buffer"){m->buffer=animationArray(value);return true;}
        if(key=="_mixBufferRegion"||key=="_mixBufferRegionAdditive") {
            auto fn=PropertyMixer::MixFunction::managed(value,[](const JSValue& fn,PropertyMixer& target,AnimationArray array,double dst,double src,JSValue t,double stride){
                const auto self=nativePropertyMixer(target.shared_from_this());fn.call(self,{nativeArray(array),dst,src,t,stride});
            });(key=="_mixBufferRegion"?m->_mixBufferRegion:m->_mixBufferRegionAdditive)=std::move(fn);return true;
        }
        if(key=="_setIdentity") {m->_setIdentity=PropertyMixer::IdentityFunction::managed(value,[](const JSValue& fn,PropertyMixer& target){
            const auto self=nativePropertyMixer(target.shared_from_this());fn.call(self);
        });return true;}
    } else if(self.isHostObject(interpolantTag)) {
        const auto v=payload<Interpolant>(self,interpolantTag);
        if(key=="parameterPositions")v->parameterPositions=animationArray(value);
        else if(key=="sampleValues")v->sampleValues=animationArray(value);
        else if(key=="resultBuffer")v->resultBuffer=animationArray(value);
        else if(key=="valueSize")v->valueSize=value.toNumber();
        else if(key=="_cachedIndex")v->_cachedIndex=indexValue(value);
        else if(key=="settings")v->settings=value;
        else if(key=="DefaultSettings_")v->DefaultSettings_=value;
        else if(key=="__cacheIndex")v->__cacheIndex=value;
        else if(auto c=dynamic_cast<CubicInterpolant*>(v.get())) {
#define NUMBER(N) if(key==#N){c->N=value.toNumber();return true;}
            NUMBER(_weightPrev) NUMBER(_offsetPrev) NUMBER(_weightNext) NUMBER(_offsetNext)
#undef NUMBER
            return false;
        } else if(auto b=dynamic_cast<BezierInterpolant*>(v.get())) {
            if(key=="inTangents")b->inTangents=animationArray(value);else if(key=="outTangents")b->outTangents=animationArray(value);else return false;
        } else return false;
        return true;
    } else if(self.isHostObject(clipTag)) {
        const auto c=payload<AnimationClip>(self,clipTag);
#define FIELD(N) if(key==#N){c->N=std::move(value);return true;}
        FIELD(name) FIELD(uuid) FIELD(blendMode) FIELD(userData)
#undef FIELD
        if(key=="duration")c->duration=value.toNumber();else if(key=="tracks")c->tracks=readTracks(value);else return false;return true;
    } else if(self.isHostObject(trackTag)) {
        const auto t=payload<KeyframeTrack>(self,trackTag);
        if(key=="name")t->name=value;else if(key=="times")t->times=animationArray(value);else if(key=="values")t->values=animationArray(value);
        else if(key=="settings")t->settings=readSettings(value);
        else if(key=="createInterpolant") {
            auto fn=std::make_shared<InterpolantFactory::Function>();fn->runtimeFunction=value;
            const std::weak_ptr<InterpolantFactory::Function> weak=fn;
            fn->callback=[weak](KeyframeTrack& track,AnimationArray buffer){const auto fn=weak.lock();
                const auto self=nativeKeyframeTrack(track.shared_from_this());
                return interpolantFromNative(fn->runtimeFunction.call(self,{nativeArray(buffer)}));};
            t->createInterpolant=std::move(fn);
        } else return false;return true;
    } else if(self.isHostObject(settingsTag)) {
        const auto t=payload<AnimationSettings::Tangents>(self,settingsTag);
        if(key!="inTangents"&&key!="outTangents")return false;
        (key=="inTangents"?t->in:t->out)=animationArray(value);return true;
    }
    return false;
}

JSValue invoke(const JSValue& self,const Args& args,const Args& captures) {
    const auto name=captures[0].string();
    if(self.isHostObject(bindingTag)) {
        const auto b=payload<PropertyBinding>(self,bindingTag);
        if(name=="bind"){b->bind();return {};}
        if(name=="unbind"){b->unbind();return {};}
        const auto array=animationArray(arg(args,0));const auto offset=arg(args,1);
#define CALL(N) if(name==#N){b->N(array,offset);return {};}
        CALL(_getValue_unbound) CALL(_setValue_unbound) CALL(_getValue_unavailable) CALL(_setValue_unavailable)
        CALL(_getValue_direct) CALL(_getValue_array) CALL(_getValue_arrayElement) CALL(_getValue_toArray)
        CALL(_setValue_direct) CALL(_setValue_array) CALL(_setValue_arrayElement) CALL(_setValue_fromArray)
        CALL(_setValue_direct_setNeedsUpdate) CALL(_setValue_direct_setMatrixWorldNeedsUpdate)
        CALL(_setValue_array_setNeedsUpdate) CALL(_setValue_array_setMatrixWorldNeedsUpdate)
        CALL(_setValue_arrayElement_setNeedsUpdate) CALL(_setValue_arrayElement_setMatrixWorldNeedsUpdate)
        CALL(_setValue_fromArray_setNeedsUpdate) CALL(_setValue_fromArray_setMatrixWorldNeedsUpdate)
#undef CALL
    } else if(self.isHostObject(compositeTag)) {
        const auto b=payload<PropertyBindingComposite>(self,compositeTag);
        if(name=="bind")b->bind();else if(name=="unbind")b->unbind();
        else if(name=="getValue")b->getValueAt(animationArray(arg(args,0)),arg(args,1));
        else if(name=="setValue")b->setValueAt(animationArray(arg(args,0)),arg(args,1));return {};
    } else if(self.isHostObject(mixerTag)) {
        const auto m=payload<PropertyMixer>(self,mixerTag);
        if(name=="accumulate")m->accumulate(arg(args,0),arg(args,1));
        else if(name=="accumulateAdditive")m->accumulateAdditive(arg(args,0));
        else if(name=="apply")m->apply(arg(args,0));
        else if(name=="saveOriginalState")m->saveOriginalState();else if(name=="restoreOriginalState")m->restoreOriginalState();
        else if(name=="_setAdditiveIdentityNumeric")m->_setAdditiveIdentityNumeric();
        else if(name=="_setAdditiveIdentityQuaternion")m->_setAdditiveIdentityQuaternion();
        else if(name=="_setAdditiveIdentityOther")m->_setAdditiveIdentityOther();
        else {auto b=animationArray(arg(args,0));const auto dst=arg(args,1).toNumber(),src=arg(args,2).toNumber();
            const auto t=arg(args,3);if(name=="_slerp")m->_slerp(b,dst,src,t);
            else {const auto stride=arg(args,4).toNumber();
                if(name=="_select")m->_select(b,dst,src,t,stride);else if(name=="_lerp")m->_lerp(b,dst,src,t,stride);
                else if(name=="_lerpAdditive")m->_lerpAdditive(b,dst,src,t,stride);else if(name=="_slerpAdditive")m->_slerpAdditive(b,dst,src,t,stride);
            }
        }return {};
    } else if(self.isHostObject(interpolantTag)) {
        const auto v=payload<Interpolant>(self,interpolantTag);
        if(name=="evaluate")return nativeArray(v->evaluate(arg(args,0).toNumber()));
        if(name=="getSettings_")return v->getSettings_();
        if(name=="copySampleValue_")return nativeArray(v->copySampleValue_(indexValue(arg(args,0))));
        const auto i=indexValue(arg(args,0));const auto t0=arg(args,1);
        if(name=="intervalChanged_"){v->intervalChanged_(i,t0,arg(args,2));return {};}
        return nativeArray(v->interpolate_(i,t0,arg(args,2).toNumber(),arg(args,3)));
    } else if(self.isHostObject(clipTag)) {
        const auto c=payload<AnimationClip>(self,clipTag);
        if(name=="resetDuration")c->resetDuration();else if(name=="trim")c->trim();else if(name=="optimize")c->optimize();
        else if(name=="validate")return c->validate();else if(name=="clone")return nativeAnimationClip(c->clone());else if(name=="toJSON")return c->toJSON();return self;
    } else if(self.isHostObject(trackTag)) {
        const auto t=payload<KeyframeTrack>(self,trackTag);
        if(name=="getValueSize")return t->getValueSize();if(name=="getInterpolation")return t->getInterpolation();
        if(name=="validate")return t->validate();if(name=="clone")return nativeKeyframeTrack(t->clone());
        if(name=="setInterpolation")t->setInterpolation(arg(args,0));else if(name=="shift")t->shift(arg(args,0));else if(name=="scale")t->scale(arg(args,0));
        else if(name=="trim"){const auto start=arg(args,0).toNumber(),end=arg(args,1).toNumber();t->trim(start,end);}
        else if(name=="optimize")t->optimize();
        else {const auto buffer=animationArray(arg(args,0));
            if(name=="InterpolantFactoryMethodDiscrete")return nativeInterpolant(t->InterpolantFactoryMethodDiscrete(buffer));
            if(name=="InterpolantFactoryMethodLinear")return nativeInterpolant(t->InterpolantFactoryMethodLinear(buffer));
            if(name=="QuaternionFactoryLinear")return nativeInterpolant(std::make_shared<QuaternionLinearInterpolant>(t->times,t->values,t->getValueSize(),buffer));
            if(name=="InterpolantFactoryMethodSmooth")return nativeInterpolant(t->InterpolantFactoryMethodSmooth(buffer));
            if(name=="InterpolantFactoryMethodBezier")return nativeInterpolant(t->InterpolantFactoryMethodBezier(buffer));
        }return self;
    }
    throw JSTypeError("Incompatible native animation method receiver");
}

JSValue prototype(const std::string& tag) {
    static thread_local std::map<std::string,JSValue> prototypes;
    if(const auto found=prototypes.find(tag);found!=prototypes.end())return found->second;
    auto value=JSValue::object();prototypes.emplace(tag,value);
    const auto methods=[&](std::initializer_list<const char*> names){for(const auto name:names){
        const std::string n=name;unsigned arity=0;
        if(tag==bindingTag&&n!="bind"&&n!="unbind")arity=2;
        if(tag==compositeTag&&(n=="getValue"||n=="setValue"))arity=2;
        if(tag==mixerTag){if(n=="accumulate")arity=2;else if(n=="apply"||n=="accumulateAdditive")arity=1;
            else if(n=="_slerp")arity=4;else if(n=="_select"||n=="_slerpAdditive"||n=="_lerp"||n=="_lerpAdditive")arity=5;}
        if(tag==interpolantTag){if(n=="evaluate"||n=="copySampleValue_")arity=1;else if(n=="interpolate_")arity=4;else if(n=="intervalChanged_")arity=3;}
        if(tag==trackTag){if(n.starts_with("InterpolantFactoryMethod")||n=="setInterpolation"||n=="shift"||n=="scale")arity=1;else if(n=="trim")arity=2;}
        value.defineProperty(name,descriptor(JSValue::function(invoke,{name},name,arity),false));
    }};
    if(tag==bindingTag)methods({"bind","unbind","_getValue_unbound","_setValue_unbound","_getValue_unavailable","_setValue_unavailable",
        "_getValue_direct","_getValue_array","_getValue_arrayElement","_getValue_toArray","_setValue_direct","_setValue_array","_setValue_arrayElement","_setValue_fromArray",
        "_setValue_direct_setNeedsUpdate","_setValue_direct_setMatrixWorldNeedsUpdate","_setValue_array_setNeedsUpdate","_setValue_array_setMatrixWorldNeedsUpdate",
        "_setValue_arrayElement_setNeedsUpdate","_setValue_arrayElement_setMatrixWorldNeedsUpdate","_setValue_fromArray_setNeedsUpdate","_setValue_fromArray_setMatrixWorldNeedsUpdate"});
    else if(tag==compositeTag)methods({"getValue","setValue","bind","unbind"});
    else if(tag==mixerTag)methods({"accumulate","accumulateAdditive","apply","saveOriginalState","restoreOriginalState","_setAdditiveIdentityNumeric",
        "_setAdditiveIdentityQuaternion","_setAdditiveIdentityOther","_select","_slerp","_slerpAdditive","_lerp","_lerpAdditive"});
    else if(tag==interpolantTag)methods({"evaluate","getSettings_","copySampleValue_","interpolate_","intervalChanged_"});
    else if(tag==clipTag)methods({"resetDuration","trim","validate","optimize","clone","toJSON"});
    else if(tag==trackTag)methods({"InterpolantFactoryMethodDiscrete","InterpolantFactoryMethodLinear","InterpolantFactoryMethodSmooth","InterpolantFactoryMethodBezier",
        "setInterpolation","getInterpolation","getValueSize","shift","scale","trim","validate","optimize","clone"});
    else if(tag.starts_with(std::string(trackTag)+":")) {
        value.setPrototype(prototype(trackTag));
        const auto type=tag.substr(std::string(trackTag).size()+1);
        value.set("ValueTypeName",type);
        value.set("DefaultInterpolation",type=="bool"||type=="string"?InterpolateDiscrete:InterpolateLinear);
        if(type=="quaternion")value.defineProperty("InterpolantFactoryMethodLinear",descriptor(JSValue::function(invoke,{"QuaternionFactoryLinear"},"InterpolantFactoryMethodLinear",1),false));
    }
    if(tag==bindingTag){PropertyBinding defaults(JSValue::object(),".x");value.set("BindingType",defaults.BindingType);value.set("Versioning",defaults.Versioning);
        value.set("GetterByBindingType",nativeGetterTable(defaults.GetterByBindingType));value.set("SetterByBindingTypeAndVersioning",nativeSetterTable(defaults.SetterByBindingTypeAndVersioning));}
    return value;
}

// Each native strong reference is listed by its actual storage slot. Root fields
// are borrowed addresses, never JSValue copies, so GC can distinguish native
// internal edges from independent C++ handles.
void bindingOwnership(std::vector<JSNativeOwnerNode>& nodes,const std::shared_ptr<PropertyBindingInterface>& owner) {
    if(!owner)return;
    JSNativeOwnerNode node{owner,{},{}};
    if(const auto b=dynamic_cast<PropertyBinding*>(owner.get())) {
        node.roots={&b->path,&b->parsedPath,&b->node,&b->rootNode,&b->targetObject,&b->resolvedProperty,&b->propertyName,&b->propertyIndex,&b->BindingType,&b->Versioning};
        callbackOwner(nodes,node,b->getValueFunction);callbackOwner(nodes,node,b->setValueFunction);
        const auto getters=b->GetterByBindingType.storage();nativeEdge(node,&b->GetterByBindingType,getters);
        JSNativeOwnerNode getterNode{getters,{},{}};for(const auto& fn:getters->values)callbackOwner(nodes,getterNode,fn);nodes.push_back(std::move(getterNode));
        const auto setters=b->SetterByBindingTypeAndVersioning.storage();nativeEdge(node,&b->SetterByBindingTypeAndVersioning,setters);
        JSNativeOwnerNode setterNode{setters,{},{}};
        for(const auto& row:setters->values){const auto storage=row.storage();nativeEdge(setterNode,&row,storage);JSNativeOwnerNode rowNode{storage,{},{}};
            for(const auto& fn:storage->values)callbackOwner(nodes,rowNode,fn);nodes.push_back(std::move(rowNode));}
        nodes.push_back(std::move(setterNode));
    } else if(const auto b=dynamic_cast<PropertyBindingComposite*>(owner.get()))node.roots={&b->_targetGroup,&b->_bindings};
    nodes.push_back(std::move(node));
}
void trackOwnership(std::vector<JSNativeOwnerNode>& nodes,const std::shared_ptr<KeyframeTrack>& owner) {
    if(!owner)return;JSNativeOwnerNode node{owner,{&owner->name,&owner->DefaultInterpolation},{}};
    arrayOwnership(nodes,node,owner->times);arrayOwnership(nodes,node,owner->values);
    if(const auto value=std::get_if<JSValue>(&owner->settings.storage()))node.roots.push_back(value);
    else {const auto& tangents=std::get<std::shared_ptr<AnimationSettings::Tangents>>(owner->settings.storage());nativeEdge(node,&owner->settings,tangents);
        JSNativeOwnerNode tangentNode{tangents,{},{}};arrayOwnership(nodes,tangentNode,tangents->in);arrayOwnership(nodes,tangentNode,tangents->out);nodes.push_back(std::move(tangentNode));}
    const auto factory=owner->createInterpolant.function();nativeEdge(node,&owner->createInterpolant,factory);
    if(factory)nodes.push_back({factory,{&factory->runtimeFunction},{}});
    nodes.push_back(std::move(node));
}
void tracksOwnership(std::vector<JSNativeOwnerNode>& nodes,const AnimationClip::TracksPtr& tracks) {
    if(!tracks)return;JSNativeOwnerNode node{tracks,{},{}};
    for(const auto& track:*tracks){nativeEdge(node,&track,track);trackOwnership(nodes,track);}nodes.push_back(std::move(node));
}
void clipOwnership(std::vector<JSNativeOwnerNode>& nodes,const std::shared_ptr<AnimationClip>& c) {
    if(!c)return;JSNativeOwnerNode node{c,{&c->name,&c->uuid,&c->blendMode,&c->userData},{}};
    nativeEdge(node,&c->tracks,c->tracks);tracksOwnership(nodes,c->tracks);nodes.push_back(std::move(node));
}
std::vector<JSNativeOwnerNode> ownership(const JSValue& self) {
    std::vector<JSNativeOwnerNode> nodes;
    if(self.isHostObject(bindingTag)||self.isHostObject(compositeTag))bindingOwnership(nodes,propertyBindingFromNative(self));
    else if(self.isHostObject(mixerTag)) {
        const auto m=payload<PropertyMixer>(self,mixerTag);JSNativeOwnerNode node{m,{&m->cumulativeWeight,&m->cumulativeWeightAdditive,&m->_workIndex,&m->_cacheIndex},{}};
        arrayOwnership(nodes,node,m->buffer);nativeEdge(node,&m->binding,m->binding);bindingOwnership(nodes,m->binding);
        callbackOwner(nodes,node,m->_mixBufferRegion);callbackOwner(nodes,node,m->_mixBufferRegionAdditive);callbackOwner(nodes,node,m->_setIdentity);nodes.push_back(std::move(node));
    } else if(self.isHostObject(interpolantTag)) {
        const auto v=payload<Interpolant>(self,interpolantTag);JSNativeOwnerNode node{v,{&v->settings,&v->DefaultSettings_,&v->__cacheIndex,&v->runtimeReceiver},{}};
        arrayOwnership(nodes,node,v->parameterPositions);arrayOwnership(nodes,node,v->resultBuffer);arrayOwnership(nodes,node,v->sampleValues);
        if(const auto b=dynamic_cast<BezierInterpolant*>(v.get())){arrayOwnership(nodes,node,b->inTangents);arrayOwnership(nodes,node,b->outTangents);}nodes.push_back(std::move(node));
    } else if(self.isHostObject(clipTag)) {
        clipOwnership(nodes,payload<AnimationClip>(self,clipTag));
    } else if(self.isHostObject(trackTag))trackOwnership(nodes,payload<KeyframeTrack>(self,trackTag));
    else if(self.isHostObject(tracksTag))tracksOwnership(nodes,payload<AnimationClip::Tracks>(self,tracksTag));
    else if(self.isHostObject(settingsTag)) {const auto t=payload<AnimationSettings::Tangents>(self,settingsTag);JSNativeOwnerNode node{t,{},{}};
        arrayOwnership(nodes,node,t->in);arrayOwnership(nodes,node,t->out);nodes.push_back(std::move(node));}
    else if(self.isHostObject(clipsTag)){const auto clips=payload<AnimationClip::Clips>(self,clipsTag);nodes=nativeAnimationClipOwners(clips,*clips);}
    else if(self.isHostObject(gettersTag)){const auto s=payload<GetterTable::Storage>(self,gettersTag);JSNativeOwnerNode n{s,{},{}};
        for(const auto& fn:s->values)callbackOwner(nodes,n,fn);nodes.push_back(std::move(n));}
    else if(self.isHostObject(settersTag)){const auto s=payload<SetterTable::Storage>(self,settersTag);JSNativeOwnerNode n{s,{},{}};
        for(const auto& row:s->values){const auto values=row.storage();nativeEdge(n,&row,values);JSNativeOwnerNode r{values,{},{}};
            for(const auto& fn:values->values)callbackOwner(nodes,r,fn);nodes.push_back(std::move(r));}nodes.push_back(std::move(n));}
    return nodes;
}
std::vector<JSValue> trace(const JSValue& self) {
    std::vector<JSValue> result;
    const auto edge=[&](const void* identity,const char* tag,JSValue::Type type=JSValue::Type::Object){
        const auto value=self.findHostObject(identity,tag,type);if(!value.isUndefined())result.push_back(value);
    };
    const auto array=[&](const AnimationArray& a){std::visit([&](const auto& v){
        if constexpr(!std::is_same_v<std::decay_t<decltype(v)>,JSValue>)edge(v.identity(),"three.AnimationArray");
    },a.storage());};
    const auto track=[&](const std::shared_ptr<KeyframeTrack>& t){
        if(!t)return;edge(t.get(),trackTag);array(t->times);array(t->values);
        if(const auto settings=std::get_if<std::shared_ptr<AnimationSettings::Tangents>>(&t->settings.storage())) {
            edge(settings->get(),settingsTag);array((*settings)->in);array((*settings)->out);
        }
    };
    if(self.isHostObject(bindingTag)){
        const auto b=payload<PropertyBinding>(self,bindingTag);edge(b->GetterByBindingType.storage().get(),gettersTag,JSValue::Type::Array);
        edge(b->SetterByBindingTypeAndVersioning.storage().get(),settersTag,JSValue::Type::Array);
        for(const auto& row:b->SetterByBindingTypeAndVersioning)edge(row.storage().get(),gettersTag,JSValue::Type::Array);
    } else if(self.isHostObject(interpolantTag)) {
        const auto v=payload<Interpolant>(self,interpolantTag);array(v->parameterPositions);array(v->sampleValues);array(v->resultBuffer);
        if(const auto b=dynamic_cast<BezierInterpolant*>(v.get())){array(b->inTangents);array(b->outTangents);}
    } else if(self.isHostObject(mixerTag)) {
        const auto m=payload<PropertyMixer>(self,mixerTag);array(m->buffer);
        if(m->binding)edge(m->binding.get(),dynamic_cast<PropertyBinding*>(m->binding.get())?bindingTag:compositeTag);
    } else if(self.isHostObject(trackTag))track(payload<KeyframeTrack>(self,trackTag));
    else if(self.isHostObject(clipTag)) {
        const auto c=payload<AnimationClip>(self,clipTag);if(c->tracks){edge(c->tracks.get(),tracksTag,JSValue::Type::Array);for(const auto& t:*c->tracks)track(t);}
    } else if(self.isHostObject(tracksTag)){for(const auto& t:*payload<AnimationClip::Tracks>(self,tracksTag))track(t);}
    else if(self.isHostObject(settingsTag)){const auto t=payload<AnimationSettings::Tangents>(self,settingsTag);array(t->in);array(t->out);}
    else if(self.isHostObject(clipsTag)){for(const auto& c:*payload<AnimationClip::Clips>(self,clipsTag))if(c)edge(c.get(),clipTag);}
    return result;
}
} // namespace

JSValue nativePropertyBinding(const std::shared_ptr<PropertyBindingInterface>& value) {
    if(!value)return nullptr;
    if(const auto binding=std::dynamic_pointer_cast<PropertyBinding>(value))return wrap(binding,bindingTag);
    if(const auto composite=std::dynamic_pointer_cast<PropertyBindingComposite>(value))return wrap(composite,compositeTag);
    throw JSTypeError("Unsupported native PropertyBindingInterface subclass");
}
JSValue nativePropertyMixer(const std::shared_ptr<PropertyMixer>& value){return wrap(value,mixerTag);}
JSValue nativeInterpolant(const std::shared_ptr<Interpolant>& value){
    auto result=wrap(value,interpolantTag);
    if(value){
        static const Interpolant::RuntimeMethods methods{
            [](JSValue self,std::ptrdiff_t index){const auto fn=self.get("copySampleValue_");return animationArray(fn.call(self,{double(index)}));},
            [](JSValue self,std::ptrdiff_t i,JSValue t0,double t,JSValue t1){const auto fn=self.get("interpolate_");return animationArray(fn.call(self,{double(i),t0,t,t1}));},
            [](JSValue self,std::ptrdiff_t i,JSValue t0,JSValue t1){const auto fn=self.get("intervalChanged_");fn.call(self,{double(i),t0,t1});},
            [](JSValue self){const auto fn=self.get("getSettings_");return fn.call(self);}
        };
        value->runtimeReceiver=result;value->runtimeMethods=&methods;
    }
    return result;
}
JSValue nativeAnimationClip(const std::shared_ptr<AnimationClip>& value){return wrap(value,clipTag);}
JSValue nativeKeyframeTrack(const std::shared_ptr<KeyframeTrack>& value){auto result=wrap(value,trackTag);
    if(value&&result.getHostSlot("animation.trackPrototype").isUndefined()){
        result.setPrototype(prototype(std::string(trackTag)+":"+value->ValueTypeName));result.setHostSlot("animation.trackPrototype",true);
    }return result;}
std::shared_ptr<PropertyBindingInterface> propertyBindingFromNative(const JSValue& value) {
    if(value.isNull())return {};
    if(value.isHostObject(bindingTag))return payload<PropertyBinding>(value,bindingTag);
    return payload<PropertyBindingComposite>(value,compositeTag);
}
std::shared_ptr<PropertyMixer> propertyMixerFromNative(const JSValue& value){return value.isNull()?nullptr:payload<PropertyMixer>(value,mixerTag);}
std::shared_ptr<Interpolant> interpolantFromNative(const JSValue& value){return value.isNull()?nullptr:payload<Interpolant>(value,interpolantTag);}
std::shared_ptr<AnimationClip> animationClipFromNative(const JSValue& value){return value.isNull()?nullptr:payload<AnimationClip>(value,clipTag);}
std::shared_ptr<KeyframeTrack> keyframeTrackFromNative(const JSValue& value){return value.isNull()?nullptr:payload<KeyframeTrack>(value,trackTag);}

std::vector<JSNativeOwnerNode> nativeAnimationClipOwners(const std::shared_ptr<void>& owner,const AnimationClip::Clips& clips) {
    std::vector<JSNativeOwnerNode> nodes;JSNativeOwnerNode node{owner,{},{}};
    for(const auto& clip:clips){nativeEdge(node,&clip,clip);clipOwnership(nodes,clip);}nodes.push_back(std::move(node));return nodes;
}
JSValue nativeAnimationClips(const std::shared_ptr<AnimationClip::Clips>& clips) {
    if(!clips)return {};
    auto result=JSValue::hostObject(clips,clips.get(),clipsTag,JSValue::Type::Array);
    if(!result.getHostSlot("animation.initialized").isUndefined())return result;
    result.setHostSlot("animation.initialized",true);
    static const auto hooks=[] {auto h=std::make_shared<JSHostObjectHooks>();
        h->getOwnProperty=[](const JSValue& self,const std::string& key)->JSHostPropertyResult{
            const auto clips=payload<AnimationClip::Clips>(self,clipsTag);
            if(key=="length"){auto d=descriptor(double(clips->size()),false);d.configurable=false;return {true,d};}
            const auto index=arrayIndex(key);if(!index)return {};
            return *index<clips->size()?JSHostPropertyResult{true,descriptor(nativeAnimationClip((*clips)[*index]))}:JSHostPropertyResult{true,{}};
        };
        h->defineOwnProperty=[](const JSValue& self,const std::string& key,const JSPropertyDescriptor& d)->std::optional<bool>{
            const auto index=arrayIndex(key);if(key!="length"&&!index)return {};
            if(d.getter||d.setter||(d.writable&&!*d.writable)||(d.enumerable&&*d.enumerable!=(key!="length"))||
               (d.configurable&&*d.configurable!=(key!="length")))throw JSTypeError("Native clip arrays require ordinary data descriptors");
            if(!d.value)return true;const auto clips=payload<AnimationClip::Clips>(self,clipsTag);
            if(key=="length"){const auto n=d.value->toNumber();if(!std::isfinite(n)||n<0||n>UINT32_MAX||std::trunc(n)!=n)throw JSRangeError("Invalid array length");clips->resize(std::size_t(n));}
            else {const auto clip=animationClipFromNative(*d.value);if(*index>=clips->size())clips->resize(*index+1);(*clips)[*index]=clip;}return true;
        };
        h->ownPropertyNames=[](const JSValue& self){std::vector<std::string> result;const auto clips=payload<AnimationClip::Clips>(self,clipsTag);
            for(std::size_t i=0;i<clips->size();++i)result.push_back(std::to_string(i));result.push_back("length");return result;};
        h->erase=nativeSequenceErase;h->nativeOwnership=ownership;h->trace=trace;return h;
    }();result.setHostHooks(hooks);return result;
}
