#ifndef NORMALMAPPING_GLSL
#define NORMALMAPPING_GLSL

vec3 NormalMapRGB(vec3 rgb)
{
	return (rgb * 2.0) - 1.0;
}

#endif