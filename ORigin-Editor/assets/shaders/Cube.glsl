// OpenGL-Shader

// type vertex
#version 450 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in int a_EntityID;

layout(std140, binding = 0) uniform GlobalUBO
{
	mat4 ViewProjection;
	vec3 LightPosition;
	vec4 LightColor;
};

struct Vertex
{
	vec4 Color;
};

layout (location = 0) out Vertex Output;
layout (location = 1) out flat int v_EntityID;

void main()
{
	Output.Color = a_Color;
	v_EntityID = a_EntityID;

	gl_Position = ViewProjection * vec4(a_Position, 1.0);
}

// type fragment
#version 450 core
layout(location = 0) out vec4 color;
layout(location = 1) out int entColor;

layout(std140, binding = 0) uniform GlobalUBO
{
	mat4 ViewProjection;
	vec3 LightPosition;
	vec4 LightColor;
};

struct Vertex
{
	vec4 Color;
};

layout (location = 0) in Vertex Input;
layout (location = 1) in flat int v_EntityID;

void main()
{
	color = Input.Color;
	entColor = v_EntityID;
}