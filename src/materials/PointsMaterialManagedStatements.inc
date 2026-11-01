// Generated static native statements; fixed source SHA256 031ec51a3b975ef29cd1b2a89917de3089cc54e9610fbba2b2552c42a5d8baea

JSValue constructor(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto parameters=arg(args,0);JSValue{};
([&]() -> JSValue { auto v1=self;auto v2=JSValue("isPointsMaterial");auto v3=JSValue(true);v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self;auto v5=JSValue("type");auto v6=JSValue("PointsMaterial");v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v7=self;auto v8=JSValue("color");auto v9=([&]() -> JSValue { auto v10=imports.get("Color");auto v11=JSValue(16777215); return v10.construct({v11}); }());v7.set(v8,v9); return v9; }());
([&]() -> JSValue { auto v12=self;auto v13=JSValue("map");auto v14=JSValue(nullptr);v12.set(v13,v14); return v14; }());
([&]() -> JSValue { auto v15=self;auto v16=JSValue("alphaMap");auto v17=JSValue(nullptr);v15.set(v16,v17); return v17; }());
([&]() -> JSValue { auto v18=self;auto v19=JSValue("size");auto v20=JSValue(1);v18.set(v19,v20); return v20; }());
([&]() -> JSValue { auto v21=self;auto v22=JSValue("sizeAttenuation");auto v23=JSValue(true);v21.set(v22,v23); return v23; }());
([&]() -> JSValue { auto v24=self;auto v25=JSValue("fog");auto v26=JSValue(true);v24.set(v25,v26); return v26; }());
([&]() -> JSValue { auto v28=self;auto v27=v28.get("setValues");auto v29=parameters; return v27.call(v28,{v29}); }());return self;}

JSValue copy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto source=arg(args,0);([&]() -> JSValue { auto v31=home.getPrototype();auto v30=v31.get("copy",self);auto v32=source; return v30.call(self,{v32}); }());
([&]() -> JSValue { auto v34=([&]() -> JSValue { auto v35=self;auto v36=JSValue("color"); return v35.get(v36); }());auto v33=v34.get("copy");auto v37=([&]() -> JSValue { auto v38=source;auto v39=JSValue("color"); return v38.get(v39); }()); return v33.call(v34,{v37}); }());
([&]() -> JSValue { auto v40=self;auto v41=JSValue("map");auto v42=([&]() -> JSValue { auto v43=source;auto v44=JSValue("map"); return v43.get(v44); }());v40.set(v41,v42); return v42; }());
([&]() -> JSValue { auto v45=self;auto v46=JSValue("alphaMap");auto v47=([&]() -> JSValue { auto v48=source;auto v49=JSValue("alphaMap"); return v48.get(v49); }());v45.set(v46,v47); return v47; }());
([&]() -> JSValue { auto v50=self;auto v51=JSValue("size");auto v52=([&]() -> JSValue { auto v53=source;auto v54=JSValue("size"); return v53.get(v54); }());v50.set(v51,v52); return v52; }());
([&]() -> JSValue { auto v55=self;auto v56=JSValue("sizeAttenuation");auto v57=([&]() -> JSValue { auto v58=source;auto v59=JSValue("sizeAttenuation"); return v58.get(v59); }());v55.set(v56,v57); return v57; }());
([&]() -> JSValue { auto v60=self;auto v61=JSValue("fog");auto v62=([&]() -> JSValue { auto v63=source;auto v64=JSValue("fog"); return v63.get(v64); }());v60.set(v61,v62); return v62; }());
return self;return JSValue{};}
