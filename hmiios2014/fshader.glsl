#version 330 core
in vec4 pos;
out vec4 fragColor;
uniform int color_id;

void main() {
		if(color_id == 0)
			fragColor = vec4(.0, .6, .0,1.0);
		else if(color_id == 1)
			fragColor = vec4(.0, .5, .0,1.0);
		else if(color_id == 2)
			fragColor = vec4(.9, .1, .2,.7);
		else if(color_id == 3)
			fragColor =vec4(.9, .1, .2,.6);
		else if(color_id == 4)
			fragColor =vec4(.9, .1, .2,.6);
		else if(color_id == 5)
			fragColor =vec4(.1, .9, .2,.6);
		else if(color_id == 6)
			fragColor =vec4(0.35, 0.35, 0.35,.5);
		else if(color_id == 7)
			fragColor =vec4(.8, .4, .2,.6);
		else if(color_id == 8)
			fragColor =vec4(.0, .0, .0,1.);
		else if(color_id == 9)
			fragColor =vec4(.8, .1, .4,.7);
		else if(color_id == 10)
			fragColor =vec4(.1, .1, .1,.5);
		else if(color_id == 11)
			fragColor =vec4(.0, .0, .0,.0);
		else if(color_id == 12)
			fragColor =vec4(0.7, 0.0, 0.4,.4);
		else if(color_id == 13)
			fragColor =vec4(.0, .0, .0,.0);
		else if(color_id == 14)
			fragColor =vec4(.9, .9, .1,.1);
		else if(color_id == 15)
			fragColor =vec4(.0, .0, .0,.0);
		else if(color_id == 16)
			fragColor =vec4(.7, 0.7, .7,.3);
		else if(color_id == 17)
			fragColor =vec4(.2, .6, .1,1.);
		else if(color_id == 18)
			fragColor =vec4(.1, .1, .2,.1);
		else if(color_id == 19)
			fragColor =vec4(.2, .6, .1,1.);
		else if(color_id == 20)
			fragColor =vec4(.0, 1.0, .0,.6);
		else if(color_id == 21)
			fragColor = vec4(.2, .6, .1,1.);
		else if(color_id == 22)
			fragColor = vec4(.0, .6, .0,1.);    
		else
            fragColor = vec4(1.0);
}
