#include "animation/AnimationMixer.h"
#include "animation/AnimationClip.h"
#include "animation/AnimationRuntime.h"
#include "animation/PropertyBinding.h"
#include "core/EventDispatcher.h"
#include "core/JSOperators.h"
#include "core/NativeObject.h"
#include "math/interpolants/LinearInterpolant.h"
#include <cmath>
#include <set>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue(); }
JSValue get(JSValue object, JSValue key) {
    if (object.isNull() || object.isUndefined()) throw JSTypeError("Cannot read property of null/undefined");
    return object.get(key.toString());
}
void put(JSValue object, JSValue key, JSValue value) {
    if (object.isNull() || object.isUndefined()) throw JSTypeError("Cannot write property of null/undefined");
    object.set(key.toString(),value);
}
JSValue invoke(JSValue object, const char* name, const Args& args = {}) {
    const auto method = object.get(name); return method.call(object,args);
}
JSValue rootOf(JSValue action, JSValue mixer) {
    const auto root = action.get("_localRoot"); return root.toBoolean() ? root : mixer.get("_root");
}
JSValue countChange(JSValue object, const char* name, double delta, bool previous = false) {
    const auto old = object.get(name).toNumber(); object.set(name,old + delta); return previous ? old : old + delta;
}
void progress(double index) { if (index + 1 == index) throw JSRangeError("Animation cache loop cannot progress"); }
JSValue last(JSValue array) { const auto length = array.get("length"); return get(array, Interpolant_detail::subtract(length,1)); }
JSValue statsGetter(const JSValue&, const Args&, const Args& captures) {
    const auto field = captures[0].get(captures[1].string()); return captures[2].toBoolean() ? field.get("length") : field;
}
JSValue mixerMethod(const JSValue&, const Args&, const Args&);
JSValue makeStats(JSValue self, const char* array, const char* active) {
    auto stats = JSValue::object();
    for (bool total : {true,false}) {
        JSPropertyDescriptor descriptor;
        descriptor.getter = JSValue::function(statsGetter,{self,total ? array : active,total});
        descriptor.enumerable = true; descriptor.configurable = true; stats.defineProperty(total ? "total" : "inUse",descriptor);
    }
    return stats;
}
void initMemory(JSValue self) {
    self.set("_actions",JSValue::array()); self.set("_nActiveActions",0); self.set("_actionsByClip",JSValue::object());
    self.set("_bindings",JSValue::array()); self.set("_nActiveBindings",0); self.set("_bindingsByRootAndName",JSValue::object());
    self.set("_controlInterpolants",JSValue::array()); self.set("_nActiveControlInterpolants",0);
    auto stats = JSValue::object();
    stats.set("actions",makeStats(self,"_actions","_nActiveActions"));
    stats.set("bindings",makeStats(self,"_bindings","_nActiveBindings"));
    stats.set("controlInterpolants",makeStats(self,"_controlInterpolants","_nActiveControlInterpolants")); self.set("stats",stats);
}
JSValue enumerableReceiver(JSValue value) {
    if (value.isObject()) return value;
    if (value.isNull() || value.isUndefined()) return JSValue::object(nullptr);
    auto boxed = JSValue::object();
    if (value.type() == JSValue::Type::String) {
        const auto length = value.get("length").toNumber();
        for (double i = 0; i < length; ++i) {
            const auto key = JSValue(i).toString(); JSPropertyDescriptor descriptor;
            descriptor.value = value.get(key); descriptor.writable = false;
            descriptor.enumerable = true; descriptor.configurable = false;
            boxed.defineProperty(key,descriptor);
        }
        JSPropertyDescriptor descriptor; descriptor.value = length; descriptor.writable = false;
        descriptor.enumerable = false; descriptor.configurable = false; boxed.defineProperty("length",descriptor);
    }
    return boxed;
}
std::vector<std::string> enumerableKeys(JSValue object) {
    std::vector<std::string> result; std::set<std::string> visited;
    while (!object.isNull()) {
        for (const auto& key : object.ownPropertyNames()) {
            if (!visited.insert(key).second) continue;
            const auto d = object.getOwnPropertyDescriptor(key);
            if (d && d->enumerable.value_or(false)) result.push_back(key);
        }
        object = object.getPrototype();
    }
    return result;
}
JSValue mixerMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto method = captures[0].string(); auto action = arg(args,0);
    if (method == "_initMemoryManager") { initMemory(self); return {}; }
    if (method == "_isActiveAction") {
        const auto index = action.get("_cacheIndex");
        return !index.isNull() && JSOperators::less(index,self.get("_nActiveActions"));
    }
    if (method == "_bindAction") {
        const auto root = rootOf(action,self); const auto tracks = action.get("_clip").get("tracks");
        const auto count = tracks.get("length"), bindings = action.get("_propertyBindings"), interpolants = action.get("_interpolants");
        const auto uuid = root.get("uuid"), byRoot = self.get("_bindingsByRootAndName");
        auto byName = get(byRoot,uuid);
        if (byName.isUndefined()) { byName = JSValue::object(); put(byRoot,uuid,byName); }
        for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
            const auto track = get(tracks,i), name = track.get("name"); auto binding = get(byName,name);
            if (!binding.isUndefined()) { countChange(binding,"referenceCount",1); put(bindings,i,binding); }
            else {
                binding = get(bindings,i);
                if (!binding.isUndefined()) {
                    if (binding.get("_cacheIndex").isNull()) { countChange(binding,"referenceCount",1); invoke(self,"_addInactiveBinding",{binding,uuid,name}); }
                    progress(i); continue;
                }
                const auto prototype = arg(args,1);
                auto path = prototype;
                if (prototype.toBoolean()) path = get(prototype.get("_propertyBindings"),i).get("binding").get("parsedPath");
                const auto propBinding = PropertyBinding::create(root,name,path);
                const auto type = track.get("ValueTypeName"); const auto size = invoke(track,"getValueSize").toNumber();
                binding = nativePropertyMixer(std::make_shared<PropertyMixer>(propBinding,type,size));
                countChange(binding,"referenceCount",1); invoke(self,"_addInactiveBinding",{binding,uuid,name}); put(bindings,i,binding);
            }
            auto interpolant = get(interpolants,i); interpolant.set("resultBuffer",binding.get("buffer")); progress(i);
        }
        return {};
    }
    if (method == "_activateAction" || method == "_deactivateAction") {
        const bool activate = method == "_activateAction";
        const bool active = invoke(self,"_isActiveAction",{action}).toBoolean();
        if (activate == active) return {};
        if (activate && action.get("_cacheIndex").isNull()) {
            const auto rootUuid = rootOf(action,self).get("uuid"), clipUuid = action.get("_clip").get("uuid");
            const auto actionsForClip = get(self.get("_actionsByClip"),clipUuid);
            const auto callback = self.get("_bindAction"); auto prototype = actionsForClip;
            if (prototype.toBoolean()) prototype = get(actionsForClip.get("knownActions"),0);
            callback.call(self,{action,prototype}); invoke(self,"_addInactiveAction",{action,clipUuid,rootUuid});
        }
        const auto bindings = action.get("_propertyBindings"), count = bindings.get("length");
        for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
            const auto binding = get(bindings,i);
            if (countChange(binding,"useCount",activate ? 1 : -1,activate).strictEquals(0)) {
                if (activate) { invoke(self,"_lendBinding",{binding}); invoke(binding,"saveOriginalState"); }
                else { invoke(binding,"restoreOriginalState"); invoke(self,"_takeBackBinding",{binding}); }
            }
            progress(i);
        }
        invoke(self,activate ? "_lendAction" : "_takeBackAction",{action}); return {};
    }
    if (method == "_addInactiveAction") {
        const auto actions = self.get("_actions"), byClip = self.get("_actionsByClip");
        const auto clipUuid = arg(args,1), rootUuid = arg(args,2); auto clip = get(byClip,clipUuid);
        if (clip.isUndefined()) {
            clip = JSValue::object(); auto known = JSValue::array(); known.set("0",action);
            clip.set("knownActions",known); clip.set("actionByRoot",JSValue::object());
            action.set("_byClipCacheIndex",0); put(byClip,clipUuid,clip);
        } else {
            const auto known = clip.get("knownActions"); action.set("_byClipCacheIndex",known.get("length")); invoke(known,"push",{action});
        }
        action.set("_cacheIndex",actions.get("length")); invoke(actions,"push",{action}); put(clip.get("actionByRoot"),rootUuid,action); return {};
    }
    if (method == "_removeInactiveAction") {
        const auto actions = self.get("_actions"); auto inactive = last(actions); const auto index = action.get("_cacheIndex");
        inactive.set("_cacheIndex",index); put(actions,index,inactive); invoke(actions,"pop"); action.set("_cacheIndex",nullptr);
        const auto uuid = action.get("_clip").get("uuid"), byClip = self.get("_actionsByClip"), clip = get(byClip,uuid), known = clip.get("knownActions");
        auto lastKnown = last(known); const auto byIndex = action.get("_byClipCacheIndex");
        lastKnown.set("_byClipCacheIndex",byIndex); put(known,byIndex,lastKnown); invoke(known,"pop"); action.set("_byClipCacheIndex",nullptr);
        auto byRoot = clip.get("actionByRoot"); const auto rootUuid = rootOf(action,self).get("uuid");
        if (!byRoot.erase(rootUuid.toString())) throw JSTypeError("Cannot delete action root cache");
        if (known.get("length").strictEquals(0)) { auto object = byClip; if (!object.erase(uuid.toString())) throw JSTypeError("Cannot delete clip cache"); }
        invoke(self,"_removeInactiveBindingsForAction",{action}); return {};
    }
    if (method == "_removeInactiveBindingsForAction") {
        const auto bindings = action.get("_propertyBindings"), count = bindings.get("length");
        for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
            const auto binding = get(bindings,i);
            if (countChange(binding,"referenceCount",-1).strictEquals(0)) invoke(self,"_removeInactiveBinding",{binding});
            progress(i);
        }
        return {};
    }
    if (method == "_lendAction" || method == "_takeBackAction" || method == "_lendBinding" || method == "_takeBackBinding" || method == "_takeBackControlInterpolant") {
        const bool binding = method == "_lendBinding" || method == "_takeBackBinding";
        const bool control = method == "_takeBackControlInterpolant", lend = method == "_lendAction" || method == "_lendBinding";
        const auto array = self.get(control ? "_controlInterpolants" : binding ? "_bindings" : "_actions");
        const auto key = control ? "__cacheIndex" : "_cacheIndex"; const auto previous = action.get(key);
        const auto index = countChange(self,control ? "_nActiveControlInterpolants" : binding ? "_nActiveBindings" : "_nActiveActions",lend ? 1 : -1,lend);
        auto other = get(array,index); action.set(key,index); put(array,index,action); other.set(key,previous); put(array,previous,other); return {};
    }
    if (method == "_addInactiveBinding") {
        const auto byRoot = self.get("_bindingsByRootAndName"), bindings = self.get("_bindings");
        const auto rootUuid = arg(args,1), name = arg(args,2); auto byName = get(byRoot,rootUuid);
        if (byName.isUndefined()) { byName = JSValue::object(); put(byRoot,rootUuid,byName); }
        put(byName,name,action); action.set("_cacheIndex",bindings.get("length")); invoke(bindings,"push",{action}); return {};
    }
    if (method == "_removeInactiveBinding") {
        const auto bindings = self.get("_bindings"), prop = action.get("binding"), rootUuid = prop.get("rootNode").get("uuid"), name = prop.get("path");
        auto byRoot = self.get("_bindingsByRootAndName"), byName = get(byRoot,rootUuid), inactive = last(bindings); const auto index = action.get("_cacheIndex");
        inactive.set("_cacheIndex",index); put(bindings,index,inactive); invoke(bindings,"pop");
        if (!byName.erase(name.toString())) throw JSTypeError("Cannot delete binding cache");
        if (byName.keys().empty() && !byRoot.erase(rootUuid.toString())) throw JSTypeError("Cannot delete binding root cache");
        return {};
    }
    if (method == "_lendControlInterpolant") {
        const auto array = self.get("_controlInterpolants"); const auto index = countChange(self,"_nActiveControlInterpolants",1,true); auto interpolant = get(array,index);
        if (interpolant.isUndefined()) {
            static thread_local Float32Array result(1);
            interpolant = nativeInterpolant(std::make_shared<LinearInterpolant>(Float32Array(2),Float32Array(2),1,result));
            interpolant.set("__cacheIndex",index); put(array,index,interpolant);
        }
        return interpolant;
    }
    if (method == "clipAction" || method == "existingAction") {
        const auto optionalRoot = arg(args,1); const auto root = optionalRoot.toBoolean() ? optionalRoot : self.get("_root"); const auto rootUuid = root.get("uuid");
        auto clip = action.type() == JSValue::Type::String ? AnimationClip::findByName(root,action) : action;
        const auto uuid = (method == "existingAction" ? clip.toBoolean() : !clip.isNull()) ? clip.get("uuid") : action;
        const auto forClip = get(self.get("_actionsByClip"),uuid);
        if (method == "existingAction") {
            if (forClip.isUndefined()) return nullptr;
            const auto result = get(forClip.get("actionByRoot"),rootUuid); return result.toBoolean() ? result : JSValue(nullptr);
        }
        auto prototype = JSValue(nullptr), blend = arg(args,2);
        if (blend.isUndefined()) blend = clip.isNull() ? JSValue(NormalAnimationBlendMode) : clip.get("blendMode");
        if (!forClip.isUndefined()) {
            const auto existing = get(forClip.get("actionByRoot"),rootUuid);
            if (!existing.isUndefined() && existing.get("blendMode").strictEquals(blend)) return existing;
            prototype = get(forClip.get("knownActions"),0); if (clip.isNull()) clip = prototype.get("_clip");
        }
        if (clip.isNull()) return nullptr;
        const auto newAction = AnimationAction(self,clip,optionalRoot,blend).object();
        invoke(self,"_bindAction",{newAction,prototype}); invoke(self,"_addInactiveAction",{newAction,uuid,rootUuid}); return newAction;
    }
    if (method == "stopAllAction") {
        const auto actions = self.get("_actions"), count = self.get("_nActiveActions");
        for (double i = Interpolant_detail::subtract(count,1); i >= 0; --i) { invoke(get(actions,i),"stop"); if (i - 1 == i) throw JSRangeError("Action loop cannot progress"); }
        return self;
    }
    if (method == "update") {
        const auto scale = self.get("timeScale"); const auto delta = Interpolant_detail::multiply(action,scale);
        const auto actions = self.get("_actions"), count = self.get("_nActiveActions"), oldTime = self.get("time");
        const auto time = Interpolant_detail::add(oldTime,delta); self.set("time",time);
        const double direction = std::isnan(delta) || delta == 0 ? delta : delta > 0 ? 1 : -1;
        const auto accu = JSOperators::int32(self.get("_accuIndex")) ^ 1; self.set("_accuIndex",accu);
        for (double i = 0; !JSValue(i).strictEquals(count); ++i) { invoke(get(actions,i),"_update",{time,delta,direction,accu}); progress(i); }
        const auto bindings = self.get("_bindings"), nBindings = self.get("_nActiveBindings");
        for (double i = 0; !JSValue(i).strictEquals(nBindings); ++i) { invoke(get(bindings,i),"apply",{accu}); progress(i); }
        return self;
    }
    if (method == "setTime") {
        self.set("time",0);
        for (double i = 0; JSOperators::less(i,self.get("_actions").get("length")); ++i) { auto entry = get(self.get("_actions"),i); entry.set("time",0); progress(i); }
        return invoke(self,"update",{action});
    }
    if (method == "getRoot") return self.get("_root");
    if (method == "uncacheClip") {
        const auto actions = self.get("_actions"), uuid = action.get("uuid"); auto byClip = self.get("_actionsByClip"); const auto clip = get(byClip,uuid);
        if (!clip.isUndefined()) {
            const auto remove = clip.get("knownActions"), count = remove.get("length");
            for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
                auto entry = get(remove,i); invoke(self,"_deactivateAction",{entry});
                const auto index = entry.get("_cacheIndex"); auto inactive = last(actions);
                entry.set("_cacheIndex",nullptr); entry.set("_byClipCacheIndex",nullptr);
                inactive.set("_cacheIndex",index); put(actions,index,inactive); invoke(actions,"pop"); invoke(self,"_removeInactiveBindingsForAction",{entry}); progress(i);
            }
            if (!byClip.erase(uuid.toString())) throw JSTypeError("Cannot delete clip cache");
        }
        return {};
    }
    if (method == "uncacheRoot") {
        const auto uuid = action.get("uuid"), byClip = self.get("_actionsByClip");
        const auto clipReceiver = enumerableReceiver(byClip);
        for (const auto& key : enumerableKeys(clipReceiver)) {
            if (!clipReceiver.has(key)) continue;
            const auto byRoot = clipReceiver.get(key,byClip).get("actionByRoot"), entry = get(byRoot,uuid);
            if (!entry.isUndefined()) { invoke(self,"_deactivateAction",{entry}); invoke(self,"_removeInactiveAction",{entry}); }
        }
        const auto byRoot = self.get("_bindingsByRootAndName"), byName = get(byRoot,uuid);
        const auto bindingReceiver = enumerableReceiver(byName);
        if (!byName.isUndefined()) for (const auto& key : enumerableKeys(bindingReceiver)) {
            if (!bindingReceiver.has(key)) continue;
            const auto binding = bindingReceiver.get(key,byName); invoke(binding,"restoreOriginalState"); invoke(self,"_removeInactiveBinding",{binding});
        }
        return {};
    }
    if (method == "uncacheAction") {
        const auto entry = invoke(self,"existingAction",{action,arg(args,1)});
        if (!entry.isNull()) { invoke(self,"_deactivateAction",{entry}); invoke(self,"_removeInactiveAction",{entry}); }
        return {};
    }
    throw JSTypeError("Unknown mixer operation");
}
}

