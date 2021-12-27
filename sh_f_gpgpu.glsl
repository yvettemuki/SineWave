#version 330

smooth in vec2 fragTC;		// Interpolated texture coordinates

uniform sampler2D prevTex;	// Texture sampler

out vec4 outCol;	// Final pixel color


// Use Gray-Scott reaction-diffusion model to update U and V,
// and U and V are stored in red and blue components of the pixel color, respectively.

void main() {
	// Get pixel location of this fragment
	ivec2 texelCoord = ivec2(fragTC * textureSize(prevTex, 0));

	vec4 addCol = vec4(0.f, 0.f, 1.f, 1.0f);
	outCol = texture(prevTex, fragTC).rgba;
}
