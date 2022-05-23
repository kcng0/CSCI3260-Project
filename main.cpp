#include "Dependencies/glew/glew.h"
#include "Dependencies/GLFW/glfw3.h"
#include "Dependencies/freeglut/freeglut.h"
#include "Dependencies/glm/glm.hpp"
#include "Dependencies/glm/gtc/matrix_transform.hpp"
#include "Dependencies/stb_image/stb_image.h"

#include "Shader.h"
#include "Texture.h"
#include "Camera.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
// struct for storing the obj file
struct Vertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 normal;
};

struct Model {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
};
Model catObj;
GLuint penguinVAO, penguinEBO, penguinVBO;
GLuint programID, Skybox_programID;
double mouse_x, mouse_y, c_mouse_x, c_mouse_y = 0.0f;
unsigned int cubemapTexture;
Model craftObj, earthObj, rockObj, spacecraftObj;
// texture for all objects
Texture texture[10];
unsigned int num_vertices = 0;
GLuint craftVAO, craftEBO, craftVBO;
GLuint catVAO, catEBO, catVBO;
GLuint earthVAO, earthEBO, earthVBO;
GLuint rockVAO, rockEBO, rockVBO;
GLuint spacecraftVAO, spacecraftEBO, spacecraftVBO;
GLuint skyboxVAO, skyboxEBO, skyboxVBO;
glm::mat4 eyePos;
Camera camera(glm::vec3(0.0f, 3.0f, 10.0f));

const float speed = 0.005f;
const float ratio = 1 / speed;
int counter0 = 4 * ratio;
int flag0 = 1;
int counter1 = -2 * ratio;
int flag1 = 1;
int counter2 = 3 * ratio;
int flag2 = 1;

float dx = 2.0f, dy = 0.05f, rotate = 0.0f;
float dbrightness = 0.8f; float pbrightness = 0.6f; float fbrightness = 0.0f; float sbrightness = 1.0f;
double znear, camera_dy, camera_dr;
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;
// mouse controller
struct MouseController {
	bool LEFT_BUTTON = false;
	double MOUSE_Clickx = 0.0, MOUSE_Clicky = 0.0;
	double MOUSE_X = 0.0, MOUSE_Y = 0.0;
};
MouseController mouseCtl;
const float threshold = 3.0f;
// screen setting
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;
float timer = (float)glutGet(GLUT_ELAPSED_TIME) * 0.001;
Shader shader, skybox_shader;

