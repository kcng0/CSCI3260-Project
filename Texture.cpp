/*
Student Information
Student ID: 1155143490
Student Name: NG KA CHUN
*/

#include "Dependencies/glew/glew.h"
#include "Dependencies/GLFW/glfw3.h"
#include "Dependencies/glm/glm.hpp"
#include "Dependencies/glm/gtc/matrix_transform.hpp"

#include "Shader.h"
#include "Texture.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

GLint programID;
double mouse_x, mouse_y, c_mouse_x, c_mouse_y = 0.0f;

GLint catTex = 1;
GLint floorTex = 3;
Shader shader;
float shininess;
float dbrightness = 0.8f; float pbrightness = 0.6f; float fbrightness = 1.0f; float sbrightness = 0.8f;
glm::vec3 ambient, diffuse, specular;
float camera_dy = 0.0f; float camera_dx = 0.0f;
float camera_dr = 0.0f;
float dx = 2.0f, dy = 0.05f;
int x_press_num = 0, y_press_num = 0;
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// mouse controller
struct MouseController {
	bool LEFT_BUTTON = false;
	double MOUSE_Clickx = 0.0, MOUSE_Clicky = 0.0;
	double MOUSE_X = 0.0, MOUSE_Y = 0.0;
	int click_time = glfwGetTime();
};
MouseController mouseCtl;
// screen setting
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;

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

