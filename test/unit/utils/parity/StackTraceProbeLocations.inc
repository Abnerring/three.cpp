// Generated from real current oracle AST. No input/expected output is used for source locations.
#pragma once
#include "core/JSSourceTrace.h"
namespace StackTraceProbeLocations {
inline JSSourceTrace::Frame frame(std::string_view key,const std::string& uri){
if(key=="leaf")return {"leaf",{uri,3,8,117,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"},"","",false,false,false,{uri,3,8,117,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"}};
if(key=="middle")return {"middle",{uri,19,8,663,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"},"","",false,false,false,{uri,19,8,663,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"}};
if(key=="outer")return {"outer",{uri,22,8,723,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"},"","",false,false,false,{uri,22,8,723,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"}};
if(key=="absent")return {"absent",{uri,25,8,784,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"},"","",false,false,false,{uri,25,8,784,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"}};
if(key=="formatter")return {"formatter",{uri,26,8,812,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"},"","",false,false,false,{uri,26,8,812,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"}};
if(key=="resume")return {"resume",{uri,30,8,1003,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"},"","",false,false,false,{uri,30,8,1003,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"}};
if(key=="schedule")return {"schedule",{uri,33,8,1069,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"},"","",false,false,false,{uri,33,8,1069,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"}};
throw std::logic_error("Unknown probe source frame");}
inline JSSourceTrace::Location position(std::string_view key,const std::string& uri){
if(key=="capture-middle")return {uri,7,11,270,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="capture-absent")return {uri,12,11,415,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="error-lazy")return {uri,15,44,522,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="default")return {uri,16,56,596,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="read-boom-stack")return {uri,17,28,647,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="error-boom")return {uri,17,10,629,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="call-leaf")return {uri,20,10,699,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="call-middle")return {uri,23,10,758,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="read-nested-stack")return {uri,27,38,885,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="error-nested")return {uri,27,18,865,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="read-resumed-stack")return {uri,31,31,1053,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
if(key=="error-resumed")return {uri,31,10,1032,"d7330dbc8d89029cc74f5d1b9076dd282e97d0e541044d5f9a1c72d10cc1d335"};
throw std::logic_error("Unknown probe source position");}
inline JSSourceTrace::Frame hostFrame(std::string_view key){
if(key=="attempt")return {"attempt",{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",11,1,871,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"},"","",false,false,false,{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",11,1,871,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"}};
if(key=="source")return {"source",{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",13,1,1522,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"},"","",false,false,false,{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",13,1,1522,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"}};
if(key=="stack")return {"stack",{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",24,1,2644,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"},"","",false,false,false,{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",24,1,2644,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"}};
if(key=="integer")return {"integer",{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",36,1,4750,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"},"","",false,false,false,{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",36,1,4750,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"}};
if(key=="oracle")return {"oracle",{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",48,8,5804,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"},"","",false,false,false,{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",48,8,5804,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"}};
if(key=="comparator.module")return {"",{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/compare-stacktrace-source.mjs",1,1,0,"46a3dc1980af449666094da1396783e74ca3b3ac35f47e7f72b24c689cdfed98"},"","",false,false,false,{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/compare-stacktrace-source.mjs",1,1,0,"46a3dc1980af449666094da1396783e74ca3b3ac35f47e7f72b24c689cdfed98"}};
if(key=="source.throw-first")return {"",{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",19,62,2156,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"},"","",false,false,false,{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",19,62,2156,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"}};
if(key=="source.throw-second")return {"",{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",19,133,2227,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"},"","",false,false,false,{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",19,133,2227,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"}};
if(key=="source.default")return {"",{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",21,18,2430,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"},"","",false,false,false,{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",21,18,2430,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"}};
if(key=="stack.result")return {"",{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",33,24,4397,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"},"","",false,false,false,{"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",33,24,4397,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"}};
throw std::logic_error("Unknown probe host frame");}
inline JSSourceTrace::Location hostPosition(std::string_view key){
if(key=="attempt.invoke")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",11,47,917,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="source.throw-first.attempt")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",19,54,2148,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="source.throw-first.outer")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",19,73,2167,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="source.throw-second.attempt")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",19,125,2219,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="source.throw-second.outer")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",19,144,2238,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="source.default.attempt")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",21,10,2422,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="source.default.outer")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",21,29,2441,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="source.lazy.outer")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",18,53,1987,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="source.microtask.schedule")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",20,84,2385,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="stack.result.attempt")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",33,16,4389,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="stack.result.construct")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",33,41,4414,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="stack.result.getLocation")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",33,104,4477,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="stack.result.getError")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",33,130,4503,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="oracle.source")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",48,175,5971,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="oracle.stack")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",48,220,6016,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="oracle.integer")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/stacktrace-source-oracle.mjs",48,235,6031,"65d7b9a49c22fd0468cfd326e541258f93088ffca06ffbaf4e034a33f86b78d8"};
if(key=="comparator.oracle")return {"file:///D:/project/DX/GitHub/three.cpp/test/unit/utils/parity/compare-stacktrace-source.mjs",5,140,1434,"46a3dc1980af449666094da1396783e74ca3b3ac35f47e7f72b24c689cdfed98"};
throw std::logic_error("Unknown probe host position");}
}
