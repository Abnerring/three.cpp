#include "core/NativeObject.h"
#include "math/Quaternion.h"
#include "math/Euler.h"
#include <iostream>
#include <memory>
template<class T,class F>void probe(const char* kind,F wrap){
 unsigned late=0,old=0,next=0;
 auto a=std::make_shared<T>();auto view=wrap(a);a->_onChange([&]{++late;});
 const bool own=view.hasOwn("_onChangeCallback");view.get("_onChangeCallback").call(view,{});
 auto b=std::make_shared<T>();b->_onChange([&]{++old;});auto second=wrap(b);const auto saved=second.get("_onChangeCallback");
 b->_onChange([&]{++next;});saved.call(second,{});
 std::cout<<JsonValue(JsonValue::Object{{"kind",kind},{"late",JsonValue::Object{{"own",own},{"calls",late}}},{"snapshot",JsonValue::Object{{"old",old},{"new",next}}}}).dump()<<'\n';
}
int main(){probe<Quaternion>("Quaternion",[](auto v){return nativeQuaternion(v);});probe<Euler>("Euler",[](auto v){return nativeEuler(v);});}
