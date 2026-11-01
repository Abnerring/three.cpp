// Generated static native statements; fixed source SHA256 2a6102973ef8b04e656f7504e8e0b755718b5c16a87fab30d24e62e8a46a4fff

JSValue constructor(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto parameters=arg(args,0);JSValue{};
([&]() -> JSValue { auto v1=self;auto v2=JSValue("isMeshMatcapMaterial");auto v3=JSValue(true);v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self;auto v5=JSValue("defines");auto v6=([&]() -> JSValue { auto v7=JSValue::object();own(v7,"MATCAP",JSValue("")); return v7; }());v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v8=self;auto v9=JSValue("type");auto v10=JSValue("MeshMatcapMaterial");v8.set(v9,v10); return v10; }());
([&]() -> JSValue { auto v11=self;auto v12=JSValue("color");auto v13=([&]() -> JSValue { auto v14=imports.get("Color");auto v15=JSValue(16777215); return v14.construct({v15}); }());v11.set(v12,v13); return v13; }());
([&]() -> JSValue { auto v16=self;auto v17=JSValue("matcap");auto v18=JSValue(nullptr);v16.set(v17,v18); return v18; }());
([&]() -> JSValue { auto v19=self;auto v20=JSValue("map");auto v21=JSValue(nullptr);v19.set(v20,v21); return v21; }());
([&]() -> JSValue { auto v22=self;auto v23=JSValue("bumpMap");auto v24=JSValue(nullptr);v22.set(v23,v24); return v24; }());
([&]() -> JSValue { auto v25=self;auto v26=JSValue("bumpScale");auto v27=JSValue(1);v25.set(v26,v27); return v27; }());
([&]() -> JSValue { auto v28=self;auto v29=JSValue("normalMap");auto v30=JSValue(nullptr);v28.set(v29,v30); return v30; }());
([&]() -> JSValue { auto v31=self;auto v32=JSValue("normalMapType");auto v33=imports.get("TangentSpaceNormalMap");v31.set(v32,v33); return v33; }());
([&]() -> JSValue { auto v34=self;auto v35=JSValue("normalScale");auto v36=([&]() -> JSValue { auto v37=imports.get("Vector2");auto v38=JSValue(1);auto v39=JSValue(1); return v37.construct({v38,v39}); }());v34.set(v35,v36); return v36; }());
([&]() -> JSValue { auto v40=self;auto v41=JSValue("displacementMap");auto v42=JSValue(nullptr);v40.set(v41,v42); return v42; }());
([&]() -> JSValue { auto v43=self;auto v44=JSValue("displacementScale");auto v45=JSValue(1);v43.set(v44,v45); return v45; }());
([&]() -> JSValue { auto v46=self;auto v47=JSValue("displacementBias");auto v48=JSValue(0);v46.set(v47,v48); return v48; }());
([&]() -> JSValue { auto v49=self;auto v50=JSValue("alphaMap");auto v51=JSValue(nullptr);v49.set(v50,v51); return v51; }());
([&]() -> JSValue { auto v52=self;auto v53=JSValue("wireframe");auto v54=JSValue(false);v52.set(v53,v54); return v54; }());
([&]() -> JSValue { auto v55=self;auto v56=JSValue("wireframeLinewidth");auto v57=JSValue(1);v55.set(v56,v57); return v57; }());
([&]() -> JSValue { auto v58=self;auto v59=JSValue("flatShading");auto v60=JSValue(false);v58.set(v59,v60); return v60; }());
([&]() -> JSValue { auto v61=self;auto v62=JSValue("fog");auto v63=JSValue(true);v61.set(v62,v63); return v63; }());
([&]() -> JSValue { auto v65=self;auto v64=v65.get("setValues");auto v66=parameters; return v64.call(v65,{v66}); }());return self;}

