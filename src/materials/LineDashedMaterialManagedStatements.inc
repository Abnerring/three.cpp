// Generated static native statements; fixed source SHA256 6771d9b2d760ccf7b9b4b0a57875d49a87a5125205f002d8d0a96853bfdd85c2

JSValue constructor(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto parameters=arg(args,0);JSValue{};
([&]() -> JSValue { auto v1=self;auto v2=JSValue("isLineDashedMaterial");auto v3=JSValue(true);v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self;auto v5=JSValue("type");auto v6=JSValue("LineDashedMaterial");v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v7=self;auto v8=JSValue("scale");auto v9=JSValue(1);v7.set(v8,v9); return v9; }());
([&]() -> JSValue { auto v10=self;auto v11=JSValue("dashSize");auto v12=JSValue(3);v10.set(v11,v12); return v12; }());
([&]() -> JSValue { auto v13=self;auto v14=JSValue("gapSize");auto v15=JSValue(1);v13.set(v14,v15); return v15; }());
([&]() -> JSValue { auto v17=self;auto v16=v17.get("setValues");auto v18=parameters; return v16.call(v17,{v18}); }());return self;}

JSValue copy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto source=arg(args,0);([&]() -> JSValue { auto v20=home.getPrototype();auto v19=v20.get("copy",self);auto v21=source; return v19.call(self,{v21}); }());
([&]() -> JSValue { auto v22=self;auto v23=JSValue("scale");auto v24=([&]() -> JSValue { auto v25=source;auto v26=JSValue("scale"); return v25.get(v26); }());v22.set(v23,v24); return v24; }());
([&]() -> JSValue { auto v27=self;auto v28=JSValue("dashSize");auto v29=([&]() -> JSValue { auto v30=source;auto v31=JSValue("dashSize"); return v30.get(v31); }());v27.set(v28,v29); return v29; }());
([&]() -> JSValue { auto v32=self;auto v33=JSValue("gapSize");auto v34=([&]() -> JSValue { auto v35=source;auto v36=JSValue("gapSize"); return v35.get(v36); }());v32.set(v33,v34); return v34; }());
return self;return JSValue{};}
