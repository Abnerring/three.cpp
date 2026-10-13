// Ported from three.js WebGLUtils.js; see THREEJS-LICENSE.
#include "renderers/webgl/WebGLUtils.h"
#include "math/ColorManagement.h"
#include "constants.h"
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue();}
JSValue convert(const JSValue&,const Args& args,const Args& captures) {
    const auto gl=captures[0],extensions=captures[1],p=arg(args,0);
    auto colorSpace=arg(args,1);if(colorSpace.isUndefined())colorSpace=NoColorSpace;
    JSValue extension;
    const auto management=nativeColorManagement(),method=management.get("getTransfer");
    const auto transfer=method.call(management,{colorSpace});
		if ( p.strictEquals(UnsignedByteType) ) return gl.get("UNSIGNED_BYTE");
		if ( p.strictEquals(UnsignedShort4444Type) ) return gl.get("UNSIGNED_SHORT_4_4_4_4");
		if ( p.strictEquals(UnsignedShort5551Type) ) return gl.get("UNSIGNED_SHORT_5_5_5_1");
		if ( p.strictEquals(UnsignedInt5999Type) ) return gl.get("UNSIGNED_INT_5_9_9_9_REV");
		if ( p.strictEquals(UnsignedInt101111Type) ) return gl.get("UNSIGNED_INT_10F_11F_11F_REV");

		if ( p.strictEquals(ByteType) ) return gl.get("BYTE");
		if ( p.strictEquals(ShortType) ) return gl.get("SHORT");
		if ( p.strictEquals(UnsignedShortType) ) return gl.get("UNSIGNED_SHORT");
		if ( p.strictEquals(IntType) ) return gl.get("INT");
		if ( p.strictEquals(UnsignedIntType) ) return gl.get("UNSIGNED_INT");
		if ( p.strictEquals(FloatType) ) return gl.get("FLOAT");
		if ( p.strictEquals(HalfFloatType) ) return gl.get("HALF_FLOAT");

		if ( p.strictEquals(AlphaFormat) ) return gl.get("ALPHA");
		if ( p.strictEquals(RGBFormat) ) return gl.get("RGB");
		if ( p.strictEquals(RGBAFormat) ) return gl.get("RGBA");
		if ( p.strictEquals(DepthFormat) ) return gl.get("DEPTH_COMPONENT");
		if ( p.strictEquals(DepthStencilFormat) ) return gl.get("DEPTH_STENCIL");

		// WebGL2 formats.

		if ( p.strictEquals(RedFormat) ) return gl.get("RED");
		if ( p.strictEquals(RedIntegerFormat) ) return gl.get("RED_INTEGER");
		if ( p.strictEquals(RGFormat) ) return gl.get("RG");
		if ( p.strictEquals(RGIntegerFormat) ) return gl.get("RG_INTEGER");
		if ( p.strictEquals(RGBAIntegerFormat) ) return gl.get("RGBA_INTEGER");

		// S3TC

		if ( p.strictEquals(RGB_S3TC_DXT1_Format) || p.strictEquals(RGBA_S3TC_DXT1_Format) || p.strictEquals(RGBA_S3TC_DXT3_Format) || p.strictEquals(RGBA_S3TC_DXT5_Format) ) {

			if ( transfer.strictEquals(SRGBTransfer) ) {

				extension = extensions.get("get").call(extensions,{"WEBGL_compressed_texture_s3tc_srgb"});

				if ( !extension.isNull() ) {

					if ( p.strictEquals(RGB_S3TC_DXT1_Format) ) return extension.get("COMPRESSED_SRGB_S3TC_DXT1_EXT");
					if ( p.strictEquals(RGBA_S3TC_DXT1_Format) ) return extension.get("COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT");
					if ( p.strictEquals(RGBA_S3TC_DXT3_Format) ) return extension.get("COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT");
					if ( p.strictEquals(RGBA_S3TC_DXT5_Format) ) return extension.get("COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT");

				} else {

					return nullptr;

				}

			} else {

				extension = extensions.get("get").call(extensions,{"WEBGL_compressed_texture_s3tc"});

				if ( !extension.isNull() ) {

					if ( p.strictEquals(RGB_S3TC_DXT1_Format) ) return extension.get("COMPRESSED_RGB_S3TC_DXT1_EXT");
					if ( p.strictEquals(RGBA_S3TC_DXT1_Format) ) return extension.get("COMPRESSED_RGBA_S3TC_DXT1_EXT");
					if ( p.strictEquals(RGBA_S3TC_DXT3_Format) ) return extension.get("COMPRESSED_RGBA_S3TC_DXT3_EXT");
					if ( p.strictEquals(RGBA_S3TC_DXT5_Format) ) return extension.get("COMPRESSED_RGBA_S3TC_DXT5_EXT");

				} else {

					return nullptr;

				}

			}

		}

		// PVRTC

		if ( p.strictEquals(RGB_PVRTC_4BPPV1_Format) || p.strictEquals(RGB_PVRTC_2BPPV1_Format) || p.strictEquals(RGBA_PVRTC_4BPPV1_Format) || p.strictEquals(RGBA_PVRTC_2BPPV1_Format) ) {

			extension = extensions.get("get").call(extensions,{"WEBGL_compressed_texture_pvrtc"});

			if ( !extension.isNull() ) {

				if ( p.strictEquals(RGB_PVRTC_4BPPV1_Format) ) return extension.get("COMPRESSED_RGB_PVRTC_4BPPV1_IMG");
				if ( p.strictEquals(RGB_PVRTC_2BPPV1_Format) ) return extension.get("COMPRESSED_RGB_PVRTC_2BPPV1_IMG");
				if ( p.strictEquals(RGBA_PVRTC_4BPPV1_Format) ) return extension.get("COMPRESSED_RGBA_PVRTC_4BPPV1_IMG");
				if ( p.strictEquals(RGBA_PVRTC_2BPPV1_Format) ) return extension.get("COMPRESSED_RGBA_PVRTC_2BPPV1_IMG");

			} else {

				return nullptr;

			}

		}

		// ETC

		if ( p.strictEquals(RGB_ETC1_Format) || p.strictEquals(RGB_ETC2_Format) || p.strictEquals(RGBA_ETC2_EAC_Format) || p.strictEquals(R11_EAC_Format) || p.strictEquals(SIGNED_R11_EAC_Format) || p.strictEquals(RG11_EAC_Format) || p.strictEquals(SIGNED_RG11_EAC_Format) ) {

			extension = extensions.get("get").call(extensions,{"WEBGL_compressed_texture_etc"});

			if ( !extension.isNull() ) {

				if ( p.strictEquals(RGB_ETC1_Format) || p.strictEquals(RGB_ETC2_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ETC2") : extension.get("COMPRESSED_RGB8_ETC2");
				if ( p.strictEquals(RGBA_ETC2_EAC_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ETC2_EAC") : extension.get("COMPRESSED_RGBA8_ETC2_EAC");
				if ( p.strictEquals(R11_EAC_Format) ) return extension.get("COMPRESSED_R11_EAC");
				if ( p.strictEquals(SIGNED_R11_EAC_Format) ) return extension.get("COMPRESSED_SIGNED_R11_EAC");
				if ( p.strictEquals(RG11_EAC_Format) ) return extension.get("COMPRESSED_RG11_EAC");
				if ( p.strictEquals(SIGNED_RG11_EAC_Format) ) return extension.get("COMPRESSED_SIGNED_RG11_EAC");

			} else {

				return nullptr;

			}

		}

		// ASTC

		if ( p.strictEquals(RGBA_ASTC_4x4_Format) || p.strictEquals(RGBA_ASTC_5x4_Format) || p.strictEquals(RGBA_ASTC_5x5_Format) ||
			p.strictEquals(RGBA_ASTC_6x5_Format) || p.strictEquals(RGBA_ASTC_6x6_Format) || p.strictEquals(RGBA_ASTC_8x5_Format) ||
			p.strictEquals(RGBA_ASTC_8x6_Format) || p.strictEquals(RGBA_ASTC_8x8_Format) || p.strictEquals(RGBA_ASTC_10x5_Format) ||
			p.strictEquals(RGBA_ASTC_10x6_Format) || p.strictEquals(RGBA_ASTC_10x8_Format) || p.strictEquals(RGBA_ASTC_10x10_Format) ||
			p.strictEquals(RGBA_ASTC_12x10_Format) || p.strictEquals(RGBA_ASTC_12x12_Format) ) {

			extension = extensions.get("get").call(extensions,{"WEBGL_compressed_texture_astc"});

			if ( !extension.isNull() ) {

				if ( p.strictEquals(RGBA_ASTC_4x4_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR") : extension.get("COMPRESSED_RGBA_ASTC_4x4_KHR");
				if ( p.strictEquals(RGBA_ASTC_5x4_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR") : extension.get("COMPRESSED_RGBA_ASTC_5x4_KHR");
				if ( p.strictEquals(RGBA_ASTC_5x5_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR") : extension.get("COMPRESSED_RGBA_ASTC_5x5_KHR");
				if ( p.strictEquals(RGBA_ASTC_6x5_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR") : extension.get("COMPRESSED_RGBA_ASTC_6x5_KHR");
				if ( p.strictEquals(RGBA_ASTC_6x6_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR") : extension.get("COMPRESSED_RGBA_ASTC_6x6_KHR");
				if ( p.strictEquals(RGBA_ASTC_8x5_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR") : extension.get("COMPRESSED_RGBA_ASTC_8x5_KHR");
				if ( p.strictEquals(RGBA_ASTC_8x6_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR") : extension.get("COMPRESSED_RGBA_ASTC_8x6_KHR");
				if ( p.strictEquals(RGBA_ASTC_8x8_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR") : extension.get("COMPRESSED_RGBA_ASTC_8x8_KHR");
				if ( p.strictEquals(RGBA_ASTC_10x5_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR") : extension.get("COMPRESSED_RGBA_ASTC_10x5_KHR");
				if ( p.strictEquals(RGBA_ASTC_10x6_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR") : extension.get("COMPRESSED_RGBA_ASTC_10x6_KHR");
				if ( p.strictEquals(RGBA_ASTC_10x8_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR") : extension.get("COMPRESSED_RGBA_ASTC_10x8_KHR");
				if ( p.strictEquals(RGBA_ASTC_10x10_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR") : extension.get("COMPRESSED_RGBA_ASTC_10x10_KHR");
				if ( p.strictEquals(RGBA_ASTC_12x10_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR") : extension.get("COMPRESSED_RGBA_ASTC_12x10_KHR");
				if ( p.strictEquals(RGBA_ASTC_12x12_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR") : extension.get("COMPRESSED_RGBA_ASTC_12x12_KHR");

			} else {

				return nullptr;

			}

		}

		// BPTC

		if ( p.strictEquals(RGBA_BPTC_Format) || p.strictEquals(RGB_BPTC_SIGNED_Format) || p.strictEquals(RGB_BPTC_UNSIGNED_Format) ) {

			extension = extensions.get("get").call(extensions,{"EXT_texture_compression_bptc"});

			if ( !extension.isNull() ) {

				if ( p.strictEquals(RGBA_BPTC_Format) ) return ( transfer.strictEquals(SRGBTransfer) ) ? extension.get("COMPRESSED_SRGB_ALPHA_BPTC_UNORM_EXT") : extension.get("COMPRESSED_RGBA_BPTC_UNORM_EXT");
				if ( p.strictEquals(RGB_BPTC_SIGNED_Format) ) return extension.get("COMPRESSED_RGB_BPTC_SIGNED_FLOAT_EXT");
				if ( p.strictEquals(RGB_BPTC_UNSIGNED_Format) ) return extension.get("COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_EXT");

			} else {

				return nullptr;

			}

		}

		// RGTC

		if ( p.strictEquals(RED_RGTC1_Format) || p.strictEquals(SIGNED_RED_RGTC1_Format) || p.strictEquals(RED_GREEN_RGTC2_Format) || p.strictEquals(SIGNED_RED_GREEN_RGTC2_Format) ) {

			extension = extensions.get("get").call(extensions,{"EXT_texture_compression_rgtc"});

			if ( !extension.isNull() ) {

				if ( p.strictEquals(RED_RGTC1_Format) ) return extension.get("COMPRESSED_RED_RGTC1_EXT");
				if ( p.strictEquals(SIGNED_RED_RGTC1_Format) ) return extension.get("COMPRESSED_SIGNED_RED_RGTC1_EXT");
				if ( p.strictEquals(RED_GREEN_RGTC2_Format) ) return extension.get("COMPRESSED_RED_GREEN_RGTC2_EXT");
				if ( p.strictEquals(SIGNED_RED_GREEN_RGTC2_Format) ) return extension.get("COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT");

			} else {

				return nullptr;

			}

		}

		//

		if ( p.strictEquals(UnsignedInt248Type) ) return gl.get("UNSIGNED_INT_24_8");

    const auto first=gl.get(p);return !first.isUndefined()?gl.get(p):JSValue(nullptr);
}
JSValue initialize(const JSValue&,const Args& args,const Args&) {
    const auto gl=arg(args,0),extensions=arg(args,1);auto object=JSValue::object();
    JSPropertyDescriptor d;d.value=JSValue::constructor(convert,{gl,extensions},"convert",1,JSValue::ConstructorKind::Callable);
    d.writable=d.enumerable=d.configurable=true;object.defineProperty("convert",d);return object;
}
}
JSValue nativeWebGLUtilsConstructor(){static thread_local JSWeakValue cached;if(auto value=cached.lock();value&&value->isCallable())return *value;auto value=JSValue::constructor(initialize,{},"WebGLUtils",2,JSValue::ConstructorKind::Callable);value.retainForHeapLifetime();cached=JSWeakValue(value);return value;}
WebGLUtils::WebGLUtils(const JSValue& gl,const JSValue& extensions):object_(nativeWebGLUtilsConstructor().construct({gl,extensions})){}
JSValue WebGLUtils::convert(const JSValue& p,const JSValue& colorSpace){return object().get("convert").call(object_,{p,colorSpace});}
