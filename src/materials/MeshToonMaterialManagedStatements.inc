// Generated static native statements; fixed source SHA256 9124966fc5e27f71d10366c193395795f552468d01f62bd7bfe1bccac6da4c12

JSValue constructor(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto parameters=arg(args,0);JSValue{};
([&]() -> JSValue { auto v1=self;auto v2=JSValue("isMeshToonMaterial");auto v3=JSValue(true);v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self;auto v5=JSValue("defines");auto v6=([&]() -> JSValue { auto v7=JSValue::object();own(v7,"TOON",JSValue("")); return v7; }());v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v8=self;auto v9=JSValue("type");auto v10=JSValue("MeshToonMaterial");v8.set(v9,v10); return v10; }());
([&]() -> JSValue { auto v11=self;auto v12=JSValue("color");auto v13=([&]() -> JSValue { auto v14=imports.get("Color");auto v15=JSValue(16777215); return v14.construct({v15}); }());v11.set(v12,v13); return v13; }());
([&]() -> JSValue { auto v16=self;auto v17=JSValue("map");auto v18=JSValue(nullptr);v16.set(v17,v18); return v18; }());
([&]() -> JSValue { auto v19=self;auto v20=JSValue("gradientMap");auto v21=JSValue(nullptr);v19.set(v20,v21); return v21; }());
([&]() -> JSValue { auto v22=self;auto v23=JSValue("lightMap");auto v24=JSValue(nullptr);v22.set(v23,v24); return v24; }());
([&]() -> JSValue { auto v25=self;auto v26=JSValue("lightMapIntensity");auto v27=JSValue(1);v25.set(v26,v27); return v27; }());
([&]() -> JSValue { auto v28=self;auto v29=JSValue("aoMap");auto v30=JSValue(nullptr);v28.set(v29,v30); return v30; }());
([&]() -> JSValue { auto v31=self;auto v32=JSValue("aoMapIntensity");auto v33=JSValue(1);v31.set(v32,v33); return v33; }());
([&]() -> JSValue { auto v34=self;auto v35=JSValue("emissive");auto v36=([&]() -> JSValue { auto v37=imports.get("Color");auto v38=JSValue(0); return v37.construct({v38}); }());v34.set(v35,v36); return v36; }());
([&]() -> JSValue { auto v39=self;auto v40=JSValue("emissiveIntensity");auto v41=JSValue(1);v39.set(v40,v41); return v41; }());
([&]() -> JSValue { auto v42=self;auto v43=JSValue("emissiveMap");auto v44=JSValue(nullptr);v42.set(v43,v44); return v44; }());
([&]() -> JSValue { auto v45=self;auto v46=JSValue("bumpMap");auto v47=JSValue(nullptr);v45.set(v46,v47); return v47; }());
([&]() -> JSValue { auto v48=self;auto v49=JSValue("bumpScale");auto v50=JSValue(1);v48.set(v49,v50); return v50; }());
([&]() -> JSValue { auto v51=self;auto v52=JSValue("normalMap");auto v53=JSValue(nullptr);v51.set(v52,v53); return v53; }());
([&]() -> JSValue { auto v54=self;auto v55=JSValue("normalMapType");auto v56=imports.get("TangentSpaceNormalMap");v54.set(v55,v56); return v56; }());
([&]() -> JSValue { auto v57=self;auto v58=JSValue("normalScale");auto v59=([&]() -> JSValue { auto v60=imports.get("Vector2");auto v61=JSValue(1);auto v62=JSValue(1); return v60.construct({v61,v62}); }());v57.set(v58,v59); return v59; }());
([&]() -> JSValue { auto v63=self;auto v64=JSValue("displacementMap");auto v65=JSValue(nullptr);v63.set(v64,v65); return v65; }());
([&]() -> JSValue { auto v66=self;auto v67=JSValue("displacementScale");auto v68=JSValue(1);v66.set(v67,v68); return v68; }());
([&]() -> JSValue { auto v69=self;auto v70=JSValue("displacementBias");auto v71=JSValue(0);v69.set(v70,v71); return v71; }());
([&]() -> JSValue { auto v72=self;auto v73=JSValue("alphaMap");auto v74=JSValue(nullptr);v72.set(v73,v74); return v74; }());
([&]() -> JSValue { auto v75=self;auto v76=JSValue("wireframe");auto v77=JSValue(false);v75.set(v76,v77); return v77; }());
([&]() -> JSValue { auto v78=self;auto v79=JSValue("wireframeLinewidth");auto v80=JSValue(1);v78.set(v79,v80); return v80; }());
([&]() -> JSValue { auto v81=self;auto v82=JSValue("wireframeLinecap");auto v83=JSValue("round");v81.set(v82,v83); return v83; }());
([&]() -> JSValue { auto v84=self;auto v85=JSValue("wireframeLinejoin");auto v86=JSValue("round");v84.set(v85,v86); return v86; }());
([&]() -> JSValue { auto v87=self;auto v88=JSValue("fog");auto v89=JSValue(true);v87.set(v88,v89); return v89; }());
([&]() -> JSValue { auto v91=self;auto v90=v91.get("setValues");auto v92=parameters; return v90.call(v91,{v92}); }());return self;}

