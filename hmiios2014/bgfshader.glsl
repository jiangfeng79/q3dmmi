#version 330 core

in vec2 uv;
out vec4 fragColor;

uniform vec2 resolution;
uniform vec2 mouse;
uniform vec2 mouseDelta;
uniform float time;
uniform int shader_id;

#define iTime time
#define iResolution resolution

#define EPS 0.001
#define MAX_STEPS 32
#define ITR 8
#define VEL 1.5
#define PI 3.14159265359

mat2 rot(float a){
	float s = sin(a);
	float c = cos(a);
	return mat2(c, s, -s, c);
}

float map(vec3 p){
	p += vec3(1.0, 1.0, iTime * 0.2);
	p.xy *= rot(iTime * 0.05);
	p.yz *= rot(iTime * 0.05);
	float s = 3.0;
	for(int i = 0; i < ITR; i++){
		p = mod(p - 1.0, 2.0) - 1.0;
		float r = 1.53 / dot(p, p);
		p *= r;
		s *= r;
	}
	return dot(abs(p), normalize(vec3(0.0, 1.0, 1.0))) / s;
}

float circle(vec2 center, vec2 position, float radius) {
	vec2 d = center - position;
	d.x = mod(d.x + 0.5, 1.0) - 0.5;
	d.y = mod(d.y + 0.5, 1.0) - 0.5;
	return clamp((radius - length(d)) * resolution.y, 0.0, 1.0);
}

// Fixed float pow(0.0) NaN state for NVIDIA
float rnd(int seed) {
	float fseed = max(float(seed), 0.0001);
	return fract(sin(1.34232 + pow(fseed, 1.014) * 89.72342433) * 328.2653653);
}

mat2 m7(float a) {
	float c = cos(a), s = sin(a);
	return mat2(c, -s, s, c);
}

float map7(vec3 p) {
	p.xz *= m7(time * 0.4);
	p.xy *= m7(time * 0.1);
	vec3 q = p * 2.0 + time;
	return length(p + vec3(sin(time * 0.7))) * log(length(p) + 1.0) + sin(q.x + sin(q.z + sin(q.y))) * 0.5 - 1.0;
}

