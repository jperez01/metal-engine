//
//  camera.cpp
//  metal_engine
//
//  Created by Juan Perez on 11/30/22.
//

#include "camera.hpp"
#include <cmath>
#include <sstream>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) :
    m_position(position),
    m_worldUp(up),
    m_yaw(yaw), m_pitch(pitch),
    m_front(glm::vec3(0.0f, 0.0f, -1.0f)),
    movementSpeed(SPEED),
    mouseSensitivity(SENSITIVITY),
    zoom(ZOOM) {
    updateCameraVectors();
}

Camera::Camera(float posx, float posy, float posz, float upx, float upy, float upz, float yaw, float pitch) :
    m_yaw(yaw), m_pitch(pitch),
    m_front(glm::vec3(0.0f, 0.0f, -1.0f)),
    movementSpeed(SPEED),
    mouseSensitivity(SENSITIVITY),
    zoom(ZOOM) {
        m_position = glm::vec3(posx, posy, posz);
        m_worldUp = glm::vec3(upx, upy, upz);
        updateCameraVectors();
}

simd::float4x4 Camera::getViewMatrix() {
    glm::mat4 lookAtMatrix = glm::lookAt(m_position, m_position + m_front, m_up);
    
    simd::float4x4 lookAtSimd = simd_matrix_from_rows(
          (simd::float4) {lookAtMatrix[0][0], lookAtMatrix[1][0], lookAtMatrix[2][0], lookAtMatrix[3][0]},
        (simd::float4) {lookAtMatrix[0][1], lookAtMatrix[1][1], lookAtMatrix[2][1], lookAtMatrix[3][1]},
      (simd::float4) {lookAtMatrix[0][2], lookAtMatrix[1][2], lookAtMatrix[2][2], lookAtMatrix[3][2]},
      (simd::float4) {lookAtMatrix[0][3], lookAtMatrix[1][3], lookAtMatrix[2][3], lookAtMatrix[3][3]}
      );
    
    return lookAtSimd;
}

simd::float4x4 Camera::getModifiedView() {
    glm::mat4 lookAtMatrix = glm::lookAt(m_position, m_position + m_front, m_up);
    
    simd::float4x4 lookAtSimd = simd_matrix_from_rows(
      (simd::float4) {lookAtMatrix[0][0], lookAtMatrix[1][0], lookAtMatrix[2][0], 0},
        (simd::float4) {lookAtMatrix[0][1], lookAtMatrix[1][1], lookAtMatrix[2][1], 0},
      (simd::float4) {lookAtMatrix[0][2], lookAtMatrix[1][2], lookAtMatrix[2][2], 0},
      (simd::float4) {lookAtMatrix[0][3], lookAtMatrix[1][3], lookAtMatrix[2][3], lookAtMatrix[3][3]}
      );
    
    return lookAtSimd;
}

simd::float4x4 Camera::getPerspectiveMatrix(float aspect) {
    glm::mat4 perspective = glm::perspective(glm::radians(zoom), aspect, 0.1f, 100.0f);
    
    simd::float4x4 perspectiveSimd = simd_matrix_from_rows(
       (simd::float4) {perspective[0][0], perspective[1][0], perspective[2][0], perspective[3][0]},
     (simd::float4) {perspective[0][1], perspective[1][1], perspective[2][1], perspective[3][1]},
   (simd::float4) {perspective[0][2], perspective[1][2], perspective[2][2], perspective[3][2]},
   (simd::float4) {perspective[0][3], perspective[1][3], perspective[2][3], perspective[3][3]}
   );
    
    return perspectiveSimd;
}

simd::float3 Camera::getPosition() {
    return simd_make_float3(m_position.x, m_position.y, m_position.z);
}

std::string Camera::getViewDebug() {
    simd::float4x4 matrix = getViewMatrix();
    std::stringstream stream;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            stream << matrix.columns[x][y];
            if (x == 3) stream << "\n";
            else stream << " ";
        }
    }
    
    return stream.str();
}

void Camera::processKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    if (direction == FORWARD)
        m_position += m_front * velocity;
    if (direction == BACKWARD)
        m_position -= m_front * velocity;
    if (direction == LEFT)
        m_position -= m_right * velocity;
    if (direction == RIGHT)
        m_position += m_right * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    yoffset *= mouseSensitivity;
    xoffset *= mouseSensitivity;
    
    m_yaw += xoffset;
    m_pitch += yoffset;
    
    if (constrainPitch) {
        if (m_pitch > 89.0) m_pitch = 89.0;
        if (m_pitch < -89.0) m_pitch = -89.0;
    }
    
    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
    zoom -= yoffset;
    if (zoom < 1.0f) zoom = 1.0f;
    if (zoom > 45.0f) zoom = 45.0f;
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}
