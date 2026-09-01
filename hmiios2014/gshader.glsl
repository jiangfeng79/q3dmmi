#version 330 core
// Geometry shader that expands each line segment into a filled quad so lines
// can be rendered thicker than 1px. This is needed because the GL driver
// rejects glLineWidth() > 1.0 (see gl-driver-quirks), so the MRT layer
// (color_id 20) is thickened here in the shader instead of via glLineWidth.
//
// layout(any) in: the shader receives every primitive type drawn through this
// program. Line primitives are thickened; points and polygons (triangles) are
// passed straight through to the rasterizer unchanged.
layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

// No custom `in` is needed: the geometry stage reads gl_in[i].gl_Position
// (a built-in). The vertex shader's `pos` output is simply not consumed here.
uniform int color_id;
uniform vec2 resolution; // viewport size in pixels
uniform float time;

void main() {
	// Thicken the MRT layer (color_id 20), flight trails (color_id 27),
    // and bus route lines (color_id 28 & 29).
   
    float width = (color_id == 20) ? 1.2 : (color_id == 27) ? 0.3 : (color_id == 28 || color_id == 29) ? 0.4 : 0.6;

    // Segment endpoints in NDC.
    vec2 a = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    vec2 b = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
    vec2 dir = b - a;
    float len = length(dir);
    vec2 n = (len > 1e-6) ? normalize(vec2(-dir.y, dir.x)) : vec2(0.0);

/*
    float amp = 0.002;

    float wave =
        sin(time * 4.0 +
            length((a+b)*0.5) * 15.0);

    a += n * wave * amp;
    b -= n * wave * amp;
*/
    vec2 center = vec2(0.0);
    float dist = distance((a + b) * 0.5, center);
    float ripple = sin(dist * 30.0 - time * 10.0);
    float amp = ripple * 0.0005;
    a += n * amp;
    b += n * amp;

    // Perpendicular offset in NDC that corresponds to `width` pixels. The
    // viewport maps NDC [-1,1] to [0, resolution], so 1 NDC unit = resolution/2
    // pixels; hence 2*width/resolution NDC units == width pixels.
    vec2 offset = n * (2.0 * width / resolution);

    gl_Position = vec4((a + offset) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.z, gl_in[0].gl_Position.w);
    EmitVertex();
    gl_Position = vec4((a - offset) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.z, gl_in[0].gl_Position.w);
    EmitVertex();
    gl_Position = vec4((b + offset) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.z, gl_in[1].gl_Position.w);
    EmitVertex();
    gl_Position = vec4((b - offset) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.z, gl_in[1].gl_Position.w);
    EmitVertex();
    EndPrimitive();
    
}
