#include <iostream>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/type_precision.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_core_3_3.h"
#include <GL/freeglut.h>
#include "util.hpp"
#include "mesh.hpp"
#include "stb_image.h"
using namespace std;
using namespace glm;

struct vert {
	vec3 pos;
	vec2 tc;
	vec3 norm;
	int type;
};

// Constants
const int MENU_EXIT = 0;			// Exit application
const int NONE_WAVE_MODE = 1;		// Menu mode
const int SET_ON_WAVE_MODE = 2;		
const int SET_ON_TEXTURE = 3;
const int SET_OFF_TEXTURE = 4;
const int SET_SUNSET_MODE = 5;
const int SET_DAYTIME_MODE = 6;
const int ADD_PEAK_WAVE = 7;
const int CLOSE_PEAK_WAVE = 8;
const int CURRTEXTURE_NUMBER = 2;	// Texture array number in display shader

// Global state
GLint width, height;				// Window size
int texWidth, texHeight;			// Texture size
int gridWidth, gridHeight;			// grid for water simulation
int islandTexWidth, islandTexHeight;
vector<glm::u8vec4> initTexData;	// Initial texture data for water
unsigned char* oceanTexData;		// ocean texture data for water
GLuint prevTexture;					// Texture objects
GLuint currTextures[CURRTEXTURE_NUMBER];
GLuint islandTexture;	
GLuint gpgpuShader;					// Shader programs
GLuint dispShader;
GLuint fbo;							// Framebuffer object
GLuint uniXform;					// Shader location of xform mtx
GLuint model;						// model matrix location in display shader
GLuint uniLightPos;					// light position in display shader
GLuint timeLoc;						// time location in display shader
GLuint modeLoc;						// display mode location in display shader
GLuint lightModeLoc;				// display light mode location in display shader
GLuint waveModeLoc;					// display wave mode location
GLuint vao;							// (Water) Vertex array object
GLuint vbuf;						// Vertex buffer
GLuint ibuf;						// Index buffer
GLsizei vcount;						// Number of vertices
GLuint quadVao;						// Quad for gugpu shader texture
GLuint quadVbuf;
GLuint quadIbuf;
GLsizei quadVcount;
int displayMode;		// Display mode controlled by menu
int textureMode;
int lightMode;
int waveMode;
vector<int> indexs;		// geometry init data for water simulation
vector<vert> vertices;
Mesh* islandMesh;		
unsigned char* islandTexData;

// Camera state
vec3 camCoords;			// Spherical coordinates (theta, phi, radius) of the camera
bool camRot;			// Whether the camera is currently rotating
vec2 camOrigin;			// Original camera coordinates upon clicking
vec2 mouseOrigin;		// Original mouse coordinates upon clicking

// Time
float oldTime;
float deltaTime;

// Initialization functions
void initState();
void initGLUT(int* argc, char** argv);
void initOpenGL();
void initGeometry();
void initTextures();

// Callback functions
void display();
void reshape(GLint width, GLint height);
void keyRelease(unsigned char key, int x, int y);
void mouseBtn(int button, int state, int x, int y);
void mouseMove(int x, int y);
void idle();
void menu(int cmd);
void cleanup();


int main(int argc, char** argv) {
	try {
		// Initialize
		/*Instruction of the application*/
		cout << "------------------------------------ Instrucion ---------------------------------------" << endl;
		cout << "- Use mouse button to rotate the view | Use mouse middle button to zoom in and out." << endl;
		cout << "- Use sine summation to realize the ocean wave | Right click button to change mode." << endl;
		cout << "- Add simple lighting system and use depth differnce to render the wave." << endl;
		cout << "- Add peak wave | Add simple two light mode: daytime and sunset | Add texture." << endl;

		initState();
		initGLUT(&argc, argv);
		initOpenGL();
		initTextures();
		initGeometry();

	} catch (const exception& e) {
		// Handle any errors
		cerr << "Fatal error: " << e.what() << endl;
		cleanup();
		return -1;
	}

	// Execute main loop
	glutMainLoop();

	return 0;
}

