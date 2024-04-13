#pragma once
#include <glm/glm.hpp>


struct GLFWwindow;

class Camera
{
public:
    Camera(const glm::vec3& cameraPos, float fov, float ar)
       : m_CameraPos{cameraPos},
        m_FieldOfView{fov},
        m_AspecRatio{ar}, 
        m_Up{glm::vec3{0.f, 0.f, 1.f} }
    {
        calculateProjectionMat();
    }
    ~Camera() = default;

    void Init(const glm::vec3& cameraPos, float fov, float ar);
    glm::mat4 GetProjectionMat()const { return m_ProjectionMat; };
    glm::vec3 GetPosition()const { return m_CameraPos; };
    glm::vec3 GetForwardView()const { return m_Forward; };
    glm::vec3 GetWorldCenterPosition()const { return m_WorldCenter; };
    float GetNearPlane()const { return m_NearPlane; };
    float GetFarPlane()const { return m_FarPlane; };
    float GetAspectRatio()const { return m_AspecRatio; };
    float GetfieldOfView()const { return m_FieldOfView; };
    
    void MoveCamera(const glm::vec3& offset) ;
    void keyEvent(int key, int scancode, int action, int mods);
    void mouseMove(GLFWwindow* window, double xpos, double ypos);
    void mouseEvent(GLFWwindow* window, int button, int action, int mods);

    void CalculateViewMatrix();

    void calculateProjectionMat();

private:
    glm::mat4 m_ProjectionMat{ glm::mat4(1) };
    glm::mat4 m_ViewMat{ glm::mat4(1) };
    glm::vec3 m_CameraPos{ 2.0f, 2.0f, 2.0f };
    const glm::vec3 m_WorldCenter{ 0.0f, 0.0f, 0.0f };
    float m_FieldOfView{ glm::radians(-45.f) };
    float m_AspecRatio{ 1.f };
    float m_NearPlane{ 0.1f };
    float m_FarPlane{ 10.0f };

    glm::vec3 m_Forward{};
    glm::vec3 m_Right{};
    glm::vec3 m_Up{0.f, 0.f, 1.f};
    float m_Yaw{ 0.0f };
    float m_Pitch{ glm::radians(45.f)};
    glm::vec3 m_DragStart{};
    float m_Radius{ 3.0f };
    float m_Rotation{ 0.f };

};