void main() {
	vec2 fragCoord = gl_FragCoord.xy - mouseDelta.xy;

	switch(shader_id) {
		case 1: {
			const int complexity = 47;
			const float fluid_speed = 27.0;
			const float color_intensity = 0.8;
			vec2 p = (2.0 * fragCoord - resolution) / max(resolution.x, resolution.y);
			for(int i = 1; i < complexity; i++) {
				vec2 newp = p + time * 0.001;
				newp.x += 0.6 / float(i) * sin(float(i) * p.y + time / fluid_speed + 0.3 * float(i)) + 0.5;
				newp.y += 0.6 / float(i) * sin(float(i) * p.x + time / fluid_speed + 0.3 * float(i + 10)) - 0.5;
				p = newp;
			}
			vec3 col = vec3(
				color_intensity * sin(3.0 * p.x) + color_intensity,
				color_intensity * sin(3.0 * p.y) + color_intensity,
				color_intensity * sin(p.x + p.y) + color_intensity
			);
			fragColor = vec4(col, 1.0);
			break;
		}
		case 2: {
			vec3 rColor = vec3(0.6, 0.5, 10.0 * mouse.x);
			vec2 p = (fragCoord * 2.0 - resolution);
			p /= min(resolution.x, resolution.y);
			p *= 1.1;

			float m = cos(time * 0.1);
			float d = cos(VEL * time * 1.0 - p.x * 4.0 * m + 0.0 * 2.0 * PI / 3.0);
			float e = cos(-VEL * time * 1.1 - p.x * 4.0 * m / 4.0 + 1.0 * 2.0 * PI / 3.0);
			float f = cos(VEL * time * 1.2 - p.x * 4.0 * m / 2.0 + 2.0 * 2.0 * PI / 3.0);

			float r = 1.09 / abs(p.x + d);
			float g = 0.09 / abs(p.y + e);
			float b = 0.09 / abs(p.y + f);

			fragColor = vec4(rColor * vec3(r, g, b), 1.0);
			break;
		}
		case 3: {
			vec3 c = vec3(0.0);
			vec4 o = vec4(0.0);
			float t = iTime * 0.1;

			for(int i = 0; i < 16; ++i) {
				float fi = float(i) * 0.06;
				float d = fract(fi + 0.1 * t);
				o = vec4((fragCoord - iResolution.xy * 0.5) / iResolution.y * (1.0 - d), -fi, 0.0) * 28.0;

				for (int j = 0; j < 19; ++j) {
					float dotO = max(dot(o, o), 0.0001);
					vec4 val = abs((o / dotO) - vec4(1.0 - 0.03 * sin(t), 0.9, 0.1, 0.15 - 0.14 * cos(t * 1.3)));
					o = val.xzyw;
				}
				c += o.xyz * o.yzw * (d - d * d);
			}
			fragColor = vec4(c, 1.0);
			break;
		}
		case 4: {
			// Renamed inner 'uv' to 'st_uv' to avoid shadowing global interface 'in vec2 uv'
			vec2 st_uv = (2.0 * fragCoord - resolution.xy) / resolution.y;
			vec3 rd = normalize(vec3(st_uv, 1.0));
			vec3 p = vec3(0.0, 0.0, iTime * 0.05);
			float d = 0.0;
			float emission = 0.0;

			for(int i = 0; i < MAX_STEPS; i++) {
				d = map(p);
				p += rd * d;
				emission += exp(d * -0.4);
				if(d < EPS) break;
			}

			vec4 color = 0.02 * emission * vec4(sin(iTime), 1.0, sin(iTime), 1.0);
			fragColor = vec4(color.xyz, 1.0);
			break;
		}
		case 5: {
			vec2 position = (fragCoord / resolution.xy);
			position -= 0.5;
			position.x *= resolution.x / resolution.y;
			vec3 o = vec3(0.0);

			for (int i = 0; i < 300; i += 10) {
				vec2 center = vec2(rnd(i), rnd(i + 1)) + time * 0.1 * vec2(rnd(i + 2), rnd(i + 3));
				float radius = 0.03 + 0.25 * rnd(i + 4);
				vec3 color = vec3(rnd(i + 5), rnd(i + 6), rnd(i + 7));
				o += circle(center, position, radius) * 0.5 * color;
			}
			fragColor = vec4(o, 1.0);
			break;
		}
		case 6: {
			const int complexity = 3;
			const float fluid_speed = 108.0;
			const float color_intensity = 0.8;

			vec2 p = (2.0 * fragCoord - resolution) / max(resolution.x, resolution.y);
			for(int i = 1; i < complexity; i++) {
				vec2 newp = p + time * 0.1;
				newp.x += 0.6 / float(i) * sin(float(i) * p.y + time / fluid_speed + 0.3 * float(i)) + 0.5;
				newp.y += 0.6 / float(i) * sin(float(i) * p.x + time / fluid_speed + 0.3 * float(i + 10)) - 0.5;
				p = newp;
			}
			vec3 col = vec3(
				color_intensity * sin(3.0 * p.x) + color_intensity,
				color_intensity * sin(3.0 * p.y) + color_intensity,
				color_intensity * sin(p.x + p.y) + color_intensity
			);
			fragColor = vec4(col, 1.0);
			break;
		}
		case 7: {
			vec2 a = (fragCoord - (resolution.xy * 0.4)) / resolution.xy - vec2(0.9, 0.5);
			vec3 cl = vec3(0.0);
			float d = 2.5;

			for (int i = 0; i <= 5; i++) {
				vec3 p = vec3(0.0, 0.0, 4.0) + normalize(vec3(a, -1.0)) * d;
				float rz = map7(p);
				float f = clamp((rz - map7(p + 0.1)) * 0.5, -0.1, 1.0);
				vec3 l = vec3(0.1, 0.3, 0.4) + vec3(5.0, 2.5, 3.0) * f;
				cl = cl * l + smoothstep(2.5, 0.0, rz) * 0.6 * l;
				d += min(rz, 1.0);
			}

			fragColor = vec4(cl, 1.0);
			break;
		}
		default: {
			fragColor = vec4(0.0, 0.6, 0.0, 1.0);
			break;
		}
	}
}