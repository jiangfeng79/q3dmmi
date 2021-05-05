attribute highp vec4 posAttr;
//attribute lowp vec4 colAttr;
//varying lowp vec4 col;
varying lowp vec4 pos;
uniform lowp vec2 mouse;
uniform lowp vec2 mouseDelta;
uniform lowp vec2 resolution;
uniform float time;
uniform highp mat4 matrix;
void main() {
   //col = colAttr;
   pos = posAttr;
   gl_Position = matrix * posAttr;
}