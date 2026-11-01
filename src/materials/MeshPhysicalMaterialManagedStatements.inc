// Generated static native statements; fixed source SHA256 baeaaca0987887e0f017067e512b275323d24df7b2f5d7bc1ba80ab916221046

JSValue constructor(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto parameters=arg(args,0);JSValue{};
([&]() -> JSValue { auto v1=self;auto v2=JSValue("isMeshPhysicalMaterial");auto v3=JSValue(true);v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self;auto v5=JSValue("defines");auto v6=([&]() -> JSValue { auto v7=JSValue::object();own(v7,"STANDARD",JSValue(""));own(v7,"PHYSICAL",JSValue("")); return v7; }());v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v8=self;auto v9=JSValue("type");auto v10=JSValue("MeshPhysicalMaterial");v8.set(v9,v10); return v10; }());
([&]() -> JSValue { auto v11=self;auto v12=JSValue("anisotropyRotation");auto v13=JSValue(0);v11.set(v12,v13); return v13; }());
([&]() -> JSValue { auto v14=self;auto v15=JSValue("anisotropyMap");auto v16=JSValue(nullptr);v14.set(v15,v16); return v16; }());
([&]() -> JSValue { auto v17=self;auto v18=JSValue("clearcoatMap");auto v19=JSValue(nullptr);v17.set(v18,v19); return v19; }());
([&]() -> JSValue { auto v20=self;auto v21=JSValue("clearcoatRoughness");auto v22=JSValue(0);v20.set(v21,v22); return v22; }());
([&]() -> JSValue { auto v23=self;auto v24=JSValue("clearcoatRoughnessMap");auto v25=JSValue(nullptr);v23.set(v24,v25); return v25; }());
([&]() -> JSValue { auto v26=self;auto v27=JSValue("clearcoatNormalScale");auto v28=([&]() -> JSValue { auto v29=imports.get("Vector2");auto v30=JSValue(1);auto v31=JSValue(1); return v29.construct({v30,v31}); }());v26.set(v27,v28); return v28; }());
([&]() -> JSValue { auto v32=self;auto v33=JSValue("clearcoatNormalMap");auto v34=JSValue(nullptr);v32.set(v33,v34); return v34; }());
([&]() -> JSValue { auto v35=self;auto v36=JSValue("diffuseRoughnessMap");auto v37=JSValue(nullptr);v35.set(v36,v37); return v37; }());
([&]() -> JSValue { auto v38=self;auto v39=JSValue("ior");auto v40=JSValue(1.5);v38.set(v39,v40); return v40; }());
([&]() -> JSValue { auto v42=nativeGlobalBinding("Object");auto v41=v42.get("defineProperty");auto v43=self;auto v44=JSValue("reflectivity");auto v45=([&]() -> JSValue { auto v46=JSValue::object();own(v46,"get",([&]() -> JSValue { auto v47=JSValue::constructor(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue {const auto imports=captures[0];return ([&]() -> JSValue { auto v48=imports.get("clamp");auto v50=([&]() -> JSValue { auto v51=([&]() -> JSValue { auto v53=JSValue(2.5);auto v54=([&]() -> JSValue { auto v55=([&]() -> JSValue { auto v57=self;auto v58=JSValue("ior"); return v57.get(v58); }());auto v56=JSValue(1); return numeric(v55,v56,'-'); }()); return numeric(v53,v54,'*'); }());auto v52=([&]() -> JSValue { auto v59=([&]() -> JSValue { auto v61=self;auto v62=JSValue("ior"); return v61.get(v62); }());auto v60=JSValue(1); return add(v59,v60); }()); return numeric(v51,v52,'/'); }());auto v63=JSValue(0);auto v64=JSValue(1); return v48.call(JSValue{},{v50,v63,v64}); }());return {};},{imports},"get",0,JSValue::ConstructorKind::Callable);v47.setFunctionSourceText("function () {\r\n\r\n\t\t\t\treturn ( clamp( 2.5 * ( this.ior - 1 ) / ( this.ior + 1 ), 0, 1 ) );\r\n\r\n\t\t\t}"); return v47; }()));own(v46,"set",([&]() -> JSValue { auto v65=JSValue::constructor(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue {const auto imports=captures[0];auto reflectivity=arg(args,0);([&]() -> JSValue { auto v66=self;auto v67=JSValue("ior");auto v68=([&]() -> JSValue { auto v69=([&]() -> JSValue { auto v71=JSValue(1);auto v72=([&]() -> JSValue { auto v73=JSValue(0.4);auto v74=reflectivity; return numeric(v73,v74,'*'); }()); return add(v71,v72); }());auto v70=([&]() -> JSValue { auto v75=JSValue(1);auto v76=([&]() -> JSValue { auto v77=JSValue(0.4);auto v78=reflectivity; return numeric(v77,v78,'*'); }()); return numeric(v75,v76,'-'); }()); return numeric(v69,v70,'/'); }());v66.set(v67,v68); return v68; }());return {};},{imports},"set",1,JSValue::ConstructorKind::Callable);v65.setFunctionSourceText("function ( reflectivity ) {\r\n\r\n\t\t\t\tthis.ior = ( 1 + 0.4 * reflectivity ) / ( 1 - 0.4 * reflectivity );\r\n\r\n\t\t\t}"); return v65; }())); return v46; }()); return v41.call(v42,{v43,v44,v45}); }());
([&]() -> JSValue { auto v79=self;auto v80=JSValue("iridescenceMap");auto v81=JSValue(nullptr);v79.set(v80,v81); return v81; }());
([&]() -> JSValue { auto v82=self;auto v83=JSValue("iridescenceIOR");auto v84=JSValue(1.3);v82.set(v83,v84); return v84; }());
([&]() -> JSValue { auto v85=self;auto v86=JSValue("iridescenceThicknessRange");auto v87=([&]() -> JSValue { auto v88=JSValue::array();std::size_t v89=0;own(v88,std::to_string(v89++),JSValue(100));own(v88,std::to_string(v89++),JSValue(400));v88.set("length",v89); return v88; }());v85.set(v86,v87); return v87; }());
([&]() -> JSValue { auto v90=self;auto v91=JSValue("iridescenceThicknessMap");auto v92=JSValue(nullptr);v90.set(v91,v92); return v92; }());
([&]() -> JSValue { auto v93=self;auto v94=JSValue("sheenColor");auto v95=([&]() -> JSValue { auto v96=imports.get("Color");auto v97=JSValue(0); return v96.construct({v97}); }());v93.set(v94,v95); return v95; }());
([&]() -> JSValue { auto v98=self;auto v99=JSValue("sheenColorMap");auto v100=JSValue(nullptr);v98.set(v99,v100); return v100; }());
([&]() -> JSValue { auto v101=self;auto v102=JSValue("sheenRoughness");auto v103=JSValue(1);v101.set(v102,v103); return v103; }());
([&]() -> JSValue { auto v104=self;auto v105=JSValue("sheenRoughnessMap");auto v106=JSValue(nullptr);v104.set(v105,v106); return v106; }());
([&]() -> JSValue { auto v107=self;auto v108=JSValue("transmissionMap");auto v109=JSValue(nullptr);v107.set(v108,v109); return v109; }());
([&]() -> JSValue { auto v110=self;auto v111=JSValue("thickness");auto v112=JSValue(0);v110.set(v111,v112); return v112; }());
([&]() -> JSValue { auto v113=self;auto v114=JSValue("thicknessMap");auto v115=JSValue(nullptr);v113.set(v114,v115); return v115; }());
([&]() -> JSValue { auto v116=self;auto v117=JSValue("attenuationDistance");auto v118=nativeGlobalBinding("Infinity");v116.set(v117,v118); return v118; }());
([&]() -> JSValue { auto v119=self;auto v120=JSValue("attenuationColor");auto v121=([&]() -> JSValue { auto v122=imports.get("Color");auto v123=JSValue(1);auto v124=JSValue(1);auto v125=JSValue(1); return v122.construct({v123,v124,v125}); }());v119.set(v120,v121); return v121; }());
([&]() -> JSValue { auto v126=self;auto v127=JSValue("specularIntensity");auto v128=JSValue(1);v126.set(v127,v128); return v128; }());
([&]() -> JSValue { auto v129=self;auto v130=JSValue("specularIntensityMap");auto v131=JSValue(nullptr);v129.set(v130,v131); return v131; }());
([&]() -> JSValue { auto v132=self;auto v133=JSValue("specularColor");auto v134=([&]() -> JSValue { auto v135=imports.get("Color");auto v136=JSValue(1);auto v137=JSValue(1);auto v138=JSValue(1); return v135.construct({v136,v137,v138}); }());v132.set(v133,v134); return v134; }());
([&]() -> JSValue { auto v139=self;auto v140=JSValue("specularColorMap");auto v141=JSValue(nullptr);v139.set(v140,v141); return v141; }());
([&]() -> JSValue { auto v142=self;auto v143=JSValue("_anisotropy");auto v144=JSValue(0);v142.set(v143,v144); return v144; }());
([&]() -> JSValue { auto v145=self;auto v146=JSValue("_clearcoat");auto v147=JSValue(0);v145.set(v146,v147); return v147; }());
([&]() -> JSValue { auto v148=self;auto v149=JSValue("_diffuseRoughness");auto v150=JSValue(0);v148.set(v149,v150); return v150; }());
([&]() -> JSValue { auto v151=self;auto v152=JSValue("_dispersion");auto v153=JSValue(0);v151.set(v152,v153); return v153; }());
([&]() -> JSValue { auto v154=self;auto v155=JSValue("_iridescence");auto v156=JSValue(0);v154.set(v155,v156); return v156; }());
([&]() -> JSValue { auto v157=self;auto v158=JSValue("_retroreflectivity");auto v159=JSValue(0);v157.set(v158,v159); return v159; }());
([&]() -> JSValue { auto v160=self;auto v161=JSValue("_sheen");auto v162=JSValue(0);v160.set(v161,v162); return v162; }());
([&]() -> JSValue { auto v163=self;auto v164=JSValue("_transmission");auto v165=JSValue(0);v163.set(v164,v165); return v165; }());
([&]() -> JSValue { auto v167=self;auto v166=v167.get("setValues");auto v168=parameters; return v166.call(v167,{v168}); }());return self;}

