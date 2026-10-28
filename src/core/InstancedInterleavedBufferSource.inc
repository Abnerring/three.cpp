// Original fixed-upstream source metadata; see THREEJS-LICENSE.
namespace InstancedInterleavedBufferSource {
inline constexpr const char* classSource="class InstancedInterleavedBuffer extends InterleavedBuffer {\r\n\r\n\t/**\r\n\t * Constructs a new instanced interleaved buffer.\r\n\t *\r\n\t * @param {TypedArray} array - A typed array with a shared buffer storing attribute data.\r\n\t * @param {number} stride - The number of typed-array elements per vertex.\r\n\t * @param {number} [meshPerAttribute=1] - Defines how often a value of this interleaved buffer should be repeated.\r\n\t */\r\n\tconstructor( array, stride, meshPerAttribute = 1 ) {\r\n\r\n\t\tsuper( array, stride );\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isInstancedInterleavedBuffer = true;\r\n\r\n\t\t/**\r\n\t\t * Defines how often a value of this buffer attribute should be repeated,\r\n\t\t * see {@link InstancedBufferAttribute#meshPerAttribute}.\r\n\t\t *\r\n\t\t * @type {number}\r\n\t\t * @default 1\r\n\t\t */\r\n\t\tthis.meshPerAttribute = meshPerAttribute;\r\n\r\n\t}\r\n\r\n\tcopy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.meshPerAttribute = source.meshPerAttribute;\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\tclone( data ) {\r\n\r\n\t\tconst ib = super.clone( data );\r\n\r\n\t\tib.meshPerAttribute = this.meshPerAttribute;\r\n\r\n\t\treturn ib;\r\n\r\n\t}\r\n\r\n\ttoJSON( data ) {\r\n\r\n\t\tconst json = super.toJSON( data );\r\n\r\n\t\tjson.isInstancedInterleavedBuffer = true;\r\n\t\tjson.meshPerAttribute = this.meshPerAttribute;\r\n\r\n\t\treturn json;\r\n\r\n\t}\r\n\r\n}";
inline constexpr const char* copySource="copy( source ) {\r\n\r\n\t\tsuper.copy( source );\r\n\r\n\t\tthis.meshPerAttribute = source.meshPerAttribute;\r\n\r\n\t\treturn this;\r\n\r\n\t}";
inline constexpr const char* cloneSource="clone( data ) {\r\n\r\n\t\tconst ib = super.clone( data );\r\n\r\n\t\tib.meshPerAttribute = this.meshPerAttribute;\r\n\r\n\t\treturn ib;\r\n\r\n\t}";
inline constexpr const char* toJSONSource="toJSON( data ) {\r\n\r\n\t\tconst json = super.toJSON( data );\r\n\r\n\t\tjson.isInstancedInterleavedBuffer = true;\r\n\t\tjson.meshPerAttribute = this.meshPerAttribute;\r\n\r\n\t\treturn json;\r\n\r\n\t}";
struct MethodSource { const char* name; unsigned arity; const char* source; bool setter; };
inline constexpr MethodSource methods[]={
{"copy",1,copySource,false},
{"clone",1,cloneSource,false},
{"toJSON",1,toJSONSource,false},
};
}
