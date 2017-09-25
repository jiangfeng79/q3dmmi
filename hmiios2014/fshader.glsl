uniform vec2 resolution;
uniform vec2 mouse;
uniform float time;
uniform int color_id;
varying lowp vec4 col;
varying highp vec4 pos;

vec3 color = vec3(0.0);

#define LINES 3.0
#define BRIGHTNESS 0.9

uniform  vec4 colors[23] = {
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
};

const vec4 ORANGE = vec4(1.4, 0.8, 0.4,0.4);
const vec4 BLUE = vec4(0.5, 0.9, 1.3, .4);
const vec4 SOLID_GREEN = vec4(.0, .6, .0,1.);
const vec4 RED = vec4(1.8, 0.4, 0.3,.4);

const float Pi = 3.14159;

//uniform sampler2D backbuffer;

#define PI 3.14159


float random(vec2 p)
{
	// We need irrationals for pseudo randomness.
	// Most (all?) known transcendental numbers will (generally) work.
	const vec2 r = vec2(
		23.1406926327792690,  // e^pi (Gelfond's constant)
		54.21426902251); // 2^sqrt(2) (Gelfond–Schneider constant)
	return fract( cos( mod(123456789., 1e-7 + 256. * dot(p,r) ) ) );  
}

void main() {
	/*
	if(color_id == 0 || color_id == 22)
	{
		
		float c = 1.0;
		float inten = .05;
		float t;
		float d = distance((gl_FragCoord.yy/resolution.yy), vec2(0.5,0.5));
		float e = distance((gl_FragCoord.xy/resolution.xy), vec2(0.5,0.5));	
		vec4 texColor = vec4(0.0, 0.1, 0.15, 1.0);
		vec2 v_texCoord = gl_FragCoord.xy / resolution;	
		vec2 p =  v_texCoord * 8.0 - vec2(20.0);
		vec2 i = p;

		t = (time * 1.0)* (1.0 - (3.0 / float(0+1)));	
		i = p + vec2(cos(t - i.x) + sin(t + i.y),sin(t - i.y) + cos(t + i.x));			 
		c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten), p.y / (cos(i.y+t)/inten)));

		t = time * (1.0 - (3.0 / float(1+1)));	
		i = p + vec2(cos(t - i.x) + sin(t + i.y),sin(t - i.y) + cos(t + i.x));			 
		c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten), p.y / (cos(i.y+t)/inten)));

		t = time * (1.0 - (3.0 / float(2+1)));	
		i = p + vec2(cos(t - i.x) + sin(t + i.y),sin(t - i.y) + cos(t + i.x));			 
		c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten), p.y / (cos(i.y+t)/inten)));

		t = time * (1.0 - (3.0 / float(3+1)));	
		i = p + vec2(cos(t - i.x) + sin(t + i.y),sin(t - i.y) + cos(t + i.x));			 
		c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten), p.y / (cos(i.y+t)/inten)));

		c /= 4.0;
		c = 1.46 - sqrt(c);

		texColor.rgb *= (1.0 / (1.0 - (c + 0.05)));
		texColor *= smoothstep(0.5, 0.0, d);
		texColor *= smoothstep(0.6, 0.0, e);

		gl_FragColor = vec4(texColor.b, texColor.b, texColor.r, 1.0);
		
		
		
	float c = 1.0;
	float inten = .05;
	float t;
	float d = distance((gl_FragCoord.yy/resolution.yy), vec2(0.5,0.5))*0.5;
	float e = distance((gl_FragCoord.xy/resolution.xy), vec2(0.5,0.5))*0.5;	
	vec4 texColor = vec4(1.0, 0.15, 0.0, 1.0);
	vec2 v_texCoord = gl_FragCoord.xy / resolution;	
	vec2 p =  v_texCoord * 8.0 - vec2(20.0);
	vec2 i = p;
	
	t = (time * 1.0)* (1.0 - (3.0 / float(0.0+1.0)));
	i = p + vec2(cos(t - i.x) + sin(t + i.y),sin(t - i.y) + cos(t + i.x));			 
	c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten), p.y / (cos(i.y+t)/inten)));
	
	t = time * (1.0 - (3.0 / float(1+1)));	
	i = p + vec2(cos(t - i.x) + sin(t + i.y),sin(t - i.y) + cos(t + i.x));			 
	c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten), p.y / (cos(i.y+t)/inten)));
	
	t = time * (1.0 - (3.0 / float(2+1)));	
	i = p + vec2(cos(t - i.x) + sin(t + i.y),sin(t - i.y) + cos(t + i.x));			 
	c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten), p.y / (cos(i.y+t)/inten)));
	
	t = time * (1.0 - (3.0 / float(3+1)));	
	i = p + vec2(cos(t - i.x) + sin(t + i.y),sin(t - i.y) + cos(t + i.x));			 
	c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten), p.y / (cos(i.y+t)/inten)));
	
	c /= 4.0;
	c = 1.46 - sqrt(c);

	texColor.rgb *= (1.0 / (1.0 - (c + 0.05)));

	
	gl_FragColor = vec4(1.0-texColor.rgb, 1.0);
	*/
	
		//		
		//		
		//		
		//			//gl_FragColor =  SOLID_GREEN;
		//
		//			vec3 col = vec3(0., 0., 0.);
		//	const int itrCount = 8;
		//	for (int i = 0; i < itrCount; ++i)
		//	{
		//		float offset = float(i) / float(itrCount);
		//		float t = time + (offset * offset * 2.);
		//		
		//		vec2 pos=(gl_FragCoord.xy/resolution.xy);
		//		pos.y-=0.5;
		//		pos.y+=sin(pos.x*9.0+t)*0.2*sin(t*.8);
		//		float color=1.0-pow(abs(pos.y),0.2);
		//		float colora=pow(1.,0.2*abs(pos.y));
		//		
		//		float rColMod = ((offset * .5) + .5) * colora;
		//		float bColMod = (1. - (offset * .5) + .5) * colora;
		//		
		//		col += vec3(color * rColMod, color*bColMod, color*rColMod ) * (1. / float(itrCount));
		//	}
		//	col = clamp(col, 0., 1.);
		//	
		//	gl_FragColor=vec4(col.x, col.y, col.z ,1.0);
		//		
		//		//float x = sin(pos.x + time);
		//		//float y = cos(pos.y + time);
		//
		//		//vec2 seed = vec2(x,y);
		//		//float color = random(seed);
		//
		//		//gl_FragColor = vec4(.2, color*.35, .2, 1.0);
		//		/*
		//		vec2 p = (gl_FragCoord.xy - 0.5 * resolution) / min(resolution.x, resolution.y);
		//	vec2 t = vec2(gl_FragCoord.xy / resolution);
		//	
		//	vec3 c = vec3(0);
		//	
		//	for(int i = 0; i < 20; i++) {
		//		float t = 0.4 * PI * float(i) / 30.0 * time * 0.5;
		//		float x = cos(3.0*t);
		//		float y = sin(4.0*t);
		//		vec2 o = 0.40 * vec2(x, y);
		//		float r = fract(x);
		//		float g = 0.5 - r;
		//		c += 0.01 / (length(p-o)) * vec3(r, g, 0.9);
		//	}
		//	
		//	gl_FragColor = vec4(c, 1);
		//		*/
		//	}
		//	else if(color_id == 1)
		//		gl_FragColor = vec4(.0, .5, .0,1.0);
		//	else if(color_id == 2)
		//		gl_FragColor = vec4(.9, .1, .2,.7);
		//	else if(color_id == 4)
		//		gl_FragColor = vec4(.9, .1, .2,.6);
		//	else if(color_id == 6)
		//		gl_FragColor = vec4(0.25, 0.25, 0.25,.5);
		//	else if(color_id == 8)
		//	{
		//		float x = sin(pos.x + time);
		//		float y = cos(pos.y + time);
		//
		//		vec2 seed = vec2(x,y);
		//		float color = random(seed);
		//
		//		gl_FragColor = vec4(.0, .0, color*.55, 1.0);
		//		//gl_FragColor = vec4(.0, 0.0, .3,1.0);
		//	
		//	}
		//	else if(color_id == 10)
		//		gl_FragColor = vec4(0.0, 0.0, 0.0,.4);
		//	else if(color_id == 12)
		//		gl_FragColor = vec4(0.3, 0.0, 0.1,.2);
		//	else if(color_id == 14)
		//		gl_FragColor = vec4(.1, .1, .2,.1);
		//	else if(color_id == 16)
		//	{
		//
		//	
		//		gl_FragColor = vec4(.7, 0.0, 0.0,.2);
		//	}
		//	else if(color_id == 18)
		//		gl_FragColor = vec4(.1, .1, .2,.1);
		//	else if(color_id == 20)
		//		gl_FragColor = vec4(.0, .9, .0,.4);
		//	//else if(color_id == 22)
		////		gl_FragColor = SOLID_GREEN;
		//	else if(color_id == 3)
		//	{
		//		
		//		gl_FragColor = vec4(.9, .1, .2,.7);
		//	}
		//	else if(color_id == 5)
		//		gl_FragColor = vec4(.1, .9, .2,.7);
		//	else if(color_id == 7)
		//		gl_FragColor = vec4(.8, .4, .2,.7);
		//	else if(color_id == 9)
		//		gl_FragColor = vec4(.8, .1, .4,.7);
		//	else
		//		gl_FragColor = ORANGE;

		/*
		if(color_id == 0 || color_id == 22)
		{
		vec3 col = vec3(0., 0., 0.);
		const int itrCount = 8;
		for (int i = 0; i < itrCount; ++i)
		{+
		float offset = float(i) / float(itrCount);
		float t = time + (offset * offset * 2.);

		vec2 pos=(gl_FragCoord.xy/resolution.xy);
		pos.y-=0.5;
		pos.y+=sin(pos.x*9.0+t)*0.2*sin(t*.8);
		float color=1.0-pow(abs(pos.y),0.2);
		float colora=pow(1.,0.2*abs(pos.y));

		float rColMod = ((offset * .5) + .5) * colora;
		float bColMod = (1. - (offset * .5) + .5) * colora;

		col += vec3(color * rColMod, color*bColMod, color*rColMod ) * (1. / float(itrCount));
		}
		col = clamp(col, 0., 1.);

		gl_FragColor=vec4(col.x, col.y, col.z ,1.0);
		}
		*/
	//}
	//else
	if(color_id == 56)
	{
		float t = abs(sin(time*4)*.5)+0.3;
		//if(t<.2)
//			t += .2;
		gl_FragColor = vec4(.9, .1, .2, t);
		
	}
	else
	{
		gl_FragColor = colors[color_id];
	}
}
