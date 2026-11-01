export function fixtures(){return [
 ...['undefined','null','plain'].map(material=>({kind:'constructor',material})),
 ...['undefined',false,true,0,null,'flip'].map(flipY=>({kind:'geometry',flipY})),
 ...['normal','throw','reentrant','material-getter','material-change','render-getter-throw'].map(mode=>({kind:'render',mode})),
 ...['resolve','primitive','reject','init-getter-throw','init-call-throw','then-getter-throw','thenable','render-throw','render-getter-throw'].map(mode=>({kind:'async',mode})),
 {kind:'module-private-bindings'}
];}
