#version 330 core
in vec4 pos;
out vec4 fragColor;
uniform int color_id;

// Color lookup table, indexed by color_id (0..22). Replaces the old if/else
// ladder; GLSL 3.30 core allows dynamic indexing of constant arrays.
const vec4 kColorTable[23] = vec4[23](
	vec4(.0, .6, .0, 1.0),    // 0
	vec4(.0, .5, .0, 1.0),    // 1
	vec4(.9, .1, .2, .7),     // 2
	vec4(.9, .1, .2, .6),     // 3
	vec4(.9, .1, .2, .6),     // 4
	vec4(.1, .9, .2, .6),     // 5
	vec4(0.35, 0.35, 0.35, .5), // 6
	vec4(.8, .4, .2, .6),     // 7
	vec4(.0, .0, .0, 1.0),    // 8
	vec4(.8, .1, .4, .7),     // 9
	vec4(.1, .1, .1, .5),     // 10
	vec4(.0, .0, .0, .0),     // 11
	vec4(0.7, 0.0, 0.4, .4),  // 12
	vec4(.0, .0, .0, .0),     // 13
	vec4(.9, .9, .1, .1),     // 14
	vec4(.0, .0, .0, .0),     // 15
	vec4(.7, 0.7, .7, .3),    // 16
	vec4(.2, .6, .1, 1.0),    // 17
	vec4(.1, .1, .2, .1),     // 18
	vec4(.2, .6, .1, 1.0),    // 19
	vec4(.0, 1.0, .0, .6),    // 20
	vec4(.2, .6, .1, 1.0),    // 21
	vec4(.0, .6, .0, 1.0)     // 22
);

void main() {
	// Clamp to the valid range (out-of-range ids now map to entry 0/22
	// instead of the old white default; all real ids are in 0..22).
	fragColor = kColorTable[clamp(color_id, 0, 22)];
}