bool collision_detection(glm::vec3 vectorA, glm::vec3 vectorB) {
	if (distance(vectorA, vectorB) <= threshold)
		return true;
	else return false;
}
Model loadOBJ(const char* objPath)
{
	// function to load the obj file
	// Note: this simple function cannot load all obj files.

	struct V {
		// struct for identify if a vertex has showed up
		unsigned int index_position, index_uv, index_normal;
		bool operator == (const V& v) const {
			return index_position == v.index_position && index_uv == v.index_uv && index_normal == v.index_normal;
		}
		bool operator < (const V& v) const {
			return (index_position < v.index_position) ||
				(index_position == v.index_position && index_uv < v.index_uv) ||
				(index_position == v.index_position && index_uv == v.index_uv && index_normal < v.index_normal);
		}
	};

	std::vector<glm::vec3> temp_positions;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	std::map<V, unsigned int> temp_vertices;

	Model model;
	unsigned int num_vertices = 0;

	std::cout << "\nLoading OBJ file " << objPath << "..." << std::endl;

	std::ifstream file;
	file.open(objPath);

	// Check for Error
	if (file.fail()) {
		std::cerr << "Impossible to open the file! Do you use the right path? See Tutorial 6 for details" << std::endl;
		exit(1);
	}

	while (!file.eof()) {
		// process the object file
		char lineHeader[128];
		file >> lineHeader;

		if (strcmp(lineHeader, "v") == 0) {
			// geometric vertices
			glm::vec3 position;
			file >> position.x >> position.y >> position.z;
			temp_positions.push_back(position);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			// texture coordinates
			glm::vec2 uv;
			file >> uv.x >> uv.y;
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			// vertex normals
			glm::vec3 normal;
			file >> normal.x >> normal.y >> normal.z;
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			// Face elements
			V vertices[3];
			for (int i = 0; i < 3; i++) {
				char ch;
				file >> vertices[i].index_position >> ch >> vertices[i].index_uv >> ch >> vertices[i].index_normal;
			}

			// Check if there are more than three vertices in one face.
			std::string redundency;
			std::getline(file, redundency);
			if (redundency.length() >= 5) {
				std::cerr << "There may exist some errors while load the obj file. Error content: [" << redundency << " ]" << std::endl;
				std::cerr << "Please note that we only support the faces drawing with triangles. There are more than three vertices in one face." << std::endl;
				std::cerr << "Your obj file can't be read properly by our simple parser :-( Try exporting with other options." << std::endl;
				exit(1);
			}

			for (int i = 0; i < 3; i++) {
				if (temp_vertices.find(vertices[i]) == temp_vertices.end()) {
					// the vertex never shows before
					Vertex vertex;
					vertex.position = temp_positions[vertices[i].index_position - 1];
					vertex.uv = temp_uvs[vertices[i].index_uv - 1];
					vertex.normal = temp_normals[vertices[i].index_normal - 1];

					model.vertices.push_back(vertex);
					model.indices.push_back(num_vertices);
					temp_vertices[vertices[i]] = num_vertices;
					num_vertices += 1;
				}
				else {
					// reuse the existing vertex
					unsigned int index = temp_vertices[vertices[i]];
					model.indices.push_back(index);
				}
			} // for
		} // else if
		else {
			// it's not a vertex, texture coordinate, normal or face
			char stupidBuffer[1024];
			file.getline(stupidBuffer, 1024);
		}
	}
	file.close();

	std::cout << "There are " << num_vertices << " vertices in the obj file.\n" << std::endl;
	return model;
}
unsigned int loadCubeMap(std::vector<std::string> faces) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void sendcat(void) {
	catObj = loadOBJ("./CourseProjectMaterials/object/cat/cat.obj");
	texture[7].setupTexture("./CourseProjectMaterials/object/cat/cat_01.jpg");
	texture[8].setupTexture("./CourseProjectMaterials/object/cat/cat_02.jpg");
	glGenVertexArrays(1, &catVAO);
	glBindVertexArray(catVAO);  //first VAO

	// vertex buffer object


	glGenBuffers(1, &catVBO);
	glBindBuffer(GL_ARRAY_BUFFER, catVBO);
	glBufferData(GL_ARRAY_BUFFER, catObj.vertices.size() * sizeof(Vertex), &catObj.vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &catEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, catEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, catObj.indices.size() * sizeof(unsigned int), &catObj.indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void sendcraft() {
	craftObj = loadOBJ("./CourseProjectMaterials/object/craft.obj");
	texture[0].loadBMPtoTexture("./CourseProjectMaterials/texture/ringTexture.bmp");
	texture[1].loadBMPtoTexture("./CourseProjectMaterials/texture/red.bmp");
	glGenVertexArrays(1, &craftVAO);
	glBindVertexArray(craftVAO);  //first VAO

	// vertex buffer object

	glGenBuffers(1, &craftVBO);
	glBindBuffer(GL_ARRAY_BUFFER, craftVBO);
	glBufferData(GL_ARRAY_BUFFER, craftObj.vertices.size() * sizeof(Vertex), &craftObj.vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &craftEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, craftEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, craftObj.indices.size() * sizeof(unsigned int), &craftObj.indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void sendearth() {
	earthObj = loadOBJ("./CourseProjectMaterials/object/planet.obj");
	texture[2].loadBMPtoTexture("./CourseProjectMaterials/texture/earthTexture.bmp");
	texture[3].loadBMPtoTexture("./CourseProjectMaterials/texture/earthNormal.bmp");
	glGenVertexArrays(1, &earthVAO);
	glBindVertexArray(earthVAO);  //first VAO

	// vertex buffer object

	glGenBuffers(1, &earthVBO);
	glBindBuffer(GL_ARRAY_BUFFER, earthVBO);
	glBufferData(GL_ARRAY_BUFFER, earthObj.vertices.size() * sizeof(Vertex), &earthObj.vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &earthEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, earthEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, earthObj.indices.size() * sizeof(unsigned int), &earthObj.indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void sendrock() {
	rockObj = loadOBJ("./CourseProjectMaterials/object/rock.obj");
	texture[4].loadBMPtoTexture("./CourseProjectMaterials/texture/rockTexture.bmp");
	glGenVertexArrays(1, &rockVAO);
	glBindVertexArray(rockVAO);  //first VAO

	// vertex buffer object

	glGenBuffers(1, &rockVBO);
	glBindBuffer(GL_ARRAY_BUFFER, rockVBO);
	glBufferData(GL_ARRAY_BUFFER, rockObj.vertices.size() * sizeof(Vertex), &rockObj.vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &rockEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rockEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, rockObj.indices.size() * sizeof(unsigned int), &rockObj.indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void sendspacecraft() {
	spacecraftObj = loadOBJ("./CourseProjectMaterials/object/spacecraft.obj");
	texture[5].loadBMPtoTexture("./CourseProjectMaterials/texture/spacecraftTexture.bmp");
	texture[6].loadBMPtoTexture("./CourseProjectMaterials/texture/gold.bmp");
	glGenVertexArrays(1, &spacecraftVAO);
	glBindVertexArray(spacecraftVAO);  //first VAO

	// vertex buffer object

	glGenBuffers(1, &spacecraftVBO);
	glBindBuffer(GL_ARRAY_BUFFER, spacecraftVBO);
	glBufferData(GL_ARRAY_BUFFER, spacecraftObj.vertices.size() * sizeof(Vertex), &spacecraftObj.vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &spacecraftEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spacecraftEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, spacecraftObj.indices.size() * sizeof(unsigned int), &spacecraftObj.indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void sendskybox() {
	// Cubemap
	GLfloat skyboxVertices[] =
	{
	-100.0f,  100.0f, -100.0f,
	-100.0f, -100.0f, -100.0f,
	 100.0f, -100.0f, -100.0f,
	 100.0f, -100.0f, -100.0f,
	 100.0f,  100.0f, -100.0f,
	-100.0f,  100.0f, -100.0f,

	-100.0f, -100.0f,  100.0f,
	-100.0f, -100.0f, -100.0f,
	-100.0f,  100.0f, -100.0f,
	-100.0f,  100.0f, -100.0f,
	-100.0f,  100.0f,  100.0f,
	-100.0f, -100.0f,  100.0f,

	 100.0f, -100.0f, -100.0f,
	 100.0f, -100.0f,  100.0f,
	 100.0f,  100.0f,  100.0f,
	 100.0f,  100.0f,  100.0f,
	 100.0f,  100.0f, -100.0f,
	 100.0f, -100.0f, -100.0f,

	-100.0f, -100.0f,  100.0f,
	-100.0f,  100.0f,  100.0f,
	 100.0f,  100.0f,  100.0f,
	 100.0f,  100.0f,  100.0f,
	 100.0f, -100.0f,  100.0f,
	-100.0f, -100.0f,  100.0f,

	-100.0f,  100.0f, -100.0f,
	 100.0f,  100.0f, -100.0f,
	 100.0f,  100.0f,  100.0f,
	 100.0f,  100.0f,  100.0f,
	-100.0f,  100.0f,  100.0f,
	-100.0f,  100.0f, -100.0f,

	-100.0f, -100.0f, -100.0f,
	-100.0f, -100.0f,  100.0f,
	 100.0f, -100.0f, -100.0f,
	 100.0f, -100.0f, -100.0f,
	-100.0f, -100.0f,  100.0f,
	 100.0f, -100.0f,  100.0f

	};

	std::vector<std::string> skybox_faces;
	skybox_faces.push_back("./skyboxtextures/bottom.bmp");
	skybox_faces.push_back("./skyboxtextures/left.bmp");
	skybox_faces.push_back("./skyboxtextures/right.bmp");
	skybox_faces.push_back("./skyboxtextures/front.bmp");
	skybox_faces.push_back("./skyboxtextures/back.bmp");
	skybox_faces.push_back("./skyboxtextures/top.bmp");
	cubemapTexture = loadCubeMap(skybox_faces);
	// vertex buffer object
	glGenVertexArrays(1, &skyboxVAO);
	glBindVertexArray(skyboxVAO);  //first VAO

	glGenBuffers(1, &skyboxVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

	// vertex position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);


}




void get_OpenGL_info()
{
	// OpenGL information
	const GLubyte* name = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* glversion = glGetString(GL_VERSION);
	std::cout << "OpenGL company: " << name << std::endl;
	std::cout << "Renderer name: " << renderer << std::endl;
	std::cout << "OpenGL version: " << glversion << std::endl;
}

void sendDataToOpenGL()
{
	//TODO
	//Load objects and bind to VAO and VBO
	//Load textures
	sendcat();
	sendcraft();
	sendearth();
	sendrock();
	sendspacecraft();
	sendskybox();
}

unsigned int amount = 500;
glm::mat4* modelMatrices;
glm::mat4* goldMatrices;
void generate_rock() {

	modelMatrices = new glm::mat4[amount];
	srand(glfwGetTime()); // initialize random seed	
	float radius = 8.0;
	float offset = 0.5f;
	for (unsigned int i = 0; i < amount; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		// 1. translation: displace along circle with 'radius' in range [-offset, offset]
		float angle = (float)i / (float)amount * 360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;
		model = glm::translate(model, glm::vec3(x, y + 2, z));

		// 2. scale: scale between 0.05 and 0.25f
		float scale = (rand() % 2) / 100.0f + 0.05;
		model = glm::scale(model, glm::vec3(scale));

		// 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector

		// 4. now add to list of matrices
		modelMatrices[i] = model;
	}
}
glm::vec3 goldpos[3];
void generate_gold() {

	goldMatrices = new glm::mat4[3];
	srand(glfwGetTime()); // initialize random seed	
	float radius = 8.0;
	float offset = 0.5f;
	for (unsigned int i = 0; i < 3; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);
		// 1. translation: displace along circle with 'radius' in range [-offset, offset]
		float angle = (float)i / (float)3 * 360.0f;
		float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float y = displacement * 0.4f; // keep height of field smaller compared to width of x and z
		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;
		model = glm::translate(model, glm::vec3(x, y + 2, z));
		// 2. scale: scale between 0.05 and 0.25f
		float scale = (rand() % 2) / 100.0f + 0.05;
		model = glm::scale(model, glm::vec3(scale));

		// 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector

		// 4. now add to list of matrices
		goldMatrices[i] = model;
	}
}
void initializedGL(void) //run only once
{
	if (glewInit() != GLEW_OK) {
		std::cout << "GLEW not OK." << std::endl;
	}

	get_OpenGL_info();
	sendDataToOpenGL();

	//TODO: set up the camera parameters	
	//TODO: set up the vertex shader and fragment shader
	skybox_shader.setupShader("./SkyboxVS.glsl", "./SkyboxFS.glsl");
	shader.setupShader("./VertexShaderCode.glsl", "./FragmentShaderCode.glsl");
	shader.use();
	generate_rock();
	generate_gold();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	float currentFrame = glfwGetTime();
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}

void initializeviewMatrix(void) {
	glm::mat4 transform(1.0f);
	glm::mat4 rotate(1.0f);


	glm::mat4 viewMatrix = camera.GetViewMatrix();
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera.Zoom), (float)3 / (float)4, 0.1f, 20000.0f);

	eyePos = glm::inverse(camera.GetViewMatrix());
	shader.setMat4("projectionMatrix", projectionMatrix);
	shader.setMat4("viewMatrix", viewMatrix);
}

void lightsetup(void) {
	shader.use();
	double current_time = glfwGetTime();
	// directional light
	glm::vec3 dirltc = dbrightness * glm::vec3(0.5f, 0.5f, 0.5f);
	shader.setVec3("dirlt.intensity", dirltc);
	glm::vec3 dirltdir = glm::vec3(0.0f, -1.0f, 0.0f);
	shader.setVec3("dirlt.direction", dirltdir);
	glm::vec3 dirambient(1.0f, 0.99f, 0.85f);
	shader.setVec3("dirlt.ambient", dirambient);
	glm::vec3 dirdiffuse(0.3f, 0.3f, 0.3f);
	shader.setVec3("dirlt.diffuse", dirdiffuse);
	glm::vec3 dirspecular(0.4f, 0.4f, 0.4f);
	shader.setVec3("dirlt.specular", dirspecular);
	glm::vec3 lightPositionWorld(15.0f, 15.0f, -10.0f);
	shader.setVec3("lightPositionWorld", lightPositionWorld);

	// point light 1
	glm::vec3 ptltpos1(0.0f, 4.0f, -36.0f);
	glm::vec3 ptlt1_color = pbrightness * (glm::vec3(1.0f, 0.98f, 0.14f) + glm::vec3(-1.0f * sin(current_time), -0.98f * cos(current_time), 0.96f * sin(current_time)));
	shader.setVec3("ptlt[0].position", ptltpos1);
	shader.setVec3("ptlt[0].ambient", ptlt1_color);
	shader.setVec3("ptlt[0].diffuse", glm::vec3(0.5f, 0.5f, 0.5f) * pbrightness);
	shader.setVec3("ptlt[0].specular", glm::vec3(1.0f, 1.0f, 1.0f) * pbrightness);
	shader.setFloat("ptlt[0].constant", 0.5f);
	shader.setFloat("ptlt[0].linear", 0.25f);
	shader.setFloat("ptlt[0].quadratic", 0.10f);

	// point light 2
	glm::vec3 ptltpos2(0.0f, 4.0f, -43.0f);
	glm::vec3 ptlt2_color = pbrightness * (glm::vec3(0.0f, 0.16f, 1.0f) + glm::vec3(1.0f * sin(current_time), 0.84 * cos(current_time), -1.0 * sin(current_time)));
	shader.setVec3("ptlt[1].position", ptltpos2);
	shader.setVec3("ptlt[1].ambient", ptlt2_color);
	shader.setVec3("ptlt[1].diffuse", glm::vec3(0.9f, 0.9f, 0.9f) * pbrightness);
	shader.setVec3("ptlt[1].specular", glm::vec3(0.8f, 0.8f, 0.8f) * pbrightness);
	shader.setFloat("ptlt[1].constant", 0.5f);
	shader.setFloat("ptlt[1].linear", 0.25f);
	shader.setFloat("ptlt[1].quadratic", 0.10f);

	// point light 3
	glm::vec3 ptltpos3(7.0f, 4.0f, -40.0f);
	glm::vec3 ptlt3_color = pbrightness * (glm::vec3(1.0f, 0.49f, 0.0f) + glm::vec3(-1.0f * cos(current_time), 0.51 * sin(current_time), 1.0f * cos(current_time)));
	shader.setVec3("ptlt[2].position", ptltpos3);
	shader.setVec3("ptlt[2].ambient", ptlt3_color);
	shader.setVec3("ptlt[2].diffuse", glm::vec3(0.7f, 0.7f, 0.7f) * pbrightness);
	shader.setVec3("ptlt[2].specular", glm::vec3(0.15f, 0.15f, 0.15f) * pbrightness);
	shader.setFloat("ptlt[2].constant", 0.5f);
	shader.setFloat("ptlt[2].linear", 0.25f);
	shader.setFloat("ptlt[2].quadratic", 0.10f);

	// point light 4
	glm::vec3 ptltpos4(-7.0f, 4.0f, -40.0f);
	glm::vec3 ptlt4_color = pbrightness * (glm::vec3(1.0f, 0.13f, 0.13f) + glm::vec3(-1.0f * cos(current_time), 0.67f * sin(current_time), 0.76f * cos(current_time)));
	shader.setVec3("ptlt[3].position", ptltpos4);
	shader.setVec3("ptlt[3].ambient", ptlt4_color);
	shader.setVec3("ptlt[3].diffuse", glm::vec3(0.9f, 0.9f, 0.9f) * pbrightness);
	shader.setVec3("ptlt[3].specular", glm::vec3(0.9f, 0.9f, 0.9f) * pbrightness);
	shader.setFloat("ptlt[3].constant", 0.5f);
	shader.setFloat("ptlt[3].linear", 0.25f);
	shader.setFloat("ptlt[3].quadratic", 0.10f);

	shader.setVec3("eyePositionWorld", camera.Position);

	// lamp0
	shader.setVec3("stlt[0].position", glm::vec3(4.0f, 3.0f, 13.0f));
	shader.setVec3("stlt[0].direction", glm::vec3(0.0f, 0.0f, -1.0f));
	shader.setFloat("stlt[0].cutOff", glm::cos(glm::radians(12.0f)));
	shader.setFloat("stlt[0].outerCutOff", glm::cos(glm::radians(20.5f)));

	shader.setVec3("stlt[0].ambient", glm::vec3(1.0f, 1.0f, 1.0f) * sbrightness);
	shader.setVec3("stlt[0].diffuse", glm::vec3(0.8f, 0.8f, 0.8f) * sbrightness);
	shader.setVec3("stlt[0].specular", glm::vec3(1.0f, 1.0f, 1.0f) * sbrightness);
	shader.setFloat("stlt[0].constant", 0.5f);
	shader.setFloat("stlt[0].linear", 0.04f);
	shader.setFloat("stlt[0].quadratic", 0.016f);

	// lamp1
	shader.setVec3("stlt[1].position", 0.0f, 3.0f, 10.0f);
	shader.setVec3("stlt[1].direction", 0.0f, 0.0f, -1.0f);
	shader.setFloat("stlt[1].cutOff", glm::cos(glm::radians(12.0f)));
	shader.setFloat("stlt[1].outerCutOff", glm::cos(glm::radians(20.5f)));

	shader.setVec3("stlt[1].ambient", glm::vec3(0.97f, 0.11f, 1.0f) * sbrightness);
	shader.setVec3("stlt[1].diffuse", glm::vec3(0.8f, 0.8f, 0.8f) * sbrightness);
	shader.setVec3("stlt[1].specular", glm::vec3(1.0f, 1.0f, 1.0f) * sbrightness);
	shader.setFloat("stlt[1].constant", 1.0f);
	shader.setFloat("stlt[1].linear", 0.09f);
	shader.setFloat("stlt[1].quadratic", 0.032f);

	// lamp2
	shader.setVec3("stlt[2].position", 5.0f, 4.0f, -40.0);
	shader.setVec3("stlt[2].direction", -1.0f, 0.0f, 0.0f);
	shader.setFloat("stlt[2].cutOff", glm::cos(glm::radians(12.0f)));
	shader.setFloat("stlt[2].outerCutOff", glm::cos(glm::radians(20.5f)));

	shader.setVec3("stlt[2].ambient", glm::vec3(0.97f, 0.11f, 1.0f) * sbrightness);
	shader.setVec3("stlt[2].diffuse", glm::vec3(0.8f, 0.8f, 0.8f) * sbrightness);
	shader.setVec3("stlt[2].specular", glm::vec3(1.0f, 1.0f, 1.0f) * sbrightness);
	shader.setFloat("stlt[2].constant", 1.0f);
	shader.setFloat("stlt[2].linear", 0.09f);
	shader.setFloat("stlt[2].quadratic", 0.032f);

	// lamp 3
	shader.setVec3("stlt[3].position", -5.0f, 4.0f, -40.0);
	shader.setVec3("stlt[3].direction", 1.0f, 0.0f, 0.0f);
	shader.setFloat("stlt[3].cutOff", glm::cos(glm::radians(12.0f)));
	shader.setFloat("stlt[3].outerCutOff", glm::cos(glm::radians(20.5f)));

	shader.setVec3("stlt[3].ambient", glm::vec3(0.97f, 0.11f, 1.0f) * sbrightness);
	shader.setVec3("stlt[3].diffuse", glm::vec3(0.8f, 0.8f, 0.8f) * sbrightness);
	shader.setVec3("stlt[3].specular", glm::vec3(1.0f, 1.0f, 1.0f) * sbrightness);
	shader.setFloat("stlt[3].constant", 1.0f);
	shader.setFloat("stlt[3].linear", 0.09f);
	shader.setFloat("stlt[3].quadratic", 0.032f);

	// lamp 4
	shader.setVec3("stlt[4].position", -4.0f, 3.0f, 10.0f);
	shader.setVec3("stlt[4].direction", 0.0f, 0.0f, -1.0f);
	shader.setFloat("stlt[4].cutOff", glm::cos(glm::radians(12.0f)));
	shader.setFloat("stlt[4].outerCutOff", glm::cos(glm::radians(20.5f)));

	shader.setVec3("stlt[4].ambient", glm::vec3(0.97f, 0.11f, 1.0f) * sbrightness);
	shader.setVec3("stlt[4].diffuse", glm::vec3(0.8f, 0.8f, 0.8f) * sbrightness);
	shader.setVec3("stlt[4].specular", glm::vec3(1.0f, 1.0f, 1.0f) * sbrightness);
	shader.setFloat("stlt[4].constant", 1.0f);
	shader.setFloat("stlt[4].linear", 0.09f);
	shader.setFloat("stlt[4].quadratic", 0.032f);
}

unsigned int spacecraftTex = 1;
unsigned int craft0Tex = 1;
unsigned int craft1Tex = 1;
unsigned int craft2Tex = 1;
glm::vec3 ambient, diffuse, specular;
float shininess;
void drawspacecraft() {
	shader.use();
	GLint slot = 0;
	glm::mat4 View = glm::lookAt(glm::vec3(0, 8, 10), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	shader.setMat4("viewMatrix", View);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	if (spacecraftTex == 1)
		texture[5].bind(slot);
	else if (spacecraftTex == 2)
		texture[6].bind(slot);
	glUniform1i(TextureID, slot);
	// material coe
	shininess = 30.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(1.0f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.2f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setVec3("material.specular", specular);

	modelTransformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0F, 2.5f, 6.0F));
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindVertexArray(spacecraftVAO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spacecraftEBO);
	glDrawElements(GL_TRIANGLES, spacecraftObj.indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

int catTex0 = 1;
void drawcat0() {
	shader.use();
	GLint slot = 0;


	glBindVertexArray(catVAO);
	glm::mat4 viewMatrix = camera.GetViewMatrix();
	shader.setMat4("viewMatrix", viewMatrix);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	if (catTex0 == 1)
		texture[7].bind(slot);
	else if (catTex0 == 2)
		texture[8].bind(slot);
	glUniform1i(TextureID, slot);
	// material coe
	shininess = 10.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.1f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.8f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setVec3("material.specular", specular);

	modelTransformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 2.0f, -20.0f));

	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, catEBO);
	glDrawElements(GL_TRIANGLES, catObj.indices.size(), GL_UNSIGNED_INT, 0);
}

int catTex1 = 1;
void drawcat1() {
	shader.use();
	GLint slot = 0;


	glBindVertexArray(catVAO);
	glm::mat4 viewMatrix = camera.GetViewMatrix();
	shader.setMat4("viewMatrix", viewMatrix);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	if (catTex1 == 1)
		texture[7].bind(slot);
	else if (catTex1 == 2)
		texture[8].bind(slot);
	glUniform1i(TextureID, slot);
	// material coe
	shininess = 10.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.1f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.8f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setVec3("material.specular", specular);

	modelTransformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, 2.0f, -20.0f));

	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, catEBO);
	glDrawElements(GL_TRIANGLES, catObj.indices.size(), GL_UNSIGNED_INT, 0);
}

void drawcraft() {
	shader.use();
	//std::cout << (counter0)*speed << std::endl;
	GLint slot = 0;
	// adding viewMatrix don't move with camera
	glBindVertexArray(craftVAO);
	glm::mat4 viewMatrix = camera.GetViewMatrix();
	shader.setMat4("viewMatrix", viewMatrix);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	if (craft0Tex == 1)
		texture[0].bind(slot);
	else if (craft0Tex == 2)
		texture[1].bind(slot);
	glUniform1i(TextureID, slot);
	// material coe
	shininess = 10.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.5f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.2f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setVec3("material.specular", specular);
	if (flag0 == 1) {
		counter0--;
	}
	else counter0++;
	if (counter0 * speed > 4 || counter0 * speed < -4)
		flag0 *= -1;
	modelTransformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3((counter0 * speed), 1.0f, 0.0f));
	modelRotationMatrix = glm::rotate(modelTransformMatrix, glm::radians(rotate), glm::vec3(0.0f, 1.0f, 0.0f));
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, craftEBO);
	glDrawElements(GL_TRIANGLES, craftObj.indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}


void drawcraft1() {
	shader.use();
	GLint slot = 0;
	// adding viewMatrix don't move with camera

	glBindVertexArray(craftVAO);
	glm::mat4 viewMatrix = camera.GetViewMatrix();
	shader.setMat4("viewMatrix", viewMatrix);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	if (craft1Tex == 1)
		texture[0].bind(slot);
	else if (craft1Tex == 2)
		texture[1].bind(slot);
	glUniform1i(TextureID, slot);
	// material coe
	shininess = 10.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.5f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.2f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setVec3("material.specular", specular);


	if (flag1 == 1) {
		counter1++;
	}
	else counter1--;
	if (counter1 * speed > 4 || counter1 * speed < -4)
		flag1 *= -1;
	modelTransformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3((counter1)*speed, 1.0f, -5.0f));
	modelRotationMatrix = glm::rotate(modelTransformMatrix, glm::radians(rotate), glm::vec3(0.0f, 1.0f, 0.0f));
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, craftEBO);
	glDrawElements(GL_TRIANGLES, craftObj.indices.size(), GL_UNSIGNED_INT, 0);
}

void drawcraft2() {
	shader.use();
	GLint slot = 0;

	// adding viewMatrix don't move with camera

	glBindVertexArray(craftVAO);
	glm::mat4 viewMatrix = camera.GetViewMatrix();
	shader.setMat4("viewMatrix", viewMatrix);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	if (craft2Tex == 1)
		texture[0].bind(slot);
	else if (craft2Tex == 2)
		texture[1].bind(slot);
	glUniform1i(TextureID, slot);
	// material coe
	shininess = 10.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.5f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.2f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setVec3("material.specular", specular);
	if (flag2 == 1) {
		counter2++;
	}
	else counter2--;
	if (counter2 * speed > 4 || counter2 * speed < -4)
		flag2 *= -1;
	modelTransformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3((counter2)*speed, 1.0f, -10.0f));
	modelRotationMatrix = glm::rotate(modelTransformMatrix, glm::radians(rotate), glm::vec3(0.0f, 1.0f, 0.0f));
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, craftEBO);
	glDrawElements(GL_TRIANGLES, craftObj.indices.size(), GL_UNSIGNED_INT, 0);
}
bool a = true;
void drawearth() {
	shader.use();
	GLint slot = 0;
	// adding viewMatrix don't move with camera

	glBindVertexArray(earthVAO);
	glm::mat4 viewMatrix = camera.GetViewMatrix();
	shader.setMat4("viewMatrix", viewMatrix);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	shader.setBool("normalMap_flag", a);
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	GLint TextureID_1 = glGetUniformLocation(programID, "NM");
	texture[2].bind(0);
	glUniform1i(TextureID, 0);
	texture[3].bind(1);
	glUniform1i(TextureID_1, 1);
	// material coe
	shininess = 20.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(1.0f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.8f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setVec3("material.specular", specular);
	modelTransformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -13.5f));
	modelTransformMatrix = glm::scale(modelTransformMatrix, glm::vec3(2.0f, 2.0f, 2.0f));
	modelRotationMatrix = glm::rotate(modelTransformMatrix, glm::radians(rotate), glm::vec3(0.0f, -1.0f, 0.0f));
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, earthEBO);
	glDrawElements(GL_TRIANGLES, earthObj.indices.size(), GL_UNSIGNED_INT, 0);

}
void drawskybox() {
	glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
	skybox_shader.use();
	skybox_shader.setInt("skybox", 0);
	glm::mat4 view = camera.GetViewMatrix(); // remove translation from the view matrix
	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)3 / (float)4, 0.1f, 20.0f);
	skybox_shader.setMat4("view", view);
	skybox_shader.setMat4("projection", projection);
	// skybox cube
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // set depth function back to default
}
glm::mat4 rockModel;
glm::mat4 goldModel;
int goldTex[3] = { 1,1,1 };
glm::vec3 goldcopos[3];
void drawrock_gold() {
	float rotate2 = rotate * 0.1;
	glm::mat4 rockOrbitIni = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -40.0f));
	glm::mat4 rockOrbitIni_M = glm::rotate(rockOrbitIni, glm::radians(rotate2), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	// draw meteorites
	for (unsigned int i = 0; i < amount; i++)
	{
		shader.use();
		GLint slot = 0;
		glm::mat4 viewMatrix = camera.GetViewMatrix();
		shader.setMat4("viewMatrix", viewMatrix);
		GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
		glActiveTexture(GL_TEXTURE0);
		texture[4].bind(slot);
		glUniform1i(TextureID, slot);
		// material coe
		shininess = 20.0f;
		shader.setFloat("material.shininess", shininess);
		ambient = glm::vec3(0.4f);
		shader.setVec3("material.ambient", ambient);
		diffuse = glm::vec3(0.8f);
		shader.setVec3("material.diffuse", diffuse);
		specular = glm::vec3(3.0f);
		shader.setVec3("material.specular", specular);
		rockModel = modelMatrices[i];
		rockModel = rockOrbitIni_M * rockModel;
		modelRotationMatrix = glm::rotate(modelRotationMatrix, glm::radians(rotate * 0.1f), glm::vec3(0.4f, 0.6f, 0.8f));
		shader.setMat4("modelTransformMatrix", rockModel);
		shader.setMat4("modelRotationMatrix", modelRotationMatrix);
		glBindVertexArray(rockVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rockEBO);
		glDrawElements(GL_TRIANGLES, rockObj.indices.size(), GL_UNSIGNED_INT, 0);

	}

	for (unsigned int i = 0; i < 3; i++)
	{
		shader.use();
		GLint slot = 0;
		glm::mat4 viewMatrix = camera.GetViewMatrix();
		shader.setMat4("viewMatrix", viewMatrix);
		GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
		glActiveTexture(GL_TEXTURE0);
		if (goldTex[i] == 1)
			texture[6].bind(slot);
		else if (goldTex[i] == 2)
			texture[4].bind(slot);
		glUniform1i(TextureID, slot);
		// material coe
		shininess = 20.0f;
		shader.setFloat("material.shininess", shininess);
		ambient = glm::vec3(1.0f);
		shader.setVec3("material.ambient", ambient);
		diffuse = glm::vec3(0.8f);
		shader.setVec3("material.diffuse", diffuse);
		specular = glm::vec3(1.0f);
		shader.setVec3("material.specular", specular);
		goldModel = goldMatrices[i];
		goldModel = rockOrbitIni_M * goldModel;
		modelRotationMatrix = glm::rotate(modelRotationMatrix, glm::radians(rotate * 0.1f), glm::vec3(0.4f, 0.6f, 0.8f));
		shader.setMat4("modelTransformMatrix", goldModel);
		shader.setMat4("modelRotationMatrix", modelRotationMatrix);
		glBindVertexArray(rockVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rockEBO);
		glDrawElements(GL_TRIANGLES, rockObj.indices.size(), GL_UNSIGNED_INT, 0);
		goldcopos[i] = goldModel * glm::vec4(goldpos[i], 1.0f);

	}
}
bool craft0col, craft1col, craft2col = false;
int collected_gold = 0;
void updatestatusofcollision() {
	float timer = glfwGetTime();
	if (collision_detection(glm::vec3(speed * counter0, 3.0, 6.0), camera.Position)) {
		craft0col = true;
	}
	if (collision_detection(glm::vec3(speed * counter1, 3.0, -3.0), camera.Position)) {
		craft1col = true;
	}
	if (collision_detection(glm::vec3((counter2)*speed, 3.0f, -13.0f), camera.Position)) {
		craft2col = true;
	}
	if (craft0col) craft0Tex = ((int)(timer * 5)) % 2 + 1;
	if (craft1col) craft1Tex = ((int)(timer * 5)) % 2 + 1;
	if (craft2col) craft2Tex = ((int)(timer * 5)) % 2 + 1;

	if (collision_detection(glm::vec3(5.0f, 3.0f, -20.0f) , camera.Position) && catTex0 == 1) {
		catTex0 = 2;
		collected_gold++;
	}

	if (collision_detection(glm::vec3(-5.0f, 3.0f, -20.0f), camera.Position) && catTex1 == 1) {
		catTex1 = 2;
		collected_gold++;
	}

	for (int i = 0; i < 3; i++) {


		if (collision_detection(glm::vec3(goldcopos[i].x, goldcopos[i].y, goldcopos[i].z), camera.Position) && goldTex[i] != 2) {
			goldTex[i] = 2;
			collected_gold++;
			if (collected_gold >= 3) {
				spacecraftTex = 2;
			}
		}
	}


}
void paintGL(void)  //always run
{

	glClearColor(0.5f, 0.2f, 0.6f, 0.5f); //specify the background color, this is just an example
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//TODO:
	//Set lighting information, such as position and color of lighting source
	//Set transformation matrix
	//Bind different textures
	initializeviewMatrix();
	lightsetup();

	if (timer > 0) {
		rotate += 1.0f * 0.2;
		if (rotate == 360.0f) {
			rotate = 0.0f;
		}
	}
	drawcat0();
	drawcat1();
	drawcraft();
	drawcraft1();
	drawcraft2();
	drawearth();
	drawrock_gold();
	drawspacecraft();
	updatestatusofcollision();
	drawskybox();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	// Sets the mouse-button callback for the current window.	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouseCtl.LEFT_BUTTON = true;
		glfwGetCursorPos(window, &mouseCtl.MOUSE_Clickx, &mouseCtl.MOUSE_Clicky);
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		mouseCtl.LEFT_BUTTON = false;

	}
}

