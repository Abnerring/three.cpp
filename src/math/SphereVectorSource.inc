// Exact metadata for Sphere/Frustum/Plane Vector dependencies; see THREEJS-LICENSE.
namespace SphereVectorSource {
struct Method { unsigned dimensions; const char* name; unsigned arity; const char* source; };
constexpr Method methods[]={
{3,"lengthSq",0,"lengthSq() {\r\n\r\n\t\treturn this.x * this.x + this.y * this.y + this.z * this.z;\r\n\r\n\t}"},
{3,"addScaledVector",2,"addScaledVector( v, s ) {\r\n\r\n\t\tthis.x += v.x * s;\r\n\t\tthis.y += v.y * s;\r\n\t\tthis.z += v.z * s;\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
{3,"setLength",1,"setLength( length ) {\r\n\r\n\t\treturn this.normalize().multiplyScalar( length );\r\n\r\n\t}"},
{3,"cross",1,"cross( v ) {\r\n\r\n\t\treturn this.crossVectors( this, v );\r\n\r\n\t}"},
{3,"negate",0,"negate() {\r\n\r\n\t\tthis.x = - this.x;\r\n\t\tthis.y = - this.y;\r\n\t\tthis.z = - this.z;\r\n\r\n\t\treturn this;\r\n\r\n\t}"},
{2,"distanceTo",1,"distanceTo( v ) {\r\n\r\n\t\treturn Math.sqrt( this.distanceToSquared( v ) );\r\n\r\n\t}"},
{2,"distanceToSquared",1,"distanceToSquared( v ) {\r\n\r\n\t\tconst dx = this.x - v.x, dy = this.y - v.y;\r\n\t\treturn dx * dx + dy * dy;\r\n\r\n\t}"},
};
}
