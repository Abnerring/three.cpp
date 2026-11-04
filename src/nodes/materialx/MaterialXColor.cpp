#include "nodes/materialx/MaterialXColor.h"

#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"

#include <string>
#include <vector>

namespace {
using Args = std::vector<JSValue>;

void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}

struct Invocation {
    JSValue receiver, method;
    JSValue operator()(const Args& args = {}) const { return method.call(receiver, args); }
};
Invocation invoke(const JSValue& receiver, const char* name) { return {receiver, receiver.get(name)}; }
JSValue firstInput(const Args& args) {
    if (args.empty()) return {};
    const auto value = args[0];
    return value.isObject() && !value.get("0").isUndefined() ? value.get("0") : value;
}
JSValue assign(const JSValue&, const Args&, const Args& captures) {
    invoke(captures[0], "assign")({captures[1]});
    return {};
}
std::string crlf(std::string source) {
    std::string result;
    result.reserve(source.size() + source.size() / 16);
    for (std::size_t i = 0; i < source.size(); ++i) {
        if (source[i] == '\n' && (i == 0 || source[i - 1] != '\r')) result.push_back('\r');
        result.push_back(source[i]);
    }
    return result;
}

JSValue hsvBody(const JSValue&, const Args& args, const Args& captures) {
    const auto module = captures[0], hsv = firstInput(args);
    const auto sat = hsv.get("y"), val = hsv.get("z");
    const auto result = invoke(module.get("vec3").call({}, {}), "toVar")();
    const auto zeroSat = JSValue::function(assign, {result, module.get("vec3").call({}, {val, val, val})}, "", 0);
    const auto ifNode = module.get("If").call({}, {invoke(sat, "lessThan")({0.0001}), zeroSat});
    auto hue = invoke(hsv.get("x"), "sub")({module.get("floor").call({}, {hsv.get("x")})});
    hue = invoke(hue, "mul")({6.0});
    const auto hi = module.get("int").call({}, {module.get("trunc").call({}, {hue})});
    const auto frac = invoke(hue, "sub")({module.get("float").call({}, {hi})});
    const auto p = invoke(val, "mul")({invoke(sat, "oneMinus")()});
    const auto q = invoke(val, "mul")({invoke(invoke(sat, "mul")({frac}), "oneMinus")()});
    const auto t = invoke(val, "mul")({invoke(invoke(sat, "mul")({invoke(frac, "oneMinus")()}), "oneMinus")()});
    auto current = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
        const auto module = c[0], result = c[1], hi = c[2], p = c[3], q = c[4], t = c[5], val = c[6];
        auto chain = module.get("If").call({}, {invoke(hi, "equal")({module.get("int").call({}, {0})}), JSValue::function(assign, {result, module.get("vec3").call({}, {val, t, p})}, "", 0)});
        chain = invoke(chain, "ElseIf")({invoke(hi, "equal")({module.get("int").call({}, {1})}), JSValue::function(assign, {result, module.get("vec3").call({}, {q, val, p})}, "", 0)});
        chain = invoke(chain, "ElseIf")({invoke(hi, "equal")({module.get("int").call({}, {2})}), JSValue::function(assign, {result, module.get("vec3").call({}, {p, val, t})}, "", 0)});
        chain = invoke(chain, "ElseIf")({invoke(hi, "equal")({module.get("int").call({}, {3})}), JSValue::function(assign, {result, module.get("vec3").call({}, {p, q, val})}, "", 0)});
        chain = invoke(chain, "ElseIf")({invoke(hi, "equal")({module.get("int").call({}, {4})}), JSValue::function(assign, {result, module.get("vec3").call({}, {t, p, val})}, "", 0)});
        invoke(chain, "Else")({JSValue::function(assign, {result, module.get("vec3").call({}, {val, p, q})}, "", 0)});
        return {};
    }, {module, result, hi, p, q, t, val}, "", 0);
    invoke(ifNode, "Else")({current});
    return result;
}

JSValue rgbBody(const JSValue&, const Args& args, const Args& captures) {
    const auto module = captures[0], c = module.get("vec3").call({}, {firstInput(args)});
    const auto red = invoke(module.get("float").call({}, {c.get("x")}), "toVar")();
    const auto green = invoke(module.get("float").call({}, {c.get("y")}), "toVar")();
    const auto blue = invoke(module.get("float").call({}, {c.get("z")}), "toVar")();
    const auto minComp = invoke(module.get("float").call({}, {module.get("min").call({}, {red, module.get("min").call({}, {green, blue})})}), "toVar")();
    const auto maxComp = invoke(module.get("float").call({}, {module.get("max").call({}, {red, module.get("max").call({}, {green, blue})})}), "toVar")();
    const auto delta = invoke(module.get("float").call({}, {invoke(maxComp, "sub")({minComp})}), "toVar")();
    const auto hueValue = invoke(module.get("float").call({}, {}), "toVar")();
    const auto satValue = invoke(module.get("float").call({}, {}), "toVar")();
    const auto valValue = invoke(module.get("float").call({}, {}), "toVar")();
    invoke(valValue, "assign")({maxComp});
    auto satIf = module.get("If").call({}, {invoke(maxComp, "greaterThan")({0.0}), JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue { invoke(c[0], "assign")({invoke(c[1], "div")({c[2]})}); return {}; }, {satValue, delta, maxComp}, "", 0)});
    invoke(satIf, "Else")({JSValue::function(assign, {satValue, JSValue(0.0)}, "", 0)});
    auto hueIf = module.get("If").call({}, {invoke(satValue, "lessThanEqual")({0.0}), JSValue::function(assign, {hueValue, JSValue(0.0)}, "", 0)});
    auto hueElse = JSValue::function([](const JSValue&, const Args&, const Args& c) -> JSValue {
        const auto module = c[0], hue = c[1], red = c[2], green = c[3], blue = c[4], maxComp = c[5], delta = c[6];
        const auto gb = invoke(green, "sub")({blue});
        const auto br = invoke(blue, "sub")({red});
        const auto rg = invoke(red, "sub")({green});
        auto chain = module.get("If").call({}, {invoke(red, "greaterThanEqual")({maxComp}), JSValue::function(assign, {hue, invoke(gb, "div")({delta})}, "", 0)});
        chain = invoke(chain, "ElseIf")({invoke(green, "greaterThanEqual")({maxComp}), JSValue::function(assign, {hue, module.get("add").call({}, {2.0, invoke(br, "div")({delta})})}, "", 0)});
        invoke(chain, "Else")({JSValue::function(assign, {hue, module.get("add").call({}, {4.0, invoke(rg, "div")({delta})})}, "", 0)});
        invoke(hue, "mulAssign")({1.0 / 6.0});
        module.get("If").call({}, {invoke(hue, "lessThan")({0.0}), JSValue::function([](const JSValue&, const Args&, const Args& d) -> JSValue { invoke(d[0], "addAssign")({1.0}); return {}; }, {hue}, "", 0)});
        return {};
    }, {module, hueValue, red, green, blue, maxComp, delta}, "", 0);
    invoke(hueIf, "Else")({hueElse});
    return module.get("vec3").call({}, {hueValue, satValue, valValue});
}
}