void initState() {
	// Initialize global state
	width = 0;
	height = 0;
	texWidth = 512;
	texHeight = 512;
	islandTexWidth = 0;
	islandTexHeight = 0;
	gridWidth = 512;
	gridHeight = 512;
	oceanTexData = NULL;
	prevTexture = 0;
	islandTexture = 0;
	gpgpuShader = 0;
	dispShader = 0;
	fbo = 0;
	uniXform = 0;
	model = 0;
	uniLightPos = 0;
	modeLoc = 0;
	lightModeLoc = 0;
	waveModeLoc = 0;
	vao = 0;
	vbuf = 0;
	ibuf = 0;
	vcount = 0;
	islandTexData = NULL;
	quadVao = 0;
	quadVbuf = 0;
	quadIbuf = 0;
	quadVcount = 0;
	// Camera
	camCoords = vec3(0.0, 15.0, 1.0); 
	camRot = false;
	// menu variables
	displayMode = 2;
	textureMode = 3;
	lightMode = 6;
	waveMode = 8;

	// Force images to load vertically flipped
	// OpenGL expects pixel data to start at the lower-left corner
	stbi_set_flip_vertically_on_load(1);
}

void initGLUT(int* argc, char** argv) {
	// Set window and context settings
	width = 800; height = 600;
	glutInit(argc, argv);
	glutInitWindowSize(width, height);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	// Create the window
	glutCreateWindow("FreeGlut Window");

	// Create a menu
	glutCreateMenu(menu);
	glutAddMenuEntry("Exit", MENU_EXIT);
	glutAddMenuEntry("Set off wave", NONE_WAVE_MODE);
	glutAddMenuEntry("Set on wave", SET_ON_WAVE_MODE);
	glutAddMenuEntry("Set on texture", SET_ON_TEXTURE);
	glutAddMenuEntry("Set off texture", SET_OFF_TEXTURE);
	glutAddMenuEntry("Set sunset mode", SET_SUNSET_MODE);
	glutAddMenuEntry("Set daytime mode", SET_DAYTIME_MODE);
	glutAddMenuEntry("Add peak wave", ADD_PEAK_WAVE);
	glutAddMenuEntry("Close peak wave", CLOSE_PEAK_WAVE);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// GLUT callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardUpFunc(keyRelease);
	glutMouseFunc(mouseBtn);
	glutMotionFunc(mouseMove);
	glutIdleFunc(idle);
	glutCloseFunc(cleanup);
}