void cursor_position_callback(GLFWwindow* window, double x, double y) {
	// Sets the cursor position callback for the current window
	if (mouseCtl.LEFT_BUTTON) {
		glfwGetCursorPos(window, &mouseCtl.MOUSE_X, &mouseCtl.MOUSE_Y);
		camera.ProcessMouseMovement(mouseCtl.MOUSE_X - mouseCtl.MOUSE_Clickx, mouseCtl.MOUSE_Clicky - mouseCtl.MOUSE_Y);
	}


}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Sets the scoll callback for the current window.
	camera.ProcessMouseScroll(yoffset);

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Sets the Keyboard callback for the current window.
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}


	if (key == GLFW_KEY_Q) {
		a = true;
	}
	if (key == GLFW_KEY_E) {
		a = false;
	}
	float dscale = 0.05f;
	if (key == GLFW_KEY_UP) {
		camera.ProcessKeyboard(FORWARD, 0.05f * deltaTime);
	}

	if (key == GLFW_KEY_DOWN) {
		camera.ProcessKeyboard(BACKWARD, 0.05f * deltaTime);
	}

	if (key == GLFW_KEY_LEFT) {
		camera.ProcessKeyboard(LEFT, 0.05f * deltaTime);
	}

	if (key == GLFW_KEY_RIGHT) {
		camera.ProcessKeyboard(RIGHT, 0.05f * deltaTime);
	}

	if (key == GLFW_KEY_Z) {
		sbrightness -= 0.01f;
		if (sbrightness < 0) sbrightness = 0.0f;
	}

	if (key == GLFW_KEY_X) {
		sbrightness += 0.01f;
		if (sbrightness > 2) sbrightness = 2.0f;
	}

	if (key == GLFW_KEY_C) {
		dbrightness -= 0.01f;
		if (dbrightness < 0) dbrightness = 0.0f;
	}

	if (key == GLFW_KEY_V) {
		dbrightness += 0.01f;
		if (dbrightness > 0) dbrightness = 1.0f;
	}

	if (key == GLFW_KEY_B) {
		pbrightness -= 0.01f;
		if (pbrightness < 0) pbrightness = 0.0f;
	}

	if (key == GLFW_KEY_N) {
		pbrightness += 0.01f;
		if (pbrightness > 0) pbrightness = 1.0f;
	}

}
int main(int argc, char* argv[]) {
	GLFWwindow* window;

	/* Initialize the glfw */
	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	/* glfw: configure; necessary for MAC */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Project", NULL, NULL);
	if (!window) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	/*register callback functions*/
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);                                                                  //    
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	initializedGL();

	while (!glfwWindowShouldClose(window)) {
		/* Render here */
		paintGL();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}