JSValue nativeMaterialXColorModule() {
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    auto module = JSValue::object(nullptr); module.retainForHeapLifetime(); weak = JSWeakValue(module);
    for (const auto* name : {"float", "int", "vec3", "If", "add"}) own(module, name, nativeTSLBaseBinding(name));
    for (const auto* name : {"floor", "trunc", "min", "max"}) own(module, name, nativeMathNodeBinding(name));
    const auto make = [&](const char* name, JSValue::NativeFunction body, const char* input) {
        auto callback = JSValue::function(body, {module}, name, 1);
        const auto source = name == std::string("mx_hsvtorgb") ? std::string(R"(( [ hsv ] ) => {

	const s = hsv.y;
	const v = hsv.z;

	const result = vec3().toVar();

	If( s.lessThan( 0.0001 ), () => {

		result.assign( vec3( v, v, v ) );

	} ).Else( () => {

		let h = hsv.x;
		h = h.sub( floor( h ) ).mul( 6.0 ).toVar(); // TODO: check what .toVar() is needed in node system cache
		const hi = int( trunc( h ) );
		const f = h.sub( float( hi ) );
		const p = v.mul( s.oneMinus() );
		const q = v.mul( s.mul( f ).oneMinus() );
		const t = v.mul( s.mul( f.oneMinus() ).oneMinus() );

		If( hi.equal( int( 0 ) ), () => {

			result.assign( vec3( v, t, p ) );

		} ).ElseIf( hi.equal( int( 1 ) ), () => {

			result.assign( vec3( q, v, p ) );

		} ).ElseIf( hi.equal( int( 2 ) ), () => {

			result.assign( vec3( p, v, t ) );

		} ).ElseIf( hi.equal( int( 3 ) ), () => {

			result.assign( vec3( p, q, v ) );

		} ).ElseIf( hi.equal( int( 4 ) ), () => {

			result.assign( vec3( t, p, v ) );

		} ).Else( () => {

			result.assign( vec3( v, p, q ) );

		} );

	} );

	return result;

} )") : std::string(R"(( [ c_immutable ] ) => {

	const c = vec3( c_immutable ).toVar();
	const r = float( c.x ).toVar();
	const g = float( c.y ).toVar();
	const b = float( c.z ).toVar();
	const mincomp = float( min( r, min( g, b ) ) ).toVar();
	const maxcomp = float( max( r, max( g, b ) ) ).toVar();
	const delta = float( maxcomp.sub( mincomp ) ).toVar();
	const h = float().toVar(), s = float().toVar(), v = float().toVar();
	v.assign( maxcomp );

	If( maxcomp.greaterThan( 0.0 ), () => {

		s.assign( delta.div( maxcomp ) );

	} ).Else( () => {

		s.assign( 0.0 );

	} );

	If( s.lessThanEqual( 0.0 ), () => {

		h.assign( 0.0 );

	} ).Else( () => {

		If( r.greaterThanEqual( maxcomp ), () => {

			h.assign( g.sub( b ).div( delta ) );

		} ).ElseIf( g.greaterThanEqual( maxcomp ), () => {

			h.assign( add( 2.0, b.sub( r ).div( delta ) ) );

		} ).Else( () => {

			h.assign( add( 4.0, r.sub( g ).div( delta ) ) );

		} );

		h.mulAssign( 1.0 / 6.0 );

		If( h.lessThan( 0.0 ), () => {

			h.addAssign( 1.0 );

		} );

	} );

	return vec3( h, s, v );

} )");
        callback.setFunctionSourceText(crlf(source));
        auto layout = JSValue::object(); own(layout, input, "vec3"); own(layout, "return", "vec3");
        own(module, name, nativeTSLBaseBinding("Fn").call({}, {callback, layout}));
    };
    make("mx_hsvtorgb", hsvBody, "hsv");
    make("mx_rgbtohsv", rgbBody, "c");
    auto facade = JSValue::object(); own(facade, "mx_hsvtorgb", module.get("mx_hsvtorgb")); own(facade, "mx_rgbtohsv", module.get("mx_rgbtohsv")); return facade;
}

JSValue nativeMaterialXColorBinding(const std::string& name) { return nativeMaterialXColorModule().get(name); }

