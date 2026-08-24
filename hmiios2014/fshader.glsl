#version 330 core
in vec4 pos;
out vec4 fragColor;
uniform int color_id;

// Radius (in gl_PointCoord units, 0..1) used when rendering a point sprite
// as a circle instead of the default square. 0.5 gives a full circle that
// touches the sprite edges. Hardcoded (not a uniform) so it never depends on
// the uniform being set while the program is bound.
const float kCircleRadius = 0.5;

// Color lookup table, indexed by color_id (0..22). Replaces the old if/else
// ladder; GLSL 3.30 core allows dynamic indexing of constant arrays.
//
// color_id is set in TSDWindow.cpp. For map layers it is myLog2(l_layer->m_id),
// i.e. the bit index of the layer's DisplayMaskBits flag (COASTAL=1<<0 -> 0,
// PLACES=1<<2 -> 2, ... MAN_MADE=1<<22 -> 22). A few ids are also set directly
// for the MRT station points (drawMRTStation) and the EBL range ring
// (drawEBL). The meaning of each entry is annotated below.
const vec4 kColorTable[23] = vec4[23](
	vec4(.0, .6, .0, 1.0),        // 0  COASTAL (1<<0)
	vec4(.0, .5, .0, 1.0),        // 1  (COASTAL_TEXT bit; unused as a fill color)
	vec4(.9, .1, .2, .7),         // 2  PLACES (1<<2)
	vec4(.9, .1, .2, .7),         // 3  NS MRT line / EBL range ring line
	vec4(.9, .1, .2, .6),         // 4  AMENITIES (1<<4)
	vec4(.1, .9, .2, .8),         // 5  EW MRT line / Expo & Changi stations
	vec4(0.35, 0.35, 0.35, .5),   // 6  LAND_USAGE (1<<6)
	vec4(.8, .4, .2, .7),         // 7  Circle & Marina Bay MRT stations
	vec4(.0, .0, .0, 1.0),        // 8  WATER_AREA (1<<8)
	vec4(.8, .1, .4, .7),         // 9  NE MRT line
	vec4(.1, .1, .1, .5),         // 10 BUILDING (1<<10)
	vec4(.0, .0, .0, .0),         // 11 (BUILDING_TEXT bit; unused)
	vec4(0.7, 0.0, 0.4, .4),      // 12 MAIN_ROADS (1<<12)
	vec4(.0, .0, .0, .0),         // 13 (MAIN_ROADS_TEXT bit; unused)
	vec4(.9, .9, .1, .1),         // 14 MINOR_ROADS (1<<14)
	vec4(.0, .0, .0, .0),         // 15 (MINOR_ROADS_TEXT bit; unused)
	vec4(.7, 0.7, .7, .3),        // 16 MOTOR_WAYS (1<<16) / EBL range ring fill
	vec4(.2, .6, .1, 1.0),        // 17 (MOTOR_WAYS_TEXT bit; unused)
	vec4(.1, .1, .2, .1),         // 18 AIR_WAYS (1<<18)
	vec4(.1, .1, .9, .7),         // 19 (AIR_WAYS_TEXT bit; used by downtown line color)
	vec4(.0, 1.0, .0, .6),        // 20 MRT (1<<20)
	vec4(0.24, 0.14, 0.08, 0.7),  // 21 (MRT_TEXT bit; used by thomason east coast line)
	vec4(.0, .6, .0, 1.0)         // 22 MAN_MADE (1<<22)
);

void main() {
	// Clamp to the valid range (out-of-range ids now map to entry 0/22
	// instead of the old white default; all real ids are in 0..22).
	fragColor = kColorTable[clamp(color_id, 0, 22)];

	// For color_id 5 (EW MRT line / Expo & Changi stations) the points are
	// drawn as GL_POINTS sprites, which are square by default. Discard every
	// fragment outside a circle centered in the sprite to round the square
	// into a circle. gl_PointCoord is (0,0) at the sprite's upper-left and
	// (1,1) at its lower-right, so the center is (0.5, 0.5).
	if (color_id == 2 || color_id == 3 || color_id == 4 || color_id == 5 || color_id == 7 || color_id == 9 || color_id == 19 || color_id == 21) {
		vec2 d = gl_PointCoord - vec2(0.5);
		if (dot(d, d) > kCircleRadius * kCircleRadius)
			discard;
	}
}