JSValue get_anisotropy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){return ([&]() -> JSValue { auto v169=self;auto v170=JSValue("_anisotropy"); return v169.get(v170); }());return JSValue{};}

JSValue set_anisotropy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto value=arg(args,0);if(([&]() -> JSValue { auto v171=([&]() -> JSValue { auto v173=([&]() -> JSValue { auto v175=self;auto v176=JSValue("_anisotropy"); return v175.get(v176); }());auto v174=JSValue(0); return JSValue(greater(v173,v174)); }());auto v172=([&]() -> JSValue { auto v177=value;auto v178=JSValue(0); return JSValue(greater(v177,v178)); }()); return JSValue(!v171.strictEquals(v172)); }()).toBoolean()){
([&]() -> JSValue { auto v179=self;auto v180=JSValue("version");const auto v181=v179.get(v180).toNumber();JSValue v182(v181+1);v179.set(v180,v182); return JSValue(v181); }());
}
([&]() -> JSValue { auto v183=self;auto v184=JSValue("_anisotropy");auto v185=value;v183.set(v184,v185); return v185; }());return JSValue{};}

JSValue get_clearcoat(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){return ([&]() -> JSValue { auto v186=self;auto v187=JSValue("_clearcoat"); return v186.get(v187); }());return JSValue{};}

