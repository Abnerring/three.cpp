#include "nodes/display/ScreenNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSValue imports() {
    static thread_local JSWeakValue weak;
    if (const auto table=weak.lock();table&&table->isObject()) return *table;
    const auto node=nativeNodeConstructor(),constants=nativeNodeConstantsBinding("NodeUpdateType"),group=nativeUniformGroupNodeBinding("renderGroup"),uniform=nativeUniformNodeConstructor(),tsl=nativeTSLBase();
    auto table=JSValue::object(nullptr);
    own(table,"NodeUpdateType",constants); own(table,"renderGroup",group); own(table,"uniform",nativeUniformFactory());
    own(table,"nodeImmutable",nativeTSLBaseBinding("nodeImmutable")); own(table,"vec2",nativeTSLBaseBinding("vec2"));
    own(table,"Vector2",nativeVector2Constructor()); own(table,"Vector4",nativeVector4Constructor());
    own(table,"_screenSizeVec",{}); own(table,"_viewportVec",{});
    table.retainForHeapLifetime(); weak=JSWeakValue(table); return table;
}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures) {
    auto self=receiver,table=captures[2]; const auto name=captures[0],ctor=table.get("constructor");
    if (name.strictEquals("isCacheable")) return false;
    if (name.strictEquals("generateNodeType")) return self.get("scope").strictEquals(ctor.get("VIEWPORT"))?"vec4":"vec2";
    if (name.strictEquals("update")) {
        const auto renderer=arg(args,0).get("renderer"),target=invoke(renderer,"getRenderTarget")();
        if (self.get("scope").strictEquals(ctor.get("VIEWPORT"))) {
            if (!target.isNull()) invoke(table.get("_viewportVec"),"copy")({target.get("viewport")});
            else {
                invoke(renderer,"getViewport")({table.get("_viewportVec")});
                invoke(table.get("_viewportVec"),"multiplyScalar")({invoke(renderer,"getPixelRatio")()});
            }
        } else if (!target.isNull()) {
            auto size=table.get("_screenSizeVec"); size.set("width",target.get("width"));
            size=table.get("_screenSizeVec"); size.set("height",target.get("height"));
        } else invoke(renderer,"getDrawingBufferSize")({table.get("_screenSizeVec")});
        return {};
    }
    if (name.strictEquals("setup")) {
        const auto scope=self.get("scope"); auto output=JSValue(nullptr);
        if (scope.strictEquals(ctor.get("SIZE"))) {
            auto value=table.get("_screenSizeVec"); if (!value.toBoolean()) {value=table.get("Vector2").construct();table.set("_screenSizeVec",value);}
            output=invoke(table.get("uniform").call({}, {value}),"setGroup")({table.get("renderGroup")});
        } else if (scope.strictEquals(ctor.get("VIEWPORT"))) {
            auto value=table.get("_viewportVec"); if (!value.toBoolean()) {value=table.get("Vector4").construct();table.set("_viewportVec",value);}
            output=invoke(table.get("uniform").call({}, {value}),"setGroup")({table.get("renderGroup")});
        } else output=table.get("vec2").call({}, {invoke(table.get("screenCoordinate"),"div")({table.get("screenSize")})});
        auto updateType=table.get("NodeUpdateType").get("NONE");
        if (self.get("scope").strictEquals(ctor.get("SIZE"))||self.get("scope").strictEquals(ctor.get("VIEWPORT"))) updateType=table.get("NodeUpdateType").get("RENDER");
        self.set("updateType",updateType); return output;
    }
    if (name.strictEquals("generate")) {
        const auto builder=arg(args,0);
        if (self.get("scope").strictEquals(ctor.get("COORDINATE"))) {
            auto coord=invoke(builder,"getFragCoord")();
            if (invoke(builder,"isFlipY")().toBoolean()) {
                const auto size=invoke(invoke(builder,"getNodeProperties")({table.get("screenSize")}).get("outputNode"),"build")({builder});
                const auto type=invoke(builder,"getType")({"vec2"}).toString(),first=coord.toString(),s=size.toString(),second=coord.toString();
                coord=type+"( "+first+".x, "+s+".y - "+second+".y )";
            }
            return coord;
        }
        return captures[1].getPrototype().get("generate",self).call(self,{builder});
    }
    throw std::logic_error("Unknown ScreenNode method");
}
}
JSValue nativeScreenNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak; if (const auto ctor=weak.lock();ctor&&ctor->isConstructible()) return *ctor;
    auto table=imports();
    auto ctor=makeConstructor("ScreenNode",1,+[](const JSValue& target,const Args& args,const Args& captures)->JSValue {
        auto self=superConstruct(target,captures); self.set("scope",arg(args,0)); self.set("isViewportNode",true); return self;
    },{{"isCacheable",0},{"generateNodeType",0},{"update",1},{"setup",0},{"generate",1}},method,{table});
    table.set("constructor",ctor);
    for (const auto& [name,value]:std::initializer_list<std::pair<const char*,const char*>>{{"COORDINATE","coordinate"},{"VIEWPORT","viewport"},{"SIZE","size"},{"UV","uv"}}) own(ctor,name,value);
    weak=JSWeakValue(ctor); return ctor;
}
JSValue nativeScreenNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak; if (const auto module=weak.lock();module&&module->isObject()) return *module;
    static thread_local bool initializing=false; JSModuleInitializationScope scope(initializing,"ScreenNode exports");
    const auto ctor=nativeScreenNodeConstructor(),table=imports(); auto bindings=JSValue::object(nullptr); own(bindings,"default",ctor);
    const auto dpr=JSValue::function(+[](const JSValue&,const Args& args,const Args&)->JSValue{return invoke(arg(args,0).get("renderer"),"getPixelRatio")();},{},"",1);
    own(bindings,"screenDPR",invoke(invoke(table.get("uniform").call({}, {1}),"setGroup")({table.get("renderGroup")}),"onRenderUpdate")({dpr}));
    for (const auto& [name,key]:std::initializer_list<std::pair<const char*,const char*>>{{"screenUV","UV"},{"screenSize","SIZE"},{"screenCoordinate","COORDINATE"},{"viewport","VIEWPORT"}}) {
        const auto value=table.get("nodeImmutable").call({}, {ctor,ctor.get(key)}); own(bindings,name,value);own(table,name,value);
    }
    own(bindings,"viewportSize",bindings.get("viewport").get("zw"));
    own(bindings,"viewportCoordinate",invoke(bindings.get("screenCoordinate"),"sub")({bindings.get("viewport").get("xy")}));
    own(bindings,"viewportUV",invoke(bindings.get("viewportCoordinate"),"div")({bindings.get("viewportSize")}));
    auto module=JSValue::object(); auto names=bindings.ownPropertyNames(); std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime(); lexical()=JSWeakValue(bindings); module.retainForHeapLifetime(); weak=JSWeakValue(module); return module;
}
JSValue nativeScreenNodeBinding(const std::string& name) { const auto keeper=nativeScreenNodeModule(); return lexical().lock()->get(name); }
