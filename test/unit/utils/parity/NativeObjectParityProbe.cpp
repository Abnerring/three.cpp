#include "core/NativeObject.h"
#include "animation/PropertyBinding.h"
#include "objects/Mesh.h"
#include "textures/Texture.h"
#include <iostream>

namespace {
using Args = std::vector<JSValue>;
void push(JSValue array, JSValue value) { array.set(std::to_string(array.length()), value); }
JSPropertyDescriptor getter(JSValue function) {
    JSPropertyDescriptor d; d.getter = function; d.enumerable = true; d.configurable = true; return d;
}
struct CallbackObject : Object3D {
    std::function<void()> onWorld, onMatrix, onClone;
    void updateMatrixWorld(bool = false) override { if (onMatrix) onMatrix(); }
    void updateWorldMatrix(bool = false, bool = false, bool = false) override { if (onWorld) onWorld(); }
    std::shared_ptr<Object3D> clone(bool = true) const override {
        if (onClone) onClone(); auto result = std::make_shared<Object3D>(); result->name = name; return result;
    }
};
}
int main() {
    try {
        auto result = JSValue::object();
        auto root = std::make_shared<Object3D>(); root->name = "root";
        auto mesh = std::make_shared<Mesh>(); mesh->name = "mesh"; root->add(mesh);
        auto object = nativeObject(root), node = nativeObject(mesh);
        result.set("wrapperIdentity", object.sameValue(nativeObject(root)));
        result.set("componentIdentity", node.get("position").sameValue(node.get("position")));
        result.set("methodIdentity", node.get("position").get("toArray").sameValue(object.get("position").get("toArray")));
        result.set("ownMethod", node.get("position").hasOwn("toArray"));
        auto buffer = Float64Array({9, 1, 2, 3, 9});
        PropertyBinding position(object, "mesh.position"); position.setValue(buffer, 1);
        result.set("position", Animation_detail::ordinary(AnimationArray(mesh->position.toArray())));
        result.set("positionDirty", mesh->matrixWorldNeedsUpdate);
        auto output = Float64Array(5); position.getValue(output, 1);
        result.set("positionRead", Animation_detail::ordinary(output));
        result.set("typedIdentity", nativeArray(buffer).sameValue(nativeArray(buffer)));
        result.set("typedSubarrayIdentity", nativeArray(buffer).sameValue(nativeArray(buffer.subarray())));
        result.set("typedRoundtrip", animationArray(nativeArray(buffer)).sameIdentity(buffer));
        auto positionObject = node.get("position"); auto events = JSValue::array();
        auto input = JSValue::array(3);
        for (int i = 0; i < 3; ++i) input.defineProperty(std::to_string(i), getter(JSValue::function(
            [](const JSValue&, const Args&, const Args& c) -> JSValue {
                auto row = JSValue::array(); push(row, c[2]); push(row, c[0].get("x")); push(row, c[0].get("y")); push(row, c[0].get("z"));
                push(c[1], row); return 4 + c[2].number();
            }, {positionObject, events, i})));
        auto returned = positionObject.get("fromArray").call(positionObject, {input});
        result.set("fromArrayEvents", events); result.set("fromArrayIdentity", returned.sameValue(positionObject));
        result.set("fromArrayPosition", Animation_detail::ordinary(AnimationArray(mesh->position.toArray())));
        auto changed = getter(JSValue::function([](const JSValue&, const Args&, const Args&) { return JSValue(17); }));
        positionObject.defineProperty("x", changed);
        auto plain = JSValue::array();
        auto arrayReturn = positionObject.get("toArray").call(positionObject, {plain, 1});
        result.set("redefinedComponent", plain); result.set("toArrayIdentity", arrayReturn.sameValue(plain));
        auto q = node.get("quaternion"), qevents = JSValue::array(), qinput = JSValue::array(4);
        for (int i = 0; i < 4; ++i) qinput.defineProperty(std::to_string(i), getter(JSValue::function(
            [](const JSValue&, const Args&, const Args& c) -> JSValue {
                auto row = JSValue::array(); push(row, c[2]); push(row, c[0].get("_x")); push(row, c[0].get("_y")); push(row, c[0].get("_z")); push(row, c[0].get("_w"));
                push(c[1], row); return c[2].number() == 3 ? 1 : 0.1 * (c[2].number() + 1);
            }, {q, qevents, i})));
        q.get("fromArray").call(q, {qinput}); result.set("quaternionEvents", qevents);
        result.set("quaternion", Animation_detail::ordinary(AnimationArray(mesh->quaternion.toArray())));
        auto material = mesh->material;
        PropertyBinding opacity(object, "mesh.material.opacity"); opacity.setValue(Float64Array({.25}), 0);
        auto oldWrapper = nativeMaterial(material); mesh->material = std::make_shared<MeshBasicMaterial>();
        opacity.setValue(Float64Array({.75}), 0);
        result.set("oldMaterialOpacity", material->opacity); result.set("newMaterialOpacity", mesh->material->opacity);
        result.set("materialVersion", double(material->version));
        result.set("oldMaterialIdentity", opacity.targetObject.sameValue(oldWrapper));
        mesh->geometry->morphAttributes["position"].push_back(std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0,0}, 3));
        std::dynamic_pointer_cast<BufferAttribute>(mesh->geometry->morphAttributes["position"][0])->name = "smile"; mesh->updateMorphTargets();
        auto morph = node.get("morphTargetInfluences");
        PropertyBinding morphBinding(object, "mesh.morphTargetInfluences[smile]"); morphBinding.setValue(Float64Array({.4}), 0);
        result.set("morphValue", mesh->morphTargetInfluences->at(0));
        result.set("morphIdentity", morph.sameValue(node.get("morphTargetInfluences")));
        auto oldMorph = *mesh->morphTargetInfluences; mesh->updateMorphTargets();
        morphBinding.setValue(Float64Array({.8}), 0);
        result.set("oldMorphValue", oldMorph.at(0)); result.set("newMorphValue", mesh->morphTargetInfluences->at(0));
        auto children = object.get("children"); auto extra = std::make_shared<Object3D>(); extra->name = "extra"; root->add(extra);
        result.set("childrenIdentity", children.sameValue(object.get("children")));
        result.set("childrenGrowth", children.get("length"));
        result.set("findAddedChild", PropertyBinding::findNode(object, "extra").sameValue(nativeObject(extra)));
        node.set("customWeight", 2.);
        PropertyBinding dynamic(object, "mesh.customWeight"); dynamic.setValue(Float64Array({7}), 0);
        result.set("dynamicProperty", node.get("customWeight"));
        auto number = JSValue::object();
        number.set("valueOf", JSValue::function([](const JSValue&, const Args&, const Args& c) {
            auto state = c[0]; const auto n = state.get("n").number(); state.set("n", n + 1); return JSValue(n == 0 ? 4294967298. : 2.);
        }, {[] { auto s = JSValue::object(); s.set("n", 0); return s; }()}));
        auto currentMorph = node.get("morphTargetInfluences"); currentMorph.set("length", number);
        result.set("lengthConversion", currentMorph.get("length"));
        auto basic = std::dynamic_pointer_cast<MeshBasicMaterial>(mesh->material); basic->map = std::make_shared<Texture>();
        PropertyBinding textureOffset(object, "mesh.map.offset"); textureOffset.setValue(Float64Array({.25,.75}), 0);
        result.set("textureOffset", Animation_detail::ordinary(AnimationArray(basic->map->offset.toArray())));
        result.set("textureIdentity", nativeMaterial(basic).get("map").sameValue(nativeTexture(basic->map)));
        auto typed = nativeArray(Float64Array({1})); auto typedCases = JSValue::array();
        for (const char* key : {"0", "-1", "-0", "NaN", "2", "0.5"}) {
            auto calls = JSValue::array(); auto value = JSValue::object();
            value.set("valueOf", JSValue::function([](const JSValue&,const Args&,const Args& c) { push(c[0], "convert"); return JSValue(7); }, {calls}));
            typed.set(key, value);
            auto item = JSValue::object(); item.set("key",key); item.set("calls",calls); item.set("value",typed.get("0"));
            auto receiver = JSValue::object(), receiverCalls = JSValue::array(), receiverValue = JSValue::object();
            receiverValue.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args& c) {push(c[0],"convert");return JSValue(9);},{receiverCalls}));
            item.set("reflect",typed.reflectSet(key,receiverValue,receiver)); item.set("receiverOwn",receiver.hasOwn(key));
            item.set("receiverIdentity",receiver.get(key).sameValue(receiverValue));item.set("receiverCalls",receiverCalls);push(typedCases,item);
        }
        result.set("typedSet",typedCases);
        auto throwing = JSValue::object(); throwing.set("valueOf", JSValue::function([](const JSValue&,const Args&,const Args&) -> JSValue {throw JSTypeError("conversion");}));
        try { typed.set("-1",throwing); result.set("typedInvalidThrows",false); }
        catch(const JSTypeError&) { result.set("typedInvalidThrows",true); }
        result.set("typedOwnLength",typed.hasOwn("length"));
        JSPropertyDescriptor lengthShadow;lengthShadow.value=7;lengthShadow.configurable=true;typed.defineProperty("length",lengthShadow);
        result.set("typedShadowLength",typed.get("length"));typed.erase("length");result.set("typedRestoredLength",typed.get("length"));
        auto math = nativeVector(std::make_shared<Vector3>(1,2,3)), offsetValue = JSValue::object(), offsetEvents = JSValue::array();
        offsetValue.set("toString",JSValue::function([](const JSValue&,const Args&,const Args& c){push(c[1],"string");auto v=c[0];v.set("x",99);return JSValue("slot");},{math,offsetEvents}));
        offsetValue.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args& c){push(c[0],"number");return JSValue(0);},{offsetEvents}));
        auto destination=JSValue::array();math.get("toArray").call(math,{destination,offsetValue});
        result.set("computedFirstValue",destination.get("slot"));result.set("computedEvents",offsetEvents);
        auto euler = nativeEuler(std::make_shared<Euler>()), eulerInput = Animation_detail::ordinary(AnimationArray(std::vector<double>{1,2,3})), eulerEvents = JSValue::array();
        eulerInput.defineProperty("3",getter(JSValue::function([](const JSValue&,const Args&,const Args& c) {
            push(c[0],"order");return JSValue(c[0].length()==1 ? "XYZ" : "ZYX");
        },{eulerEvents})));
        auto ignoredOffset=JSValue::object();ignoredOffset.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args&) -> JSValue {throw JSTypeError("offset");}));
        euler.get("fromArray").call(euler,{eulerInput,ignoredOffset});
        result.set("eulerFromArray",euler.get("toArray").call(euler));result.set("eulerOrderReads",eulerEvents);
        for (bool world : {false,true}) {
            auto parent=std::make_shared<Object3D>();auto first=std::make_shared<CallbackObject>(),second=std::make_shared<CallbackObject>();
            auto events=JSValue::array();parent->children={first,second};
            auto replace=[&] {push(events,"first");parent->children=std::vector<std::shared_ptr<Object3D>>{};};
            auto visit=[&] {push(events,"second");};first->onMatrix=replace;first->onWorld=replace;second->onMatrix=visit;second->onWorld=visit;
            if(world)parent->updateWorldMatrix(false,true);else parent->updateMatrixWorld();
            result.set(world ? "worldChildrenSnapshot" : "matrixChildrenSnapshot",events);
        }
        auto ancestorRoot=std::make_shared<Object3D>(),ancestorParent=std::make_shared<Object3D>(),ancestorChild=std::make_shared<Object3D>();
        ancestorRoot->name="grand";ancestorParent->name="parent";ancestorRoot->add(ancestorParent);ancestorParent->add(ancestorChild);
        auto ancestors=JSValue::array();ancestorChild->traverseAncestors([&](Object3D& visited){push(ancestors,visited.name);ancestorChild->parent=nullptr;});result.set("ancestorSnapshot",ancestors);
        auto copySource=std::make_shared<Object3D>(),copyTarget=std::make_shared<Object3D>();auto firstClone=std::make_shared<CallbackObject>(),secondClone=std::make_shared<CallbackObject>();
        firstClone->name="first";secondClone->name="second";copySource->children={firstClone};
        firstClone->onClone=[&]{copySource->children.push_back(secondClone);};copyTarget->copy(*copySource);
        auto copies=JSValue::array();for(const auto& child:copyTarget->children)push(copies,child->name);result.set("copyDynamicLength",copies);
        auto removeParent=std::make_shared<Object3D>(),removeFirst=std::make_shared<Object3D>(),removeSecond=std::make_shared<Object3D>();
        removeParent->add(removeFirst);removeParent->add(removeSecond);auto removeChildren=nativeObject(removeParent).get("children");
        JSPropertyDescriptor readonlyLength;readonlyLength.writable=false;removeChildren.defineProperty("length",readonlyLength);
        auto removeResult=JSValue::array();try{removeParent->remove(removeFirst);push(removeResult,false);}catch(const JSTypeError&){push(removeResult,true);}
        push(removeResult,removeFirst->parent==nullptr);push(removeResult,removeChildren.get("length"));
        push(removeResult,removeChildren.get("0").sameValue(nativeObject(removeSecond)));push(removeResult,removeChildren.hasOwn("1"));result.set("removePartialWrites",removeResult);
        std::cout << result.toJSON()->dump() << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
