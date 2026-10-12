// Generated from the pinned three.js UniformsLib.js by utils/generate-uniforms.mjs.
#include "renderers/shaders/UniformsLib.h"
#include "core/NativeObject.h"
#include "math/Color.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Matrix3.h"
#include <initializer_list>
#include <utility>

namespace {
JSValue object(std::initializer_list<std::pair<const char*,JSValue>> fields) {
    auto result = JSValue::object();
    for (const auto& [name,value] : fields) result.set(name,value);
    return result;
}
}
thread_local JSValue UniformsLib = object({
    {"common", object({
        {"diffuse", object({
            {"value", nativeColor(std::make_shared<Color>(16777215))}
        })},
        {"opacity", object({
            {"value", 1}
        })},
        {"map", object({
            {"value", nullptr}
        })},
        {"mapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })},
        {"alphaMap", object({
            {"value", nullptr}
        })},
        {"alphaMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })},
        {"alphaTest", object({
            {"value", 0}
        })}
    })},
    {"specularmap", object({
        {"specularMap", object({
            {"value", nullptr}
        })},
        {"specularMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })}
    })},
    {"envmap", object({
        {"envMap", object({
            {"value", nullptr}
        })},
        {"envMapRotation", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })},
        {"reflectivity", object({
            {"value", 1}
        })},
        {"ior", object({
            {"value", 1.5}
        })},
        {"refractionRatio", object({
            {"value", 0.98}
        })},
        {"dfgLUT", object({
            {"value", nullptr}
        })}
    })},
    {"aomap", object({
        {"aoMap", object({
            {"value", nullptr}
        })},
        {"aoMapIntensity", object({
            {"value", 1}
        })},
        {"aoMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })}
    })},
    {"lightmap", object({
        {"lightMap", object({
            {"value", nullptr}
        })},
        {"lightMapIntensity", object({
            {"value", 1}
        })},
        {"lightMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })}
    })},
    {"bumpmap", object({
        {"bumpMap", object({
            {"value", nullptr}
        })},
        {"bumpMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })},
        {"bumpScale", object({
            {"value", 1}
        })}
    })},
    {"normalmap", object({
        {"normalMap", object({
            {"value", nullptr}
        })},
        {"normalMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })},
        {"normalScale", object({
            {"value", nativeVector(std::make_shared<Vector2>(1, 1))}
        })}
    })},
    {"displacementmap", object({
        {"displacementMap", object({
            {"value", nullptr}
        })},
        {"displacementMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })},
        {"displacementScale", object({
            {"value", 1}
        })},
        {"displacementBias", object({
            {"value", 0}
        })}
    })},
    {"emissivemap", object({
        {"emissiveMap", object({
            {"value", nullptr}
        })},
        {"emissiveMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })}
    })},
    {"metalnessmap", object({
        {"metalnessMap", object({
            {"value", nullptr}
        })},
        {"metalnessMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })}
    })},
    {"roughnessmap", object({
        {"roughnessMap", object({
            {"value", nullptr}
        })},
        {"roughnessMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })}
    })},
    {"gradientmap", object({
        {"gradientMap", object({
            {"value", nullptr}
        })}
    })},
    {"fog", object({
        {"fogDensity", object({
            {"value", 0.00025}
        })},
        {"fogNear", object({
            {"value", 1}
        })},
        {"fogFar", object({
            {"value", 2000}
        })},
        {"fogColor", object({
            {"value", nativeColor(std::make_shared<Color>(16777215))}
        })}
    })},
    {"lights", object({
        {"ambientLightColor", object({
            {"value", JSValue::array()}
        })},
        {"lightProbe", object({
            {"value", JSValue::array()}
        })},
        {"sunLights", object({
            {"value", JSValue::array()},
            {"properties", object({
                {"direction", object({

                })},
                {"color", object({

                })}
            })}
        })},
        {"sunLightShadows", object({
            {"value", JSValue::array()},
            {"properties", object({
                {"shadowIntensity", 1},
                {"shadowBias", object({

                })},
                {"shadowNormalBias", object({

                })},
                {"shadowRadius", object({

                })},
                {"shadowMapSize", object({

                })}
            })}
        })},
        {"sunShadowMatrix", object({
            {"value", JSValue::array()}
        })},
        {"sunShadowCascade", object({
            {"value", JSValue::array()}
        })},
        {"directionalLights", object({
            {"value", JSValue::array()},
            {"properties", object({
                {"direction", object({

                })},
                {"color", object({

                })}
            })}
        })},
        {"directionalLightShadows", object({
            {"value", JSValue::array()},
            {"properties", object({
                {"shadowIntensity", 1},
                {"shadowBias", object({

                })},
                {"shadowNormalBias", object({

                })},
                {"shadowRadius", object({

                })},
                {"shadowMapSize", object({

                })}
            })}
        })},
        {"directionalShadowMatrix", object({
            {"value", JSValue::array()}
        })},
        {"spotLights", object({
            {"value", JSValue::array()},
            {"properties", object({
                {"color", object({

                })},
                {"position", object({

                })},
                {"direction", object({

                })},
                {"distance", object({

                })},
                {"coneCos", object({

                })},
                {"penumbraCos", object({

                })},
                {"decay", object({

                })}
            })}
        })},
        {"spotLightShadows", object({
            {"value", JSValue::array()},
            {"properties", object({
                {"shadowIntensity", 1},
                {"shadowBias", object({

                })},
                {"shadowNormalBias", object({

                })},
                {"shadowRadius", object({

                })},
                {"shadowMapSize", object({

                })}
            })}
        })},
        {"spotLightMap", object({
            {"value", JSValue::array()}
        })},
        {"spotLightMatrix", object({
            {"value", JSValue::array()}
        })},
        {"pointLights", object({
            {"value", JSValue::array()},
            {"properties", object({
                {"color", object({

                })},
                {"position", object({

                })},
                {"decay", object({

                })},
                {"distance", object({

                })}
            })}
        })},
        {"pointLightShadows", object({
            {"value", JSValue::array()},
            {"properties", object({
                {"shadowIntensity", 1},
                {"shadowBias", object({

                })},
                {"shadowNormalBias", object({

                })},
                {"shadowRadius", object({

                })},
                {"shadowMapSize", object({

                })},
                {"shadowCameraNear", object({

                })},
                {"shadowCameraFar", object({

                })}
            })}
        })},
        {"pointShadowMatrix", object({
            {"value", JSValue::array()}
        })},
        {"hemisphereLights", object({
            {"value", JSValue::array()},
            {"properties", object({
                {"direction", object({

                })},
                {"skyColor", object({

                })},
                {"groundColor", object({

                })}
            })}
        })},
        {"rectAreaLights", object({
            {"value", JSValue::array()},
            {"properties", object({
                {"color", object({

                })},
                {"position", object({

                })},
                {"width", object({

                })},
                {"height", object({

                })}
            })}
        })},
        {"ltc_1", object({
            {"value", nullptr}
        })},
        {"ltc_2", object({
            {"value", nullptr}
        })},
        {"probesSH", object({
            {"value", nullptr}
        })},
        {"probesMin", object({
            {"value", nativeVector(std::make_shared<Vector3>())}
        })},
        {"probesMax", object({
            {"value", nativeVector(std::make_shared<Vector3>())}
        })},
        {"probesResolution", object({
            {"value", nativeVector(std::make_shared<Vector3>())}
        })}
    })},
    {"points", object({
        {"diffuse", object({
            {"value", nativeColor(std::make_shared<Color>(16777215))}
        })},
        {"opacity", object({
            {"value", 1}
        })},
        {"size", object({
            {"value", 1}
        })},
        {"scale", object({
            {"value", 1}
        })},
        {"map", object({
            {"value", nullptr}
        })},
        {"alphaMap", object({
            {"value", nullptr}
        })},
        {"alphaMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })},
        {"alphaTest", object({
            {"value", 0}
        })},
        {"uvTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })}
    })},
    {"sprite", object({
        {"diffuse", object({
            {"value", nativeColor(std::make_shared<Color>(16777215))}
        })},
        {"opacity", object({
            {"value", 1}
        })},
        {"center", object({
            {"value", nativeVector(std::make_shared<Vector2>(0.5, 0.5))}
        })},
        {"rotation", object({
            {"value", 0}
        })},
        {"map", object({
            {"value", nullptr}
        })},
        {"mapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })},
        {"alphaMap", object({
            {"value", nullptr}
        })},
        {"alphaMapTransform", object({
            {"value", nativeMatrix(std::make_shared<Matrix3>())}
        })},
        {"alphaTest", object({
            {"value", 0}
        })}
    })}
});
