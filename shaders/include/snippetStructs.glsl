#ifndef SNIPPETSTRUCTS_GLSL
#define SNIPPETSTRUCTS_GLSL
/*Vertex struct format for Snippets*/
struct Vertex_t
{
	vec3 Pos;
	vec3 Normal;
	vec3 Tangent;
	vec3 Binormal;
	vec4 Color;
	vec2 UV[2];
};

/*Default Constructor*/
Vertex_t Vertex_Init()
{
	Vertex_t result;
	result.Pos = vec3(0.0f, 0.0f, 0.0f);
	result.Normal = vec3(0.0f, 1.0f, 0.0f);
	result.Tangent = vec3(0.0f, 0.0f, 1.0f);
	result.Binormal = vec3(1.0f, 0.0f, 0.0f);
	result.Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	result.UV0 = vec2(0.0f, 0.0f);
	result.UV1 = vec2(0.0f, 0.0f);
	return result;
}

/*Model struct format for Snippets*/
struct Model_t
{
	mat4 Transform;
	vec3 Pos;
	vec3 Up;
	vec3 Forward;
	vec3 Right;
	vec3 Scale;
	vec4 Custom;
};

/*Default Constructor*/
Model_t Model_Init()
{
	Model_t result;
	result.Transform = mat4(1.0);
	result.Pos = vec3(0.0f, 0.0f, 0.0f);
	result.Up = vec3(0.0f, 1.0f, 0.0f);
	result.Forward = vec3(0.0f, 0.0f, 1.0f);
	result.Right = vec3(1.0f, 0.0f, 0.0f);
	result.Scale = vec3(1.0f, 1.0f, 1.0f);
	result.Custom = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	return result;
}

/*Surface struct format for Snippets*/
struct Surface_t
{
	vec3 BaseColor;
	vec3 Emission;
	vec3 TangentNormal;
	vec3 Subsurface;
	float Reflectance;
	float Roughness;
	float Metal;
	float Opacity;
	float Refract;
	float Fade;
};

/*Default Constructor*/
Surface_t Surface_Init()
{
	Surface_t result;
	result.BaseColor = vec3(0.8f, 0.8f, 0.8f);
	result.Emission = vec3(0.0f, 0.0f, 0.0f);
	result.TangentNormal = vec3(0.0f, 0.0f, 1.0f);
	result.Subsurface = vec3(0.0f, 0.0f, 0.0f);
	result.Reflectance = 0.5f;
	result.Roughness = 0.5f;
	result.Metal = 0.0f;
	result.Opacity = 1.0f;
	result.Refract = 0.0f;
	result.Fade = 0.0f;
	return result;
}
#endif