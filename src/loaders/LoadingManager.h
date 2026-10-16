#pragma once
#include "core/JSMember.h"
#include "core/JSModuleObject.h"

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
JSValue nativeDefaultLoadingManager();

// The ESM singleton is a managed heap export. This view contributes no hidden
// heap root; hold an explicit JSValue to configure or use it across calls.
class DefaultLoadingManagerFacade {
  public:
    constexpr DefaultLoadingManagerFacade() = default;
    constexpr DefaultLoadingManagerFacade(const DefaultLoadingManagerFacade&) = default;
    DefaultLoadingManagerFacade& operator=(const DefaultLoadingManagerFacade&) = delete;
    JSModuleMember onStart{nativeDefaultLoadingManager,"onStart"},onLoad{nativeDefaultLoadingManager,"onLoad"};
    JSModuleMember onProgress{nativeDefaultLoadingManager,"onProgress"},onError{nativeDefaultLoadingManager,"onError"};
    JSModuleMember _abortController{nativeDefaultLoadingManager,"_abortController"},abortController{nativeDefaultLoadingManager,"abortController"};
    JSValue object() const{return nativeDefaultLoadingManager();}
    operator JSValue() const{return object();}
    void itemStart(const JSValue& url);
    void itemEnd(const JSValue& url);
    void itemError(const JSValue& url);
    JSValue resolveURL(const JSValue& url);
    DefaultLoadingManagerFacade& setURLModifier(const JSValue& transform);
    DefaultLoadingManagerFacade& addHandler(const JSValue& regex,const JSValue& loader);
    DefaultLoadingManagerFacade& removeHandler(const JSValue& regex);
    JSValue getHandler(const JSValue& file);
    DefaultLoadingManagerFacade& abort();
};
extern DefaultLoadingManagerFacade DefaultLoadingManager;
