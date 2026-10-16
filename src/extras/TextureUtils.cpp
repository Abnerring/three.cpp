#include "TextureUtils.h"
#include "constants.h"
#include "TextureUtilsSource.inc"
#include <cmath>
#include <initializer_list>
#include <stdexcept>

namespace {
double imageAspect(const JSValue& texture) {
    if (!texture.get("image").toBoolean() || !texture.get("image").get("width").toBoolean()) return 1;
    const auto width = texture.get("image").get("width");
    const auto height = texture.get("image").get("height");
    const auto numerator = width.toNumber();
    return numerator / height.toNumber();
}
JSValue transform(const JSValue& texture, const JSValue& aspect, bool containImage) {
    const auto ratio = imageAspect(texture);
    const bool wider = ratio > aspect.toNumber();
    // Capture each assignment's target before its RHS conversions and getters.
    if (wider == containImage) {
        texture.get("repeat").set("x", 1);
        auto target = texture.get("repeat");
        const auto value = ratio / aspect.toNumber();
        target.set("y", value);
        texture.get("offset").set("x", 0);
        target = texture.get("offset");
        const auto repeat = texture.get("repeat").get("y");
        target.set("y", (1 - repeat.toNumber()) / 2);
    } else {
        auto target = texture.get("repeat");
        const auto value = aspect.toNumber() / ratio;
        target.set("x", value);
        texture.get("repeat").set("y", 1);
        target = texture.get("offset");
        const auto repeat = texture.get("repeat").get("x");
        target.set("x", (1 - repeat.toNumber()) / 2);
        texture.get("offset").set("y", 0);
    }
    return texture;
}
}

JSValue contain(const JSValue& texture, const JSValue& aspect) { return transform(texture, aspect, true); }
JSValue cover(const JSValue& texture, const JSValue& aspect) { return transform(texture, aspect, false); }
JSValue fill(const JSValue& texture) {
    texture.get("repeat").set("x", 1);
    texture.get("repeat").set("y", 1);
    texture.get("offset").set("x", 0);
    texture.get("offset").set("y", 0);
    return texture;
}

namespace {
bool oneOf(const JSValue& value,std::initializer_list<int> values) {
    for(const auto item:values)if(value.strictEquals(item))return true;
    return false;
}
JSValue typeLength(int bytes,int components) {
    auto out=JSValue::object();
    JSPropertyDescriptor descriptor;descriptor.writable=descriptor.enumerable=descriptor.configurable=true;
    descriptor.value=bytes;out.defineProperty("byteLength",descriptor);
    descriptor.value=components;out.defineProperty("components",descriptor);
    return out;
}
JSValue add(const JSValue& value,int offset) {
    const auto primitive=value.toPrimitive();
    if(primitive.type()==JSValue::Type::String)return primitive.toString()+JSValue(offset).toString();
    return primitive.toNumber()+offset;
}
double product(const JSValue& left,const JSValue& right) {
    const auto a=left.toNumber();const auto b=right.toNumber();return a*b;
}
double floorBlocks(const JSValue& width,const JSValue& height,int blockWidth,int blockHeight,int bytes) {
    const auto x=std::floor(add(width,blockWidth-1).toNumber()/blockWidth);
    const auto y=std::floor(add(height,blockHeight-1).toNumber()/blockHeight);
    return x*y*bytes;
}
double ceilBlocks(const JSValue& width,const JSValue& height,int bytes) {
    const auto x=std::ceil(width.toNumber()/4);const auto y=std::ceil(height.toNumber()/4);return x*y*bytes;
}
double maximum(const JSValue& value,int minimum) {
    const auto x=value.toNumber();if(std::isnan(x))return x;return x>minimum?x:minimum;
}
}
JSValue nativeTextureGetTextureTypeByteLength(const JSValue& type) {
    if(oneOf(type,{UnsignedByteType,ByteType}))return typeLength(1,1);
    if(oneOf(type,{UnsignedShortType,ShortType,HalfFloatType}))return typeLength(2,1);
    if(oneOf(type,{UnsignedShort4444Type,UnsignedShort5551Type}))return typeLength(2,4);
    if(oneOf(type,{UnsignedIntType,IntType,FloatType}))return typeLength(4,1);
    if(oneOf(type,{UnsignedInt5999Type,UnsignedInt101111Type}))return typeLength(4,3);
    throw std::runtime_error("THREE.TextureUtils: Unknown texture type "+type.toString()+".");
}
JSValue nativeTextureGetByteLength(const JSValue& width,const JSValue& height,const JSValue& format,const JSValue& type) {
    const auto typeByteLength=nativeTextureGetTextureTypeByteLength(type);
    if(format.strictEquals(AlphaFormat))return product(width,height);
    int channels=0;
    if(oneOf(format,{RedFormat,RedIntegerFormat}))channels=1;
    else if(oneOf(format,{RGFormat,RGIntegerFormat}))channels=2;
    else if(format.strictEquals(RGBFormat))channels=3;
    else if(oneOf(format,{RGBAFormat,RGBAIntegerFormat}))channels=4;
    if(channels) {
        const auto pixels=product(width,height);const auto total=channels==1?pixels:pixels*channels;
        const auto components=typeByteLength.get("components").toNumber();
        const auto divided=total/components;const auto bytes=typeByteLength.get("byteLength").toNumber();
        return divided*bytes;
    }
    if(oneOf(format,{RGB_S3TC_DXT1_Format,RGBA_S3TC_DXT1_Format,RGB_ETC1_Format,RGB_ETC2_Format,R11_EAC_Format,SIGNED_R11_EAC_Format}))return floorBlocks(width,height,4,4,8);
    if(oneOf(format,{RGBA_S3TC_DXT3_Format,RGBA_S3TC_DXT5_Format,RGBA_ETC2_EAC_Format,RG11_EAC_Format,SIGNED_RG11_EAC_Format}))return floorBlocks(width,height,4,4,16);
    if(oneOf(format,{RGB_PVRTC_2BPPV1_Format,RGBA_PVRTC_2BPPV1_Format})) {
        const auto x=maximum(width,16);const auto y=maximum(height,8);return x*y/4;
    }
    if(oneOf(format,{RGB_PVRTC_4BPPV1_Format,RGBA_PVRTC_4BPPV1_Format})) {
        const auto x=maximum(width,8);const auto y=maximum(height,8);return x*y/2;
    }
    if(format.strictEquals(RGBA_ASTC_4x4_Format))return floorBlocks(width,height,4,4,16);
    if(format.strictEquals(RGBA_ASTC_5x4_Format))return floorBlocks(width,height,5,4,16);
    if(format.strictEquals(RGBA_ASTC_5x5_Format))return floorBlocks(width,height,5,5,16);
    if(format.strictEquals(RGBA_ASTC_6x5_Format))return floorBlocks(width,height,6,5,16);
    if(format.strictEquals(RGBA_ASTC_6x6_Format))return floorBlocks(width,height,6,6,16);
    if(format.strictEquals(RGBA_ASTC_8x5_Format))return floorBlocks(width,height,8,5,16);
    if(format.strictEquals(RGBA_ASTC_8x6_Format))return floorBlocks(width,height,8,6,16);
    if(format.strictEquals(RGBA_ASTC_8x8_Format))return floorBlocks(width,height,8,8,16);
    if(format.strictEquals(RGBA_ASTC_10x5_Format))return floorBlocks(width,height,10,5,16);
    if(format.strictEquals(RGBA_ASTC_10x6_Format))return floorBlocks(width,height,10,6,16);
    if(format.strictEquals(RGBA_ASTC_10x8_Format))return floorBlocks(width,height,10,8,16);
    if(format.strictEquals(RGBA_ASTC_10x10_Format))return floorBlocks(width,height,10,10,16);
    if(format.strictEquals(RGBA_ASTC_12x10_Format))return floorBlocks(width,height,12,10,16);
    if(format.strictEquals(RGBA_ASTC_12x12_Format))return floorBlocks(width,height,12,12,16);
    if(oneOf(format,{RGBA_BPTC_Format,RGB_BPTC_SIGNED_Format,RGB_BPTC_UNSIGNED_Format}))return ceilBlocks(width,height,16);
    if(oneOf(format,{RED_RGTC1_Format,SIGNED_RED_RGTC1_Format}))return ceilBlocks(width,height,8);
    if(oneOf(format,{RED_GREEN_RGTC2_Format,SIGNED_RED_GREEN_RGTC2_Format}))return ceilBlocks(width,height,16);
    throw std::runtime_error("Unable to determine texture byte length for "+format.toString()+" format.");
}

