// Generated static native statements; fixed source SHA256 945728e5503683bbf73c21fb8aef57bc36aac1360ceb33699526092e160db0dc

JSValue constructor(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto parameters=arg(args,0);JSValue{};
([&]() -> JSValue { auto v1=self;auto v2=JSValue("isMeshDistanceMaterial");auto v3=JSValue(true);v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self;auto v5=JSValue("type");auto v6=JSValue("MeshDistanceMaterial");v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v7=self;auto v8=JSValue("map");auto v9=JSValue(nullptr);v7.set(v8,v9); return v9; }());
([&]() -> JSValue { auto v10=self;auto v11=JSValue("alphaMap");auto v12=JSValue(nullptr);v10.set(v11,v12); return v12; }());
([&]() -> JSValue { auto v13=self;auto v14=JSValue("displacementMap");auto v15=JSValue(nullptr);v13.set(v14,v15); return v15; }());
([&]() -> JSValue { auto v16=self;auto v17=JSValue("displacementScale");auto v18=JSValue(1);v16.set(v17,v18); return v18; }());
([&]() -> JSValue { auto v19=self;auto v20=JSValue("displacementBias");auto v21=JSValue(0);v19.set(v20,v21); return v21; }());
([&]() -> JSValue { auto v23=self;auto v22=v23.get("setValues");auto v24=parameters; return v22.call(v23,{v24}); }());return self;}

JSValue copy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto source=arg(args,0);([&]() -> JSValue { auto v26=home.getPrototype();auto v25=v26.get("copy",self);auto v27=source; return v25.call(self,{v27}); }());
([&]() -> JSValue { auto v28=self;auto v29=JSValue("map");auto v30=([&]() -> JSValue { auto v31=source;auto v32=JSValue("map"); return v31.get(v32); }());v28.set(v29,v30); return v30; }());
([&]() -> JSValue { auto v33=self;auto v34=JSValue("alphaMap");auto v35=([&]() -> JSValue { auto v36=source;auto v37=JSValue("alphaMap"); return v36.get(v37); }());v33.set(v34,v35); return v35; }());
([&]() -> JSValue { auto v38=self;auto v39=JSValue("displacementMap");auto v40=([&]() -> JSValue { auto v41=source;auto v42=JSValue("displacementMap"); return v41.get(v42); }());v38.set(v39,v40); return v40; }());
([&]() -> JSValue { auto v43=self;auto v44=JSValue("displacementScale");auto v45=([&]() -> JSValue { auto v46=source;auto v47=JSValue("displacementScale"); return v46.get(v47); }());v43.set(v44,v45); return v45; }());
([&]() -> JSValue { auto v48=self;auto v49=JSValue("displacementBias");auto v50=([&]() -> JSValue { auto v51=source;auto v52=JSValue("displacementBias"); return v51.get(v52); }());v48.set(v49,v50); return v50; }());
return self;return JSValue{};}
