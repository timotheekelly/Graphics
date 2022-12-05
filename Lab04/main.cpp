// Code source built upon code provided in lab04, I do not attempt to pass this code as my own work
// Models from https://www.cgtrader.com/



// Windows includes (For Time, IO, etc.)
#include <windows.h>
#include <mmsystem.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <math.h>
#include <vector> // STL dynamic memory.
#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

// OpenGL includes
#include <GL/glew.h>
#include <GL/freeglut.h>

// Assimp includes
#include <assimp/cimport.h> // scene importer
#include <assimp/scene.h> // collects data
#include <assimp/postprocess.h> // various extra operations

// Project includes
#include "maths_funcs.h"


// Buffers
GLuint VAO[4], VP_VBO[6], VN_VBO[6], VT_VBO[6], VTO[6];
// vertex positions
// vertex normals

// Camera
float camera_x = 0.0f;
float camera_y = 10.0f;
float camera_z = 15.0f;
float target_x = 0.0f;
float target_y = 3.0f;
float target_z = 0.0f;
vec3 camera_pos = vec3(0.0f, 0.0f, 0.0f);		// initial position of eye
vec3 camera_target = vec3(0.0f, 0.0f, 0.0f);	// initial position of target
vec3 up = vec3(0.0f, 1.0f, 0.0f);				// up vector

												// Mouse Movement
float mouse_x = 0.0f;
float mouse_y = 0.0f;
float old_x = 0.0f;
float old_y = 0.0f;
float frontX = 5.0f;
float frontY = 5.0f;
float frontZ = 5.0f;
float speed = 0.01f;

/*----------------------------------------------------------------------------
MESH TO LOAD
----------------------------------------------------------------------------*/
// this mesh is a dae file format but you should be able to use any other format too, obj is typically what is used
// put the mesh in your project directory, or provide a filepath for it here
#define SNOWMAN_NAME "snwmnnnSmooth.dae"
#define SLED_NAME "sled.dae"
#define GROUND_NAME "landscape.dae"
#define PRESENT_NAME "present.dae"

// Textures
const char* snowFloor = "winter.jpg";
const char* snowmanTexture = "snowman.jpg";
const char* sledTexture = "sled.jpg";

/*----------------------------------------------------------------------------
----------------------------------------------------------------------------*/

#pragma region SimpleTypes
typedef struct a
{
	size_t mPointCount = 0;
	std::vector<vec3> mVertices;
	std::vector<vec3> mNormals;
	std::vector<vec2> mTextureCoords;
} ModelData;
#pragma endregion SimpleTypes

using namespace std;
GLuint shaderProgramID;
GLuint groundShader;
GLuint loc1, loc2, loc3;

ModelData snowman, sled, ground, present;
int width = 800;
int height = 600;
GLfloat rotate_y = 0.0f;

// Animations
boolean startAnim = false;
boolean jump = false;
boolean towards = 1;
float sledDirection = 15.0f;
float sledVertical = 0.0f;
unsigned int snowmanRotateX = 0;
unsigned int snowmanRotateY = 180;



#pragma region MESH LOADING
/*----------------------------------------------------------------------------
MESH LOADING FUNCTION
----------------------------------------------------------------------------*/

ModelData load_mesh(const char* file_name) {
	ModelData modelData;

	/* Use assimp to read the model file, forcing it to be read as    */
	/* triangles. The second flag (aiProcess_PreTransformVertices) is */
	/* relevant if there are multiple meshes in the model file that   */
	/* are offset from the origin. This is pre-transform them so      */
	/* they're in the right position.                                 */
	const aiScene* scene = aiImportFile(
		file_name, 
		aiProcess_Triangulate | aiProcess_PreTransformVertices
	); 

	if (!scene) {
		fprintf(stderr, "ERROR: reading mesh %s\n", file_name);
		return modelData;
	}

	printf("  %i materials\n", scene->mNumMaterials);
	printf("  %i meshes\n", scene->mNumMeshes);
	printf("  %i textures\n", scene->mNumTextures);

	for (unsigned int m_i = 0; m_i < scene->mNumMeshes; m_i++) {
		const aiMesh* mesh = scene->mMeshes[m_i];
		printf("    %i vertices in mesh\n", mesh->mNumVertices);
		modelData.mPointCount += mesh->mNumVertices;
		for (unsigned int v_i = 0; v_i < mesh->mNumVertices; v_i++) {
			if (mesh->HasPositions()) {
				const aiVector3D* vp = &(mesh->mVertices[v_i]);
				modelData.mVertices.push_back(vec3(vp->x, vp->y, vp->z));
			}
			if (mesh->HasNormals()) {
				const aiVector3D* vn = &(mesh->mNormals[v_i]);
				modelData.mNormals.push_back(vec3(vn->x, vn->y, vn->z));
			}
			if (mesh->HasTextureCoords(0)) {
				const aiVector3D* vt = &(mesh->mTextureCoords[0][v_i]);
				modelData.mTextureCoords.push_back(vec2(vt->x, vt->y));
			}
			if (mesh->HasTangentsAndBitangents()) {
				/* You can extract tangents and bitangents here              */
				/* Note that you might need to make Assimp generate this     */
				/* data for you. Take a look at the flags that aiImportFile  */
				/* can take.                                                 */
			}
		}
	}

	aiReleaseImport(scene);
	return modelData;
}

