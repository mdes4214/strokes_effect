#version 440

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_color;

out vec3 vert_color;

void main() {
	vert_color = in_color;

	gl_Position = vec4(in_pos, 1.0);
}