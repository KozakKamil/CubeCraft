#version 330 core

// Input: position of the vortex (x,y,z) - slot 0 in VAO
layout (location = 0) in vec3 aPos;

// Input: color of the vortex (r,g,b) - slot 1 in VAO
layout (location = 1) in vec3 aColor;

//Output: color for shader
out vec3 vertexColor;

void main() {
	gl_Position = vec4(aPos, 1.0);
	vertexColor = aColor;
}