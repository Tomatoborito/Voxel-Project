#include "input.h"
#include <iostream>

void Inputs::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
	auto instance = static_cast<AppContext*>(glfwGetWindowUserPointer(window));

	if(instance && instance->inputs){
		instance->inputs->handleKeyPress(key, action, window);
	}
}

void Inputs::mouse_callback(GLFWwindow* window, double x, double y){
	auto instance = static_cast<AppContext*>(glfwGetWindowUserPointer(window));

	if(instance && instance->inputs){
		instance->inputs->handllemousemove(x, y, window);
	}
}

void Inputs::scroll_callback(GLFWwindow* window, double xoffset, double yoffset){
	auto instance = static_cast<AppContext*>(glfwGetWindowUserPointer(window));

	if(instance && instance->inputs){
		instance->inputs->handleMousewheel(yoffset);
	}
}

void Inputs::mouse_button_callback(GLFWwindow* window, int button, int action, int mods){
	auto instance = static_cast<AppContext*>(glfwGetWindowUserPointer(window));

	if(instance && instance->inputs){
		instance->inputs->handleMousebutton(button, action);
	}
}

void Inputs::handleMousebutton(int button, int action){
	if(action == GLFW_PRESS){
		if(button == GLFW_MOUSE_BUTTON_LEFT){
			breackvoxels = true;
		}
	}
}

void Inputs::handleKeyPress(int key, int action, GLFWwindow* window){

	if(action == GLFW_PRESS){
		keys[key] = true;

		if(key == GLFW_KEY_ESCAPE){
			test = true;
			if(glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED){
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
			else{ glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); }
		}
		if(key == GLFW_KEY_L){
			generate = !generate;
		}
		if(key == GLFW_KEY_P){
			cull = !cull;
		}
		if(key == GLFW_KEY_O){
			printtime = true;
		}

	}
	else if(action == GLFW_RELEASE){
		keys[key] = false;
	}


}

void Inputs::handleallkeys(){
	for(auto const& [key, action] : keyActions){
		if(keys[key]){
			action();
		}
	}
}

void Inputs::setupInputs(){
	keyActions[GLFW_KEY_W] = [&](){ moveplayer(GLFW_KEY_W); };
	keyActions[GLFW_KEY_S] = [&](){ moveplayer(GLFW_KEY_S); };
	keyActions[GLFW_KEY_D] = [&](){ moveplayer(GLFW_KEY_D); };
	keyActions[GLFW_KEY_A] = [&](){ moveplayer(GLFW_KEY_A); };
	keyActions[GLFW_KEY_SPACE] = [&](){ moveplayer(GLFW_KEY_SPACE); };
	keyActions[GLFW_KEY_LEFT_SHIFT] = [&](){ moveplayer(GLFW_KEY_LEFT_SHIFT); };

}


void Inputs::moveplayer(int key){
	glm::vec3 walkforward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
	glm::vec3 right = glm::normalize(glm::cross(walkforward, glm::vec3(0.0f, 1.0f, 0.0f)));
	switch(key){
		case GLFW_KEY_W:
			cameraPos += walkforward * deltaTime * speed;
			break;
		case GLFW_KEY_S:
			cameraPos -= walkforward * deltaTime * speed;
			break;
		case GLFW_KEY_D:
			cameraPos += right * deltaTime * speed;
			break;
		case GLFW_KEY_A:
			cameraPos -= right * deltaTime * speed;
			break;
		case GLFW_KEY_SPACE:
			cameraPos.y += 1 * deltaTime * speed;
			break;
		case GLFW_KEY_LEFT_SHIFT:
			cameraPos.y -= 1 * deltaTime * speed;
			break;
		default:
			break;
	}
}

void Inputs::handllemousemove(double x, double y, GLFWwindow* window){
	if(glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED){
		return;
	}
	if(test == true){
		lastypos = y;
		lastxpos = x;
		test = false;
	}

	double differencex = (lastxpos - x) * sensitivity;
	cameraanglex -= differencex;
	lastxpos = x;

	double differencey = (lastypos - y) * sensitivity;
	cameraangley -= differencey;
	if(cameraangley > 1.55f) cameraangley = 1.55f;
	if(cameraangley < -1.55f) cameraangley = -1.55f;
	lastypos = y;
}

void Inputs::handleMousewheel(double yoffset){
	const float sensitivity = 1.2f;

	if(yoffset > 0){
		speed *= std::pow(sensitivity, yoffset);
	}
	else if(yoffset < 0){
		speed /= std::pow(sensitivity, std::abs(yoffset));
	}
}

void Inputs::onInit(GLFWwindow* window){
	setupInputs();
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
}

void Inputs::onFrame(float delta){
	deltaTime = delta;
	using glm::vec3, glm::mat4x4;
	{
		forward.x = sin(cameraanglex) * cos(cameraangley);
		forward.y = -sin(cameraangley);
		forward.z = -cos(cameraanglex) * cos(cameraangley);

		uniforms.viewMatrix = glm::lookAt(cameraPos, cameraPos + forward, glm::vec3(0.0f, 1.0f, 0.0f));
	}
	handleallkeys();
}