JSValue set_clearcoat(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto value=arg(args,0);if(([&]() -> JSValue { auto v188=([&]() -> JSValue { auto v190=([&]() -> JSValue { auto v192=self;auto v193=JSValue("_clearcoat"); return v192.get(v193); }());auto v191=JSValue(0); return JSValue(greater(v190,v191)); }());auto v189=([&]() -> JSValue { auto v194=value;auto v195=JSValue(0); return JSValue(greater(v194,v195)); }()); return JSValue(!v188.strictEquals(v189)); }()).toBoolean()){
([&]() -> JSValue { auto v196=self;auto v197=JSValue("version");const auto v198=v196.get(v197).toNumber();JSValue v199(v198+1);v196.set(v197,v199); return JSValue(v198); }());
}
([&]() -> JSValue { auto v200=self;auto v201=JSValue("_clearcoat");auto v202=value;v200.set(v201,v202); return v202; }());return JSValue{};}

JSValue get_diffuseRoughness(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){return ([&]() -> JSValue { auto v203=self;auto v204=JSValue("_diffuseRoughness"); return v203.get(v204); }());return JSValue{};}

JSValue set_diffuseRoughness(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto value=arg(args,0);if(([&]() -> JSValue { auto v205=([&]() -> JSValue { auto v207=([&]() -> JSValue { auto v209=self;auto v210=JSValue("_diffuseRoughness"); return v209.get(v210); }());auto v208=JSValue(0); return JSValue(greater(v207,v208)); }());auto v206=([&]() -> JSValue { auto v211=value;auto v212=JSValue(0); return JSValue(greater(v211,v212)); }()); return JSValue(!v205.strictEquals(v206)); }()).toBoolean()){
([&]() -> JSValue { auto v213=self;auto v214=JSValue("version");const auto v215=v213.get(v214).toNumber();JSValue v216(v215+1);v213.set(v214,v216); return JSValue(v215); }());
}
([&]() -> JSValue { auto v217=self;auto v218=JSValue("_diffuseRoughness");auto v219=value;v217.set(v218,v219); return v219; }());return JSValue{};}