void initOpenGL() {
	// Set clear color and depth
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClearDepth(1.0f);
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);
	// Allow unpacking non-aligned pixel data
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// Enable transparent
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Compile and link display shader
	vector<GLuint> shaders;
	shaders.push_back(compileShader(GL_VERTEX_SHADER, "sh_v_disp.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "sh_f_disp.glsl"));
	dispShader = linkProgram(shaders);
	// Release shader sources
	for (auto s = shaders.begin(); s != shaders.end(); ++s)
		glDeleteShader(*s);
	shaders.clear();

	// Compile and link GPGPU shader
	shaders.push_back(compileShader(GL_VERTEX_SHADER, "sh_v_gpgpu.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "sh_f_gpgpu.glsl"));
	gpgpuShader = linkProgram(shaders);
	// Release shader sources
	for (auto s = shaders.begin(); s != shaders.end(); ++s)
		glDeleteShader(*s);
	shaders.clear();


	// Locate uniforms in display shader
	uniXform = glGetUniformLocation(dispShader, "xform");
	model = glGetUniformLocation(dispShader, "model");
	timeLoc = glGetUniformLocation(dispShader, "time");
	uniLightPos = glGetUniformLocation(dispShader, "lightPos");
	modeLoc = glGetUniformLocation(dispShader, "mode");
	lightModeLoc = glGetUniformLocation(dispShader, "lightMode");
	waveModeLoc = glGetUniformLocation(dispShader, "waveMode");


	// Bind texture image units
	GLuint uniTex = glGetUniformLocation(dispShader, "tex");
	GLuint uniTex1 = glGetUniformLocation(dispShader, "islandTex");
	glUseProgram(dispShader);
	glUniform1i(uniTex, 0);
	glUniform1i(uniTex1, 1);

	uniTex = glGetUniformLocation(gpgpuShader, "prevTex");
	glUseProgram(gpgpuShader);
	glUniform1i(uniTex, 0);


	glUseProgram(0);

	assert(glGetError() == GL_NO_ERROR);
}

void genGrid(int width, int height, GLuint type) {
	if (vertices.size() > 0) {
		vertices.clear();
	}
	if (indexs.size() > 0) {
		indexs.clear();
	}

	float du = 0, dv = 0;
	int offsetU = width / 2, offsetV = height / 2;
	if (width % 2 == 0) du = 0.5;
	if (height % 2 == 0) dv = 0.5;

	for (int v = 0; v < height; v++) {
		for (int u = 0; u < width; u++) {
			// Create vertex to draw grid
			int idx = u + v * width;
			vert v0 = {
			 glm::vec3((float)(u - offsetU + du), 0.f, -(float)(v - offsetV + dv)),
			 glm::vec2((float)u / (float)(width - 1), (float)v / (float)(height - 1)),
			 glm::vec3(0.f, 1.f, 0.f),
			 type
			};
			vertices.push_back(v0);
			/*cout << v0.pos.x << ", " << v0.pos.y << endl;
			cout << v0.tc.x << "- " << v0.tc.y << endl;*/

			// Create element idx to draw grid
			if (u != width - 1 && v != height - 1) {
				// When not to the grid edge index
				// Horizontal line index
				indexs.push_back(idx);
				indexs.push_back(idx + 1);
				indexs.push_back(idx + width + 1);
				// Vertical line index
				indexs.push_back(idx);
				indexs.push_back(idx + width);
				indexs.push_back(idx + width + 1);
			}
		}
	}

	vcount = indexs.size();
}

void initGeometry() {
	// Create a surface (quad) to draw the texture onto
	vector<vert> verts = {
		{ glm::vec3(-1.0f, -1.0f, 0.f), glm::vec2(0.0f, 0.0f) },
		{ glm::vec3( 1.0f, -1.0f, 0.f), glm::vec2(1.0f, 0.0f) },
		{ glm::vec3( 1.0f,  1.0f, 0.f), glm::vec2(1.0f, 1.0f) },
		{ glm::vec3(-1.0f,  1.0f, 0.f), glm::vec2(0.0f, 1.0f) },
	};
	// Vertex indices for triangles
	vector<GLuint> ids = {
		0, 1, 2,	// Triangle 1
		2, 3, 0		// Triangle 2
	};
	quadVcount = ids.size();

	// Create vertex array object
	glGenVertexArrays(1, &quadVao);
	glBindVertexArray(quadVao);

	// Create vertex buffer
	glGenBuffers(1, &quadVbuf);
	glBindBuffer(GL_ARRAY_BUFFER, quadVbuf);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vert), verts.data(), GL_DYNAMIC_DRAW);
	// Specify vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vert), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vert), (GLvoid*)sizeof(glm::vec3));
	// Create index buffer
	glGenBuffers(1, &quadIbuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIbuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ids.size() * sizeof(GLuint), ids.data(), GL_DYNAMIC_DRAW);


	// Create water object (type = 2)
	genGrid(gridWidth, gridHeight, 2);

	// Create vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create vertex buffer
	glGenBuffers(1, &vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vert), vertices.data(), GL_DYNAMIC_DRAW);
	// Specify vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vert), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vert), (GLvoid*)sizeof(glm::vec3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vert), (GLvoid*)(sizeof(glm::vec3) + sizeof(glm::vec2)));
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(vert), (GLvoid*)(sizeof(glm::vec3) + sizeof(glm::vec2) + sizeof(glm::vec3)));
	// Create index buffer
	glGenBuffers(1, &ibuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexs.size() * sizeof(GLuint), indexs.data(), GL_DYNAMIC_DRAW);


	// Cleanup state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void initTextures() {
	// Create ocean texture data
	glGenTextures(1, &prevTexture);
	glBindTexture(GL_TEXTURE_2D, prevTexture);
	if (textureMode == 3) {
		int ch_n0;
		oceanTexData = stbi_load("ocean1.jpg", &texWidth, &texHeight, &ch_n0, 4);
		if (!oceanTexData) {
			throw std::runtime_error("failed to load the ocean image");
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, oceanTexData);
	}
	else if (textureMode == 4) {
		// Create ocean texture data
		initTexData = vector<glm::u8vec4>(texWidth * texHeight, glm::u8vec4(6, 66, 115, 255));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, initTexData.data());
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenTextures(2, currTextures);
	glBindTexture(GL_TEXTURE_2D, currTextures[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glBindTexture(GL_TEXTURE_2D, currTextures[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, islandTexHeight, islandTexWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, islandTexData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);

	// Create framebuffer object (draw to currTexture)
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currTextures[0], 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	assert(glGetError() == GL_NO_ERROR);
}

// Mesh draw 
void drawMesh(Mesh* mesh, mat4 xform) {
	// Scale and center mesh using bounding box
	pair<vec3, vec3> meshBB = mesh->boundingBox();
	mat4 fixBB = scale(mat4(1.0f), vec3(1.0f / length(meshBB.second - meshBB.first)));
	fixBB = glm::translate(fixBB, -(meshBB.first + meshBB.second) / 2.0f);
	// Concatenate all transformations and upload to shader
	xform = xform * fixBB;
	glUniformMatrix4fv(uniXform, 1, GL_FALSE, value_ptr(xform));

	// Draw mesh
	mesh->draw();
}

void display() {
	try {
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(quadVao);

		// Pass 1: GPGPU output to texture =============================

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);		// Enable render-to-texture
		glViewport(0, 0, texWidth, texHeight);		// Reshape to texture size
		glUseProgram(gpgpuShader);


		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, prevTexture);
		// Draw the quad to invoke the shader
		glDrawElements(GL_TRIANGLES, quadVcount, GL_UNSIGNED_INT, NULL);


		// Pass 2: Display GPGPU output ===============================
		glBindVertexArray(vao);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);		// Restore default framebuffer (draw to window)
		glViewport(0, 0, width, height);			// Reshape to window size
		glUseProgram(dispShader);

		/* -----------------------------------------------------------------*/
		// Draw water (type = 2)
		// aspect ratio
		glm::mat4 xform(1.0f);
		float winAspect = (float)width / (float)height;

		// Set transformation matrix
		// Scale
		mat4 scal = scale(mat4(1.f), vec3(1.f / (float)gridWidth, 1.f, 1.f / (float)gridHeight));
		// Create perspective projection matrix
		mat4 proj = perspective(45.0f, winAspect, 0.1f, 100.0f);
		// Create view transformation matrix
		mat4 view = translate(mat4(1.0f), vec3(0.f, 0.f, -camCoords.z));
		mat4 rot = rotate(mat4(1.0f), radians(camCoords.y), vec3(1.0, 0.0, 0.0));
		rot = rotate(rot, radians(camCoords.x), vec3(0.0, 1.0, 0.0));
		xform = proj * view * rot * scal;

		// Send uniform variables to display shader
		// xform (mvp) matrix
		glUniformMatrix4fv(uniXform, 1, GL_FALSE, value_ptr(xform));
		// model matrix
		glUniformMatrix4fv(model, 1, GL_FALSE, value_ptr( view * scal));
		// light position
		float lightPos[3] = { 0.f, 1.0f, -camCoords.z - 0.5f };
		glUniform3fv(uniLightPos, 1, lightPos);
		// Send time to shader
		float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
		glUniform1f(timeLoc, t);
		// Set water display mode
		glUniform1i(modeLoc, displayMode);
		// Set ambient light mode
		glUniform1i(lightModeLoc, lightMode);
		// Set wave mode
		glUniform1i(waveModeLoc, waveMode);

		// Clear the window
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Draw the current texture
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, currTextures[0]);
		// Draw the quad as water
		glDrawElements(GL_TRIANGLES, vcount, GL_UNSIGNED_INT, NULL);

		/* -----------------------------------------------------------------*/
		// Draw the island object (type = 1)
		if (!islandMesh) islandMesh = new Mesh("models/island1.obj", 1);

		// Set the xform
		xform = mat4(1.f);
		scal = scale(mat4(1.f), vec3(1.0, 1.0, 1.0));
		mat4 trans = translate(mat4(1.f), vec3(0.f, 0.040f, 0.f));
		view = translate(mat4(1.0f), vec3(0.f, 0.f, -camCoords.z));
		// Create perspective projection matrix
		proj = perspective(45.0f, winAspect, 0.1f, 100.0f);
		// Create view transformation matrix
		rot = rotate(mat4(1.0f), radians(camCoords.y), vec3(1.0, 0.0, 0.0));
		rot = rotate(rot, radians(camCoords.x), vec3(0.0, 1.0, 0.0));
		xform = proj * view * rot * trans * scal;
		glUniformMatrix4fv(model, 1, GL_FALSE, value_ptr(view * trans * scal));
		// Set the texture
		glActiveTexture(GL_TEXTURE0 + 1);
		glBindTexture(GL_TEXTURE_2D, currTextures[1]);
		drawMesh(islandMesh, xform);


		// Revert state
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);
		glUseProgram(0);


		// Display the back buffer
		glutSwapBuffers();

	} catch (const exception& e) {
		cerr << "Fatal error: " << e.what() << endl;
		glutLeaveMainLoop();
	}
}

