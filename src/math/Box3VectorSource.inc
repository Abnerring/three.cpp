// Exact source metadata for the added Vector3 methods; see THREEJS-LICENSE.
namespace Box3VectorSource {
struct Method { const char* name; unsigned arity; const char* source; };
constexpr Method methods[]={
{"add",1,"add( v ) {\r\n\r\n\t\tthis.x += v.x;\r\n\t\tthis.y += v.y;\r\n\t\tthis.z += v.z;\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
{"addScalar",1,"addScalar( s ) {\r\n\r\n\t\tthis.x += s;\r\n\t\tthis.y += s;\r\n\t\tthis.z += s;\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
{"addVectors",2,"addVectors( a, b ) {\r\n\r\n\t\tthis.x = a.x + b.x;\r\n\t\tthis.y = a.y + b.y;\r\n\t\tthis.z = a.z + b.z;\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
{"sub",1,"sub( v ) {\r\n\r\n\t\tthis.x -= v.x;\r\n\t\tthis.y -= v.y;\r\n\t\tthis.z -= v.z;\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
{"subVectors",2,"subVectors( a, b ) {\r\n\r\n\t\tthis.x = a.x - b.x;\r\n\t\tthis.y = a.y - b.y;\r\n\t\tthis.z = a.z - b.z;\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
{"min",1,"min( v ) {\r\n\r\n\t\tthis.x = Math.min( this.x, v.x );\r\n\t\tthis.y = Math.min( this.y, v.y );\r\n\t\tthis.z = Math.min( this.z, v.z );\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
{"max",1,"max( v ) {\r\n\r\n\t\tthis.x = Math.max( this.x, v.x );\r\n\t\tthis.y = Math.max( this.y, v.y );\r\n\t\tthis.z = Math.max( this.z, v.z );\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
{"clamp",2,"clamp( min, max ) {\r\n\r\n\t\t// assumes min < max, componentwise\r\n\r\n\t\tthis.x = clamp( this.x, min.x, max.x );\r\n\t\tthis.y = clamp( this.y, min.y, max.y );\r\n\t\tthis.z = clamp( this.z, min.z, max.z );\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
{"distanceTo",1,"distanceTo( v ) {\r\n\r\n\t\treturn Math.sqrt( this.distanceToSquared( v ) );\r\n\r\n\t}"},
{"distanceToSquared",1,"distanceToSquared( v ) {\r\n\r\n\t\tconst dx = this.x - v.x, dy = this.y - v.y, dz = this.z - v.z;\r\n\r\n\t\treturn dx * dx + dy * dy + dz * dz;\r\n\r\n\t}"},
{"dot",1,"dot( v ) {\r\n\r\n\t\treturn this.x * v.x + this.y * v.y + this.z * v.z;\r\n\r\n\t}"},
{"crossVectors",2,"crossVectors( a, b ) {\r\n\r\n\t\tconst ax = a.x, ay = a.y, az = a.z;\r\n\t\tconst bx = b.x, by = b.y, bz = b.z;\r\n\r\n\t\tthis.x = ay * bz - az * by;\r\n\t\tthis.y = az * bx - ax * bz;\r\n\t\tthis.z = ax * by - ay * bx;\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
};
}
