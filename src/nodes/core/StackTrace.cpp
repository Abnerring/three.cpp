#include "nodes/core/StackTrace.h"

#include "core/JSModuleObject.h"
#include "core/JSRegExp.h"
#include <cstdlib>
#include <regex>
#include <sstream>

namespace {
// Files whose frames say nothing useful about the node graph.
const std::regex ignored[] = {std::regex("^StackTrace\\.js$"), std::regex("^TSLCore\\.js$"),
                              std::regex("^.*Node\\.js$"), std::regex("^three\\.webgpu.*\\.js$")};

std::string stripPath(const std::string& file) {
    const auto slash = file.find_last_of('/');
    return slash == std::string::npos ? file : file.substr(slash + 1);
}
}  // namespace

std::vector<NodeStackFrame> getFilteredStack(const std::string& stack) {
    // Same shape as upstream: an optional "at fn (" prefix, an optional "fn@" prefix, then
    // file:line:column.
    static const std::regex pattern(
        R"((?:at\s+(.+?)\s+\()?(?:(.+?)@)?([^@\s()]+):(\d+):(\d+))");
    std::vector<NodeStackFrame> frames;
    std::istringstream lines(stack);
    std::string line;
    while (std::getline(lines, line)) {
        std::smatch match;
        if (!std::regex_search(line, match, pattern)) continue;
        NodeStackFrame frame;
        frame.fn = match[1].matched ? match[1].str() : (match[2].matched ? match[2].str() : "");
        std::string file = match[3].str();
        const auto query = file.find('?');  // strip Vite/HMR suffixes
        if (query != std::string::npos) file = file.substr(0, query);
        frame.file = stripPath(file);
        frame.line = std::stoi(match[4].str());
        frame.column = std::stoi(match[5].str());
        bool skip = false;
        for (const auto& pattern2 : ignored)
            if (std::regex_match(frame.file, pattern2)) skip = true;
        if (!skip) frames.push_back(std::move(frame));
    }
    return frames;
}

StackTrace::StackTrace(const std::string& stackMessage) {
    if (!stackMessage.empty()) _stack = getFilteredStack(stackMessage);
}

std::string StackTrace::getLocation() const {
    if (_stack.empty()) return "[Unknown location]";
    const auto& main = _stack.front();
    const std::string fnName = main.fn.empty() ? std::string() : '"' + main.fn + "()\" at ";
    return fnName + '"' + main.file + ":" + std::to_string(main.line) + '"';
}

std::string StackTrace::getError(const std::string& message) const {
    if (_stack.empty()) return message;
    std::string out = message + "\n";
    for (std::size_t i = 0; i < _stack.size(); ++i) {
        const auto& frame = _stack[i];
        const std::string location =
            frame.file + ":" + std::to_string(frame.line) + ":" + std::to_string(frame.column);
        out += frame.fn.empty() ? "    at " + location : "    at " + frame.fn + " (" + location + ")";
        if (i + 1 != _stack.size()) out += "\n";
    }
    return out;
}