JSValue copy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto source=arg(args,0);([&]() -> JSValue { auto v94=home.getPrototype();auto v93=v94.get("copy",self);auto v95=source; return v93.call(self,{v95}); }());
([&]() -> JSValue { auto v97=([&]() -> JSValue { auto v98=self;auto v99=JSValue("color"); return v98.get(v99); }());auto v96=v97.get("copy");auto v100=([&]() -> JSValue { auto v101=source;auto v102=JSValue("color"); return v101.get(v102); }()); return v96.call(v97,{v100}); }());
([&]() -> JSValue { auto v103=self;auto v104=JSValue("map");auto v105=([&]() -> JSValue { auto v106=source;auto v107=JSValue("map"); return v106.get(v107); }());v103.set(v104,v105); return v105; }());
([&]() -> JSValue { auto v108=self;auto v109=JSValue("gradientMap");auto v110=([&]() -> JSValue { auto v111=source;auto v112=JSValue("gradientMap"); return v111.get(v112); }());v108.set(v109,v110); return v110; }());
([&]() -> JSValue { auto v113=self;auto v114=JSValue("lightMap");auto v115=([&]() -> JSValue { auto v116=source;auto v117=JSValue("lightMap"); return v116.get(v117); }());v113.set(v114,v115); return v115; }());
([&]() -> JSValue { auto v118=self;auto v119=JSValue("lightMapIntensity");auto v120=([&]() -> JSValue { auto v121=source;auto v122=JSValue("lightMapIntensity"); return v121.get(v122); }());v118.set(v119,v120); return v120; }());
([&]() -> JSValue { auto v123=self;auto v124=JSValue("aoMap");auto v125=([&]() -> JSValue { auto v126=source;auto v127=JSValue("aoMap"); return v126.get(v127); }());v123.set(v124,v125); return v125; }());
([&]() -> JSValue { auto v128=self;auto v129=JSValue("aoMapIntensity");auto v130=([&]() -> JSValue { auto v131=source;auto v132=JSValue("aoMapIntensity"); return v131.get(v132); }());v128.set(v129,v130); return v130; }());
([&]() -> JSValue { auto v134=([&]() -> JSValue { auto v135=self;auto v136=JSValue("emissive"); return v135.get(v136); }());auto v133=v134.get("copy");auto v137=([&]() -> JSValue { auto v138=source;auto v139=JSValue("emissive"); return v138.get(v139); }()); return v133.call(v134,{v137}); }());
([&]() -> JSValue { auto v140=self;auto v141=JSValue("emissiveMap");auto v142=([&]() -> JSValue { auto v143=source;auto v144=JSValue("emissiveMap"); return v143.get(v144); }());v140.set(v141,v142); return v142; }());
([&]() -> JSValue { auto v145=self;auto v146=JSValue("emissiveIntensity");auto v147=([&]() -> JSValue { auto v148=source;auto v149=JSValue("emissiveIntensity"); return v148.get(v149); }());v145.set(v146,v147); return v147; }());
([&]() -> JSValue { auto v150=self;auto v151=JSValue("bumpMap");auto v152=([&]() -> JSValue { auto v153=source;auto v154=JSValue("bumpMap"); return v153.get(v154); }());v150.set(v151,v152); return v152; }());
([&]() -> JSValue { auto v155=self;auto v156=JSValue("bumpScale");auto v157=([&]() -> JSValue { auto v158=source;auto v159=JSValue("bumpScale"); return v158.get(v159); }());v155.set(v156,v157); return v157; }());
([&]() -> JSValue { auto v160=self;auto v161=JSValue("normalMap");auto v162=([&]() -> JSValue { auto v163=source;auto v164=JSValue("normalMap"); return v163.get(v164); }());v160.set(v161,v162); return v162; }());
([&]() -> JSValue { auto v165=self;auto v166=JSValue("normalMapType");auto v167=([&]() -> JSValue { auto v168=source;auto v169=JSValue("normalMapType"); return v168.get(v169); }());v165.set(v166,v167); return v167; }());
([&]() -> JSValue { auto v171=([&]() -> JSValue { auto v172=self;auto v173=JSValue("normalScale"); return v172.get(v173); }());auto v170=v171.get("copy");auto v174=([&]() -> JSValue { auto v175=source;auto v176=JSValue("normalScale"); return v175.get(v176); }()); return v170.call(v171,{v174}); }());
([&]() -> JSValue { auto v177=self;auto v178=JSValue("displacementMap");auto v179=([&]() -> JSValue { auto v180=source;auto v181=JSValue("displacementMap"); return v180.get(v181); }());v177.set(v178,v179); return v179; }());
([&]() -> JSValue { auto v182=self;auto v183=JSValue("displacementScale");auto v184=([&]() -> JSValue { auto v185=source;auto v186=JSValue("displacementScale"); return v185.get(v186); }());v182.set(v183,v184); return v184; }());
([&]() -> JSValue { auto v187=self;auto v188=JSValue("displacementBias");auto v189=([&]() -> JSValue { auto v190=source;auto v191=JSValue("displacementBias"); return v190.get(v191); }());v187.set(v188,v189); return v189; }());
([&]() -> JSValue { auto v192=self;auto v193=JSValue("alphaMap");auto v194=([&]() -> JSValue { auto v195=source;auto v196=JSValue("alphaMap"); return v195.get(v196); }());v192.set(v193,v194); return v194; }());
([&]() -> JSValue { auto v197=self;auto v198=JSValue("wireframe");auto v199=([&]() -> JSValue { auto v200=source;auto v201=JSValue("wireframe"); return v200.get(v201); }());v197.set(v198,v199); return v199; }());
([&]() -> JSValue { auto v202=self;auto v203=JSValue("wireframeLinewidth");auto v204=([&]() -> JSValue { auto v205=source;auto v206=JSValue("wireframeLinewidth"); return v205.get(v206); }());v202.set(v203,v204); return v204; }());
([&]() -> JSValue { auto v207=self;auto v208=JSValue("wireframeLinecap");auto v209=([&]() -> JSValue { auto v210=source;auto v211=JSValue("wireframeLinecap"); return v210.get(v211); }());v207.set(v208,v209); return v209; }());
([&]() -> JSValue { auto v212=self;auto v213=JSValue("wireframeLinejoin");auto v214=([&]() -> JSValue { auto v215=source;auto v216=JSValue("wireframeLinejoin"); return v215.get(v216); }());v212.set(v213,v214); return v214; }());
([&]() -> JSValue { auto v217=self;auto v218=JSValue("fog");auto v219=([&]() -> JSValue { auto v220=source;auto v221=JSValue("fog"); return v220.get(v221); }());v217.set(v218,v219); return v219; }());
return self;return JSValue{};}