JSValue get_iridescence(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){return ([&]() -> JSValue { auto v220=self;auto v221=JSValue("_iridescence"); return v220.get(v221); }());return JSValue{};}

JSValue set_iridescence(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto value=arg(args,0);if(([&]() -> JSValue { auto v222=([&]() -> JSValue { auto v224=([&]() -> JSValue { auto v226=self;auto v227=JSValue("_iridescence"); return v226.get(v227); }());auto v225=JSValue(0); return JSValue(greater(v224,v225)); }());auto v223=([&]() -> JSValue { auto v228=value;auto v229=JSValue(0); return JSValue(greater(v228,v229)); }()); return JSValue(!v222.strictEquals(v223)); }()).toBoolean()){
([&]() -> JSValue { auto v230=self;auto v231=JSValue("version");const auto v232=v230.get(v231).toNumber();JSValue v233(v232+1);v230.set(v231,v233); return JSValue(v232); }());
}
([&]() -> JSValue { auto v234=self;auto v235=JSValue("_iridescence");auto v236=value;v234.set(v235,v236); return v236; }());return JSValue{};}

JSValue get_dispersion(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){return ([&]() -> JSValue { auto v237=self;auto v238=JSValue("_dispersion"); return v237.get(v238); }());return JSValue{};}

JSValue set_dispersion(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto value=arg(args,0);if(([&]() -> JSValue { auto v239=([&]() -> JSValue { auto v241=([&]() -> JSValue { auto v243=self;auto v244=JSValue("_dispersion"); return v243.get(v244); }());auto v242=JSValue(0); return JSValue(greater(v241,v242)); }());auto v240=([&]() -> JSValue { auto v245=value;auto v246=JSValue(0); return JSValue(greater(v245,v246)); }()); return JSValue(!v239.strictEquals(v240)); }()).toBoolean()){
([&]() -> JSValue { auto v247=self;auto v248=JSValue("version");const auto v249=v247.get(v248).toNumber();JSValue v250(v249+1);v247.set(v248,v250); return JSValue(v249); }());
}
([&]() -> JSValue { auto v251=self;auto v252=JSValue("_dispersion");auto v253=value;v251.set(v252,v253); return v253; }());return JSValue{};}

JSValue get_retroreflectivity(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){return ([&]() -> JSValue { auto v254=self;auto v255=JSValue("_retroreflectivity"); return v254.get(v255); }());return JSValue{};}

JSValue set_retroreflectivity(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto value=arg(args,0);if(([&]() -> JSValue { auto v256=([&]() -> JSValue { auto v258=([&]() -> JSValue { auto v260=self;auto v261=JSValue("_retroreflectivity"); return v260.get(v261); }());auto v259=JSValue(0); return JSValue(greater(v258,v259)); }());auto v257=([&]() -> JSValue { auto v262=value;auto v263=JSValue(0); return JSValue(greater(v262,v263)); }()); return JSValue(!v256.strictEquals(v257)); }()).toBoolean()){
([&]() -> JSValue { auto v264=self;auto v265=JSValue("version");const auto v266=v264.get(v265).toNumber();JSValue v267(v266+1);v264.set(v265,v267); return JSValue(v266); }());
}
([&]() -> JSValue { auto v268=self;auto v269=JSValue("_retroreflectivity");auto v270=value;v268.set(v269,v270); return v270; }());return JSValue{};}

