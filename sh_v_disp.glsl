#version 330

layout(location = 0) in vec3 pos;		// Position
layout(location = 1) in vec2 tc;		// Texture coordinates
layout(location = 2) in vec3 norm;
layout(location = 3) in unsigned int type;

smooth out vec2 fragTC;		// Interpolated texture coordinate
smooth out vec2 islandTC;
smooth out float W_Sum;
smooth out vec3 normal;
smooth out vec3 fragPos;
flat out unsigned int fragType;

uniform mat4 xform;			// Transformation matrix
uniform float time;
uniform mat4 model;
uniform int mode;
uniform int waveMode;

void main() {
	// island
	if (type == uint(1)) {
		gl_Position = xform * vec4(pos, 1.f);
		normal = norm;
		islandTC = tc;
	}

	// water
	if (type == uint(2)) { 

		// No wave mode
		if (mode == 1) 
		{
			gl_Position = xform * vec4(pos, 1.f);
			// interpolate output value
			fragTC = tc;
			W_Sum = 0;
			normal = vec3(normalize(xform * vec4(norm, 0.f)));
		} 
		// Wave mode
		else if (mode == 2)
		{

			// Sine summation formula
			float L_i = 30.f;
			float w_i = 2.f / L_i;
			float A_i = 0.01f;
			float S_i = 15.f;
			float alpha_i = S_i * w_i;
			vec2 D_i = vec2(2.f, 5.f);
			// calculate height deviatioin
			float W_i = A_i * sin(dot(D_i, pos.xz) * w_i + time * alpha_i);
			// binormal(bitangent): calculate 3rd elem partial derivative
			float B_i = A_i * w_i * D_i.x * cos(dot(D_i, pos.xz) * w_i + time * alpha_i);
			// tangent: calculate 3rd elem partial derivative
			float T_i = A_i * w_i * D_i.y * cos(dot(D_i, pos.xz) * w_i + time * alpha_i);

			float L_i1 = 50.f;
			float w_i1 = 2.f / L_i1;
			float A_i1 = 0.02f;
			float S_i1 = 20.f;
			float alpha_i1 = S_i1 * w_i1;
			vec2 D_i1 = vec2(-3.f, -2.f);
			float W_i1 = A_i1 * sin(dot(D_i1, pos.xz) * w_i1 + time * alpha_i1);
			float B_i1 = A_i1 * W_i1 * D_i1.x * cos(dot(D_i1, pos.xz) * w_i1 + time * alpha_i1);
			float T_i1 = A_i1 * w_i1 * D_i1.y * cos(dot(D_i1, pos.xz) * w_i1 + time * alpha_i1);

			float L_i2 = 40.f;
			float w_i2 = 2.f / L_i2;
			float A_i2 = 0.001f;
			float S_i2 = 20.f;
			float alpha_i2 = S_i2 * w_i2;
			vec2 D_i2 = vec2(5.f, -3.f);
			float W_i2 = A_i2 * sin(dot(D_i2, pos.xy) * w_i2 + time * alpha_i2);
			float B_i2 = A_i2 * W_i2 * D_i2.x * cos(dot(D_i2, pos.xz) * w_i2 + time * alpha_i2);
			float T_i2 = A_i2 * w_i2 * D_i2.y * cos(dot(D_i2, pos.xz) * w_i2 + time * alpha_i2);

			float L_i3 = 60.f;
			float w_i3 = 2.f / L_i3; 
			float A_i3 = 0.003f;
			float S_i3 = 10.f;
			float alpha_i3 = S_i3 * w_i3;
			vec2 D_i3 = vec2(-1.f, 4.f);
			float W_i3 = A_i3 * sin(dot(D_i3, pos.xy) * w_i3 + time * alpha_i3);
			float B_i3 = A_i3 * W_i3 * D_i3.x * cos(dot(D_i3, pos.xz) * w_i3 + time * alpha_i3);
			float T_i3 = A_i3 * w_i3 * D_i3.y * cos(dot(D_i3, pos.xz) * w_i3 + time * alpha_i3);

			// set peak
			float k = 2.5f;
			float L_i4 = 60.f;
			float w_i4 = 2.f / L_i4; 
			float A_i4 = 0.003f;
			float S_i4 = 30.f;
			float alpha_i4 = S_i4 * w_i4;
			vec2 D_i4 = vec2(-4.f, 5.f);
			float W_i4 = 2 * A_i4 * pow(0.5 * (sin(dot(D_i4, pos.xy) * w_i4 + time * alpha_i4) + 1), k);
			float B_i4 = (k * A_i4 * W_i4 * D_i4.x) * pow(0.5 * (sin(dot(D_i4, pos.xz) * w_i4 + time * alpha_i4) + 1), k-1) *
						cos(dot(D_i4, pos.xz) * w_i4 + time * alpha_i4);
			float T_i4 = (k * A_i4 * W_i4 * D_i4.y) * pow(0.5 * (sin(dot(D_i4, pos.xz) * w_i4 + time * alpha_i4) + 1), k-1) *
						cos(dot(D_i4, pos.xz) * w_i4 + time * alpha_i4);

			// sum z deviation direction
			float W = 0.2 * sin(W_i + W_i1 + W_i2 + W_i3);
			if (waveMode == 7) W += W_i4;
			// sum binormal 
			vec3 B = vec3(1.f, 0.f, B_i + B_i1 + B_i2 + B_i3 + B_i4);
			// sum tangent direction
			vec3 T = vec3(0.f, 1.f, T_i + T_i1 + T_i2 + T_i3 + T_i4);

			// calculate offset
			vec3 offset = vec3(0.f, W, 0.f);
			// Transform vertex position
			gl_Position = xform * vec4(pos + offset, 1.0);

			// Interpolate output
			fragTC = tc;
			W_Sum  = W;
			// calculate new normal
			normal = vec3(normalize(xform * vec4(vec3(-B.z, 1.f, -T.z), 0.f)));
		
		}

	}

	fragType = type;
	fragPos = vec3(model * vec4(pos, 1.f));
}