void reshape(GLint width, GLint height) {
	::width = width;
	::height = height;
	glViewport(0, 0, width, height);
}

void keyRelease(unsigned char key, int x, int y) {
	switch (key) {
	case 27:	// Escape key
		menu(MENU_EXIT);
		break;
	}
}

// Convert a position in screen space into texture space
glm::ivec2 mouseToTexCoord(int x, int y) {
	glm::vec3 mousePos(x, y, 1.0f);

	// Convert screen coordinates into clip space
	glm::mat3 screenToClip(1.0f);
	screenToClip[0][0] = 2.0f / width;
	screenToClip[1][1] = -2.0f / height;	// Flip y coordinate
	screenToClip[2][0] = -1.0f;
	screenToClip[2][1] = 1.0f;

	// Invert the aspect ratio correction (from display())
	float winAspect = (float)width / (float)height;
	float texAspect = (float)texWidth / (float)texHeight;
	glm::mat3 invAspect(1.0f);
	invAspect[0][0] = glm::max(1.0f, winAspect / texAspect);
	invAspect[1][1] = glm::max(1.0f, texAspect / winAspect);

	// Convert to texture coordinates
	glm::mat3 quadToTex(1.0f);
	quadToTex[0][0] = texWidth / 2.0f;
	quadToTex[1][1] = texHeight / 2.0f;
	quadToTex[2][0] = texWidth / 2.0f;
	quadToTex[2][1] = texHeight / 2.0f;

	// Get texture coordinate that was clicked on
	glm::ivec2 texPos = glm::ivec2(glm::floor(quadToTex * invAspect * screenToClip * mousePos));
	return texPos;
}

