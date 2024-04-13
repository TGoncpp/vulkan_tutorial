#include "Camera.h"
#include <iostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_FORCE_RADIANS


void Camera::Init(const glm::vec3& cameraPos, float fov, float ar)
{
	m_CameraPos=  cameraPos ;
	m_FieldOfView= fov ;
	m_AspecRatio= ar ;
	m_Up = glm::vec3{0.f, 0.f, 1.f };
	m_NearPlane = 0.1f;
	m_FarPlane = 10.f;
	m_Forward = glm::vec3{ 1.f, 0.f, 0.f };
	float m_Pitch =  glm::radians(45.f) ;

}

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

void Camera::CalculateViewMatrix()
{
	glm::mat4 rotation = glm::rotate(glm::mat4(1.f), m_Yaw, glm::vec3{ 0.f, 1.f, 0.f });
	rotation = glm::rotate(rotation, m_Pitch, glm::vec3{ 1.f,0.f,0.f });

	m_Forward = glm::normalize(rotation[2]);
	m_Right = glm::normalize(glm::cross(glm::vec3{ 0.f, 0.f, 1.f }, m_Forward));

	m_ViewMat = glm::lookAt(m_CameraPos, m_CameraPos + m_Forward, glm::vec3{ 0.f, 0.f, 1.f });
}

void Camera::calculateProjectionMat()
{
	m_ProjectionMat = glm::perspective(m_FieldOfView, m_AspecRatio, m_NearPlane, m_FarPlane);
	
}