namespace {
using Args=std::vector<JSValue>;
JSValue argument(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
JSPropertyDescriptor field(JSValue value,bool enumerable=true){JSPropertyDescriptor d;d.value=value;d.writable=d.configurable=true;d.enumerable=enumerable;return d;}
JSValue invoke(const JSValue& receiver,const char* name,const Args& args={}){return receiver.get(name).call(receiver,args);}
JSValue captureState(){
    static thread_local JSWeakValue weak;if(const auto state=weak.lock();state&&state->isObject())return *state;
    auto state=JSValue::object(nullptr);state.defineProperty("capture",field({}));state.retainForHeapLifetime();weak=JSWeakValue(state);return state;
}
JSValue parseFrames(const JSValue& input){
    // Native string parsing is the explicit host boundary. A custom JS
    // split/match pipeline requires the absent String.match intrinsic.
    if(input.type()!=JSValue::Type::String)throw StackTraceUnsupported("StackTrace: dynamic split/match input requires String.match support");
    const auto pattern=createNativeRegExp(R"((?:at\s+(.+?)\s+\()?(?:(.+?)@)?([^@\s()]+):(\d+):(\d+))");
    auto frames=JSValue::array();std::istringstream lines(input.string());std::string text;unsigned index=0;
    while(std::getline(lines,text)){
        const auto match=jsRegExpExec(pattern,text);if(match.isNull())continue;
        auto file=match.get("3").toString();if(const auto query=file.find('?');query!=std::string::npos)file.resize(query);file=stripPath(file);
        bool skip=false;for(const auto& regexp:ignored)if(std::regex_match(file,regexp)){skip=true;break;}if(skip)continue;
        const auto first=match.get("1"),second=match.get("2");auto frame=JSValue::object();frame.defineProperty("fn",field(first.toBoolean()?first:second.toBoolean()?second:JSValue("")));
        frame.defineProperty("file",field(file));frame.defineProperty("line",field(std::strtod(match.get("4").toString().c_str(),nullptr)));frame.defineProperty("column",field(std::strtod(match.get("5").toString().c_str(),nullptr)));
        frames.defineProperty(std::to_string(index++),field(frame));
    }return frames;
}
JSValue initializeStack(const JSValue& self,const Args& args,const Args& captures){
    auto receiver=self;receiver.set("isStackTrace",true);auto input=argument(args,0);
    if(!input.toBoolean()){const auto capture=captures[0].get("capture");input=capture.isUndefined()?JSValue(""):capture.call();}
    receiver.set("stack",parseFrames(input));return {};
}
JSValue location(const JSValue& self,const Args&,const Args&){
    if(self.get("stack").get("length").strictEquals(0))return "[Unknown location]";
    const auto main=self.get("stack").get("0"),fn=main.get("fn");const auto prefix=fn.toBoolean()?"\""+fn.toString()+"()\" at ":std::string{};
    const auto file=main.get("file").toString();const auto line=main.get("line").toString();return prefix+'"'+file+":"+line+'"';
}
JSValue formatFrame(const JSValue&,const Args& args,const Args&){
    const auto frame=argument(args,0);const auto file=frame.get("file").toString();const auto line=frame.get("line").toString();const auto column=frame.get("column").toString();const auto at=file+":"+line+":"+column;
    if(frame.get("fn").toBoolean())return "    at "+frame.get("fn").toString()+" ("+at+")";return "    at "+at;
}
JSValue errorMessage(const JSValue& self,const Args& args,const Args&){
    const auto message=argument(args,0);if(self.get("stack").get("length").strictEquals(0))return message;
    const auto mapped=invoke(self.get("stack"),"map",{JSValue::function(formatFrame,{},"",1)});const auto stack=invoke(mapped,"join",{"\n"});const auto prefix=message.toString();return prefix+"\n"+stack.toString();
}
}
JSValue nativeStackTraceConstructor(){
    static thread_local JSWeakValue weak;static thread_local bool initializing=false;
    if(const auto constructor=weak.lock();constructor&&constructor->isConstructible())return *constructor;
    JSModuleInitializationScope scope(initializing,"StackTrace constructor");auto constructor=JSValue::constructor(initializeStack,{captureState()},"StackTrace",0);auto prototype=constructor.get("prototype");
    prototype.defineProperty("getLocation",field(JSValue::function(location,{},"getLocation",0),false));prototype.defineProperty("getError",field(JSValue::function(errorMessage,{},"getError",1),false));
    constructor.retainForHeapLifetime();weak=JSWeakValue(constructor);return constructor;
}
void setNodeStackCaptureFunction(const JSValue& callback){
    static_cast<void>(JSWeakValue(callback));if(!callback.isUndefined()&&!callback.isCallable())throw JSTypeError("Stack capture provider must be a function or undefined");auto state=captureState();state.set("capture",callback);
}