void mouseBtn(int button, int state, int x, int y) {
	if (state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
		// Activate rotation mode
		camRot = true;
		camOrigin = vec2(camCoords);
		mouseOrigin = vec2(x, y);
	}
	if (state == GLUT_UP && button == GLUT_LEFT_BUTTON) {
		// Deactivate rotation
		camRot = false;
	}
	if (button == 3) {
		camCoords.z = clamp(camCoords.z - 0.1f, 0.1f, 10.0f);
		glutPostRedisplay();
	}
	if (button == 4) {
		camCoords.z = clamp(camCoords.z + 0.1f, 0.1f, 10.0f);
		glutPostRedisplay();
	}
}

void mouseMove(int x, int y) {
	if (camRot) {
		// Convert mouse delta into degrees, add to rotation
		float rotScale = glm::min(width / 450.0f, height / 270.0f);
		vec2 mouseDelta = vec2(x, y) - mouseOrigin;
		vec2 newAngle = camOrigin + mouseDelta / rotScale;
		newAngle.y = clamp(newAngle.y, -90.0f, 90.0f);
		while (newAngle.x > 180.0f) newAngle.x -= 360.0f;
		while (newAngle.y < -180.0f) newAngle.y += 360.0f;
		if (length(newAngle - vec2(camCoords)) > FLT_EPSILON) {
			camCoords.x = newAngle.x;
			camCoords.y = newAngle.y;
			glutPostRedisplay();
		}
	}
}

