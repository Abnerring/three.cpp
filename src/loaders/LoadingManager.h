#pragma once
#include "core/JSMember.h"

class LoadingManager {
  public:
    LoadingManager(JSValue onLoad={},JSValue onProgress={},JSValue onError={});
    LoadingManager(const LoadingManager& other):object_(other.object_) {}
    LoadingManager(LoadingManager&& other) noexcept:object_(std::move(other.object_)) {}
    LoadingManager& operator=(const LoadingManager& other){object_=other.object_;return *this;}
    LoadingManager& operator=(LoadingManager&& other) noexcept{object_=std::move(other.object_);return *this;}
    static LoadingManager fromObject(JSValue value){return LoadingManager(std::move(value),Existing{});}
    static JSValue prototype();
    JSValue object() const{return object_;}
    operator JSValue() const{return object_;}
    JSMember onStart{&object_,"onStart"},onLoad{&object_,"onLoad"},onProgress{&object_,"onProgress"},onError{&object_,"onError"};
    JSMember _abortController{&object_,"_abortController"},abortController{&object_,"abortController"};
    void itemStart(JSValue url);
    void itemEnd(JSValue url);
    void itemError(JSValue url);
    JSValue resolveURL(JSValue url);
    LoadingManager& setURLModifier(JSValue transform);
    LoadingManager& addHandler(JSValue regex,JSValue loader);
    LoadingManager& removeHandler(JSValue regex);
    JSValue getHandler(JSValue file);
    LoadingManager& abort();
  private:
    struct Existing {};
    LoadingManager(JSValue value,Existing):object_(std::move(value)) {}
    static void initialize(JSValue self,JSValue onLoad,JSValue onProgress,JSValue onError);
    JSValue object_;
};
extern thread_local LoadingManager DefaultLoadingManager;
