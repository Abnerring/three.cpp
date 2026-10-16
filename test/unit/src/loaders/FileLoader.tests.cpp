#include "catch.hpp"
#include "loaders/FileLoader.h"
#include "core/JSValue.h"
#include <string>
#include <vector>

// The host boundary is the injected transport, so these tests drive the ported state machine with
// a transport written in C++: no network, no fetch, and the same request/response shape that
// docs/HANDOFF.md item 10 freezes.
namespace {
using Args=std::vector<JSValue>;
JSValue makeTransport(int& calls,double status,const std::string& statusText,const std::string& body,const std::string& contentLength){
    return JSValue::function([&calls,status,statusText,body,contentLength](const JSValue&,const Args&,const Args&)->JSValue{
        calls+=1;
        auto response=JSValue::object();
        response.set("status",status);
        response.set("statusText",statusText);
        auto headers=JSValue::object();
        headers.set("get",JSValue::function([contentLength](const JSValue&,const Args& args,const Args&)->JSValue{
            if(args.empty())return {};
            return args[0].toString()=="Content-Length"?JSValue(contentLength):JSValue{};
        },{},"get",1));
        response.set("headers",headers);
        response.set("body",body);
        return response;
    },{},"transport",1);
}
JSValue collector(int& calls,std::string& received){
    return JSValue::function([&calls,&received](const JSValue&,const Args& args,const Args&)->JSValue{
        calls+=1;
        received=args.empty()?std::string{}:args[0].toString();
        return {};
    },{},"onLoad",1);
}
}

TEST_CASE("FileLoader loads through the injected transport and caches the result","[fileloader]"){
    int transportCalls=0,loads=0;
    std::string received;
    FileLoader::setTransport(makeTransport(transportCalls,200,"OK","hello world","11"));
    FileLoader loader;
    loader.load("asset.txt",collector(loads,received));
    REQUIRE(loads==1);
    REQUIRE(received=="hello world");
    REQUIRE(transportCalls==1);
    // The second load of the same url comes from the cache, so the transport is untouched.
    std::string second;
    loader.load("asset.txt",collector(loads,second));
    REQUIRE(loads==2);
    REQUIRE(second=="hello world");
    REQUIRE(transportCalls==1);
    // A different url goes back to the transport.
    std::string third;
    loader.load("other.txt",collector(loads,third));
    REQUIRE(transportCalls==2);
    FileLoader::setTransport(JSValue{});
}

TEST_CASE("FileLoader reports failures the way the pinned source does","[fileloader]"){
    int transportCalls=0,loads=0;
    std::string received;
    FileLoader loader;
    // No transport configured fails loudly instead of quietly doing nothing.
    FileLoader::setTransport(JSValue{});
    REQUIRE_THROWS_AS(loader.load("missing.txt",collector(loads,received)),JSTypeError);
    // A non-200 status becomes the upstream error text.
    FileLoader::setTransport(makeTransport(transportCalls,404,"Not Found","",""));
    try {
        loader.load("gone.txt",collector(loads,received));
        REQUIRE(false);
    } catch(const JSTypeError& error) {
        REQUIRE(std::string(error.what()).find("responded with 404: Not Found")!=std::string::npos);
    }
    // A status of 0 counts as success, like the pinned non-http protocol case.
    FileLoader::setTransport(makeTransport(transportCalls,0,"","zero status","11"));
    loader.load("zero.txt",collector(loads,received));
    REQUIRE(received=="zero status");
    // blob and document have no native counterpart, so they fail loudly.
    FileLoader::setTransport(makeTransport(transportCalls,200,"OK","{}","2"));
    loader.setResponseType("blob");
    REQUIRE_THROWS_AS(loader.load("blob.txt",collector(loads,received)),JSTypeError);
    loader.setResponseType("document");
    REQUIRE_THROWS_AS(loader.load("doc.txt",collector(loads,received)),JSTypeError);
    FileLoader::setTransport(JSValue{});
}