uniform vec2 resolution;
uniform vec2 mouse;
uniform float time;
uniform int color_id;
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
void main() {
		//gl_FragColor = colors[color_id];
		if(color_id == 0)
			gl_FragColor =vec4(.2, .6, .1,1.);
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
