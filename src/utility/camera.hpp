#pragma once
#include <simd/simd.h>
#include <string>
#include <glm/glm/glm.hpp>

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 0.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

class Camera {
public:
    glm::vec3 m_position;
    glm::vec3 m_front;
    glm::vec3 m_up;
    glm::vec3 m_right;
    glm::vec3 m_worldUp;
    
    float m_yaw;
    float m_pitch;
    
    float movementSpeed;
    float mouseSensitivity;
    float zoom;
    
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);
    Camera(float posx, float posy, float posz, float upx, float upy, float upz, float yaw = YAW, float pitch = PITCH);
    
    simd::float4x4 getViewMatrix();
    simd::float4x4 getModifiedView();
    simd::float4x4 getPerspectiveMatrix(float aspect);
    simd::float3 getPosition();
    std::string getViewDebug();
    void processKeyboard(Camera_Movement direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processMouseScroll(float yoffset);
    
private:
    void updateCameraVectors();
};
