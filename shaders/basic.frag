#version 330 core

//Input: color from the vertex shader
in vec3 vertexColor;

//Output: the final color of the pixel
out vec4 FragColor;

void main() {
	FragColor = vec4(vertexColor, 1.0);
}