JSValue AnimationMixer::prototype() {
    static thread_local JSValue value = [] {
        auto object = JSValue::object(EventDispatcher::runtimePrototype());
        auto constructor = JSValue::function([](const JSValue&, const Args&, const Args&) -> JSValue {
            throw JSTypeError("Class constructor AnimationMixer cannot be invoked without 'new'");
        }, {}, "AnimationMixer", 1);
        constructor.setPrototype(EventDispatcher::runtimePrototype().get("constructor"));
        JSPropertyDescriptor prototypeProperty; prototypeProperty.value = object;
        prototypeProperty.writable = prototypeProperty.enumerable = prototypeProperty.configurable = false;
        constructor.defineProperty("prototype", prototypeProperty);
        JSPropertyDescriptor constructorProperty; constructorProperty.value = constructor;
        constructorProperty.writable = constructorProperty.configurable = true; constructorProperty.enumerable = false;
        object.defineProperty("constructor", constructorProperty);
        const std::pair<const char*,std::uint32_t> methods[] = {
            {"_bindAction",2},{"_activateAction",1},{"_deactivateAction",1},{"_initMemoryManager",0},{"_isActiveAction",1},
            {"_addInactiveAction",3},{"_removeInactiveAction",1},{"_removeInactiveBindingsForAction",1},{"_lendAction",1},{"_takeBackAction",1},
            {"_addInactiveBinding",3},{"_removeInactiveBinding",1},{"_lendBinding",1},{"_takeBackBinding",1},{"_lendControlInterpolant",0},{"_takeBackControlInterpolant",1},
            {"clipAction",3},{"existingAction",2},{"stopAllAction",0},{"update",1},{"setTime",1},{"getRoot",0},{"uncacheClip",1},{"uncacheRoot",1},{"uncacheAction",2}};
        for (const auto& [name,arity] : methods) {
            JSPropertyDescriptor d; d.value = JSValue::function(mixerMethod,{name},name,arity); d.writable = true; d.enumerable = false; d.configurable = true;
            object.defineProperty(name,d);
        }
        return object;
    }(); return value;
}
AnimationMixer::AnimationMixer(JSValue root) : object_(JSValue::object(prototype())) {
    object_.set("_root",root); invoke(object_,"_initMemoryManager"); object_.set("_accuIndex",0); object_.set("time",0); object_.set("timeScale",1);
}
AnimationMixer::AnimationMixer(const std::shared_ptr<Object3D>& root) : AnimationMixer(nativeObject(root)) {}
AnimationAction AnimationMixer::clipAction(JSValue clip, JSValue root, JSValue blend) { return AnimationAction::fromObject(invoke(object_,"clipAction",{clip,root,blend})); }
AnimationAction AnimationMixer::clipAction(const std::shared_ptr<AnimationClip>& clip, JSValue root, JSValue blend) { return clipAction(nativeAnimationClip(clip),root,blend); }
AnimationAction AnimationMixer::existingAction(JSValue clip, JSValue root) { return AnimationAction::fromObject(invoke(object_,"existingAction",{clip,root})); }
AnimationAction AnimationMixer::existingAction(const std::shared_ptr<AnimationClip>& clip, JSValue root) { return existingAction(nativeAnimationClip(clip),root); }
#define MIXER0(name) JSValue AnimationMixer::name() { return invoke(object_,#name); }
#define MIXER1(name) JSValue AnimationMixer::name(JSValue value) { return invoke(object_,#name,{value}); }
#define MIXER2(name) JSValue AnimationMixer::name(JSValue a, JSValue b) { return invoke(object_,#name,{a,b}); }
#define MIXER3(name) JSValue AnimationMixer::name(JSValue a, JSValue b, JSValue c) { return invoke(object_,#name,{a,b,c}); }
MIXER0(stopAllAction) MIXER0(getRoot) MIXER1(update) MIXER1(setTime) MIXER1(uncacheClip) MIXER1(uncacheRoot)
MIXER2(uncacheAction) MIXER2(addEventListener) MIXER2(hasEventListener) MIXER2(removeEventListener) MIXER1(dispatchEvent)
MIXER2(_bindAction) MIXER1(_activateAction) MIXER1(_deactivateAction) MIXER0(_initMemoryManager) MIXER1(_isActiveAction)
MIXER3(_addInactiveAction) MIXER1(_removeInactiveAction) MIXER1(_removeInactiveBindingsForAction) MIXER1(_lendAction) MIXER1(_takeBackAction)
MIXER3(_addInactiveBinding) MIXER1(_removeInactiveBinding) MIXER1(_lendBinding) MIXER1(_takeBackBinding)
MIXER0(_lendControlInterpolant) MIXER1(_takeBackControlInterpolant)
JSValue AnimationMixer::uncacheClip(const std::shared_ptr<AnimationClip>& clip) { return uncacheClip(nativeAnimationClip(clip)); }
#undef MIXER0
#undef MIXER1
#undef MIXER2
#undef MIXER3
