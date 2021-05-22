// Shader that render ALREADY RENDERED SCENE(a texture) into companion window
#version 410 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 v2UVIn;
noperspective out vec2 v2UV;
void main()
{
	v2UV = v2UVIn;
	gl_Position = vec4(position, 1.0f);
}