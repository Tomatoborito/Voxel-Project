#pragma once

#include <dawn/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/noise.hpp>
#include <unordered_map>

#include "appcontext.h"

class Inputs {

public:

	struct MyUniforms {
		glm::mat4x4 projectionMatrix;
		glm::mat4x4 viewMatrix;
		glm::mat4x4 modelMatrix;
		glm::vec4 color;
		float time;
		float _pad[3];
	};

	MyUniforms uniforms = {};

	bool test = true;

	float deltaTime;
	float lastFrame;

	float speed = 20.0f;

	float sensitivity = 0.001f;

	float cameraanglex = 0.0f;
	float cameraangley = 0.0f;

	double lastxpos;
	double lastypos;

	bool generate = true;
	bool cull = true;
	bool printtime = false;
	bool breackvoxels = false;

	glm::vec3 forward;

	glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, -10.0f);
	glm::vec3 target = glm::vec3(32.0f, 32.0f, 32.0f);

	glm::mat4x4 T2;
	glm::mat4x4 R2;
	glm::mat4x4 R4;

	bool keys[1024] = { false };
	std::unordered_map<int, std::function<void()>> keyActions;
public:
	void onInit(GLFWwindow* window);
	void onFrame(float delta);

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouse_callback(GLFWwindow* window, double x, double y);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

	void handleMousebutton(int button, int action);
	void handleKeyPress(int key, int action, GLFWwindow* window);
	void handllemousemove(double x, double y, GLFWwindow* window);
	void handleMousewheel(double yoffset);
	void handleallkeys();
	void setupInputs();
	void moveplayer(int key);
};