void idle() {
	glutPostRedisplay();
}

void menu(int cmd) {
	switch (cmd) {
	case NONE_WAVE_MODE: 
		displayMode = NONE_WAVE_MODE;
		glutPostRedisplay();
		break;
	case SET_ON_WAVE_MODE:
		displayMode = SET_ON_WAVE_MODE;
		glutPostRedisplay();
		break;
	case SET_ON_TEXTURE:
		textureMode = SET_ON_TEXTURE;
		initTextures();
		glutPostRedisplay();
		break;
	case SET_OFF_TEXTURE:
		textureMode = SET_OFF_TEXTURE;
		initTextures();
		glutPostRedisplay();
		break;
	case SET_SUNSET_MODE:
		lightMode = SET_SUNSET_MODE;
		glutPostRedisplay();
		break;
	case SET_DAYTIME_MODE:
		lightMode = SET_DAYTIME_MODE;
		glutPostRedisplay();
		break;
	case ADD_PEAK_WAVE:
		waveMode = ADD_PEAK_WAVE;
		glutPostRedisplay();
		break;
	case CLOSE_PEAK_WAVE:
		waveMode = CLOSE_PEAK_WAVE;
		glutPostRedisplay();
		break;
	case MENU_EXIT:
		glutLeaveMainLoop();
		break;
	}
}

void cleanup() {
	// Release all resources
	if (prevTexture) { glDeleteTextures(1, &prevTexture); prevTexture = 0; }
	if (currTextures) { glDeleteTextures(1, currTextures); fill_n(currTextures, CURRTEXTURE_NUMBER, 0);
	}
	if (dispShader) { glDeleteProgram(dispShader); dispShader = 0; }
	if (gpgpuShader) { glDeleteProgram(gpgpuShader); gpgpuShader = 0; }
	uniXform = 0;
	model = 0;
	lightModeLoc = 0;
	waveModeLoc = 0;
	modeLoc = 0;
	if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
	if (vbuf) { glDeleteBuffers(1, &vbuf); vbuf = 0; }
	if (ibuf) { glDeleteBuffers(1, &ibuf); ibuf = 0; }
	vcount = 0;
	if (quadVao) { glDeleteVertexArrays(1, &quadVao); quadVao = 0; }
	if (quadVbuf) { glDeleteBuffers(1, &quadVbuf); quadVbuf = 0; }
	if (quadIbuf) { glDeleteBuffers(1, &quadIbuf); quadIbuf = 0; }
	quadVcount = 0;
	if (fbo) { glDeleteFramebuffers(1, &fbo); fbo = 0; }
	if (islandMesh) { delete islandMesh; islandMesh = NULL; }
}
