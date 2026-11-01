// Generated native statements from fixed MeshBasicMaterial.js; do not edit.

// source SHA-256: 786730109ce037e6afc07560b8c6f378192d2a24ebe25aee9ce39f9592490345

JSValue meshBasicMaterial_constructor(JSValue self,JSValue parameters,const JSValue& imports) {
JSValue{};
([&]() -> JSValue { auto v1=self; auto v2=JSValue("isMeshBasicMaterial"); auto v3=JSValue(true); v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self; auto v5=JSValue("type"); auto v6=JSValue("MeshBasicMaterial"); v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v7=self; auto v8=JSValue("color"); auto v9=([&]() -> JSValue { auto v10=imports.get("Color");auto v11=JSValue(16777215); return v10.construct({v11}); }()); v7.set(v8,v9); return v9; }());
([&]() -> JSValue { auto v12=self; auto v13=JSValue("map"); auto v14=JSValue(nullptr); v12.set(v13,v14); return v14; }());
([&]() -> JSValue { auto v15=self; auto v16=JSValue("lightMap"); auto v17=JSValue(nullptr); v15.set(v16,v17); return v17; }());
([&]() -> JSValue { auto v18=self; auto v19=JSValue("lightMapIntensity"); auto v20=JSValue(1); v18.set(v19,v20); return v20; }());
([&]() -> JSValue { auto v21=self; auto v22=JSValue("aoMap"); auto v23=JSValue(nullptr); v21.set(v22,v23); return v23; }());
([&]() -> JSValue { auto v24=self; auto v25=JSValue("aoMapIntensity"); auto v26=JSValue(1); v24.set(v25,v26); return v26; }());
([&]() -> JSValue { auto v27=self; auto v28=JSValue("specularMap"); auto v29=JSValue(nullptr); v27.set(v28,v29); return v29; }());
([&]() -> JSValue { auto v30=self; auto v31=JSValue("alphaMap"); auto v32=JSValue(nullptr); v30.set(v31,v32); return v32; }());
([&]() -> JSValue { auto v33=self; auto v34=JSValue("envMap"); auto v35=JSValue(nullptr); v33.set(v34,v35); return v35; }());
([&]() -> JSValue { auto v36=self; auto v37=JSValue("envMapRotation"); auto v38=([&]() -> JSValue { auto v39=imports.get("Euler"); return v39.construct({}); }()); v36.set(v37,v38); return v38; }());
([&]() -> JSValue { auto v40=self; auto v41=JSValue("combine"); auto v42=MultiplyOperation; v40.set(v41,v42); return v42; }());
([&]() -> JSValue { auto v43=self; auto v44=JSValue("reflectivity"); auto v45=JSValue(1); v43.set(v44,v45); return v45; }());
([&]() -> JSValue { auto v46=self; auto v47=JSValue("refractionRatio"); auto v48=JSValue(0.98); v46.set(v47,v48); return v48; }());
([&]() -> JSValue { auto v49=self; auto v50=JSValue("wireframe"); auto v51=JSValue(false); v49.set(v50,v51); return v51; }());
([&]() -> JSValue { auto v52=self; auto v53=JSValue("wireframeLinewidth"); auto v54=JSValue(1); v52.set(v53,v54); return v54; }());
([&]() -> JSValue { auto v55=self; auto v56=JSValue("wireframeLinecap"); auto v57=JSValue("round"); v55.set(v56,v57); return v57; }());
([&]() -> JSValue { auto v58=self; auto v59=JSValue("wireframeLinejoin"); auto v60=JSValue("round"); v58.set(v59,v60); return v60; }());
([&]() -> JSValue { auto v61=self; auto v62=JSValue("fog"); auto v63=JSValue(true); v61.set(v62,v63); return v63; }());
([&]() -> JSValue { auto v64=self; auto v65=v64.get("setValues");auto v66=parameters; return v65.call(v64,{v66}); }());
return self;
}

JSValue meshBasicMaterial_copy(JSValue self,JSValue source,const JSValue& home) {
([&]() -> JSValue { auto v67=home.getPrototype().get("copy",self); auto v68=source; return v67.call(self,{v68}); }());
([&]() -> JSValue { auto v69=self.get("color"); auto v70=v69.get("copy");auto v71=source.get("color"); return v70.call(v69,{v71}); }());
([&]() -> JSValue { auto v72=self; auto v73=JSValue("map"); auto v74=source.get("map"); v72.set(v73,v74); return v74; }());
([&]() -> JSValue { auto v75=self; auto v76=JSValue("lightMap"); auto v77=source.get("lightMap"); v75.set(v76,v77); return v77; }());
([&]() -> JSValue { auto v78=self; auto v79=JSValue("lightMapIntensity"); auto v80=source.get("lightMapIntensity"); v78.set(v79,v80); return v80; }());
([&]() -> JSValue { auto v81=self; auto v82=JSValue("aoMap"); auto v83=source.get("aoMap"); v81.set(v82,v83); return v83; }());
([&]() -> JSValue { auto v84=self; auto v85=JSValue("aoMapIntensity"); auto v86=source.get("aoMapIntensity"); v84.set(v85,v86); return v86; }());
([&]() -> JSValue { auto v87=self; auto v88=JSValue("specularMap"); auto v89=source.get("specularMap"); v87.set(v88,v89); return v89; }());
([&]() -> JSValue { auto v90=self; auto v91=JSValue("alphaMap"); auto v92=source.get("alphaMap"); v90.set(v91,v92); return v92; }());
([&]() -> JSValue { auto v93=self; auto v94=JSValue("envMap"); auto v95=source.get("envMap"); v93.set(v94,v95); return v95; }());
([&]() -> JSValue { auto v96=self.get("envMapRotation"); auto v97=v96.get("copy");auto v98=source.get("envMapRotation"); return v97.call(v96,{v98}); }());
([&]() -> JSValue { auto v99=self; auto v100=JSValue("combine"); auto v101=source.get("combine"); v99.set(v100,v101); return v101; }());
([&]() -> JSValue { auto v102=self; auto v103=JSValue("reflectivity"); auto v104=source.get("reflectivity"); v102.set(v103,v104); return v104; }());
([&]() -> JSValue { auto v105=self; auto v106=JSValue("refractionRatio"); auto v107=source.get("refractionRatio"); v105.set(v106,v107); return v107; }());
([&]() -> JSValue { auto v108=self; auto v109=JSValue("wireframe"); auto v110=source.get("wireframe"); v108.set(v109,v110); return v110; }());
([&]() -> JSValue { auto v111=self; auto v112=JSValue("wireframeLinewidth"); auto v113=source.get("wireframeLinewidth"); v111.set(v112,v113); return v113; }());
([&]() -> JSValue { auto v114=self; auto v115=JSValue("wireframeLinecap"); auto v116=source.get("wireframeLinecap"); v114.set(v115,v116); return v116; }());
([&]() -> JSValue { auto v117=self; auto v118=JSValue("wireframeLinejoin"); auto v119=source.get("wireframeLinejoin"); v117.set(v118,v119); return v119; }());
([&]() -> JSValue { auto v120=self; auto v121=JSValue("fog"); auto v122=source.get("fog"); v120.set(v121,v122); return v122; }());
return self;
}
