// Generated exact math method source; see THREEJS-LICENSE.
namespace NativeMathStateSource {
// src/math/Vector4.js SHA256 ffbffd6ea1f615d1f5dca9896e8c6928d7d9ce4427e67019bd9c77c4b2b4e0ff
inline constexpr const char* Vector4_set = "set( x, y, z, w ) {\r\n\r\n\t\tthis.x = x;\r\n\t\tthis.y = y;\r\n\t\tthis.z = z;\r\n\t\tthis.w = w;\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr const char* Vector4_copy = "copy( v ) {\r\n\r\n\t\tthis.x = v.x;\r\n\t\tthis.y = v.y;\r\n\t\tthis.z = v.z;\r\n\t\tthis.w = ( v.w !== undefined ) ? v.w : 1;\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr const char* Vector4_fromArray = "fromArray( array, offset = 0 ) {\r\n\r\n\t\tthis.x = array[ offset ];\r\n\t\tthis.y = array[ offset + 1 ];\r\n\t\tthis.z = array[ offset + 2 ];\r\n\t\tthis.w = array[ offset + 3 ];\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr const char* Vector4_toArray = "toArray( array = [], offset = 0 ) {\r\n\r\n\t\tarray[ offset ] = this.x;\r\n\t\tarray[ offset + 1 ] = this.y;\r\n\t\tarray[ offset + 2 ] = this.z;\r\n\t\tarray[ offset + 3 ] = this.w;\r\n\r\n\t\treturn array;\r\n\r\n\t}";
// src/math/Color.js SHA256 5030bdac4ab06cc8ee14b3cb17dfd17779e7f3b3d094ea15b282eb355d08cf99
inline constexpr const char* Color_set = "set( r, g, b ) {\r\n\r\n\t\tif ( g === undefined && b === undefined ) {\r\n\r\n\t\t\t// r is THREE.Color, hex or string\r\n\r\n\t\t\tconst value = r;\r\n\r\n\t\t\tif ( value && value.isColor ) {\r\n\r\n\t\t\t\tthis.copy( value );\r\n\r\n\t\t\t} else if ( typeof value === 'number' ) {\r\n\r\n\t\t\t\tthis.setHex( value );\r\n\r\n\t\t\t} else if ( typeof value === 'string' ) {\r\n\r\n\t\t\t\tthis.setStyle( value );\r\n\r\n\t\t\t}\r\n\r\n\t\t} else {\r\n\r\n\t\t\tthis.setRGB( r, g, b );\r\n\r\n\t\t}\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr const char* Color_setRGB = "setRGB( r, g, b, colorSpace = ColorManagement.workingColorSpace ) {\r\n\r\n\t\tthis.r = r;\r\n\t\tthis.g = g;\r\n\t\tthis.b = b;\r\n\r\n\t\tColorManagement.colorSpaceToWorking( this, colorSpace );\r\n\r\n\t\treturn this;\r\n\r\n\t}";
// src/math/Quaternion.js SHA256 9c36e4430316aa52d2f5da1c126b269600f352ecd2360d91c8e40b13878bb93a
inline constexpr const char* Quaternion_fromArray = "fromArray( array, offset = 0 ) {\r\n\r\n\t\tthis._x = array[ offset ];\r\n\t\tthis._y = array[ offset + 1 ];\r\n\t\tthis._z = array[ offset + 2 ];\r\n\t\tthis._w = array[ offset + 3 ];\r\n\r\n\t\tthis._onChangeCallback();\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr const char* Quaternion_toArray = "toArray( array = [], offset = 0 ) {\r\n\r\n\t\tarray[ offset ] = this._x;\r\n\t\tarray[ offset + 1 ] = this._y;\r\n\t\tarray[ offset + 2 ] = this._z;\r\n\t\tarray[ offset + 3 ] = this._w;\r\n\r\n\t\treturn array;\r\n\r\n\t}";
inline constexpr const char* Quaternion__onChangeCallback = "_onChangeCallback() {}";
inline constexpr const char* Quaternion__onChange = "_onChange( callback ) {\r\n\r\n\t\tthis._onChangeCallback = callback;\r\n\r\n\t\treturn this;\r\n\r\n\t}";
// src/math/Euler.js SHA256 f5b91fe25e73bd72d7f9e2ad450554c1b7180d565fc091d5acfefaddd5b00692
inline constexpr const char* Euler_fromArray = "fromArray( array ) {\r\n\r\n\t\tthis._x = array[ 0 ];\r\n\t\tthis._y = array[ 1 ];\r\n\t\tthis._z = array[ 2 ];\r\n\t\tif ( array[ 3 ] !== undefined ) this._order = array[ 3 ];\r\n\r\n\t\tthis._onChangeCallback();\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr const char* Euler_toArray = "toArray( array = [], offset = 0 ) {\r\n\r\n\t\tarray[ offset ] = this._x;\r\n\t\tarray[ offset + 1 ] = this._y;\r\n\t\tarray[ offset + 2 ] = this._z;\r\n\t\tarray[ offset + 3 ] = this._order;\r\n\r\n\t\treturn array;\r\n\r\n\t}";
inline constexpr const char* Euler__onChangeCallback = "_onChangeCallback() {}";
inline constexpr const char* Euler__onChange = "_onChange( callback ) {\r\n\r\n\t\tthis._onChangeCallback = callback;\r\n\r\n\t\treturn this;\r\n\r\n\t}";
}
