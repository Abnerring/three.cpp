// Generated static native statements; fixed source SHA256 728877095fd31a37efeb058e7656469e53f8f4ef9329e1d2784d91b2d80a286a

JSValue constructor(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto parameters=arg(args,0);JSValue{};
([&]() -> JSValue { auto v1=self;auto v2=JSValue("isShadowMaterial");auto v3=JSValue(true);v1.set(v2,v3); return v3; }());
([&]() -> JSValue { auto v4=self;auto v5=JSValue("type");auto v6=JSValue("ShadowMaterial");v4.set(v5,v6); return v6; }());
([&]() -> JSValue { auto v7=self;auto v8=JSValue("color");auto v9=([&]() -> JSValue { auto v10=imports.get("Color");auto v11=JSValue(0); return v10.construct({v11}); }());v7.set(v8,v9); return v9; }());
([&]() -> JSValue { auto v12=self;auto v13=JSValue("transparent");auto v14=JSValue(true);v12.set(v13,v14); return v14; }());
([&]() -> JSValue { auto v15=self;auto v16=JSValue("fog");auto v17=JSValue(true);v15.set(v16,v17); return v17; }());
([&]() -> JSValue { auto v19=self;auto v18=v19.get("setValues");auto v20=parameters; return v18.call(v19,{v20}); }());return self;}

JSValue copy(JSValue self,const Args& args,const JSValue& imports,const JSValue& home){auto source=arg(args,0);([&]() -> JSValue { auto v22=home.getPrototype();auto v21=v22.get("copy",self);auto v23=source; return v21.call(self,{v23}); }());
([&]() -> JSValue { auto v25=([&]() -> JSValue { auto v26=self;auto v27=JSValue("color"); return v26.get(v27); }());auto v24=v25.get("copy");auto v28=([&]() -> JSValue { auto v29=source;auto v30=JSValue("color"); return v29.get(v30); }()); return v24.call(v25,{v28}); }());
([&]() -> JSValue { auto v31=self;auto v32=JSValue("fog");auto v33=([&]() -> JSValue { auto v34=source;auto v35=JSValue("fog"); return v34.get(v35); }());v31.set(v32,v33); return v33; }());
return self;return JSValue{};}
