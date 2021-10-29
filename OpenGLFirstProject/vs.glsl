#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCord;
out vec3 ourColor;
out vec2 textureCoord;

uniform mat4 rotation;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
   gl_Position = projection*view*model*rotation*vec4(aPos.x, aPos.y, aPos.z, 1.0);
   ourColor=aColor;
   textureCoord=aTexCord;
}