#include "Camera.h"
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void Camera::MoveCamera(const glm::vec3& offset)
{
}


void Camera::keyEvent(int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		std::cout << "presses W\n";
		//m_CameraPos.x = std::max(3.0f, m_CameraPos.x - 0.2f);
		m_CameraPos.x -= 0.2f;
	}
	if (key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		std::cout << "presses S\n";
		m_CameraPos.x = std::min(30.0f, m_CameraPos.x + 0.2f);
	}
	if (key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		std::cout << "presses A\n";
		m_CameraPos.y = std::max(3.0f, m_CameraPos.y - 0.2f);
	}
	if (key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		std::cout << "presses D\n";
		m_CameraPos.y = std::min(30.0f, m_CameraPos.y + 0.2f);
	}
}
void Camera::mouseMove(GLFWwindow* window, double xpos, double ypos)
{
	int state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
	if (state == GLFW_PRESS)
	{
		float dx = static_cast<float>(xpos) - m_DragStart.x;
		if (dx > 0) {
			m_Rotation += 0.01f;
		}
		else {
			m_Rotation -= 0.01f;
		}
	}
}
void Camera::mouseEvent(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		std::cout << "right mouse button pressed\n";
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		m_DragStart.x = static_cast<float>(xpos);
		m_DragStart.y = static_cast<float>(ypos);
	}
}