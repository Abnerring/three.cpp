// Ported from the pinned three.js TSLBase.js; see THREEJS-LICENSE.
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/core/ArrayNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/AssignNode.h"
#include "nodes/code/FunctionCallNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/ConditionalNode.h"
#include "nodes/core/ContextNode.h"
#include "nodes/core/VarNode.h"
#include "nodes/core/VaryingNode.h"
#include "nodes/display/ColorSpaceNode.h"
#include "nodes/display/ToneMappingNode.h"
#include "nodes/accessors/BufferAttributeNode.h"
#include "nodes/gpgpu/ComputeNode.h"
#include "nodes/core/IsolateNode.h"
#include "nodes/core/BypassNode.h"
#include "nodes/utils/Remap.h"
#include "nodes/code/ExpressionNode.h"
#include "nodes/utils/Discard.h"
#include "nodes/display/RenderOutputNode.h"
#include "nodes/utils/DebugNode.h"
#include "nodes/core/SubBuildNode.h"
#include "nodes/core/InspectorNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "utils.h"
#include <algorithm>

namespace {
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue uniformBinding(const std::string& name){
    const auto constructor=nativeUniformNodeConstructor();
    return name=="uniform"?nativeUniformFactory():JSValue{};
}
}
JSValue nativeTSLBase(){
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"TSLBase exports");
    auto bindings=JSValue::object(nullptr);
    const auto merge=[&](JSValue(*getter)(const std::string&),std::initializer_list<const char*> names){for(const auto* name:names)own(bindings,name,getter(name));};
#include "nodes/tsl/TSLBaseExports.inc"
    own(bindings,"addNodeElement",JSValue::constructor(+[](const JSValue&,const Args& args,const Args&)->JSValue{
        ThreeUtils::warn({"TSL: AddNodeElement has been removed in favor of tree-shaking. Trying add",arg(args,0)});return {};
    },{},"addNodeElement",1,JSValue::ConstructorKind::Callable));
    auto module=JSValue::object();auto names=bindings.ownPropertyNames();std::sort(names.begin(),names.end());for(const auto& name:names)own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeTSLBaseBinding(const std::string& name){const auto keeper=nativeTSLBase();return lexical().lock()->get(name);}