JSValue getByteLength(const JSValue& width, const JSValue& height, const JSValue& format, const JSValue& type) {
    return nativeTextureGetByteLength(width, height, format, type);
}

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
JSValue invoke(const JSValue&, const Args& args, const Args& captures) {
    const auto& name = captures[0].string();
    if (name == "contain") return contain(arg(args, 0), arg(args, 1));
    if (name == "cover") return cover(arg(args, 0), arg(args, 1));
    if (name == "fill") return fill(arg(args, 0));
    return getByteLength(arg(args, 0), arg(args, 1), arg(args, 2), arg(args, 3));
}
void define(JSValue target, const char* name, JSValue value, bool enumerable = false) {
    JSPropertyDescriptor descriptor;
    descriptor.value = std::move(value);
    descriptor.writable = descriptor.configurable = true;
    descriptor.enumerable = enumerable;
    target.defineProperty(name, descriptor);
}
JSValue exports() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isObject()) return *value;
    auto environment = JSValue::object(nullptr);
    auto constructor = JSValue::constructor([](const JSValue&, const Args&, const Args&) { return JSValue{}; }, {}, "TextureUtils", 0);
    constructor.setFunctionSourceText(TextureUtilsSource::TextureUtils);
    struct Export { const char* name; unsigned arity; const char* source; const char* method; };
    for (const auto& item : {
        Export{"contain", 2, TextureUtilsSource::contain, TextureUtilsSource::static_contain},
        Export{"cover", 2, TextureUtilsSource::cover, TextureUtilsSource::static_cover},
        Export{"fill", 1, TextureUtilsSource::fill, TextureUtilsSource::static_fill},
        Export{"getByteLength", 4, TextureUtilsSource::getByteLength, TextureUtilsSource::static_getByteLength}}) {
        auto function = JSValue::constructor(invoke, {item.name}, item.name, item.arity, JSValue::ConstructorKind::Callable);
        function.setFunctionSourceText(item.source);
        define(environment, item.name, function);
        auto method = JSValue::function(invoke, {item.name}, item.name, item.arity);
        method.setFunctionSourceText(item.method);
        define(constructor, item.name, method);
    }
    define(environment, "TextureUtils", constructor);
    environment.retainForHeapLifetime();
    cached = JSWeakValue(environment);
    return environment;
}
}
JSValue nativeTextureContain() { return exports().get("contain"); }
JSValue nativeTextureCover() { return exports().get("cover"); }
JSValue nativeTextureFill() { return exports().get("fill"); }
JSValue nativeTextureByteLength() { return exports().get("getByteLength"); }
JSValue nativeTextureUtils() { return exports().get("TextureUtils"); }
