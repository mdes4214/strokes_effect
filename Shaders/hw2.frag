#version 440

in vec3 vert_color;

out vec4 outColor;

void main() {
	outColor = vec4((vec3(1.0f) - vert_color), 0.0f);
}