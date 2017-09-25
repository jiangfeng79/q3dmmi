attribute highp vec4 posAttr;
attribute lowp vec4 colAttr;
//attribute int colorIdAttr;
varying lowp vec4 col;
varying lowp vec4 pos;
uniform int color_id;
uniform lowp vec2 mouse;
uniform lowp vec2 resolution;
uniform float time;
uniform highp mat4 matrix;
void main() {
   col = colAttr;
	//color_id = colorIdAttr;
   pos = posAttr;
   gl_Position = matrix * posAttr;
}