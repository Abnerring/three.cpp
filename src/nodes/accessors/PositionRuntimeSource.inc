// Generated from pinned Position.js arrow callbacks; metadata only.
// Source SHA256: ba2f23475cff68abbf0b35c76b89d4d0fc17f31321ebef8a5f4633f2f34f2564
#pragma once
#include <string_view>
#include <stdexcept>
namespace PositionSourceMetadata {
inline std::string_view source(std::string_view name) {
    if (name == "clipSpace") return "( builder ) => {\r\n\r\n\tif ( builder.shaderStage !== 'fragment' ) {\r\n\r\n\t\twarnOnce( 'TSL: `clipSpace` is only available in fragment stage.' );\r\n\r\n\t\treturn vec4();\r\n\r\n\t}\r\n\r\n\treturn builder.context.clipSpace.toVarying( 'v_clipSpace' );\r\n\r\n}";
    if (name == "positionWorld") return "( builder ) => {\r\n\r\n\treturn modelWorldMatrix.mul( positionLocal ).xyz.toVarying( builder.getSubBuildProperty( 'v_positionWorld' ) );\r\n\r\n}";
    if (name == "positionWorldDirection") return "() => {\r\n\r\n\tconst vertexPWD = positionLocal.transformDirection( modelWorldMatrix ).toVarying( 'v_positionWorldDirection' );\r\n\r\n\treturn vertexPWD.normalize().toVar( 'positionWorldDirection' );\r\n\r\n}";
    if (name == "positionView") return "( builder ) => {\r\n\r\n\tif ( builder.shaderStage === 'fragment' && builder.material.vertexNode ) {\r\n\r\n\t\t// reconstruct view position from clip space\r\n\r\n\t\tconst viewPos = cameraProjectionMatrixInverse.mul( clipSpace );\r\n\r\n\t\treturn viewPos.xyz.div( viewPos.w ).toVar( 'positionView' );\r\n\r\n\t}\r\n\r\n\treturn builder.context.setupPositionView().toVarying( 'v_positionView' );\r\n\r\n}";
    if (name == "positionViewDirection") return "( builder ) => {\r\n\r\n\tlet output;\r\n\r\n\tif ( builder.camera.isOrthographicCamera ) {\r\n\r\n\t\toutput = vec3( 0, 0, 1 );\r\n\r\n\t} else {\r\n\r\n\t\toutput = positionView.negate().toVarying( 'v_positionViewDirection' ).normalize();\r\n\r\n\t}\r\n\r\n\treturn output.toVar( 'positionViewDirection' );\r\n\r\n}";
    throw std::logic_error("Unknown Position callback source");
}
}