JSValue get_sheen(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){return ([&]() -> JSValue { auto v271=self;auto v272=JSValue("_sheen"); return v271.get(v272); }());return JSValue{};}

JSValue set_sheen(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto value=arg(args,0);if(([&]() -> JSValue { auto v273=([&]() -> JSValue { auto v275=([&]() -> JSValue { auto v277=self;auto v278=JSValue("_sheen"); return v277.get(v278); }());auto v276=JSValue(0); return JSValue(greater(v275,v276)); }());auto v274=([&]() -> JSValue { auto v279=value;auto v280=JSValue(0); return JSValue(greater(v279,v280)); }()); return JSValue(!v273.strictEquals(v274)); }()).toBoolean()){
([&]() -> JSValue { auto v281=self;auto v282=JSValue("version");const auto v283=v281.get(v282).toNumber();JSValue v284(v283+1);v281.set(v282,v284); return JSValue(v283); }());
}
([&]() -> JSValue { auto v285=self;auto v286=JSValue("_sheen");auto v287=value;v285.set(v286,v287); return v287; }());return JSValue{};}

JSValue get_transmission(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){return ([&]() -> JSValue { auto v288=self;auto v289=JSValue("_transmission"); return v288.get(v289); }());return JSValue{};}

JSValue set_transmission(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto value=arg(args,0);if(([&]() -> JSValue { auto v290=([&]() -> JSValue { auto v292=([&]() -> JSValue { auto v294=self;auto v295=JSValue("_transmission"); return v294.get(v295); }());auto v293=JSValue(0); return JSValue(greater(v292,v293)); }());auto v291=([&]() -> JSValue { auto v296=value;auto v297=JSValue(0); return JSValue(greater(v296,v297)); }()); return JSValue(!v290.strictEquals(v291)); }()).toBoolean()){
([&]() -> JSValue { auto v298=self;auto v299=JSValue("version");const auto v300=v298.get(v299).toNumber();JSValue v301(v300+1);v298.set(v299,v301); return JSValue(v300); }());
}
([&]() -> JSValue { auto v302=self;auto v303=JSValue("_transmission");auto v304=value;v302.set(v303,v304); return v304; }());return JSValue{};}

