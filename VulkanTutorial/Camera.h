#pragma once
#include <glm/glm.hpp>


class Camera
{
public:
    Camera(const glm::vec3& cameraPos, float fov, float ar)
       : m_CameraPos{cameraPos},
        m_FieldOfView{fov},
        m_AspecRatio{ar}
    {
    }
    ~Camera() = default;
    glm::vec3 GetPosition()const { return m_CameraPos; };
    glm::vec3 GetWorldCenterPosition()const { return m_WorldCenter; };
    float GetNearPlane()const { return m_NearPlane; };
    float GetFarPlane()const { return m_FarPlane; };
    float GetAspectRatio()const { return m_AspecRatio; };
    float GetfieldOfView()const { return m_FieldOfView; };
    
    void MoveCamera(const glm::vec3& offset) { m_CameraPos += offset; };

private:
    glm::vec3 m_CameraPos{ 2.0f, 2.0f, 2.0f };
    const glm::vec3 m_WorldCenter{ 0.0f, 0.0f, 0.0f };
    float m_FieldOfView{ glm::radians(45.f) };
    float m_AspecRatio{ 1.f };
    float m_NearPlane{ 0.1f };
    float m_FarPlane{ 10.0f };
};