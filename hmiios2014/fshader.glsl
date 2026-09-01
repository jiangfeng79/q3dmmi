#version 330 core
in vec4 pos;
out vec4 fragColor;
uniform int color_id;

// Radius (in gl_PointCoord units, 0..1) used when rendering a point sprite
// as a circle instead of the default square. 0.5 gives a full circle that
// touches the sprite edges. Hardcoded (not a uniform) so it never depends on
// the uniform being set while the program is bound.
const float kCircleRadius = 0.5;

// Color lookup table, indexed by color_id (0..37). Replaces the old if/else
// ladder; GLSL 3.30 core allows dynamic indexing of constant arrays.
//
// color_id is set in TSDWindow.cpp. For map layers it is myLog2(l_layer->m_id),
// i.e. the bit index of the layer's DisplayMaskBits flag (COASTAL=1<<0 -> 0,
// PLACES=1<<2 -> 2, ... MAN_MADE=1<<22 -> 22, FLIGHTS=1<<25 -> 25,
// FLIGHT_TRAILS=1<<27 -> 27, BUS_ROUTES=1<<28 -> 28, BUS_TRACKS=1<<30 -> 30).
// A few ids are also set directly for the MRT station points
// (drawMRTStation) and the EBL range ring (drawEBL). The meaning of each
// entry is annotated below.
const vec4 kColorTable[38] = vec4[38](
	vec4(.0, .6, .0, 1.0),        // 0  COASTAL (1<<0)
	vec4(.0, .5, .0, 1.0),        // 1  (COASTAL_TEXT bit; unused as a fill color)
	vec4(.9, .1, .2, .7),         // 2  PLACES (1<<2)
	vec4(.9, .1, .2, .7),         // 3  NS MRT line / EBL range ring line
	vec4(.9, .1, .2, .6),         // 4  AMENITIES (1<<4)
	vec4(.0, .4, .0, .7),         // 5  EW MRT line / Expo & Changi stations
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
	vec4(.0, .6, .0, 1.0),        // 22 MAN_MADE (1<<22)
	vec4(.0, .0, .0, .0),         // 23 (MAN_MADE_TEXT bit; unused)
	vec4(.0, .0, .0, .0),         // 24 (MRT_POINT bit; unused)
	vec4(.0, .0, .0, .0),         // 25 (MRT_POINT_TEXT bit; unused)
	vec4(1.0, .65, 0.0, .92),     // 26 FLIGHTS (1<<25) - plane silhouettes
	vec4(.0, .0, .0, .0),         // 27 (FLIGHTS_TEXT bit; unused)
	vec4(1.0, .65, 0.0, .5),      // 28 FLIGHT_TRAILS (1<<27) - flight trails
	vec4(0.0, 0.85, 0.85, 0.9),   // 29 BUS_ROUTES (1<<28) - route lines
	vec4(0.75, 0.0, 0.0, 0.7),   // 30 BUS_ROUTES2 (1<<29) - route lines
	vec4(.0, .0, .0, .0),         // 31 (BUS_ROUTES_TEXT bit; unused)
	vec4(.15, 0.15, 0.8, 0.5),    // 32 BUS_STOPS (1<<31) - bus stop points
	vec4(1.0, 0.55, 0.0, 0.5),    // 33 BUS_STOPS_TEXT (1<<32) - bus stop points text
	vec4(1.0, .65, 0.0, .82),    // 34 BUS_TRACKS (1<<33) - bus vehicle symbol
	vec4(1.0, 0.55, 0.0, 0.5),    // 35 BUS_TRACKS_TEXT (1<<34) - bus vehicle symbol text
	vec4(0.12, 0.36, 0.48, 0.6),  // 36 BUS_TRACKS_WINDSHIELD (1<<35)
	vec4(0.80, 0.15, 0.15, 0.90)  // 37 BUS_STOPS2 (1<<36)
);

void main() {
	// Clamp to the valid range (out-of-range ids now map to entry 0/37
	// instead of the old white default; all real ids are in 0..30).
	fragColor = kColorTable[clamp(color_id, 0, 37)];

	// For color_id 5 (EW MRT line / Expo & Changi stations) the points are
	// drawn as GL_POINTS sprites, which are square by default. Discard every
	// fragment outside a circle centered in the sprite to round the square
	// into a circle. gl_PointCoord is (0,0) at the sprite's upper-left and
	// (1,1) at its lower-right, so the center is (0.5, 0.5).
	if (color_id == 2 || color_id == 3 || color_id == 4 || color_id == 5 || color_id == 7 || color_id == 9 || color_id == 19 || color_id == 21 || color_id == 32 || color_id == 37) {
		vec2 d = gl_PointCoord - vec2(0.5);
		if (dot(d, d) > kCircleRadius * kCircleRadius)
			discard;
	}
}