JSValue copy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto source=arg(args,0);([&]() -> JSValue { auto v306=home.getPrototype();auto v305=v306.get("copy",self);auto v307=source; return v305.call(self,{v307}); }());
([&]() -> JSValue { auto v308=self;auto v309=JSValue("defines");auto v310=([&]() -> JSValue { auto v311=JSValue::object();own(v311,"STANDARD",JSValue(""));own(v311,"PHYSICAL",JSValue("")); return v311; }());v308.set(v309,v310); return v310; }());
([&]() -> JSValue { auto v312=self;auto v313=JSValue("anisotropy");auto v314=([&]() -> JSValue { auto v315=source;auto v316=JSValue("anisotropy"); return v315.get(v316); }());v312.set(v313,v314); return v314; }());
([&]() -> JSValue { auto v317=self;auto v318=JSValue("anisotropyRotation");auto v319=([&]() -> JSValue { auto v320=source;auto v321=JSValue("anisotropyRotation"); return v320.get(v321); }());v317.set(v318,v319); return v319; }());
([&]() -> JSValue { auto v322=self;auto v323=JSValue("anisotropyMap");auto v324=([&]() -> JSValue { auto v325=source;auto v326=JSValue("anisotropyMap"); return v325.get(v326); }());v322.set(v323,v324); return v324; }());
([&]() -> JSValue { auto v327=self;auto v328=JSValue("clearcoat");auto v329=([&]() -> JSValue { auto v330=source;auto v331=JSValue("clearcoat"); return v330.get(v331); }());v327.set(v328,v329); return v329; }());
([&]() -> JSValue { auto v332=self;auto v333=JSValue("clearcoatMap");auto v334=([&]() -> JSValue { auto v335=source;auto v336=JSValue("clearcoatMap"); return v335.get(v336); }());v332.set(v333,v334); return v334; }());
([&]() -> JSValue { auto v337=self;auto v338=JSValue("clearcoatRoughness");auto v339=([&]() -> JSValue { auto v340=source;auto v341=JSValue("clearcoatRoughness"); return v340.get(v341); }());v337.set(v338,v339); return v339; }());
([&]() -> JSValue { auto v342=self;auto v343=JSValue("clearcoatRoughnessMap");auto v344=([&]() -> JSValue { auto v345=source;auto v346=JSValue("clearcoatRoughnessMap"); return v345.get(v346); }());v342.set(v343,v344); return v344; }());
([&]() -> JSValue { auto v347=self;auto v348=JSValue("clearcoatNormalMap");auto v349=([&]() -> JSValue { auto v350=source;auto v351=JSValue("clearcoatNormalMap"); return v350.get(v351); }());v347.set(v348,v349); return v349; }());
([&]() -> JSValue { auto v353=([&]() -> JSValue { auto v354=self;auto v355=JSValue("clearcoatNormalScale"); return v354.get(v355); }());auto v352=v353.get("copy");auto v356=([&]() -> JSValue { auto v357=source;auto v358=JSValue("clearcoatNormalScale"); return v357.get(v358); }()); return v352.call(v353,{v356}); }());
([&]() -> JSValue { auto v359=self;auto v360=JSValue("diffuseRoughness");auto v361=([&]() -> JSValue { auto v362=source;auto v363=JSValue("diffuseRoughness"); return v362.get(v363); }());v359.set(v360,v361); return v361; }());
([&]() -> JSValue { auto v364=self;auto v365=JSValue("diffuseRoughnessMap");auto v366=([&]() -> JSValue { auto v367=source;auto v368=JSValue("diffuseRoughnessMap"); return v367.get(v368); }());v364.set(v365,v366); return v366; }());
([&]() -> JSValue { auto v369=self;auto v370=JSValue("dispersion");auto v371=([&]() -> JSValue { auto v372=source;auto v373=JSValue("dispersion"); return v372.get(v373); }());v369.set(v370,v371); return v371; }());
([&]() -> JSValue { auto v374=self;auto v375=JSValue("ior");auto v376=([&]() -> JSValue { auto v377=source;auto v378=JSValue("ior"); return v377.get(v378); }());v374.set(v375,v376); return v376; }());
([&]() -> JSValue { auto v379=self;auto v380=JSValue("iridescence");auto v381=([&]() -> JSValue { auto v382=source;auto v383=JSValue("iridescence"); return v382.get(v383); }());v379.set(v380,v381); return v381; }());
([&]() -> JSValue { auto v384=self;auto v385=JSValue("iridescenceMap");auto v386=([&]() -> JSValue { auto v387=source;auto v388=JSValue("iridescenceMap"); return v387.get(v388); }());v384.set(v385,v386); return v386; }());
([&]() -> JSValue { auto v389=self;auto v390=JSValue("iridescenceIOR");auto v391=([&]() -> JSValue { auto v392=source;auto v393=JSValue("iridescenceIOR"); return v392.get(v393); }());v389.set(v390,v391); return v391; }());
([&]() -> JSValue { auto v394=self;auto v395=JSValue("iridescenceThicknessRange");auto v396=([&]() -> JSValue { auto v397=JSValue::array();std::size_t v398=0;auto v399=getJSIterator(([&]() -> JSValue { auto v401=source;auto v402=JSValue("iridescenceThicknessRange"); return v401.get(v402); }()));while(auto v400=stepJSIterator(v399)){own(v397,std::to_string(v398++),*v400);}v397.set("length",v398); return v397; }());v394.set(v395,v396); return v396; }());
([&]() -> JSValue { auto v403=self;auto v404=JSValue("iridescenceThicknessMap");auto v405=([&]() -> JSValue { auto v406=source;auto v407=JSValue("iridescenceThicknessMap"); return v406.get(v407); }());v403.set(v404,v405); return v405; }());
([&]() -> JSValue { auto v408=self;auto v409=JSValue("retroreflectivity");auto v410=([&]() -> JSValue { auto v411=source;auto v412=JSValue("retroreflectivity"); return v411.get(v412); }());v408.set(v409,v410); return v410; }());
([&]() -> JSValue { auto v413=self;auto v414=JSValue("sheen");auto v415=([&]() -> JSValue { auto v416=source;auto v417=JSValue("sheen"); return v416.get(v417); }());v413.set(v414,v415); return v415; }());
([&]() -> JSValue { auto v419=([&]() -> JSValue { auto v420=self;auto v421=JSValue("sheenColor"); return v420.get(v421); }());auto v418=v419.get("copy");auto v422=([&]() -> JSValue { auto v423=source;auto v424=JSValue("sheenColor"); return v423.get(v424); }()); return v418.call(v419,{v422}); }());
([&]() -> JSValue { auto v425=self;auto v426=JSValue("sheenColorMap");auto v427=([&]() -> JSValue { auto v428=source;auto v429=JSValue("sheenColorMap"); return v428.get(v429); }());v425.set(v426,v427); return v427; }());
([&]() -> JSValue { auto v430=self;auto v431=JSValue("sheenRoughness");auto v432=([&]() -> JSValue { auto v433=source;auto v434=JSValue("sheenRoughness"); return v433.get(v434); }());v430.set(v431,v432); return v432; }());
([&]() -> JSValue { auto v435=self;auto v436=JSValue("sheenRoughnessMap");auto v437=([&]() -> JSValue { auto v438=source;auto v439=JSValue("sheenRoughnessMap"); return v438.get(v439); }());v435.set(v436,v437); return v437; }());
([&]() -> JSValue { auto v440=self;auto v441=JSValue("transmission");auto v442=([&]() -> JSValue { auto v443=source;auto v444=JSValue("transmission"); return v443.get(v444); }());v440.set(v441,v442); return v442; }());
([&]() -> JSValue { auto v445=self;auto v446=JSValue("transmissionMap");auto v447=([&]() -> JSValue { auto v448=source;auto v449=JSValue("transmissionMap"); return v448.get(v449); }());v445.set(v446,v447); return v447; }());
([&]() -> JSValue { auto v450=self;auto v451=JSValue("thickness");auto v452=([&]() -> JSValue { auto v453=source;auto v454=JSValue("thickness"); return v453.get(v454); }());v450.set(v451,v452); return v452; }());
([&]() -> JSValue { auto v455=self;auto v456=JSValue("thicknessMap");auto v457=([&]() -> JSValue { auto v458=source;auto v459=JSValue("thicknessMap"); return v458.get(v459); }());v455.set(v456,v457); return v457; }());
([&]() -> JSValue { auto v460=self;auto v461=JSValue("attenuationDistance");auto v462=([&]() -> JSValue { auto v463=source;auto v464=JSValue("attenuationDistance"); return v463.get(v464); }());v460.set(v461,v462); return v462; }());
([&]() -> JSValue { auto v466=([&]() -> JSValue { auto v467=self;auto v468=JSValue("attenuationColor"); return v467.get(v468); }());auto v465=v466.get("copy");auto v469=([&]() -> JSValue { auto v470=source;auto v471=JSValue("attenuationColor"); return v470.get(v471); }()); return v465.call(v466,{v469}); }());
([&]() -> JSValue { auto v472=self;auto v473=JSValue("specularIntensity");auto v474=([&]() -> JSValue { auto v475=source;auto v476=JSValue("specularIntensity"); return v475.get(v476); }());v472.set(v473,v474); return v474; }());
([&]() -> JSValue { auto v477=self;auto v478=JSValue("specularIntensityMap");auto v479=([&]() -> JSValue { auto v480=source;auto v481=JSValue("specularIntensityMap"); return v480.get(v481); }());v477.set(v478,v479); return v479; }());
([&]() -> JSValue { auto v483=([&]() -> JSValue { auto v484=self;auto v485=JSValue("specularColor"); return v484.get(v485); }());auto v482=v483.get("copy");auto v486=([&]() -> JSValue { auto v487=source;auto v488=JSValue("specularColor"); return v487.get(v488); }()); return v482.call(v483,{v486}); }());
([&]() -> JSValue { auto v489=self;auto v490=JSValue("specularColorMap");auto v491=([&]() -> JSValue { auto v492=source;auto v493=JSValue("specularColorMap"); return v492.get(v493); }());v489.set(v490,v491); return v491; }());
return self;return JSValue{};}
