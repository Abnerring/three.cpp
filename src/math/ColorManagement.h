#ifndef THREE_COLOR_MANAGEMENT_H
#define THREE_COLOR_MANAGEMENT_H
#include "core/BasicType.h"
#include "core/JSValue.h"
#include <array>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
class Color;
// The ordinary mutable source singleton. All native adapters read this state.
JSValue nativeColorManagement();
namespace ColorManagement {
class Context {
    JSValue object_;
  public:
    Context():object_(nativeColorManagement()){}
    JSValue object() const {static_cast<void>(JSWeakValue(object_));return object_;}
};
// A copied field is a value snapshot, preserving existing save/restore usage.
// The module field itself carries only a key, never a second authoritative value.
template<class Native> class Field {
    const char* key_=nullptr;
    std::optional<JSValue> snapshot_;
    JSValue keeper_;
  public:
    explicit Field(const char* key):key_(key){}
    Field(const Field& other) {
        if(other.key_){keeper_=nativeColorManagement();snapshot_=keeper_.get(other.key_);}
        else {static_cast<void>(JSWeakValue(other.keeper_));keeper_=other.keeper_;snapshot_=other.value();}
    }
    Field(Field&& other):Field(static_cast<const Field&>(other)){}
    JSValue value() const {if(!key_)static_cast<void>(JSWeakValue(keeper_));return key_?nativeColorManagement().get(key_):*snapshot_;}
    operator JSValue() const {return value();}
    operator Native() const {
        if constexpr(std::is_same_v<Native,bool>)return value().toBoolean();
        else return value().toString();
    }
    void set(const JSValue& value) {
        if(key_){
            if(!JSValue::hasActiveHeap())throw JSTypeError("ColorManagement configuration requires a Context or active realm keeper");
            auto object=nativeColorManagement();object.set(key_,value);
        }else {static_cast<void>(JSWeakValue(keeper_));snapshot_=value;}
    }
    Field& operator=(const Field& other){set(other.value());return *this;}
    template<class T> requires (!std::is_same_v<std::remove_cvref_t<T>,Field>) Field& operator=(T&& value){set(JSValue(std::forward<T>(value)));return *this;}
};
inline Field<bool> enabled{"enabled"};
inline Field<std::string> workingColorSpace{"workingColorSpace"};
Color& convert(Color& color,const std::string& sourceColorSpace,const std::string& targetColorSpace);
Color& colorSpaceToWorking(Color& color,const std::string& sourceColorSpace);
Color& workingToColorSpace(Color& color,const std::string& targetColorSpace);
inline constexpr std::array<HIGH_PRECISION,3> luminanceCoefficients{.2126,.7152,.0722};
}
inline HIGH_PRECISION SRGBToLinear(HIGH_PRECISION c){return c<.04045?c*.0773993808:std::pow(c*.9478672986+.0521327014,2.4);}
inline HIGH_PRECISION LinearToSRGB(HIGH_PRECISION c){return c<.0031308?c*12.92:1.055*std::pow(c,.41666)-.055;}
#endif
