uniform vec2 resolution;
uniform vec2 mouse;
uniform vec2 mouseDelta;
uniform float time;
uniform int color_id;
uniform int shader_id;
/*uniform  vec4 colors[23] = {
	vec4(.2, .6, .1,1.) // land
	,vec4(.0, .5, .0,1.0)
	,vec4(.9, .1, .2,.7)
	,vec4(.9, .1, .2,.6) // EBL outline
	,vec4(.9, .1, .2,.6)
	,vec4(.1, .9, .2,.6)
	,vec4(0.25, 0.25, 0.25,.5)
	,vec4(.8, .4, .2,.6)
	,vec4(.0, .0, .0,1.)
	,vec4(.8, .1, .4,.7) 
	,vec4(.1, .1, .1,.1) // building
	,vec4(.0, .0, .0,.0) // 11 not used
	//,vec4(0.7, 0.5, 0.0,.5) // main road
	,vec4(0.7, 0.0, 0.4,.4) // main road
	,vec4(.0, .0, .0,.0) // 13 not used
	,vec4(.9, .9, .1,.1) // small road
	,vec4(.0, .0, .0,.0) // 15 not used
	,vec4(.7, 0.7, .7,.3) // express road ; EBL
	,vec4(.0, .0, .0,.0) // 17 not used
	,vec4(.1, .1, .2,.1) // runway
	,vec4(.0, .0, .0,.0) // 19 not used
	,vec4(.0, 1.0, .0,.6) // mrt
	,vec4(.8, .0, .0,.6) // 21 not used
	,vec4(.0, .6, .0,1.)
};*/

// shadertoy emulation
#define iTime time
#define iResolution resolution

// --------[ Original ShaderToy begins here ]---------- //
#define EPS 0.001
#define MAX_STEPS 32
#define ITR 8

mat2 rot(float a){
	float s=sin(a);
	float c=cos(a);
	return mat2(c,s,-s,c);
}

float scale;

float map(vec3 p){
	p+=vec3(1.0,1.0,iTime*0.2);
	p.xy*=rot(iTime*0.05);
	p.yz*=rot(iTime*0.05);
	float s=3.0;
	for(int i=0;i<ITR;i++){
		p=mod(p-1.0,2.0)-1.0;
		float r=1.53/dot(p,p);
		p*=r;
		s*=r;
	}
	scale=s;
	return dot(abs(p),normalize(vec3(0.0,1.0,1.0)))/s;
}

vec3 GetNormal(vec3 p){
	float d=map(p);
	vec2 e=vec2(EPS,0.0);

	vec3 n=d-vec3(
		map(p-e.xyy),
		map(p-e.yxy),
		map(p-e.yyx));
	return normalize(n);
}

float circle(vec2 center, vec2 position, float radius) {
	vec2 d = center - position;
	d.x = mod(d.x+0.5, 1.0)-0.5;
	d.y = mod(d.y+0.5, 1.0)-0.5;
	return clamp((radius - length(d)) * resolution.y, 0.0, 1.0);
}

float rnd(int seed) {
	return fract(sin(1.34232 + pow(float(seed), 1.014) * 89.72342433) * 328.2653653);
}

#define VEL 1.5
#define PI 3.14159265359


