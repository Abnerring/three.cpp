#version 300 es
precision highp float;precision highp int;
in highp float vVertex;flat in highp int vInstance;
layout(location=0)out highp vec4 color;
void main(){color=vec4(gl_FrontFacing?1.0:0.0,gl_FrontFacing?0.0:1.0,gl_FragCoord.z,1.0);}
