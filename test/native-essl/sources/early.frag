#version 300 es
precision highp float;precision highp int;
in highp float vVertex;flat in highp int vInstance;
in highp float vSourceZ;
layout(location=0)out highp vec4 color;
void main(){color=vec4(gl_FragCoord.xy,gl_FragCoord.z,float(vInstance)+vVertex/100.0+0.1*vSourceZ);}