Model catObj, floorObj, benchObj[2], lampObj[4], towerObj;
Texture texture[10];
GLuint catVAO, catEBO, catVBO;
GLuint floorVAO, floorEBO, floorVBO;
GLuint benchVAO[2], benchEBO[2], benchVBO[2];
GLuint lampVAO[4], lampEBO[4], lampVBO[4];
GLuint towerVAO, towerEBO, towerVBO;
void sendcat(void) {
	catObj = loadOBJ("./Resources/cat/cat.obj");
	texture[0].setupTexture("./Resources/cat/cat_01.jpg");
	texture[1].setupTexture("./Resources/cat/cat_02.jpg");
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

void sendfloor(void) {
	floorObj = loadOBJ("./Resources/floor/floor.obj");
	texture[2].setupTexture("./Resources/floor/floor_01.jpg");
	texture[3].setupTexture("./Resources/floor/floor_02.jpg");
	glGenVertexArrays(1, &floorVAO);
	glBindVertexArray(floorVAO);
	glGenBuffers(1, &floorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
	glBufferData(GL_ARRAY_BUFFER, floorObj.vertices.size() * sizeof(Vertex), &floorObj.vertices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &floorEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, floorObj.indices.size() * sizeof(unsigned int), &floorObj.indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void sendbench(void) {
	benchObj[0] = loadOBJ("./Resources/bench/bench1.obj");
	benchObj[1] = loadOBJ("./Resources/bench/bench2.obj");
	texture[4].setupTexture("./Resources/bench/bench.jpg");

	glGenVertexArrays(1, &benchVAO[0]);
	glBindVertexArray(benchVAO[0]);  //first VAO
	// vertex buffer object
	glGenBuffers(1, &benchVBO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, benchVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, benchObj[0].vertices.size() * sizeof(Vertex), &benchObj[0].vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &benchEBO[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, benchEBO[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, benchObj[0].indices.size() * sizeof(unsigned int), &benchObj[0].indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// bench2
	glGenVertexArrays(1, &benchVAO[1]);
	glBindVertexArray(benchVAO[1]);  //first VAO
	// vertex buffer object
	glGenBuffers(1, &benchVBO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, benchVBO[1]);
	glBufferData(GL_ARRAY_BUFFER, benchObj[1].vertices.size() * sizeof(Vertex), &benchObj[1].vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &benchEBO[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, benchEBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, benchObj[1].indices.size() * sizeof(unsigned int), &benchObj[1].indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void sendlamp(void) {
	lampObj[0] = loadOBJ("./Resources/lamp/lamp1.obj");
	lampObj[1] = loadOBJ("./Resources/lamp/lamp2.obj");
	lampObj[2] = loadOBJ("./Resources/lamp/lamp3.obj");
	lampObj[3] = loadOBJ("./Resources/lamp/lamp4.obj");
	texture[5].setupTexture("./Resources/lamp/lamp1.jpg");

	glGenVertexArrays(1, &lampVAO[0]);
	glBindVertexArray(lampVAO[0]);  //first VAO
	// vertex buffer object
	glGenBuffers(1, &lampVBO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, lampVBO[0]);
	glBufferData(GL_ARRAY_BUFFER, lampObj[0].vertices.size() * sizeof(Vertex), &lampObj[0].vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &lampEBO[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, lampObj[0].indices.size() * sizeof(unsigned int), &lampObj[0].indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// lamp2
	glGenVertexArrays(1, &lampVAO[1]);
	glBindVertexArray(lampVAO[1]);  //first VAO
	// vertex buffer object
	glGenBuffers(1, &lampVBO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, lampVBO[1]);
	glBufferData(GL_ARRAY_BUFFER, lampObj[1].vertices.size() * sizeof(Vertex), &lampObj[1].vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &lampEBO[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, lampObj[1].indices.size() * sizeof(unsigned int), &lampObj[1].indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//lamp3
	glGenVertexArrays(1, &lampVAO[2]);
	glBindVertexArray(lampVAO[2]);  //first VAO
	// vertex buffer object
	glGenBuffers(1, &lampVBO[2]);
	glBindBuffer(GL_ARRAY_BUFFER, lampVBO[2]);
	glBufferData(GL_ARRAY_BUFFER, lampObj[2].vertices.size() * sizeof(Vertex), &lampObj[2].vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &lampEBO[2]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, lampObj[2].indices.size() * sizeof(unsigned int), &lampObj[2].indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//lamp4
	glGenVertexArrays(1, &lampVAO[3]);
	glBindVertexArray(lampVAO[3]);  //first VAO
	// vertex buffer object
	glGenBuffers(1, &lampVBO[3]);
	glBindBuffer(GL_ARRAY_BUFFER, lampVBO[3]);
	glBufferData(GL_ARRAY_BUFFER, lampObj[3].vertices.size() * sizeof(Vertex), &lampObj[3].vertices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &lampEBO[3]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, lampObj[3].indices.size() * sizeof(unsigned int), &lampObj[3].indices[0], GL_STATIC_DRAW);
	// reset parameter
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void sendtower() {
	towerObj = loadOBJ("./Resources/tower/tower.obj");
	texture[6].setupTexture("./Resources/tower/tower.jpg");
	glGenVertexArrays(1, &towerVAO);
	glBindVertexArray(towerVAO);
	glGenBuffers(1, &towerVBO);
	glBindBuffer(GL_ARRAY_BUFFER, towerVBO);
	glBufferData(GL_ARRAY_BUFFER, towerObj.vertices.size() * sizeof(Vertex), &towerObj.vertices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); //vertex texture coordinate
	glEnableVertexAttribArray(1); glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv)); //vertex normal
	glEnableVertexAttribArray(2); glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal)); // index buffer 
	glGenBuffers(1, &towerEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, towerEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, towerObj.indices.size() * sizeof(unsigned int), &towerObj.indices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void sendDataToOpenGL()
{
	//TODO
	//Load objects and bind to VAO and VBO
	//Load textures

	sendcat();
	sendfloor();
	sendbench();
	sendlamp();
	sendtower();
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
	shader.setupShader("./VertexShaderCode.glsl", "./FragmentShaderCode.glsl");
	shader.use();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

glm::mat4 eyePos;
glm::mat4 viewMatrix;
void initializeviewMatrix(void) {
	glm::mat4 transform(1.0f);
	glm::mat4 rotate(1.0f);

	glm::mat4 projectionMatrix = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 20.0f);
	shader.setMat4("projectionMatrix", projectionMatrix);
	viewMatrix = glm::lookAt(cameraPos,
		cameraFront + cameraPos,
		cameraUp);
	viewMatrix = glm::translate(viewMatrix, glm::vec3(0.0f, 0.005f * camera_dy, camera_dx));
	viewMatrix = glm::rotate(viewMatrix, glm::radians(camera_dr), glm::vec3(0.0f, 1.0f, 0.0f));

	eyePos = glm::inverse(viewMatrix);

	shader.setMat4("viewMatrix", viewMatrix);




}



void lightsetup(void) {
	double current_time = glfwGetTime();

	// directional light
	glm::vec3 dirltc = dbrightness * glm::vec3(0.3f, 0.3f, 0.3f);
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
	glm::vec3 ptltpos1(1.5f, 1.0f, 1.0f + y_press_num * dy);
	glm::vec3 ptlt1_color = pbrightness * (glm::vec3(1.0f, 0.98f, 0.14f) + glm::vec3(-1.0f * sin(current_time), -0.98f * cos(current_time), 0.96f * sin(current_time)));
	shader.setVec3("ptlt[0].position", ptltpos1);
	shader.setVec3("ptlt[0].ambient", ptlt1_color);
	shader.setVec3("ptlt[0].diffuse", glm::vec3(0.5f, 0.5f, 0.5f) * pbrightness);
	shader.setVec3("ptlt[0].specular", glm::vec3(1.0f, 1.0f, 1.0f) * pbrightness);
	shader.setFloat("ptlt[0].constant", 0.5f);
	shader.setFloat("ptlt[0].linear", 0.25f);
	shader.setFloat("ptlt[0].quadratic", 0.10f);

	// point light 2
	glm::vec3 ptltpos2(-1.5f, 1.0f, 1.0f + y_press_num * dy);
	glm::vec3 ptlt2_color = pbrightness * (glm::vec3(0.0f, 0.16f, 1.0f) + glm::vec3(1.0f * sin(current_time), 0.84 * cos(current_time), -1.0 * sin(current_time)));
	shader.setVec3("ptlt[1].position", ptltpos2);
	shader.setVec3("ptlt[1].ambient", ptlt2_color);
	shader.setVec3("ptlt[1].diffuse", glm::vec3(0.9f, 0.9f, 0.9f) * pbrightness);
	shader.setVec3("ptlt[1].specular", glm::vec3(0.8f, 0.8f, 0.8f) * pbrightness);
	shader.setFloat("ptlt[1].constant", 0.5f);
	shader.setFloat("ptlt[1].linear", 0.25f);
	shader.setFloat("ptlt[1].quadratic", 0.10f);

	// point light 3
	glm::vec3 ptltpos3(1.5f, 1.0f, -2.0f + y_press_num * dy);
	glm::vec3 ptlt3_color = pbrightness * (glm::vec3(1.0f, 0.49f, 0.0f) + glm::vec3(-1.0f * cos(current_time), 0.51 * sin(current_time), 1.0f * cos(current_time)));
	shader.setVec3("ptlt[2].position", ptltpos3);
	shader.setVec3("ptlt[2].ambient", ptlt3_color);
	shader.setVec3("ptlt[2].diffuse", glm::vec3(0.7f, 0.7f, 0.7f) * pbrightness);
	shader.setVec3("ptlt[2].specular", glm::vec3(0.15f, 0.15f, 0.15f) * pbrightness);
	shader.setFloat("ptlt[2].constant", 0.5f);
	shader.setFloat("ptlt[2].linear", 0.25f);
	shader.setFloat("ptlt[2].quadratic", 0.10f);

	// point light 4
	glm::vec3 ptltpos4(-1.5f, 1.0f, -2.0f + y_press_num * dy);
	glm::vec3 ptlt4_color = pbrightness * (glm::vec3(1.0f, 0.13f, 0.13f) + glm::vec3(-1.0f * cos(current_time), 0.67f * sin(current_time), 0.76f * cos(current_time)));
	shader.setVec3("ptlt[3].position", ptltpos4);
	shader.setVec3("ptlt[3].ambient", ptlt4_color);
	shader.setVec3("ptlt[3].diffuse", glm::vec3(0.9f, 0.9f, 0.9f) * pbrightness);
	shader.setVec3("ptlt[3].specular", glm::vec3(0.9f, 0.9f, 0.9f) * pbrightness);
	shader.setFloat("ptlt[3].constant", 0.5f);
	shader.setFloat("ptlt[3].linear", 0.25f);
	shader.setFloat("ptlt[3].quadratic", 0.10f);

	shader.setVec3("eyePositionWorld", eyePos[3][0], eyePos[3][1], eyePos[3][2]);

	// flash light
	shader.setVec3("stlt[0].position", eyePos[3][0], eyePos[3][1], eyePos[3][2]);
	shader.setVec3("stlt[0].direction", cameraFront + glm::vec3(eyePos[3][0], eyePos[3][1], eyePos[3][2]));
	shader.setFloat("stlt[0].cutOff", glm::cos(glm::radians(25.0f)));
	shader.setFloat("stlt[0].outerCutOff", glm::cos(glm::radians(27.5f)));

	shader.setVec3("stlt[0].ambient", glm::vec3(1.0f, 1.0f, 1.0f) * fbrightness);
	shader.setVec3("stlt[0].diffuse", glm::vec3(0.8f, 0.8f, 0.8f) * fbrightness);
	shader.setVec3("stlt[0].specular", glm::vec3(1.0f, 1.0f, 1.0f) * fbrightness);
	shader.setFloat("stlt[0].constant", 0.5f);
	shader.setFloat("stlt[0].linear", 0.04f);
	shader.setFloat("stlt[0].quadratic", 0.016f);

	// lamp1
	shader.setVec3("stlt[1].position", 2.48f, 2.73f, -2.06);
	shader.setVec3("stlt[1].direction", -0.2f, -1.0f, -0.2f);
	shader.setFloat("stlt[1].cutOff", glm::cos(glm::radians(12.0f)));
	shader.setFloat("stlt[1].outerCutOff", glm::cos(glm::radians(20.5f)));

	shader.setVec3("stlt[1].ambient", glm::vec3(0.97f, 0.11f, 1.0f) * sbrightness);
	shader.setVec3("stlt[1].diffuse", glm::vec3(0.8f, 0.8f, 0.8f) * sbrightness);
	shader.setVec3("stlt[1].specular", glm::vec3(1.0f, 1.0f, 1.0f) * sbrightness);
	shader.setFloat("stlt[1].constant", 1.0f);
	shader.setFloat("stlt[1].linear", 0.09f);
	shader.setFloat("stlt[1].quadratic", 0.032f);

	// lamp2
	shader.setVec3("stlt[2].position", 2.53f, 2.73f, -7.9);
	shader.setVec3("stlt[2].direction", -0.2f, -1.0f, -0.2f);
	shader.setFloat("stlt[2].cutOff", glm::cos(glm::radians(12.0f)));
	shader.setFloat("stlt[2].outerCutOff", glm::cos(glm::radians(20.5f)));

	shader.setVec3("stlt[2].ambient", glm::vec3(0.97f, 0.11f, 1.0f) * sbrightness);
	shader.setVec3("stlt[2].diffuse", glm::vec3(0.8f, 0.8f, 0.8f) * sbrightness);
	shader.setVec3("stlt[2].specular", glm::vec3(1.0f, 1.0f, 1.0f) * sbrightness);
	shader.setFloat("stlt[2].constant", 1.0f);
	shader.setFloat("stlt[2].linear", 0.09f);
	shader.setFloat("stlt[2].quadratic", 0.032f);

	// lamp 3
	shader.setVec3("stlt[3].position", -3.2f, 2.78f, -2.1f);
	shader.setVec3("stlt[3].direction", 0.2f, -1.0f, -0.2f);
	shader.setFloat("stlt[3].cutOff", glm::cos(glm::radians(12.0f)));
	shader.setFloat("stlt[3].outerCutOff", glm::cos(glm::radians(20.5f)));

	shader.setVec3("stlt[3].ambient", glm::vec3(0.97f, 0.11f, 1.0f) * sbrightness);
	shader.setVec3("stlt[3].diffuse", glm::vec3(0.8f, 0.8f, 0.8f) * sbrightness);
	shader.setVec3("stlt[3].specular", glm::vec3(1.0f, 1.0f, 1.0f) * sbrightness);
	shader.setFloat("stlt[3].constant", 1.0f);
	shader.setFloat("stlt[3].linear", 0.09f);
	shader.setFloat("stlt[3].quadratic", 0.032f);

	// lamp 4
	shader.setVec3("stlt[4].position", -3.2f, 2.78f, -7.99f);
	shader.setVec3("stlt[4].direction", 0.2f, -1.0f, -0.2f);
	shader.setFloat("stlt[4].cutOff", glm::cos(glm::radians(12.0f)));
	shader.setFloat("stlt[4].outerCutOff", glm::cos(glm::radians(20.5f)));

	shader.setVec3("stlt[4].ambient", glm::vec3(0.97f, 0.11f, 1.0f) * sbrightness);
	shader.setVec3("stlt[4].diffuse", glm::vec3(0.8f, 0.8f, 0.8f) * sbrightness);
	shader.setVec3("stlt[4].specular", glm::vec3(1.0f, 1.0f, 1.0f) * sbrightness);
	shader.setFloat("stlt[4].constant", 1.0f);
	shader.setFloat("stlt[4].linear", 0.09f);
	shader.setFloat("stlt[4].quadratic", 0.032f);
}

void drawcat() {

	GLint slot = 0;


	glBindVertexArray(catVAO);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	if (catTex == 1)
		texture[0].bind(slot);
	else if (catTex == 2)
		texture[1].bind(slot);
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

	modelTransformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, y_press_num * dy));
	modelRotationMatrix = glm::rotate(modelTransformMatrix, glm::radians(x_press_num * dx), glm::vec3(0.0f, 1.0f, 0.0f));

	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, catEBO);
	glDrawElements(GL_TRIANGLES, catObj.indices.size(), GL_UNSIGNED_INT, 0);

}

void drawfloor() {
	glBindVertexArray(floorVAO);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint slot = 0;
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	if (floorTex == 3)
		texture[2].bind(slot);
	else if (floorTex == 4)
		texture[3].bind(slot);
	glUniform1i(TextureID, slot);
	shininess = 20.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.4f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.2f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(0.8f);
	shader.setVec3("material.specular", specular);
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
	glDrawElements(GL_TRIANGLES, floorObj.indices.size(), GL_UNSIGNED_INT, 0);
}

void drawbench() {
	glBindVertexArray(benchVAO[0]);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint slot = 0;
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	texture[4].bind(slot);
	glUniform1i(TextureID, slot);
	shininess = 20.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.2f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.8f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(0.5f);
	shader.setVec3("material.specular", specular);
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, benchEBO[0]);
	glDrawElements(GL_TRIANGLES, benchObj[0].indices.size(), GL_UNSIGNED_INT, 0);

	// seoncd bench
	glBindVertexArray(benchVAO[1]);
	TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	texture[4].bind(slot);
	glUniform1i(TextureID, slot);
	shininess = 20.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.2f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.8f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(0.5f);
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, benchEBO[1]);
	glDrawElements(GL_TRIANGLES, benchObj[1].indices.size(), GL_UNSIGNED_INT, 0);
}

void drawlamp() {
	glBindVertexArray(lampVAO[0]);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint slot = 0;
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	texture[5].bind(slot);
	glUniform1i(TextureID, slot);
	shininess = 30.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.4f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.1f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO[0]);
	glDrawElements(GL_TRIANGLES, lampObj[0].indices.size(), GL_UNSIGNED_INT, 0);

	// 2 lamp
	glBindVertexArray(lampVAO[1]);
	glActiveTexture(GL_TEXTURE0);
	texture[5].bind(slot);
	glUniform1i(TextureID, slot);
	shininess = 30.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.4f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.1f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO[1]);
	glDrawElements(GL_TRIANGLES, lampObj[1].indices.size(), GL_UNSIGNED_INT, 0);

	// 3 lamp
	glBindVertexArray(lampVAO[2]);
	glActiveTexture(GL_TEXTURE0);
	texture[5].bind(slot);
	glUniform1i(TextureID, slot);
	shininess = 30.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.4f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.1f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO[2]);
	glDrawElements(GL_TRIANGLES, lampObj[2].indices.size(), GL_UNSIGNED_INT, 0);

	// 4 lamp
	glBindVertexArray(lampVAO[3]);
	glActiveTexture(GL_TEXTURE0);
	texture[5].bind(slot);
	glUniform1i(TextureID, slot);
	shininess = 30.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.4f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.1f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(1.0f);
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampEBO[3]);
	glDrawElements(GL_TRIANGLES, lampObj[3].indices.size(), GL_UNSIGNED_INT, 0);
}

void drawtower() {
	glBindVertexArray(towerVAO);
	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);
	glm::mat4 modelRotationMatrix = glm::mat4(1.0f);
	GLint slot = 0;
	GLint TextureID = glGetUniformLocation(programID, "myTextureSampler");
	glActiveTexture(GL_TEXTURE0);
	texture[6].bind(slot);
	glUniform1i(TextureID, slot);
	shininess = 20.0f;
	shader.setFloat("material.shininess", shininess);
	ambient = glm::vec3(0.7f);
	shader.setVec3("material.ambient", ambient);
	diffuse = glm::vec3(0.5f);
	shader.setVec3("material.diffuse", diffuse);
	specular = glm::vec3(0.8f);
	shader.setVec3("material.specular", specular);
	shader.setMat4("modelTransformMatrix", modelTransformMatrix);
	shader.setMat4("modelRotationMatrix", modelRotationMatrix);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, towerEBO);
	glDrawElements(GL_TRIANGLES, towerObj.indices.size(), GL_UNSIGNED_INT, 0);
}

void paintGL(void)  //always run
{
	glClearColor(0.039f, 0.078f, 0.25f, 1.0f);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	initializeviewMatrix();
	lightsetup();
	drawcat();
	drawfloor();
	drawbench();
	drawlamp();
	drawtower();
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Sets the mouse-button callback for the current window.
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouseCtl.LEFT_BUTTON = true;
		glfwGetCursorPos(window, &mouseCtl.MOUSE_Clickx, &mouseCtl.MOUSE_Clicky);
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		mouseCtl.LEFT_BUTTON = false;

	}
}

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
	// Sets the cursor position callback for the current window
	if (mouseCtl.LEFT_BUTTON) {
		glfwGetCursorPos(window, &mouseCtl.MOUSE_X, &mouseCtl.MOUSE_Y);
		camera_dy += (mouseCtl.MOUSE_Y - mouseCtl.MOUSE_Clicky) * 0.1f;
		camera_dr += (mouseCtl.MOUSE_X - mouseCtl.MOUSE_Clickx) * 0.01f;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Sets the scoll callback for the current window.
	if (yoffset > 0)
		camera_dx += 0.3f;
	else camera_dx -= 0.3f;

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Sets the Keyboard callback for the current window.
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
		catTex = 1;
	}

	if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
		catTex = 2;
	}

	if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
		floorTex = 3;
	}

	if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
		floorTex = 4;
	}

	if (key == GLFW_KEY_S) {
		dbrightness -= 0.05f;
		if (dbrightness < 0.0f) dbrightness = 0.0f;
	}

	if (key == GLFW_KEY_W) {
		dbrightness += 0.05f;
		if (dbrightness > 1.0f) dbrightness = 1.0f;
	}

	if (key == GLFW_KEY_D) {
		pbrightness -= 0.05f;
		if (pbrightness < 0.0f) pbrightness = 0.0f;
	}

	if (key == GLFW_KEY_E) {
		pbrightness += 0.05f;
		if (pbrightness > 1.0f) pbrightness = 1.0f;
	}

	if (key == GLFW_KEY_F) {
		fbrightness -= 0.05f;
		if (fbrightness < 0.0f) fbrightness = 0.0f;
	}

	if (key == GLFW_KEY_R) {
		fbrightness += 0.05f;
		if (fbrightness > 1.0f) fbrightness = 1.0f;
	}

	if (key == GLFW_KEY_G) {
		sbrightness -= 0.05f;
		if (sbrightness < 0.0f) sbrightness = 0.0f;
	}

	if (key == GLFW_KEY_T) {
		sbrightness += 0.05f;
		if (sbrightness > 1.0f) sbrightness = 1.0f;
	}

	if (key == GLFW_KEY_DOWN) {
		y_press_num += 1;

	}

	if (key == GLFW_KEY_UP) {
		y_press_num -= 1;
	}

	if (key == GLFW_KEY_LEFT) {
		x_press_num -= 1;
	}

	if (key == GLFW_KEY_RIGHT) {
		x_press_num += 1;
	}

}


int main(int argc, char* argv[])
{
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
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment 2", NULL, NULL);
	if (!window) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	/*register callback functions*/
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
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






