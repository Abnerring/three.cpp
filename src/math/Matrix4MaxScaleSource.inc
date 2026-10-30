// Exact pinned Matrix4 source metadata; see THREEJS-LICENSE.
namespace Matrix4MaxScaleSource {
constexpr const char* methodSource="getMaxScaleOnAxis() {\r\n\r\n\t\tconst te = this.elements;\r\n\r\n\t\tconst scaleXSq = te[ 0 ] * te[ 0 ] + te[ 1 ] * te[ 1 ] + te[ 2 ] * te[ 2 ];\r\n\t\tconst scaleYSq = te[ 4 ] * te[ 4 ] + te[ 5 ] * te[ 5 ] + te[ 6 ] * te[ 6 ];\r\n\t\tconst scaleZSq = te[ 8 ] * te[ 8 ] + te[ 9 ] * te[ 9 ] + te[ 10 ] * te[ 10 ];\r\n\r\n\t\treturn Math.sqrt( Math.max( scaleXSq, scaleYSq, scaleZSq ) );\r\n\r\n\t}";
}
