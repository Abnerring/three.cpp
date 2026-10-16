// Generated from pinned WebGLProgram.js prefix expressions. See THREEJS-LICENSE.
JSValue buildRawPrefix(const JSValue& module, const JSValue& parameters,
                       const JSValue& customDefines, const JSValue& customVertexExtensions,
                       const JSValue& shadowMapTypeDefine, const JSValue& envMapTypeDefine,
                       const JSValue& envMapModeDefine, const JSValue& envMapBlendingDefine,
                       const JSValue& prefixVertex = {}, const JSValue& prefixFragment = {}) {
    return ([&] {
        const auto receiver = ([&] {
            const auto receiver = ([&] {
                auto array = JSValue::array();
                literal(array, "0", ([&] {
                            const auto left = JSValue("#define SHADER_TYPE ");
                            const auto right = (parameters).get("shaderType");
                            return add(left, right);
                        }()));
                literal(array, "1", ([&] {
                            const auto left = JSValue("#define SHADER_NAME ");
                            const auto right = (parameters).get("shaderName");
                            return add(left, right);
                        }()));
                literal(array, "2", customDefines);
                return array;
            }());
            const auto method = receiver.get("filter");
            const Args args{module.get("filterEmptyLine")};
            return method.call(receiver, args);
        }());
        const auto method = receiver.get("join");
        const Args args{JSValue("\n")};
        return method.call(receiver, args);
    }());
}
JSValue buildVertexPrefix(const JSValue& module, const JSValue& parameters,
                          const JSValue& customDefines, const JSValue& customVertexExtensions,
                          const JSValue& shadowMapTypeDefine, const JSValue& envMapTypeDefine,
                          const JSValue& envMapModeDefine, const JSValue& envMapBlendingDefine,
                          const JSValue& prefixVertex = {}, const JSValue& prefixFragment = {}) {
    return ([&] {
        const auto receiver = ([&] {
            const auto receiver = ([&] {
                auto array = JSValue::array();
                literal(array, "0", ([&] {
                            const Args args{parameters};
                            return generatePrecision(module, args);
                        }()));
                literal(array, "1", ([&] {
                            const auto left = JSValue("#define SHADER_TYPE ");
                            const auto right = (parameters).get("shaderType");
                            return add(left, right);
                        }()));
                literal(array, "2", ([&] {
                            const auto left = JSValue("#define SHADER_NAME ");
                            const auto right = (parameters).get("shaderName");
                            return add(left, right);
                        }()));
                literal(array, "3", customDefines);
                literal(array, "4",
                        (((parameters).get("extensionClipCullDistance")).toBoolean()
                             ? JSValue("#define USE_CLIP_DISTANCE")
                             : JSValue("")));
                literal(array, "5",
                        (((parameters).get("batching")).toBoolean()
                             ? JSValue("#define USE_BATCHING")
                             : JSValue("")));
                literal(array, "6",
                        (((parameters).get("batchingColor")).toBoolean()
                             ? JSValue("#define USE_BATCHING_COLOR")
                             : JSValue("")));
                literal(array, "7",
                        (((parameters).get("instancing")).toBoolean()
                             ? JSValue("#define USE_INSTANCING")
                             : JSValue("")));
                literal(array, "8",
                        (((parameters).get("instancingColor")).toBoolean()
                             ? JSValue("#define USE_INSTANCING_COLOR")
                             : JSValue("")));
                literal(array, "9",
                        (((parameters).get("instancingMorph")).toBoolean()
                             ? JSValue("#define USE_INSTANCING_MORPH")
                             : JSValue("")));
                literal(array, "10",
                        ((([&] {
                             const auto left = (parameters).get("useFog");
                             return left.toBoolean() ? (parameters).get("fog") : left;
                         }()))
                                 .toBoolean()
                             ? JSValue("#define USE_FOG")
                             : JSValue("")));
                literal(array, "11",
                        ((([&] {
                             const auto left = (parameters).get("useFog");
                             return left.toBoolean() ? (parameters).get("fogExp2") : left;
                         }()))
                                 .toBoolean()
                             ? JSValue("#define FOG_EXP2")
                             : JSValue("")));
                literal(array, "12",
                        (((parameters).get("map")).toBoolean() ? JSValue("#define USE_MAP")
                                                               : JSValue("")));
                literal(array, "13",
                        (((parameters).get("envMap")).toBoolean() ? JSValue("#define USE_ENVMAP")
                                                                  : JSValue("")));
                literal(array, "14",
                        (((parameters).get("envMap")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define ");
                            const auto right = envMapModeDefine;
                            return add(left, right);
                        }())
                                                                  : JSValue("")));
                literal(array, "15",
                        (((parameters).get("lightMap")).toBoolean()
                             ? JSValue("#define USE_LIGHTMAP")
                             : JSValue("")));
                literal(array, "16",
                        (((parameters).get("aoMap")).toBoolean() ? JSValue("#define USE_AOMAP")
                                                                 : JSValue("")));
                literal(array, "17",
                        (((parameters).get("bumpMap")).toBoolean() ? JSValue("#define USE_BUMPMAP")
                                                                   : JSValue("")));
                literal(array, "18",
                        (((parameters).get("normalMap")).toBoolean()
                             ? JSValue("#define USE_NORMALMAP")
                             : JSValue("")));
                literal(array, "19",
                        (((parameters).get("normalMapObjectSpace")).toBoolean()
                             ? JSValue("#define USE_NORMALMAP_OBJECTSPACE")
                             : JSValue("")));
                literal(array, "20",
                        (((parameters).get("normalMapTangentSpace")).toBoolean()
                             ? JSValue("#define USE_NORMALMAP_TANGENTSPACE")
                             : JSValue("")));
                literal(array, "21",
                        (((parameters).get("displacementMap")).toBoolean()
                             ? JSValue("#define USE_DISPLACEMENTMAP")
                             : JSValue("")));
                literal(array, "22",
                        (((parameters).get("emissiveMap")).toBoolean()
                             ? JSValue("#define USE_EMISSIVEMAP")
                             : JSValue("")));
                literal(array, "23",
                        (((parameters).get("anisotropy")).toBoolean()
                             ? JSValue("#define USE_ANISOTROPY")
                             : JSValue("")));
                literal(array, "24",
                        (((parameters).get("anisotropyMap")).toBoolean()
                             ? JSValue("#define USE_ANISOTROPYMAP")
                             : JSValue("")));
                literal(array, "25",
                        (((parameters).get("clearcoatMap")).toBoolean()
                             ? JSValue("#define USE_CLEARCOATMAP")
                             : JSValue("")));
                literal(array, "26",
                        (((parameters).get("clearcoatRoughnessMap")).toBoolean()
                             ? JSValue("#define USE_CLEARCOAT_ROUGHNESSMAP")
                             : JSValue("")));
                literal(array, "27",
                        (((parameters).get("clearcoatNormalMap")).toBoolean()
                             ? JSValue("#define USE_CLEARCOAT_NORMALMAP")
                             : JSValue("")));
                literal(array, "28",
                        (((parameters).get("diffuseRoughnessMap")).toBoolean()
                             ? JSValue("#define USE_DIFFUSE_ROUGHNESSMAP")
                             : JSValue("")));
                literal(array, "29",
                        (((parameters).get("iridescenceMap")).toBoolean()
                             ? JSValue("#define USE_IRIDESCENCEMAP")
                             : JSValue("")));
                literal(array, "30",
                        (((parameters).get("iridescenceThicknessMap")).toBoolean()
                             ? JSValue("#define USE_IRIDESCENCE_THICKNESSMAP")
                             : JSValue("")));
                literal(array, "31",
                        (((parameters).get("specularMap")).toBoolean()
                             ? JSValue("#define USE_SPECULARMAP")
                             : JSValue("")));
                literal(array, "32",
                        (((parameters).get("specularColorMap")).toBoolean()
                             ? JSValue("#define USE_SPECULAR_COLORMAP")
                             : JSValue("")));
                literal(array, "33",
                        (((parameters).get("specularIntensityMap")).toBoolean()
                             ? JSValue("#define USE_SPECULAR_INTENSITYMAP")
                             : JSValue("")));
                literal(array, "34",
                        (((parameters).get("roughnessMap")).toBoolean()
                             ? JSValue("#define USE_ROUGHNESSMAP")
                             : JSValue("")));
                literal(array, "35",
                        (((parameters).get("metalnessMap")).toBoolean()
                             ? JSValue("#define USE_METALNESSMAP")
                             : JSValue("")));
                literal(array, "36",
                        (((parameters).get("alphaMap")).toBoolean()
                             ? JSValue("#define USE_ALPHAMAP")
                             : JSValue("")));
                literal(array, "37",
                        (((parameters).get("alphaHash")).toBoolean()
                             ? JSValue("#define USE_ALPHAHASH")
                             : JSValue("")));
                literal(array, "38",
                        (((parameters).get("transmission")).toBoolean()
                             ? JSValue("#define USE_TRANSMISSION")
                             : JSValue("")));
                literal(array, "39",
                        (((parameters).get("transmissionMap")).toBoolean()
                             ? JSValue("#define USE_TRANSMISSIONMAP")
                             : JSValue("")));
                literal(array, "40",
                        (((parameters).get("thicknessMap")).toBoolean()
                             ? JSValue("#define USE_THICKNESSMAP")
                             : JSValue("")));
                literal(array, "41",
                        (((parameters).get("sheenColorMap")).toBoolean()
                             ? JSValue("#define USE_SHEEN_COLORMAP")
                             : JSValue("")));
                literal(array, "42",
                        (((parameters).get("sheenRoughnessMap")).toBoolean()
                             ? JSValue("#define USE_SHEEN_ROUGHNESSMAP")
                             : JSValue("")));
                literal(array, "43",
                        (((parameters).get("mapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define MAP_UV ");
                            const auto right = (parameters).get("mapUv");
                            return add(left, right);
                        }())
                                                                 : JSValue("")));
                literal(array, "44",
                        (((parameters).get("alphaMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define ALPHAMAP_UV ");
                            const auto right = (parameters).get("alphaMapUv");
                            return add(left, right);
                        }())
                                                                      : JSValue("")));
                literal(array, "45",
                        (((parameters).get("lightMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define LIGHTMAP_UV ");
                            const auto right = (parameters).get("lightMapUv");
                            return add(left, right);
                        }())
                                                                      : JSValue("")));
                literal(array, "46",
                        (((parameters).get("aoMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define AOMAP_UV ");
                            const auto right = (parameters).get("aoMapUv");
                            return add(left, right);
                        }())
                                                                   : JSValue("")));
                literal(array, "47",
                        (((parameters).get("emissiveMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define EMISSIVEMAP_UV ");
                            const auto right = (parameters).get("emissiveMapUv");
                            return add(left, right);
                        }())
                                                                         : JSValue("")));
                literal(array, "48",
                        (((parameters).get("bumpMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define BUMPMAP_UV ");
                            const auto right = (parameters).get("bumpMapUv");
                            return add(left, right);
                        }())
                                                                     : JSValue("")));
                literal(array, "49",
                        (((parameters).get("normalMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define NORMALMAP_UV ");
                            const auto right = (parameters).get("normalMapUv");
                            return add(left, right);
                        }())
                                                                       : JSValue("")));
                literal(array, "50",
                        (((parameters).get("displacementMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define DISPLACEMENTMAP_UV ");
                            const auto right = (parameters).get("displacementMapUv");
                            return add(left, right);
                        }())
                                                                             : JSValue("")));
                literal(array, "51",
                        (((parameters).get("metalnessMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define METALNESSMAP_UV ");
                            const auto right = (parameters).get("metalnessMapUv");
                            return add(left, right);
                        }())
                                                                          : JSValue("")));
                literal(array, "52",
                        (((parameters).get("roughnessMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define ROUGHNESSMAP_UV ");
                            const auto right = (parameters).get("roughnessMapUv");
                            return add(left, right);
                        }())
                                                                          : JSValue("")));
                literal(array, "53",
                        (((parameters).get("anisotropyMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define ANISOTROPYMAP_UV ");
                            const auto right = (parameters).get("anisotropyMapUv");
                            return add(left, right);
                        }())
                                                                           : JSValue("")));
                literal(array, "54",
                        (((parameters).get("clearcoatMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define CLEARCOATMAP_UV ");
                            const auto right = (parameters).get("clearcoatMapUv");
                            return add(left, right);
                        }())
                                                                          : JSValue("")));
                literal(array, "55",
                        (((parameters).get("clearcoatNormalMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define CLEARCOAT_NORMALMAP_UV ");
                            const auto right = (parameters).get("clearcoatNormalMapUv");
                            return add(left, right);
                        }())
                                                                                : JSValue("")));
                literal(array, "56",
                        (((parameters).get("clearcoatRoughnessMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define CLEARCOAT_ROUGHNESSMAP_UV ");
                            const auto right = (parameters).get("clearcoatRoughnessMapUv");
                            return add(left, right);
                        }())
                                                                                   : JSValue("")));
                literal(array, "57",
                        (((parameters).get("diffuseRoughnessMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define DIFFUSE_ROUGHNESSMAP_UV ");
                            const auto right = (parameters).get("diffuseRoughnessMapUv");
                            return add(left, right);
                        }())
                                                                                 : JSValue("")));
                literal(array, "58",
                        (((parameters).get("iridescenceMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define IRIDESCENCEMAP_UV ");
                            const auto right = (parameters).get("iridescenceMapUv");
                            return add(left, right);
                        }())
                                                                            : JSValue("")));
                literal(
                    array, "59",
                    (((parameters).get("iridescenceThicknessMapUv")).toBoolean() ? ([&] {
                        const auto left = JSValue("#define IRIDESCENCE_THICKNESSMAP_UV ");
                        const auto right = (parameters).get("iridescenceThicknessMapUv");
                        return add(left, right);
                    }())
                                                                                 : JSValue("")));
                literal(array, "60",
                        (((parameters).get("sheenColorMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define SHEEN_COLORMAP_UV ");
                            const auto right = (parameters).get("sheenColorMapUv");
                            return add(left, right);
                        }())
                                                                           : JSValue("")));
                literal(array, "61",
                        (((parameters).get("sheenRoughnessMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define SHEEN_ROUGHNESSMAP_UV ");
                            const auto right = (parameters).get("sheenRoughnessMapUv");
                            return add(left, right);
                        }())
                                                                               : JSValue("")));
                literal(array, "62",
                        (((parameters).get("specularMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define SPECULARMAP_UV ");
                            const auto right = (parameters).get("specularMapUv");
                            return add(left, right);
                        }())
                                                                         : JSValue("")));
                literal(array, "63",
                        (((parameters).get("specularColorMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define SPECULAR_COLORMAP_UV ");
                            const auto right = (parameters).get("specularColorMapUv");
                            return add(left, right);
                        }())
                                                                              : JSValue("")));
                literal(array, "64",
                        (((parameters).get("specularIntensityMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define SPECULAR_INTENSITYMAP_UV ");
                            const auto right = (parameters).get("specularIntensityMapUv");
                            return add(left, right);
                        }())
                                                                                  : JSValue("")));
                literal(array, "65",
                        (((parameters).get("transmissionMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define TRANSMISSIONMAP_UV ");
                            const auto right = (parameters).get("transmissionMapUv");
                            return add(left, right);
                        }())
                                                                             : JSValue("")));
                literal(array, "66",
                        (((parameters).get("thicknessMapUv")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define THICKNESSMAP_UV ");
                            const auto right = (parameters).get("thicknessMapUv");
                            return add(left, right);
                        }())
                                                                          : JSValue("")));
                literal(array, "67",
                        ((([&] {
                             const auto left = (parameters).get("vertexTangents");
                             return left.toBoolean() ? ([&] {
                                 const auto left = (parameters).get("flatShading");
                                 const auto right = JSValue(false);
                                 return JSValue(left.strictEquals(right));
                             }())
                                                     : left;
                         }()))
                                 .toBoolean()
                             ? JSValue("#define USE_TANGENT")
                             : JSValue("")));
                literal(array, "68",
                        (((parameters).get("vertexNormals")).toBoolean()
                             ? JSValue("#define HAS_NORMAL")
                             : JSValue("")));
                literal(array, "69",
                        (((parameters).get("vertexColors")).toBoolean()
                             ? JSValue("#define USE_COLOR")
                             : JSValue("")));
                literal(array, "70",
                        (((parameters).get("vertexAlphas")).toBoolean()
                             ? JSValue("#define USE_COLOR_ALPHA")
                             : JSValue("")));
                literal(array, "71",
                        (((parameters).get("vertexUv1s")).toBoolean() ? JSValue("#define USE_UV1")
                                                                      : JSValue("")));
                literal(array, "72",
                        (((parameters).get("vertexUv2s")).toBoolean() ? JSValue("#define USE_UV2")
                                                                      : JSValue("")));
                literal(array, "73",
                        (((parameters).get("vertexUv3s")).toBoolean() ? JSValue("#define USE_UV3")
                                                                      : JSValue("")));
                literal(array, "74",
                        (((parameters).get("pointsUvs")).toBoolean()
                             ? JSValue("#define USE_POINTS_UV")
                             : JSValue("")));
                literal(array, "75",
                        (((parameters).get("flatShading")).toBoolean()
                             ? JSValue("#define FLAT_SHADED")
                             : JSValue("")));
                literal(array, "76",
                        (((parameters).get("skinning")).toBoolean()
                             ? JSValue("#define USE_SKINNING")
                             : JSValue("")));
                literal(array, "77",
                        (((parameters).get("morphTargets")).toBoolean()
                             ? JSValue("#define USE_MORPHTARGETS")
                             : JSValue("")));
                literal(array, "78",
                        ((([&] {
                             const auto left = (parameters).get("morphNormals");
                             return left.toBoolean() ? ([&] {
                                 const auto left = (parameters).get("flatShading");
                                 const auto right = JSValue(false);
                                 return JSValue(left.strictEquals(right));
                             }())
                                                     : left;
                         }()))
                                 .toBoolean()
                             ? JSValue("#define USE_MORPHNORMALS")
                             : JSValue("")));
                literal(array, "79",
                        (((parameters).get("morphColors")).toBoolean()
                             ? JSValue("#define USE_MORPHCOLORS")
                             : JSValue("")));
                literal(array, "80",
                        ((([&] {
                             const auto left = (parameters).get("morphTargetsCount");
                             const auto right = JSValue(0);
                             return JSValue(greater(left, right));
                         }()))
                                 .toBoolean()
                             ? ([&] {
                                   const auto left =
                                       JSValue("#define MORPHTARGETS_TEXTURE_STRIDE ");
                                   const auto right = (parameters).get("morphTextureStride");
                                   return add(left, right);
                               }())
                             : JSValue("")));
                literal(array, "81",
                        ((([&] {
                             const auto left = (parameters).get("morphTargetsCount");
                             const auto right = JSValue(0);
                             return JSValue(greater(left, right));
                         }()))
                                 .toBoolean()
                             ? ([&] {
                                   const auto left = JSValue("#define MORPHTARGETS_COUNT ");
                                   const auto right = (parameters).get("morphTargetsCount");
                                   return add(left, right);
                               }())
                             : JSValue("")));
                literal(array, "82",
                        (((parameters).get("doubleSided")).toBoolean()
                             ? JSValue("#define DOUBLE_SIDED")
                             : JSValue("")));
                literal(array, "83",
                        (((parameters).get("flipSided")).toBoolean() ? JSValue("#define FLIP_SIDED")
                                                                     : JSValue("")));
                literal(array, "84",
                        (((parameters).get("shadowMapEnabled")).toBoolean()
                             ? JSValue("#define USE_SHADOWMAP")
                             : JSValue("")));
                literal(array, "85",
                        (((parameters).get("shadowMapEnabled")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define ");
                            const auto right = shadowMapTypeDefine;
                            return add(left, right);
                        }())
                                                                            : JSValue("")));
                literal(array, "86",
                        (((parameters).get("sizeAttenuation")).toBoolean()
                             ? JSValue("#define USE_SIZEATTENUATION")
                             : JSValue("")));
                literal(array, "87",
                        ((([&] {
                             const auto left = (parameters).get("numLightProbes");
                             const auto right = JSValue(0);
                             return JSValue(greater(left, right));
                         }()))
                                 .toBoolean()
                             ? JSValue("#define USE_LIGHT_PROBES")
                             : JSValue("")));
                literal(array, "88",
                        (((parameters).get("logarithmicDepthBuffer")).toBoolean()
                             ? JSValue("#define USE_LOGARITHMIC_DEPTH_BUFFER")
                             : JSValue("")));
                literal(array, "89",
                        (((parameters).get("reversedDepthBuffer")).toBoolean()
                             ? JSValue("#define USE_REVERSED_DEPTH_BUFFER")
                             : JSValue("")));
                literal(array, "90", JSValue("uniform mat4 modelMatrix;"));
                literal(array, "91", JSValue("uniform mat4 modelViewMatrix;"));
                literal(array, "92", JSValue("uniform mat4 projectionMatrix;"));
                literal(array, "93", JSValue("uniform mat4 viewMatrix;"));
                literal(array, "94", JSValue("uniform mat3 normalMatrix;"));
                literal(array, "95", JSValue("uniform vec3 cameraPosition;"));
                literal(array, "96", JSValue("uniform bool isOrthographic;"));
                literal(array, "97", JSValue("#ifdef USE_INSTANCING"));
                literal(array, "98", JSValue("\tattribute mat4 instanceMatrix;"));
                literal(array, "99", JSValue("#endif"));
                literal(array, "100", JSValue("#ifdef USE_INSTANCING_COLOR"));
                literal(array, "101", JSValue("\tattribute vec3 instanceColor;"));
                literal(array, "102", JSValue("#endif"));
                literal(array, "103", JSValue("#ifdef USE_INSTANCING_MORPH"));
                literal(array, "104", JSValue("\tuniform sampler2D morphTexture;"));
                literal(array, "105", JSValue("#endif"));
                literal(array, "106", JSValue("attribute vec3 position;"));
                literal(array, "107", JSValue("attribute vec3 normal;"));
                literal(array, "108", JSValue("attribute vec2 uv;"));
                literal(array, "109", JSValue("#ifdef USE_UV1"));
                literal(array, "110", JSValue("\tattribute vec2 uv1;"));
                literal(array, "111", JSValue("#endif"));
                literal(array, "112", JSValue("#ifdef USE_UV2"));
                literal(array, "113", JSValue("\tattribute vec2 uv2;"));
                literal(array, "114", JSValue("#endif"));
                literal(array, "115", JSValue("#ifdef USE_UV3"));
                literal(array, "116", JSValue("\tattribute vec2 uv3;"));
                literal(array, "117", JSValue("#endif"));
                literal(array, "118", JSValue("#ifdef USE_TANGENT"));
                literal(array, "119", JSValue("\tattribute vec4 tangent;"));
                literal(array, "120", JSValue("#endif"));
                literal(array, "121", JSValue("#if defined( USE_COLOR_ALPHA )"));
                literal(array, "122", JSValue("\tattribute vec4 color;"));
                literal(array, "123", JSValue("#elif defined( USE_COLOR )"));
                literal(array, "124", JSValue("\tattribute vec3 color;"));
                literal(array, "125", JSValue("#endif"));
                literal(array, "126", JSValue("#ifdef USE_SKINNING"));
                literal(array, "127", JSValue("\tattribute vec4 skinIndex;"));
                literal(array, "128", JSValue("\tattribute vec4 skinWeight;"));
                literal(array, "129", JSValue("#endif"));
                literal(array, "130", JSValue("\n"));
                return array;
            }());
            const auto method = receiver.get("filter");
            const Args args{module.get("filterEmptyLine")};
            return method.call(receiver, args);
        }());
        const auto method = receiver.get("join");
        const Args args{JSValue("\n")};
        return method.call(receiver, args);
    }());
}
JSValue buildFragmentPrefix(const JSValue& module, const JSValue& parameters,
                            const JSValue& customDefines, const JSValue& customVertexExtensions,
                            const JSValue& shadowMapTypeDefine, const JSValue& envMapTypeDefine,
                            const JSValue& envMapModeDefine, const JSValue& envMapBlendingDefine,
                            const JSValue& prefixVertex = {}, const JSValue& prefixFragment = {}) {
    return ([&] {
        const auto receiver = ([&] {
            const auto receiver = ([&] {
                auto array = JSValue::array();
                literal(array, "0", ([&] {
                            const Args args{parameters};
                            return generatePrecision(module, args);
                        }()));
                literal(array, "1", ([&] {
                            const auto left = JSValue("#define SHADER_TYPE ");
                            const auto right = (parameters).get("shaderType");
                            return add(left, right);
                        }()));
                literal(array, "2", ([&] {
                            const auto left = JSValue("#define SHADER_NAME ");
                            const auto right = (parameters).get("shaderName");
                            return add(left, right);
                        }()));
                literal(array, "3", customDefines);
                literal(array, "4",
                        ((([&] {
                             const auto left = (parameters).get("useFog");
                             return left.toBoolean() ? (parameters).get("fog") : left;
                         }()))
                                 .toBoolean()
                             ? JSValue("#define USE_FOG")
                             : JSValue("")));
                literal(array, "5",
                        ((([&] {
                             const auto left = (parameters).get("useFog");
                             return left.toBoolean() ? (parameters).get("fogExp2") : left;
                         }()))
                                 .toBoolean()
                             ? JSValue("#define FOG_EXP2")
                             : JSValue("")));
                literal(array, "6",
                        (((parameters).get("alphaToCoverage")).toBoolean()
                             ? JSValue("#define ALPHA_TO_COVERAGE")
                             : JSValue("")));
                literal(array, "7",
                        (((parameters).get("map")).toBoolean() ? JSValue("#define USE_MAP")
                                                               : JSValue("")));
                literal(array, "8",
                        (((parameters).get("matcap")).toBoolean() ? JSValue("#define USE_MATCAP")
                                                                  : JSValue("")));
                literal(array, "9",
                        (((parameters).get("envMap")).toBoolean() ? JSValue("#define USE_ENVMAP")
                                                                  : JSValue("")));
                literal(array, "10",
                        (((parameters).get("envMap")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define ");
                            const auto right = envMapTypeDefine;
                            return add(left, right);
                        }())
                                                                  : JSValue("")));
                literal(array, "11",
                        (((parameters).get("envMap")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define ");
                            const auto right = envMapModeDefine;
                            return add(left, right);
                        }())
                                                                  : JSValue("")));
                literal(array, "12",
                        (((parameters).get("envMap")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define ");
                            const auto right = envMapBlendingDefine;
                            return add(left, right);
                        }())
                                                                  : JSValue("")));
                literal(array, "13",
                        (((parameters).get("envMapPMREM")).toBoolean() ? ([&] {
                            const auto left = ([&] {
                                const auto left = JSValue("#define ENVMAP_MAX_LOD ");
                                const auto right = (parameters).get("envMapMaxLod");
                                return add(left, right);
                            }());
                            const auto right = JSValue(".0");
                            return add(left, right);
                        }())
                                                                       : JSValue("")));
                literal(array, "14",
                        (((parameters).get("envMapPMREM")).toBoolean() ? ([&] {
                            const auto left = ([&] {
                                const auto left = JSValue("#define ENVMAP_SIZE ");
                                const auto right = (parameters).get("envMapSize");
                                return add(left, right);
                            }());
                            const auto right = JSValue(".0");
                            return add(left, right);
                        }())
                                                                       : JSValue("")));
                literal(array, "15",
                        (((parameters).get("lightMap")).toBoolean()
                             ? JSValue("#define USE_LIGHTMAP")
                             : JSValue("")));
                literal(array, "16",
                        (((parameters).get("aoMap")).toBoolean() ? JSValue("#define USE_AOMAP")
                                                                 : JSValue("")));
                literal(array, "17",
                        (((parameters).get("bumpMap")).toBoolean() ? JSValue("#define USE_BUMPMAP")
                                                                   : JSValue("")));
                literal(array, "18",
                        (((parameters).get("normalMap")).toBoolean()
                             ? JSValue("#define USE_NORMALMAP")
                             : JSValue("")));
                literal(array, "19",
                        (((parameters).get("normalMapObjectSpace")).toBoolean()
                             ? JSValue("#define USE_NORMALMAP_OBJECTSPACE")
                             : JSValue("")));
                literal(array, "20",
                        (((parameters).get("normalMapTangentSpace")).toBoolean()
                             ? JSValue("#define USE_NORMALMAP_TANGENTSPACE")
                             : JSValue("")));
                literal(array, "21",
                        (((parameters).get("packedNormalMap")).toBoolean()
                             ? JSValue("#define USE_PACKED_NORMALMAP")
                             : JSValue("")));
                literal(array, "22",
                        (((parameters).get("emissiveMap")).toBoolean()
                             ? JSValue("#define USE_EMISSIVEMAP")
                             : JSValue("")));
                literal(array, "23",
                        (((parameters).get("anisotropy")).toBoolean()
                             ? JSValue("#define USE_ANISOTROPY")
                             : JSValue("")));
                literal(array, "24",
                        (((parameters).get("anisotropyMap")).toBoolean()
                             ? JSValue("#define USE_ANISOTROPYMAP")
                             : JSValue("")));
                literal(array, "25",
                        (((parameters).get("clearcoat")).toBoolean()
                             ? JSValue("#define USE_CLEARCOAT")
                             : JSValue("")));
                literal(array, "26",
                        (((parameters).get("clearcoatMap")).toBoolean()
                             ? JSValue("#define USE_CLEARCOATMAP")
                             : JSValue("")));
                literal(array, "27",
                        (((parameters).get("clearcoatRoughnessMap")).toBoolean()
                             ? JSValue("#define USE_CLEARCOAT_ROUGHNESSMAP")
                             : JSValue("")));
                literal(array, "28",
                        (((parameters).get("clearcoatNormalMap")).toBoolean()
                             ? JSValue("#define USE_CLEARCOAT_NORMALMAP")
                             : JSValue("")));
                literal(array, "29",
                        (((parameters).get("diffuseRoughness")).toBoolean()
                             ? JSValue("#define USE_DIFFUSE_ROUGHNESS")
                             : JSValue("")));
                literal(array, "30",
                        (((parameters).get("diffuseRoughnessMap")).toBoolean()
                             ? JSValue("#define USE_DIFFUSE_ROUGHNESSMAP")
                             : JSValue("")));
                literal(array, "31",
                        (((parameters).get("dispersion")).toBoolean()
                             ? JSValue("#define USE_DISPERSION")
                             : JSValue("")));
                literal(array, "32",
                        (((parameters).get("retroreflection")).toBoolean()
                             ? JSValue("#define USE_RETROREFLECTION")
                             : JSValue("")));
                literal(array, "33",
                        (((parameters).get("iridescence")).toBoolean()
                             ? JSValue("#define USE_IRIDESCENCE")
                             : JSValue("")));
                literal(array, "34",
                        (((parameters).get("iridescenceMap")).toBoolean()
                             ? JSValue("#define USE_IRIDESCENCEMAP")
                             : JSValue("")));
                literal(array, "35",
                        (((parameters).get("iridescenceThicknessMap")).toBoolean()
                             ? JSValue("#define USE_IRIDESCENCE_THICKNESSMAP")
                             : JSValue("")));
                literal(array, "36",
                        (((parameters).get("specularMap")).toBoolean()
                             ? JSValue("#define USE_SPECULARMAP")
                             : JSValue("")));
                literal(array, "37",
                        (((parameters).get("specularColorMap")).toBoolean()
                             ? JSValue("#define USE_SPECULAR_COLORMAP")
                             : JSValue("")));
                literal(array, "38",
                        (((parameters).get("specularIntensityMap")).toBoolean()
                             ? JSValue("#define USE_SPECULAR_INTENSITYMAP")
                             : JSValue("")));
                literal(array, "39",
                        (((parameters).get("roughnessMap")).toBoolean()
                             ? JSValue("#define USE_ROUGHNESSMAP")
                             : JSValue("")));
                literal(array, "40",
                        (((parameters).get("metalnessMap")).toBoolean()
                             ? JSValue("#define USE_METALNESSMAP")
                             : JSValue("")));
                literal(array, "41",
                        (((parameters).get("alphaMap")).toBoolean()
                             ? JSValue("#define USE_ALPHAMAP")
                             : JSValue("")));
                literal(array, "42",
                        (((parameters).get("alphaTest")).toBoolean()
                             ? JSValue("#define USE_ALPHATEST")
                             : JSValue("")));
                literal(array, "43",
                        (((parameters).get("alphaHash")).toBoolean()
                             ? JSValue("#define USE_ALPHAHASH")
                             : JSValue("")));
                literal(array, "44",
                        (((parameters).get("sheen")).toBoolean() ? JSValue("#define USE_SHEEN")
                                                                 : JSValue("")));
                literal(array, "45",
                        (((parameters).get("sheenColorMap")).toBoolean()
                             ? JSValue("#define USE_SHEEN_COLORMAP")
                             : JSValue("")));
                literal(array, "46",
                        (((parameters).get("sheenRoughnessMap")).toBoolean()
                             ? JSValue("#define USE_SHEEN_ROUGHNESSMAP")
                             : JSValue("")));
                literal(array, "47",
                        (((parameters).get("transmission")).toBoolean()
                             ? JSValue("#define USE_TRANSMISSION")
                             : JSValue("")));
                literal(array, "48",
                        (((parameters).get("transmissionMap")).toBoolean()
                             ? JSValue("#define USE_TRANSMISSIONMAP")
                             : JSValue("")));
                literal(array, "49",
                        (((parameters).get("thicknessMap")).toBoolean()
                             ? JSValue("#define USE_THICKNESSMAP")
                             : JSValue("")));
                literal(array, "50",
                        ((([&] {
                             const auto left = (parameters).get("vertexTangents");
                             return left.toBoolean() ? ([&] {
                                 const auto left = (parameters).get("flatShading");
                                 const auto right = JSValue(false);
                                 return JSValue(left.strictEquals(right));
                             }())
                                                     : left;
                         }()))
                                 .toBoolean()
                             ? JSValue("#define USE_TANGENT")
                             : JSValue("")));
                literal(array, "51",
                        ((([&] {
                             const auto left = (parameters).get("vertexColors");
                             return left.toBoolean() ? left : (parameters).get("instancingColor");
                         }()))
                                 .toBoolean()
                             ? JSValue("#define USE_COLOR")
                             : JSValue("")));
                literal(array, "52",
                        ((([&] {
                             const auto left = (parameters).get("vertexAlphas");
                             return left.toBoolean() ? left : (parameters).get("batchingColor");
                         }()))
                                 .toBoolean()
                             ? JSValue("#define USE_COLOR_ALPHA")
                             : JSValue("")));
                literal(array, "53",
                        (((parameters).get("vertexUv1s")).toBoolean() ? JSValue("#define USE_UV1")
                                                                      : JSValue("")));
                literal(array, "54",
                        (((parameters).get("vertexUv2s")).toBoolean() ? JSValue("#define USE_UV2")
                                                                      : JSValue("")));
                literal(array, "55",
                        (((parameters).get("vertexUv3s")).toBoolean() ? JSValue("#define USE_UV3")
                                                                      : JSValue("")));
                literal(array, "56",
                        (((parameters).get("pointsUvs")).toBoolean()
                             ? JSValue("#define USE_POINTS_UV")
                             : JSValue("")));
                literal(array, "57",
                        (((parameters).get("gradientMap")).toBoolean()
                             ? JSValue("#define USE_GRADIENTMAP")
                             : JSValue("")));
                literal(array, "58",
                        (((parameters).get("flatShading")).toBoolean()
                             ? JSValue("#define FLAT_SHADED")
                             : JSValue("")));
                literal(array, "59",
                        (((parameters).get("doubleSided")).toBoolean()
                             ? JSValue("#define DOUBLE_SIDED")
                             : JSValue("")));
                literal(array, "60",
                        (((parameters).get("flipSided")).toBoolean() ? JSValue("#define FLIP_SIDED")
                                                                     : JSValue("")));
                literal(array, "61",
                        (((parameters).get("shadowMapEnabled")).toBoolean()
                             ? JSValue("#define USE_SHADOWMAP")
                             : JSValue("")));
                literal(array, "62",
                        (((parameters).get("shadowMapEnabled")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define ");
                            const auto right = shadowMapTypeDefine;
                            return add(left, right);
                        }())
                                                                            : JSValue("")));
                literal(array, "63",
                        (((parameters).get("premultipliedAlpha")).toBoolean()
                             ? JSValue("#define PREMULTIPLIED_ALPHA")
                             : JSValue("")));
                literal(array, "64",
                        ((([&] {
                             const auto left = (parameters).get("numLightProbes");
                             const auto right = JSValue(0);
                             return JSValue(greater(left, right));
                         }()))
                                 .toBoolean()
                             ? JSValue("#define USE_LIGHT_PROBES")
                             : JSValue("")));
                literal(array, "65",
                        ((([&] {
                             const auto left = (parameters).get("numLightProbeGrids");
                             const auto right = JSValue(0);
                             return JSValue(greater(left, right));
                         }()))
                                 .toBoolean()
                             ? JSValue("#define USE_LIGHT_PROBES_GRID")
                             : JSValue("")));
                literal(array, "66",
                        (((parameters).get("decodeVideoTexture")).toBoolean()
                             ? JSValue("#define DECODE_VIDEO_TEXTURE")
                             : JSValue("")));
                literal(array, "67",
                        (((parameters).get("decodeVideoTextureEmissive")).toBoolean()
                             ? JSValue("#define DECODE_VIDEO_TEXTURE_EMISSIVE")
                             : JSValue("")));
                literal(array, "68",
                        (((parameters).get("logarithmicDepthBuffer")).toBoolean()
                             ? JSValue("#define USE_LOGARITHMIC_DEPTH_BUFFER")
                             : JSValue("")));
                literal(array, "69",
                        (((parameters).get("reversedDepthBuffer")).toBoolean()
                             ? JSValue("#define USE_REVERSED_DEPTH_BUFFER")
                             : JSValue("")));
                literal(array, "70", JSValue("uniform mat4 viewMatrix;"));
                literal(array, "71", JSValue("uniform vec3 cameraPosition;"));
                literal(array, "72", JSValue("uniform bool isOrthographic;"));
                literal(array, "73",
                        ((([&] {
                             const auto left = (parameters).get("toneMapping");
                             const auto right = NoToneMapping;
                             return JSValue(!left.strictEquals(right));
                         }()))
                                 .toBoolean()
                             ? JSValue("#define TONE_MAPPING")
                             : JSValue("")));
                literal(array, "74",
                        ((([&] {
                             const auto left = (parameters).get("toneMapping");
                             const auto right = NoToneMapping;
                             return JSValue(!left.strictEquals(right));
                         }()))
                                 .toBoolean()
                             ? ([&] {
                                   const auto base = module.get("ShaderChunk");
                                   const auto key = JSValue("tonemapping_pars_fragment");
                                   return base.get(key);
                               }())
                             : JSValue("")));
                literal(array, "75",
                        ((([&] {
                             const auto left = (parameters).get("toneMapping");
                             const auto right = NoToneMapping;
                             return JSValue(!left.strictEquals(right));
                         }()))
                                 .toBoolean()
                             ? ([&] {
                                   const Args args{JSValue("toneMapping"),
                                                   (parameters).get("toneMapping")};
                                   return getToneMappingFunction(module, args);
                               }())
                             : JSValue("")));
                literal(array, "76",
                        (((parameters).get("dithering")).toBoolean() ? JSValue("#define DITHERING")
                                                                     : JSValue("")));
                literal(array, "77",
                        (((parameters).get("opaque")).toBoolean() ? JSValue("#define OPAQUE")
                                                                  : JSValue("")));
                literal(array, "78", ([&] {
                            const auto base = module.get("ShaderChunk");
                            const auto key = JSValue("colorspace_pars_fragment");
                            return base.get(key);
                        }()));
                literal(array, "79", ([&] {
                            const Args args{JSValue("linearToOutputTexel"),
                                            (parameters).get("outputColorSpace")};
                            return getTexelEncodingFunction(module, args);
                        }()));
                literal(array, "80", ([&] {
                            const Args args{};
                            return getLuminanceFunction(module, args);
                        }()));
                literal(array, "81",
                        (((parameters).get("useDepthPacking")).toBoolean() ? ([&] {
                            const auto left = JSValue("#define DEPTH_PACKING ");
                            const auto right = (parameters).get("depthPacking");
                            return add(left, right);
                        }())
                                                                           : JSValue("")));
                literal(array, "82", JSValue("\n"));
                return array;
            }());
            const auto method = receiver.get("filter");
            const Args args{module.get("filterEmptyLine")};
            return method.call(receiver, args);
        }());
        const auto method = receiver.get("join");
        const Args args{JSValue("\n")};
        return method.call(receiver, args);
    }());
}
JSValue
buildVersionVertexPrefix(const JSValue& module, const JSValue& parameters,
                         const JSValue& customDefines, const JSValue& customVertexExtensions,
                         const JSValue& shadowMapTypeDefine, const JSValue& envMapTypeDefine,
                         const JSValue& envMapModeDefine, const JSValue& envMapBlendingDefine,
                         const JSValue& prefixVertex = {}, const JSValue& prefixFragment = {}) {
    return ([&] {
        const auto left = ([&] {
            const auto left = ([&] {
                const auto receiver = ([&] {
                    auto array = JSValue::array();
                    literal(array, "0", customVertexExtensions);
                    literal(array, "1", JSValue("#define attribute in"));
                    literal(array, "2", JSValue("#define varying out"));
                    literal(array, "3", JSValue("#define texture2D texture"));
                    return array;
                }());
                const auto method = receiver.get("join");
                const Args args{JSValue("\n")};
                return method.call(receiver, args);
            }());
            const auto right = JSValue("\n");
            return add(left, right);
        }());
        const auto right = prefixVertex;
        return add(left, right);
    }());
}
JSValue
buildVersionFragmentPrefix(const JSValue& module, const JSValue& parameters,
                           const JSValue& customDefines, const JSValue& customVertexExtensions,
                           const JSValue& shadowMapTypeDefine, const JSValue& envMapTypeDefine,
                           const JSValue& envMapModeDefine, const JSValue& envMapBlendingDefine,
                           const JSValue& prefixVertex = {}, const JSValue& prefixFragment = {}) {
    return ([&] {
        const auto left = ([&] {
            const auto left = ([&] {
                const auto receiver = ([&] {
                    auto array = JSValue::array();
                    literal(array, "0", JSValue("#define varying in"));
                    literal(array, "1",
                            ((([&] {
                                 const auto left = (parameters).get("glslVersion");
                                 const auto right = GLSL3;
                                 return JSValue(left.strictEquals(right));
                             }()))
                                     .toBoolean()
                                 ? JSValue("")
                                 : JSValue("layout(location = 0) out highp vec4 pc_fragColor;")));
                    literal(array, "2",
                            ((([&] {
                                 const auto left = (parameters).get("glslVersion");
                                 const auto right = GLSL3;
                                 return JSValue(left.strictEquals(right));
                             }()))
                                     .toBoolean()
                                 ? JSValue("")
                                 : JSValue("#define gl_FragColor pc_fragColor")));
                    literal(array, "3", JSValue("#define gl_FragDepthEXT gl_FragDepth"));
                    literal(array, "4", JSValue("#define texture2D texture"));
                    literal(array, "5", JSValue("#define textureCube texture"));
                    literal(array, "6", JSValue("#define texture2DProj textureProj"));
                    literal(array, "7", JSValue("#define texture2DLodEXT textureLod"));
                    literal(array, "8", JSValue("#define texture2DProjLodEXT textureProjLod"));
                    literal(array, "9", JSValue("#define textureCubeLodEXT textureLod"));
                    literal(array, "10", JSValue("#define texture2DGradEXT textureGrad"));
                    literal(array, "11", JSValue("#define texture2DProjGradEXT textureProjGrad"));
                    literal(array, "12", JSValue("#define textureCubeGradEXT textureGrad"));
                    return array;
                }());
                const auto method = receiver.get("join");
                const Args args{JSValue("\n")};
                return method.call(receiver, args);
            }());
            const auto right = JSValue("\n");
            return add(left, right);
        }());
        const auto right = prefixFragment;
        return add(left, right);
    }());
}
