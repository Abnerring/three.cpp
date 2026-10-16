#version 300 es
precision highp float;precision highp int;
out highp float vVertex;flat out highp int vInstance;
out highp float vSourceZ;
void main(){vec2 p[3]=vec2[](vec2(-1.0,-1.0),vec2(3.0,-1.0),vec2(-1.0,3.0));gl_Position=vec4(p[gl_VertexID%3],0.2,1.0);vVertex=float(gl_VertexID);vInstance=gl_InstanceID;if(gl_InstanceID==0){gl_Position.z+=0.1;vSourceZ=gl_Position.z;return;}vSourceZ=gl_Position.z;}