void main() {
		//gl_FragColor = colors[color_id];
		if(color_id == 0)
		{
			if(shader_id == 1)
			{
				const float Pi = 3.14159;
				const int   complexity      = 47;    // More points of color.
				const float mouse_factor    = 56.0;  // Makes it more/less jumpy.
				const float mouse_offset    = 0.0;   // Drives complexity in the amount of curls/cuves.  Zero is a single whirlpool.
				const float fluid_speed     = 27.0;  // Drives speed, higher number will make it slower.
				const float color_intensity = 0.8;
				vec2 p=(2.0*(gl_FragCoord.xy - mouseDelta.xy)-resolution)/max(resolution.x,resolution.y);
				for(int i=1;i<complexity;i++)
				{
					vec2 newp=p + time*0.001;
					newp.x+=0.6/float(i)*sin(float(i)*p.y+time/fluid_speed+0.3*float(i)) + 0.5; // + mouse.y/mouse_factor+mouse_offset;
					newp.y+=0.6/float(i)*sin(float(i)*p.x+time/fluid_speed+0.3*float(i+10)) - 0.5; // - mouse.x/mouse_factor+mouse_offset;
					p=newp;
				}
				vec3 col=vec3(color_intensity*sin(3.0*p.x)+color_intensity,color_intensity*sin(3.0*p.y)+color_intensity,color_intensity*sin(p.x+p.y)+color_intensity);
				gl_FragColor=vec4(col, 1);
				}
			else if(shader_id == 2)
			{
				float v = VEL * abs(cos(time/5.0)) * PI* 0.01;
				vec3 rColor = vec3(1.0, 0.3, 1.0);
				rColor = vec3(0.6, 0.5, 10.0 * mouse.x);
	
				vec2 p =((gl_FragCoord.xy - mouseDelta.xy) *2.0 -resolution);
				p /= min(resolution.x, resolution.y);
				p*=1.1;
	
				float m = cos(time*0.1);
	
				float d = cos(VEL*time*1.0 - p.x*4.0*m + 0.0*2.*PI/3.);
				float e = cos(-VEL*time*1.1 - p.x*4.0*m/4.0 + 1.0*2.*PI/3.);
				float f = cos(VEL*time*1.2 - p.x*4.0*m/2.0 + 2.0*2.*PI/3.);
	
				float r = 1.09/abs(p.x + d);
				float g = 0.09/abs(p.y + e);
				float b = 0.09/abs(p.y + f);
	
	
				vec3 destColor = rColor*vec3(r, g, b);
				gl_FragColor =vec4(destColor, 1.0);
			}
			else if(shader_id == 3)
			{
				vec3 c = vec3(0.);
				vec4 o = vec4(0.);
				float t = iTime*.1,d;
				for(float i=0.; i<1.; i+=.06)
				{
					d = fract(i+.1*t);
					o = vec4( (gl_FragCoord.xy - mouseDelta.xy -iResolution.xy*.5)/iResolution.y*(1.-d) ,-i,0)*28.;
    			for (int i=0 ; i<19;++i) o.xzyw = abs( o/dot(o,o) - vec4( 1.-.03*sin(t) , .9 , .1 , .15 -.14*cos(t*1.3)) );
						c+= o.xyz*o.yzw*(d-d*d);
					}
					o.rgb = c;
					gl_FragColor =vec4(o.xyz, 1.0);
			}
			else if(shader_id == 4)
			{
				vec2 uv=(2.0*(gl_FragCoord.xy- mouseDelta.xy)-resolution.xy)/resolution.y;
				vec3 col=vec3(0.0);
				vec3 rd=normalize(vec3(uv,1.0));
				vec3 p=vec3(0.0,0.0,iTime*0.05);
				float d;
				float emission=0.0;

				for(int i=0;i<MAX_STEPS;i++){
					d=map(p);
					p+=rd*d;
					//normal=GetNormal(p);
					emission+=exp(d*-0.4);
					if(d<EPS)break;
				}

				vec4 color=0.02*emission*vec4(sin(iTime),1.0,sin(iTime),1.0);
				gl_FragColor=vec4(color.xyz, 1.0);
			}
			else if(shader_id == 5)
			{
				vec2 position = ( (gl_FragCoord.xy - mouseDelta.xy) / resolution.xy );
				position -= 0.5;
				position.x *= resolution.x / resolution.y;
				vec3 o = vec3(0.0, 0.0, 0.0);
	
				for (int i=0; i<300; i+=10) {
					vec2 center = vec2(rnd(i+0), rnd(i+1)) + time * 0.1 * vec2(rnd(i+2), rnd(i+3));
					float radius = 0.03 + 0.25 * rnd(i+4);
					vec3 color = vec3(rnd(i+5), rnd(i+6), rnd(i+7));
					o += circle(center, position, radius) * 0.5 * color;
				}
				gl_FragColor = vec4(o.xyz, 1.0);
			}
			else if(shader_id == 6)
			{
				const int   complexity      = 3;    // More points of color.
				const float mouse_factor    = 56.0;  // Makes it more/less jumpy.
				const float mouse_offset    = 40.0;   // Drives complexity in the amount of curls/cuves.  Zero is a single whirlpool.
				const float fluid_speed     = 108.0;  // Drives speed, higher number will make it slower.
				const float color_intensity = 0.8;

				vec2 p=(2.0*(gl_FragCoord.xy - mouseDelta.xy)-resolution)/max(resolution.x,resolution.y);
				for(int i=1;i<complexity;i++)
				{
					vec2 newp=p + time*0.1;
					newp.x+=0.6/float(i)*sin(float(i)*p.y+time/fluid_speed+0.3*float(i)) + 0.5; // + mouse.y/mouse_factor+mouse_offset;
					newp.y+=0.6/float(i)*sin(float(i)*p.x+time/fluid_speed+0.3*float(i+10)) - 0.5; // - mouse.x/mouse_factor+mouse_offset;
					p=newp;
				}
				vec3 col=vec3(color_intensity*sin(3.0*p.x)+color_intensity,color_intensity*sin(3.0*p.y)+color_intensity,color_intensity*sin(p.x+p.y)+color_intensity);
				gl_FragColor=vec4(col, 1.0);
			}
			else
			{
				gl_FragColor =vec4(.0, .6, .0,1.);
			}
		}
		else if(color_id == 1)
			gl_FragColor =vec4(.0, .5, .0,1.0);
		else if(color_id == 2)
			gl_FragColor =vec4(.9, .1, .2,.7);
		else if(color_id == 3)
			gl_FragColor =vec4(.9, .1, .2,.6);
		else if(color_id == 4)
			gl_FragColor =vec4(.9, .1, .2,.6);
		else if(color_id == 5)
			gl_FragColor =vec4(.1, .9, .2,.6);
		else if(color_id == 6)
			gl_FragColor =vec4(0.25, 0.25, 0.25,.5);
		else if(color_id == 7)
			gl_FragColor =vec4(.8, .4, .2,.6);
		else if(color_id == 8)
			gl_FragColor =vec4(.0, .0, .0,1.);
		else if(color_id == 9)
			gl_FragColor =vec4(.8, .1, .4,.7);
		else if(color_id == 10)
			gl_FragColor =vec4(.1, .1, .1,.1);
		else if(color_id == 11)
			gl_FragColor =vec4(.0, .0, .0,.0);
		else if(color_id == 12)
			gl_FragColor =vec4(0.7, 0.0, 0.4,.4);
		else if(color_id == 13)
			gl_FragColor =vec4(.0, .0, .0,.0);
		else if(color_id == 14)
			gl_FragColor =vec4(.9, .9, .1,.1);
		else if(color_id == 15)
			gl_FragColor =vec4(.0, .0, .0,.0);
		else if(color_id == 16)
			gl_FragColor =vec4(.7, 0.7, .7,.3);
		else if(color_id == 17)
			gl_FragColor =vec4(.2, .6, .1,1.);
		else if(color_id == 18)
			gl_FragColor =vec4(.1, .1, .2,.1);
		else if(color_id == 19)
			gl_FragColor =vec4(.2, .6, .1,1.);
		else if(color_id == 20)
			gl_FragColor =vec4(.0, 1.0, .0,.6);
		else if(color_id == 21)
			gl_FragColor =vec4(.2, .6, .1,1.);
		else if(color_id == 22)
			gl_FragColor =vec4(.0, .6, .0,1.);	
}
