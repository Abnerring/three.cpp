#pragma once
#include "loaders/LoadingManager.h"

class Loader {
  public:
    explicit Loader(JSValue manager={});
    virtual ~Loader()=default;
    Loader(const Loader& other):object_(other.object_) {}
    Loader& operator=(const Loader& other){object_=other.object_;return *this;}
    static JSValue prototype();
    JSValue object() const{return object_;}
    operator JSValue() const{return object_;}
    JSMember manager{&object_,"manager"},crossOrigin{&object_,"crossOrigin"},withCredentials{&object_,"withCredentials"};
    JSMember path{&object_,"path"},resourcePath{&object_,"resourcePath"},requestHeader{&object_,"requestHeader"};
    virtual JSValue load(JSValue url={},JSValue onLoad={},JSValue onProgress={},JSValue onError={});
    JSValue loadAsync(JSValue url,JSValue onProgress={});
    virtual JSValue parse(JSValue data={});
    Loader& setCrossOrigin(JSValue value);
    Loader& setWithCredentials(JSValue value);
    Loader& setPath(JSValue value);
    Loader& setResourcePath(JSValue value);
    Loader& setRequestHeader(JSValue value);
    virtual Loader& abort();
    static constexpr const char* DEFAULT_MATERIAL_NAME="__DEFAULT";
  protected:
    struct Existing {};
    Loader(JSValue object,Existing):object_(std::move(object)) {}
    static void initialize(JSValue self,JSValue manager);
    JSValue object_;
};
