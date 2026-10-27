// Exact pinned-upstream callback metadata, never executed; see THREEJS-LICENSE.
namespace MaterialPropertiesSource {
inline constexpr const char* materialRefractionRatio_reference="( { material } ) => material";
inline constexpr const char* materialRefractionRatio_update="( { material } ) => material.refractionRatio";
inline constexpr const char* materialEnvIntensity_reference="( { material } ) => material";
inline constexpr const char* materialEnvIntensity_update="function ( { material, scene } ) {\r\n\r\n\treturn material.envMap ? material.envMapIntensity : scene.environmentIntensity;\r\n\r\n}";
inline constexpr const char* materialEnvRotation_reference="function ( frame ) {\r\n\r\n\treturn frame.material;\r\n\r\n}";
inline constexpr const char* materialEnvRotation_update="function ( { material, scene } ) {\r\n\r\n\tconst hasSceneEnvironment = ( scene.environment !== null ) || ( scene.environmentNode && scene.environmentNode.isNode );\r\n\tconst rotation = ( hasSceneEnvironment && material.envMap === null ) ? scene.environmentRotation : material.envMapRotation;\r\n\r\n\tif ( rotation ) {\r\n\r\n\t\t// note: since the matrix is orthonormal, we can use the more-efficient transpose() in lieu of invert()\r\n\t\t_m1.makeRotationFromEuler( rotation ).transpose();\r\n\r\n\t} else {\r\n\r\n\t\t_m1.identity();\r\n\r\n\t}\r\n\r\n\treturn _m1;\r\n\r\n}";
}
