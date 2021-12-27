#version 330

layout(location = 0) in vec3 pos;		// Position
layout(location = 1) in vec2 tc;		// Texture coordinates

uniform mat4 xform;

smooth out vec2 fragTC;		// Interpolated texture coordinate

void main() {
	// Transform vertex position
	gl_Position = vec4(pos, 1.0);

	// Interpolate texture coordinates
	fragTC = tc;
}
