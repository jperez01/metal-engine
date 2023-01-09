//
//  gizmo.cpp
//  metal_engine
//
//  Created by Juan Perez on 12/16/22.
//

#include "gizmo.hpp"

#include <simd/simd.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

Gizmo::Gizmo() : m_threshold(0.1), m_currentAxis(NONE) {
    glm::vec3 origin(0.0f, 0.0f, 0.0f);
    Ray xAxis(origin, glm::vec3(1.0, 0.0, 0.0));
    
    Ray yAxis(origin, glm::vec3(0.0, 1.0, 0.0));
    
    Ray zAxis(origin, glm::vec3(0.0, 0.0, 1.0));

    m_axes.push_back(xAxis);
    m_axes.push_back(yAxis);
    m_axes.push_back(zAxis);
    
    m_transformation = glm::mat4(1.0f);
}

bool Gizmo::findIntersection(Ray& line) {
    float shortestDistance = m_threshold;
    int possibleAxis = -1;
    Ray chosenRay = m_axes[0];
    
    for (int i = 0; i < 3; i++) {
        glm::vec4 position = m_transformation * glm::vec4(m_axes[i].origin, 1.0);
        Ray newRay = Ray(glm::vec3(position), m_axes[i].direction);
        
        float distance = gizmo_utils::closestDistanceBetweenLines(line, newRay);
        if (shortestDistance > distance) {
            chosenRay = newRay;
            shortestDistance = distance;
            possibleAxis = i;
        }
    }
    
    bool isWithinThreshold = shortestDistance <= m_threshold;
    if (isWithinThreshold) {
        m_currentAxis = static_cast<ChosenAxis>(possibleAxis);
        m_startPosition = chosenRay.calculatePoint();
    }
    
    return isWithinThreshold;
}

void Gizmo::manipulateGizmo(Ray& line) {
    Ray chosenAxis = m_axes[m_currentAxis];
    
    glm::vec3 planeVector = chosenAxis.origin - m_startPosition;
    glm::vec3 normal = glm::cross(planeVector, chosenAxis.direction);
    
    float t = glm::dot(m_startPosition - line.origin, normal) / glm::dot(line.direction, normal);
    
    glm::vec3 position = line.origin + line.direction * t;
    glm::vec3 delta = (position - m_startPosition) * chosenAxis.direction;
    
    m_transformation = glm::translate(glm::mat4(1.0f), delta);
}

float gizmo_utils::closestDistanceBetweenLines(Ray& line1, Ray& line2) {
    glm::vec3 distance = line2.origin - line1.origin;
    
    float v1v2 = glm::dot(line1.direction, line2.direction);
    float v22 = glm::dot(line2.direction, line2.direction);
    float v11 = glm::dot(line1.direction, line1.direction);
    
    float determinant = v11 * v22 - v1v2 * v1v2;
    
    if (std::abs(determinant) > 0.001) {
        float inv_determinant = 1.0f / determinant;
        float dpv1 = glm::dot(distance, line1.direction);
        float dpv2 = glm::dot(distance, line2.direction);
        
        float t1 = inv_determinant * (dpv1 * v22 - v1v2 * dpv2);
        float t2 = inv_determinant * (dpv1 * v1v2 - v11 * dpv2);
        
        line2.t = t2;
        
        return glm::length(distance + t2 * line2.direction - t1 * line1.direction);
    } else {
        glm::vec3 a = glm::cross(distance, line1.direction);
        return std::sqrt(glm::dot(a, a) / v1v2);
    }
}

void Gizmo::draw(MTL::RenderCommandEncoder* encoder) {
    simd::float3 vertices[] = {
        simd_make_float3(0.0, 0.0, 0.0),
        simd_make_float3(0.0, 1.0, 0.0),
        simd_make_float3(0.0, 0.0, 0.0),
        simd_make_float3(1.0, 0.0, 0.0),
        simd_make_float3(0.0, 0.0, 0.0),
        simd_make_float3(-1.0, 0.0, 1.0),
    };
    encoder->setVertexBytes(&vertices, sizeof(vertices), 0);
    
    simd::float4x4 transformation = simd_matrix_from_rows(
      (simd::float4) {m_transformation[0][0], m_transformation[1][0], m_transformation[2][0], 0},
        (simd::float4) {m_transformation[0][1], m_transformation[1][1], m_transformation[2][1], 0},
      (simd::float4) {m_transformation[0][2], m_transformation[1][2], m_transformation[2][2], 0},
      (simd::float4) {m_transformation[0][3], m_transformation[1][3], m_transformation[2][3], m_transformation[3][3]}
      );
    encoder->setVertexBytes(&transformation, sizeof(transformation), 1);
    
    encoder->drawPrimitives(MTL::PrimitiveTypeLine, NS::UInteger(0), 6);
}
