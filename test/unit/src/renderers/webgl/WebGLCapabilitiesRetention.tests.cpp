#include "catch.hpp"
#include "parity/WebGLCapabilitiesRetentionFixture.h"
TEST_CASE("Every WebGL capability closure retains the shared environment and excludes construction-only parameters", "[webgl-capabilities][gc]"){
 using namespace CapabilitiesRetention;auto witness=JSValue::object();
 for(unsigned method=0;method<4;++method)for(bool instance:{false,true}){
  auto value=prepare(method,instance);witness.collectGarbage();
  for(unsigned i=0;i<value.refs.size();++i){INFO(methods[method]<<" instance="<<instance<<" field="<<names[i]);REQUIRE(value.refs[i].expired()==(i==3));}
  if(instance)REQUIRE(value.keep.get("constructor").get("name").strictEquals(methods[method]));
  value.keep={};witness.collectGarbage();for(const auto& weak:value.refs)REQUIRE(weak.expired());
 }
}
