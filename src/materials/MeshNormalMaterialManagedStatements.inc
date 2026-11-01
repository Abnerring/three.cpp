// Generated static native statements; fixed source SHA256 8f85107771c13120bab934f3f7c32ec4748a7cbe76763aebb91789e71e0fc0f4

JSValue constructor(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto parameters=arg(args,0);JSValue{};
([&]() -> JSValue { auto v1=self;auto v2=JSValue("isMeshNormalMaterial");auto v3=JSValue(true);v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self;auto v5=JSValue("type");auto v6=JSValue("MeshNormalMaterial");v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v7=self;auto v8=JSValue("bumpMap");auto v9=JSValue(nullptr);v7.set(v8,v9); return v9; }());
([&]() -> JSValue { auto v10=self;auto v11=JSValue("bumpScale");auto v12=JSValue(1);v10.set(v11,v12); return v12; }());
([&]() -> JSValue { auto v13=self;auto v14=JSValue("normalMap");auto v15=JSValue(nullptr);v13.set(v14,v15); return v15; }());
([&]() -> JSValue { auto v16=self;auto v17=JSValue("normalMapType");auto v18=imports.get("TangentSpaceNormalMap");v16.set(v17,v18); return v18; }());
([&]() -> JSValue { auto v19=self;auto v20=JSValue("normalScale");auto v21=([&]() -> JSValue { auto v22=imports.get("Vector2");auto v23=JSValue(1);auto v24=JSValue(1); return v22.construct({v23,v24}); }());v19.set(v20,v21); return v21; }());
([&]() -> JSValue { auto v25=self;auto v26=JSValue("displacementMap");auto v27=JSValue(nullptr);v25.set(v26,v27); return v27; }());
([&]() -> JSValue { auto v28=self;auto v29=JSValue("displacementScale");auto v30=JSValue(1);v28.set(v29,v30); return v30; }());
([&]() -> JSValue { auto v31=self;auto v32=JSValue("displacementBias");auto v33=JSValue(0);v31.set(v32,v33); return v33; }());
([&]() -> JSValue { auto v34=self;auto v35=JSValue("wireframe");auto v36=JSValue(false);v34.set(v35,v36); return v36; }());
([&]() -> JSValue { auto v37=self;auto v38=JSValue("wireframeLinewidth");auto v39=JSValue(1);v37.set(v38,v39); return v39; }());
([&]() -> JSValue { auto v40=self;auto v41=JSValue("flatShading");auto v42=JSValue(false);v40.set(v41,v42); return v42; }());
([&]() -> JSValue { auto v44=self;auto v43=v44.get("setValues");auto v45=parameters; return v43.call(v44,{v45}); }());return self;}

JSValue copy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto source=arg(args,0);([&]() -> JSValue { auto v47=home.getPrototype();auto v46=v47.get("copy",self);auto v48=source; return v46.call(self,{v48}); }());
([&]() -> JSValue { auto v49=self;auto v50=JSValue("bumpMap");auto v51=([&]() -> JSValue { auto v52=source;auto v53=JSValue("bumpMap"); return v52.get(v53); }());v49.set(v50,v51); return v51; }());
([&]() -> JSValue { auto v54=self;auto v55=JSValue("bumpScale");auto v56=([&]() -> JSValue { auto v57=source;auto v58=JSValue("bumpScale"); return v57.get(v58); }());v54.set(v55,v56); return v56; }());
([&]() -> JSValue { auto v59=self;auto v60=JSValue("normalMap");auto v61=([&]() -> JSValue { auto v62=source;auto v63=JSValue("normalMap"); return v62.get(v63); }());v59.set(v60,v61); return v61; }());
([&]() -> JSValue { auto v64=self;auto v65=JSValue("normalMapType");auto v66=([&]() -> JSValue { auto v67=source;auto v68=JSValue("normalMapType"); return v67.get(v68); }());v64.set(v65,v66); return v66; }());
([&]() -> JSValue { auto v70=([&]() -> JSValue { auto v71=self;auto v72=JSValue("normalScale"); return v71.get(v72); }());auto v69=v70.get("copy");auto v73=([&]() -> JSValue { auto v74=source;auto v75=JSValue("normalScale"); return v74.get(v75); }()); return v69.call(v70,{v73}); }());
([&]() -> JSValue { auto v76=self;auto v77=JSValue("displacementMap");auto v78=([&]() -> JSValue { auto v79=source;auto v80=JSValue("displacementMap"); return v79.get(v80); }());v76.set(v77,v78); return v78; }());
([&]() -> JSValue { auto v81=self;auto v82=JSValue("displacementScale");auto v83=([&]() -> JSValue { auto v84=source;auto v85=JSValue("displacementScale"); return v84.get(v85); }());v81.set(v82,v83); return v83; }());
([&]() -> JSValue { auto v86=self;auto v87=JSValue("displacementBias");auto v88=([&]() -> JSValue { auto v89=source;auto v90=JSValue("displacementBias"); return v89.get(v90); }());v86.set(v87,v88); return v88; }());
([&]() -> JSValue { auto v91=self;auto v92=JSValue("wireframe");auto v93=([&]() -> JSValue { auto v94=source;auto v95=JSValue("wireframe"); return v94.get(v95); }());v91.set(v92,v93); return v93; }());
([&]() -> JSValue { auto v96=self;auto v97=JSValue("wireframeLinewidth");auto v98=([&]() -> JSValue { auto v99=source;auto v100=JSValue("wireframeLinewidth"); return v99.get(v100); }());v96.set(v97,v98); return v98; }());
([&]() -> JSValue { auto v101=self;auto v102=JSValue("flatShading");auto v103=([&]() -> JSValue { auto v104=source;auto v105=JSValue("flatShading"); return v104.get(v105); }());v101.set(v102,v103); return v103; }());
return self;return JSValue{};}