JSValue copy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto source=arg(args,0);([&]() -> JSValue { auto v68=home.getPrototype();auto v67=v68.get("copy",self);auto v69=source; return v67.call(self,{v69}); }());
([&]() -> JSValue { auto v70=self;auto v71=JSValue("defines");auto v72=([&]() -> JSValue { auto v73=JSValue::object();own(v73,"MATCAP",JSValue("")); return v73; }());v70.set(v71,v72); return v72; }());
([&]() -> JSValue { auto v75=([&]() -> JSValue { auto v76=self;auto v77=JSValue("color"); return v76.get(v77); }());auto v74=v75.get("copy");auto v78=([&]() -> JSValue { auto v79=source;auto v80=JSValue("color"); return v79.get(v80); }()); return v74.call(v75,{v78}); }());
([&]() -> JSValue { auto v81=self;auto v82=JSValue("matcap");auto v83=([&]() -> JSValue { auto v84=source;auto v85=JSValue("matcap"); return v84.get(v85); }());v81.set(v82,v83); return v83; }());
([&]() -> JSValue { auto v86=self;auto v87=JSValue("map");auto v88=([&]() -> JSValue { auto v89=source;auto v90=JSValue("map"); return v89.get(v90); }());v86.set(v87,v88); return v88; }());
([&]() -> JSValue { auto v91=self;auto v92=JSValue("bumpMap");auto v93=([&]() -> JSValue { auto v94=source;auto v95=JSValue("bumpMap"); return v94.get(v95); }());v91.set(v92,v93); return v93; }());
([&]() -> JSValue { auto v96=self;auto v97=JSValue("bumpScale");auto v98=([&]() -> JSValue { auto v99=source;auto v100=JSValue("bumpScale"); return v99.get(v100); }());v96.set(v97,v98); return v98; }());
([&]() -> JSValue { auto v101=self;auto v102=JSValue("normalMap");auto v103=([&]() -> JSValue { auto v104=source;auto v105=JSValue("normalMap"); return v104.get(v105); }());v101.set(v102,v103); return v103; }());
([&]() -> JSValue { auto v106=self;auto v107=JSValue("normalMapType");auto v108=([&]() -> JSValue { auto v109=source;auto v110=JSValue("normalMapType"); return v109.get(v110); }());v106.set(v107,v108); return v108; }());
([&]() -> JSValue { auto v112=([&]() -> JSValue { auto v113=self;auto v114=JSValue("normalScale"); return v113.get(v114); }());auto v111=v112.get("copy");auto v115=([&]() -> JSValue { auto v116=source;auto v117=JSValue("normalScale"); return v116.get(v117); }()); return v111.call(v112,{v115}); }());
([&]() -> JSValue { auto v118=self;auto v119=JSValue("displacementMap");auto v120=([&]() -> JSValue { auto v121=source;auto v122=JSValue("displacementMap"); return v121.get(v122); }());v118.set(v119,v120); return v120; }());
([&]() -> JSValue { auto v123=self;auto v124=JSValue("displacementScale");auto v125=([&]() -> JSValue { auto v126=source;auto v127=JSValue("displacementScale"); return v126.get(v127); }());v123.set(v124,v125); return v125; }());
([&]() -> JSValue { auto v128=self;auto v129=JSValue("displacementBias");auto v130=([&]() -> JSValue { auto v131=source;auto v132=JSValue("displacementBias"); return v131.get(v132); }());v128.set(v129,v130); return v130; }());
([&]() -> JSValue { auto v133=self;auto v134=JSValue("alphaMap");auto v135=([&]() -> JSValue { auto v136=source;auto v137=JSValue("alphaMap"); return v136.get(v137); }());v133.set(v134,v135); return v135; }());
([&]() -> JSValue { auto v138=self;auto v139=JSValue("wireframe");auto v140=([&]() -> JSValue { auto v141=source;auto v142=JSValue("wireframe"); return v141.get(v142); }());v138.set(v139,v140); return v140; }());
([&]() -> JSValue { auto v143=self;auto v144=JSValue("wireframeLinewidth");auto v145=([&]() -> JSValue { auto v146=source;auto v147=JSValue("wireframeLinewidth"); return v146.get(v147); }());v143.set(v144,v145); return v145; }());
([&]() -> JSValue { auto v148=self;auto v149=JSValue("flatShading");auto v150=([&]() -> JSValue { auto v151=source;auto v152=JSValue("flatShading"); return v151.get(v152); }());v148.set(v149,v150); return v150; }());
([&]() -> JSValue { auto v153=self;auto v154=JSValue("fog");auto v155=([&]() -> JSValue { auto v156=source;auto v157=JSValue("fog"); return v156.get(v157); }());v153.set(v154,v155); return v155; }());
return self;return JSValue{};}