#pragma endregion MESH LOADING

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
char* readShaderSource(const char* shaderFile) {
	FILE* fp;
	fopen_s(&fp, shaderFile, "rb");

	if (fp == NULL) { return NULL; }

	fseek(fp, 0L, SEEK_END);
	long size = ftell(fp);

	fseek(fp, 0L, SEEK_SET);
	char* buf = new char[size + 1];
	fread(buf, 1, size, fp);
	buf[size] = '\0';

	fclose(fp);

	return buf;
}


static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		std::cerr << "Error creating shader..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	const char* pShaderSource = readShaderSource(pShaderText);

	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderSource, NULL);
	// compile the shader and check for errors
	glCompileShader(ShaderObj);
	GLint success;
	// check for shader related errors using glGetShaderiv
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024] = { '\0' };
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		std::cerr << "Error compiling "
			<< (ShaderType == GL_VERTEX_SHADER ? "vertex" : "fragment")
			<< " shader program: " << InfoLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Attach the compiled shader object to the program object
	glAttachShader(ShaderProgram, ShaderObj);
}

GLuint CompileShaders(const char* vertexShader, const char* fragmentShader)
{
	GLuint shaderProgramID;
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
	shaderProgramID = glCreateProgram();
	if (shaderProgramID == 0) {
		std::cerr << "Error creating shader program..." << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// Create two shader objects, one for the vertex, and one for the fragment shader
	AddShader(shaderProgramID, vertexShader, GL_VERTEX_SHADER);
	AddShader(shaderProgramID, fragmentShader, GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = { '\0' };
	// After compiling all shader objects and attaching them to the program, we can finally link it
	glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
	glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
	glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
		std::cerr << "Press enter/return to exit..." << std::endl;
		std::cin.get();
		exit(1);
	}
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
	return shaderProgramID;
}
#pragma endregion SHADER_FUNCTIONS

void vbovaoStuff(ModelData model, const char* texture, int i) {
	int width, height, nrChannels;
	unsigned char* data;

	glGenTextures(1, &VTO[i]);
	glBindTexture(GL_TEXTURE_2D, VTO[i]);
	stbi_set_flip_vertically_on_load(1);

	// load and generate the texture
	data = stbi_load(texture, &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		std::cout << "Loaded texture" << std::endl;
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	glGenBuffers(1, &VP_VBO[i]);
	glBindBuffer(GL_ARRAY_BUFFER, VP_VBO[i]);
	glBufferData(GL_ARRAY_BUFFER, model.mPointCount * sizeof(vec3), &model.mVertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &VN_VBO[i]);
	glBindBuffer(GL_ARRAY_BUFFER, VN_VBO[i]);
	glBufferData(GL_ARRAY_BUFFER, model.mPointCount * sizeof(vec3), &model.mNormals[0], GL_STATIC_DRAW);

	glGenBuffers(1, &VT_VBO[i]);
	glBindBuffer(GL_ARRAY_BUFFER, VT_VBO[i]);
	glBufferData(GL_ARRAY_BUFFER, model.mPointCount * sizeof(vec2), &model.mTextureCoords[0], GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO[i]);
	glBindVertexArray(VAO[i]);

	glEnableVertexAttribArray(loc1);
	glBindBuffer(GL_ARRAY_BUFFER, VP_VBO[i]);
	glVertexAttribPointer(loc1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(loc2);
	glBindBuffer(GL_ARRAY_BUFFER, VN_VBO[i]);
	glVertexAttribPointer(loc2, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glEnableVertexAttribArray(loc3);
	glBindBuffer(GL_ARRAY_BUFFER, VT_VBO[i]);
	glVertexAttribPointer(loc3, 2, GL_FLOAT, GL_FALSE, 0, NULL);
}

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
void generateObjectBufferMesh() {
	/*----------------------------------------------------------------------------
	LOAD MESH HERE AND COPY INTO BUFFERS
	----------------------------------------------------------------------------*/

	//Note: you may get an error "vector subscript out of range" if you are using this code for a mesh that doesnt have positions and normals
	//Might be an idea to do a check for that before generating and binding the buffer.

	// load meshes
	snowman = load_mesh(SNOWMAN_NAME);
	sled = load_mesh(SLED_NAME);
	ground = load_mesh(GROUND_NAME);

	loc1 = glGetAttribLocation(shaderProgramID, "vertex_position");
	loc2 = glGetAttribLocation(shaderProgramID, "vertex_normal");
	loc3 = glGetAttribLocation(shaderProgramID, "vertex_texture");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Textures
	int width, height, nrChannels;
	unsigned char* data;


	// -- Snowman 1 --------------------------------------------
	vbovaoStuff(snowman, snowmanTexture, 0);

	// -- Sled ------------------------------------------------
	vbovaoStuff(sled, sledTexture, 1);

	// Ground -------------------------------------------------
	vbovaoStuff(ground, snowFloor, 2);
}
#pragma endregion VBO_FUNCTIONS


void display() {

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//camera
	mat4 view = identity_mat4();

	camera_target = vec3(target_x, target_y, target_z);
	camera_pos = vec3(camera_x, camera_y, camera_z);
	view = look_at(camera_pos, camera_target, up);
	mat4 persp_proj = perspective(45.0f, (float)width / (float)height, 0.1f, 1000.0f);

	// - Ground --------------------------------------------------------
	glUseProgram(shaderProgramID);
	glBindTexture(GL_TEXTURE_2D, VTO[2]);

	//Declare your uniform variables that will be used in your shader
	int matrix_location = glGetUniformLocation(shaderProgramID, "model");
	int view_mat_location = glGetUniformLocation(shaderProgramID, "view");
	int proj_mat_location = glGetUniformLocation(shaderProgramID, "proj");

	mat4 groundModel = identity_mat4();
	groundModel = translate(groundModel, vec3(0.0f, 0.0f, 0.0f));
	groundModel = scale(groundModel, vec3(10.0f, 10.0f, 10.0f));

	glBindVertexArray(VAO[2]);

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, groundModel.m);
	glDrawArrays(GL_TRIANGLES, 0, ground.mPointCount);

	// - SLED ----------------------------------------------------------
	glBindTexture(GL_TEXTURE_2D, VTO[1]);

	mat4 sledModel = identity_mat4();
	sledModel = translate(sledModel, vec3(0.0f, 0.0f, 0.0f));
	sledModel = rotate_y_deg(sledModel, sledDirection);
	sledModel = scale(sledModel, vec3(4.0f, 4.0f, 4.0f));
	sledModel = translate(sledModel, vec3(0.0f, 2.5f, 0.0f));

	glBindVertexArray(VAO[1]);

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, sledModel.m);
	glDrawArrays(GL_TRIANGLES, 0, sled.mPointCount);


	// -- SNOWMAN -------------------------------------------------------
	glBindTexture(GL_TEXTURE_2D, VTO[0]);

	mat4 snowmanModel = identity_mat4();
	snowmanModel = translate(snowmanModel, vec3(0.0f, 0.0f, 0.0f));
	snowmanModel = rotate_y_deg(snowmanModel, snowmanRotateY);
	snowmanModel = rotate_x_deg(snowmanModel, snowmanRotateX);
	snowmanModel = scale(snowmanModel, vec3(0.1f, 0.1f, 0.1f));
	snowmanModel = translate(snowmanModel, vec3(0.0f, 4.1f, 0.0f));

	// update uniforms & draw
	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowmanModel.m);
	glUniformMatrix4fv(proj_mat_location, 1, GL_FALSE, persp_proj.m);
	glUniformMatrix4fv(view_mat_location, 1, GL_FALSE, view.m);

	glBindVertexArray(VAO[0]);

	// Apply the root matrix to the child matrix
	snowmanModel = snowmanModel * sledModel;

	glUniformMatrix4fv(matrix_location, 1, GL_FALSE, snowmanModel.m);
	glDrawArrays(GL_TRIANGLES, 0, snowman.mPointCount);


	glutSwapBuffers();
}


void updateScene() {

	static DWORD last_time = 0;
	DWORD curr_time = timeGetTime();
	if (last_time == 0)
		last_time = curr_time;
	float delta = (curr_time - last_time) * 0.001f;
	last_time = curr_time;

	if (startAnim == true) {
		if (towards == 1) {
			sledDirection = sledDirection + 0.01f;
			if (sledDirection >= 30.0f)
			{
				towards = 0;
				cout << "towards: " << towards;
			}
		}
		else {
			sledDirection = sledDirection - 0.01f;
			if (sledDirection <= 0.0f)
			{
				towards = 1;
				cout << "away: " << towards;
			}
		}
	}

	// Draw the next frame
	glutPostRedisplay();
}

void init()
{
	// Set up the shaders
	//shaderProgramID = CompileShaders("./shaders/simpleVertexShader.txt", "./shaders/simpleFragmentShader.txt");
	//shaderProgramID = CompileShaders("./shaders/groundVertShader.txt", "./shaders/groundFragShader.txt");
	shaderProgramID = CompileShaders("./shaders/vertShaderPhong.txt", "./shaders/fragShaderPhong.txt");
	//groundShader = CompileShaders("./shaders/groundVertShader.txt", "./shaders/groundFragShader.txt");

	// load mesh into a vertex buffer array
	generateObjectBufferMesh();

}

// Placeholder code for the keypress
void keypress(unsigned char key, int x, int y) {
	
	if (key == 'w') {	// Move camera forwards
		// probably could have done this with vec3 so that will be the next improvement
		camera_x += frontX * speed;
		camera_z += frontZ * speed;

		target_x += frontX * speed;
		target_z += frontZ * speed;
		cout << "w" << camera_x;
	}
	if (key == 'a') {	// Move camera left
		vec3 left = cross(vec3(frontX, frontY, frontZ), up);
		camera_x -= left.v[0] * speed;
		camera_z -= left.v[2] * speed;

		target_x -= left.v[0] * speed;
		target_z -= left.v[2] * speed;
		cout << "a";
	}
	if (key == 's') {	// Move camera backwards
		camera_x -= frontX * speed;
		camera_z -= frontZ * speed;

		target_x -= frontX * speed;
		target_z -= frontZ * speed;
		cout << "s";
	}
	if (key == 'd') {	// Move camera left
		vec3 right = cross(vec3(frontX, frontY, frontZ), up);
		camera_x += right.v[0] * speed;
		camera_z += right.v[2] * speed;

		target_x += right.v[0] * speed;
		target_z += right.v[2] * speed;
		cout << "d";
	}
	if (key == 'x') {	// Move camera forwards
		startAnim = true;
		cout << "x" << camera_x;
	}
	if (key == 'k') {
		snowmanRotateY += 1;
		cout << "k" << snowmanRotateY;
	}
	if (key == 'l') {
		snowmanRotateY -= 1;
	}
	glutPostRedisplay();
}

void mouse(int x, int y) {
	// calc forward vector for camera movement
	frontX = target_x - camera_x;
	frontY = target_y - camera_y;
	frontZ = target_z - camera_z;
	// save old mouse vals
	old_x = mouse_x;
	old_y = mouse_y;
	mouse_x = x;
	mouse_y = y;
	// range check
	if (mouse_y < 100) {
		mouse_y = 100;
		glutWarpPointer(mouse_x, height / 2);
	}
	if (mouse_y > height - 100) {
		mouse_y = height - 100;
		glutWarpPointer(mouse_x, height / 2);
	}
	// movements
	if ((mouse_x - old_x) > 0)	// moved right
		target_x += 0.15f;
	else if ((mouse_x - old_x) < 0)	// moved left
		target_x -= 0.15f;
	if ((mouse_y - old_y) > 0)	// moved up
		target_y += 0.15f;
	else if ((mouse_y - old_y) < 0)	// moved down
		target_y -= 0.15f;
}


int main(int argc, char** argv) {

	// Set up the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(width, height);
	glutCreateWindow("Hello Triangle");

	// Tell glut where the display function is
	glutDisplayFunc(display);
	glutIdleFunc(updateScene);
	glutKeyboardFunc(keypress);
	glutPassiveMotionFunc(mouse);

	// A call to glewInit() must be done after glut is initialized!
	GLenum res = glewInit();
	// Check for any errors
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
	return 0;
}
