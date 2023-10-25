// type vertex
#version 460 core
layout(location = 0) in vec3 aPos;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;

void main()
{
	gl_Position = uLightSpaceMatrix * uModel * vec4(aPos, 1.0);
}

// type fragment
#version 460 core

void main()
{
}
