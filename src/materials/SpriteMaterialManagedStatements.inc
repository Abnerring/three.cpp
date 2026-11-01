// Generated static native statements; fixed source SHA256 41acc8db71e30450b28cd21ac753af0042f471a0e7774ce9c22a2d62f1139baa

JSValue constructor(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto parameters=arg(args,0);JSValue{};
([&]() -> JSValue { auto v1=self;auto v2=JSValue("isSpriteMaterial");auto v3=JSValue(true);v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self;auto v5=JSValue("type");auto v6=JSValue("SpriteMaterial");v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v7=self;auto v8=JSValue("color");auto v9=([&]() -> JSValue { auto v10=imports.get("Color");auto v11=JSValue(16777215); return v10.construct({v11}); }());v7.set(v8,v9); return v9; }());
([&]() -> JSValue { auto v12=self;auto v13=JSValue("map");auto v14=JSValue(nullptr);v12.set(v13,v14); return v14; }());
([&]() -> JSValue { auto v15=self;auto v16=JSValue("alphaMap");auto v17=JSValue(nullptr);v15.set(v16,v17); return v17; }());
([&]() -> JSValue { auto v18=self;auto v19=JSValue("rotation");auto v20=JSValue(0);v18.set(v19,v20); return v20; }());
([&]() -> JSValue { auto v21=self;auto v22=JSValue("sizeAttenuation");auto v23=JSValue(true);v21.set(v22,v23); return v23; }());
([&]() -> JSValue { auto v24=self;auto v25=JSValue("transparent");auto v26=JSValue(true);v24.set(v25,v26); return v26; }());
([&]() -> JSValue { auto v27=self;auto v28=JSValue("fog");auto v29=JSValue(true);v27.set(v28,v29); return v29; }());
([&]() -> JSValue { auto v31=self;auto v30=v31.get("setValues");auto v32=parameters; return v30.call(v31,{v32}); }());return self;}

JSValue copy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto source=arg(args,0);([&]() -> JSValue { auto v34=home.getPrototype();auto v33=v34.get("copy",self);auto v35=source; return v33.call(self,{v35}); }());
([&]() -> JSValue { auto v37=([&]() -> JSValue { auto v38=self;auto v39=JSValue("color"); return v38.get(v39); }());auto v36=v37.get("copy");auto v40=([&]() -> JSValue { auto v41=source;auto v42=JSValue("color"); return v41.get(v42); }()); return v36.call(v37,{v40}); }());
([&]() -> JSValue { auto v43=self;auto v44=JSValue("map");auto v45=([&]() -> JSValue { auto v46=source;auto v47=JSValue("map"); return v46.get(v47); }());v43.set(v44,v45); return v45; }());
([&]() -> JSValue { auto v48=self;auto v49=JSValue("alphaMap");auto v50=([&]() -> JSValue { auto v51=source;auto v52=JSValue("alphaMap"); return v51.get(v52); }());v48.set(v49,v50); return v50; }());
([&]() -> JSValue { auto v53=self;auto v54=JSValue("rotation");auto v55=([&]() -> JSValue { auto v56=source;auto v57=JSValue("rotation"); return v56.get(v57); }());v53.set(v54,v55); return v55; }());
([&]() -> JSValue { auto v58=self;auto v59=JSValue("sizeAttenuation");auto v60=([&]() -> JSValue { auto v61=source;auto v62=JSValue("sizeAttenuation"); return v61.get(v62); }());v58.set(v59,v60); return v60; }());
([&]() -> JSValue { auto v63=self;auto v64=JSValue("fog");auto v65=([&]() -> JSValue { auto v66=source;auto v67=JSValue("fog"); return v66.get(v67); }());v63.set(v64,v65); return v65; }());
return self;return JSValue{};}
