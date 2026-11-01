// Generated static native statements; fixed source SHA256 83ad510c8e3fea385246f14fc6589fd2b24f5d6479bf77ab5b8e78d0dd83d627

JSValue constructor(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto parameters=arg(args,0);JSValue{};
([&]() -> JSValue { auto v1=self;auto v2=JSValue("isMeshDepthMaterial");auto v3=JSValue(true);v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self;auto v5=JSValue("type");auto v6=JSValue("MeshDepthMaterial");v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v7=self;auto v8=JSValue("depthPacking");auto v9=imports.get("BasicDepthPacking");v7.set(v8,v9); return v9; }());
([&]() -> JSValue { auto v10=self;auto v11=JSValue("map");auto v12=JSValue(nullptr);v10.set(v11,v12); return v12; }());
([&]() -> JSValue { auto v13=self;auto v14=JSValue("alphaMap");auto v15=JSValue(nullptr);v13.set(v14,v15); return v15; }());
([&]() -> JSValue { auto v16=self;auto v17=JSValue("displacementMap");auto v18=JSValue(nullptr);v16.set(v17,v18); return v18; }());
([&]() -> JSValue { auto v19=self;auto v20=JSValue("displacementScale");auto v21=JSValue(1);v19.set(v20,v21); return v21; }());
([&]() -> JSValue { auto v22=self;auto v23=JSValue("displacementBias");auto v24=JSValue(0);v22.set(v23,v24); return v24; }());
([&]() -> JSValue { auto v25=self;auto v26=JSValue("wireframe");auto v27=JSValue(false);v25.set(v26,v27); return v27; }());
([&]() -> JSValue { auto v28=self;auto v29=JSValue("wireframeLinewidth");auto v30=JSValue(1);v28.set(v29,v30); return v30; }());
([&]() -> JSValue { auto v32=self;auto v31=v32.get("setValues");auto v33=parameters; return v31.call(v32,{v33}); }());return self;}

JSValue copy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto source=arg(args,0);([&]() -> JSValue { auto v35=home.getPrototype();auto v34=v35.get("copy",self);auto v36=source; return v34.call(self,{v36}); }());
([&]() -> JSValue { auto v37=self;auto v38=JSValue("depthPacking");auto v39=([&]() -> JSValue { auto v40=source;auto v41=JSValue("depthPacking"); return v40.get(v41); }());v37.set(v38,v39); return v39; }());
([&]() -> JSValue { auto v42=self;auto v43=JSValue("map");auto v44=([&]() -> JSValue { auto v45=source;auto v46=JSValue("map"); return v45.get(v46); }());v42.set(v43,v44); return v44; }());
([&]() -> JSValue { auto v47=self;auto v48=JSValue("alphaMap");auto v49=([&]() -> JSValue { auto v50=source;auto v51=JSValue("alphaMap"); return v50.get(v51); }());v47.set(v48,v49); return v49; }());
([&]() -> JSValue { auto v52=self;auto v53=JSValue("displacementMap");auto v54=([&]() -> JSValue { auto v55=source;auto v56=JSValue("displacementMap"); return v55.get(v56); }());v52.set(v53,v54); return v54; }());
([&]() -> JSValue { auto v57=self;auto v58=JSValue("displacementScale");auto v59=([&]() -> JSValue { auto v60=source;auto v61=JSValue("displacementScale"); return v60.get(v61); }());v57.set(v58,v59); return v59; }());
([&]() -> JSValue { auto v62=self;auto v63=JSValue("displacementBias");auto v64=([&]() -> JSValue { auto v65=source;auto v66=JSValue("displacementBias"); return v65.get(v66); }());v62.set(v63,v64); return v64; }());
([&]() -> JSValue { auto v67=self;auto v68=JSValue("wireframe");auto v69=([&]() -> JSValue { auto v70=source;auto v71=JSValue("wireframe"); return v70.get(v71); }());v67.set(v68,v69); return v69; }());
([&]() -> JSValue { auto v72=self;auto v73=JSValue("wireframeLinewidth");auto v74=([&]() -> JSValue { auto v75=source;auto v76=JSValue("wireframeLinewidth"); return v75.get(v76); }());v72.set(v73,v74); return v74; }());
return self;return JSValue{};}
