#version 330

smooth in vec2 fragTC;		// Interpolated texture coordinates
smooth in vec2 islandTC;	
smooth in float W_Sum;
smooth in vec3 normal;
smooth in vec3 fragPos;			// object position in world coords
flat in unsigned int fragType;	// type to decide which object to draw

uniform sampler2D tex;			// Texture sampler 0 
uniform sampler2D islandTex;	// Texture sampler 1
uniform vec3 lightPos;			// light position 
uniform int lightMode;			// light mode

out vec4 outCol;	// Final pixel color

vec3 lightColor;

void main() {
	// lighting
	if (lightMode == 5) lightColor = vec3(0.976, 0.517, 0.015);
	else if (lightMode == 6) lightColor = vec3(1.f, 1.f, 1.f);
	// Ambient 
	float ambientStrength = 0.5f;
	vec3 ambient = lightColor * ambientStrength;
	// Diffuse
	vec3 lightDir = normalize(lightPos - fragPos);
	float diff = max(dot(normal, lightDir), 0.f);
	vec3 diffuse = diff * lightColor;

	vec3 lightOffset = ambient + diffuse;
	 
	// island
	if (fragType == uint(1)) {
		outCol = vec4((normal * 0.5 + 0.5) * lightOffset, 1.f);
	}

	// water
	if (fragType == uint(2)) {
		vec3 col = texture(tex, fragTC).rgb;
		outCol = vec4((col + W_Sum * 2.f) * lightOffset, 1.f);
	}
	
}

