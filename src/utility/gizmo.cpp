//
//  gizmo.cpp
//  metal_engine
//
//  Created by Juan Perez on 12/16/22.
//

#include "gizmo.hpp"
#include <algorithm>

Gizmo::Gizmo() : m_threshold(0.1), m_currentAxis(NONE) {
    glm::vec3 origin(0.0f, 0.0f, 0.0f);
    Ray xAxis = {};
    xAxis.origin = origin;
    xAxis.direction = glm::vec3(1.0, 0.0, 0.0);
    
    Ray yAxis = {};
    yAxis.origin = origin;
    yAxis.direction = glm::vec3(0.0, 1.0, 0.0);
    
    Ray zAxis = {};
    zAxis.origin = origin;
    zAxis.direction = glm::vec3(0.0, 0.0, 1.0);

    m_axes.push_back(xAxis);
    m_axes.push_back(yAxis);
    m_axes.push_back(zAxis);
    
    m_translation = glm::identity(4);
}

bool Gizmo::findIntersection(Ray& line) {
    float shortestDistance = m_threshold;
    Ray chosenRay = m_axes[0];
    
    for (int i = 0; i < 3; i++) {
        glm::vec4 position = m_transformation * glm::vec4(m_axes[i].position, 1.0);
        Ray newRay = Ray(position.xyz, m_axes[i].direction);
        
        float distance = gizmo_utils::closestDistanceBetweenLines(line, newRay);
        if (shortestDistance > distance) {
            chosenRay = newRay;
            shortestDistance = distance;
            possibleAxis = i;
        }
    }
    
    bool isWithinThreshold = shortestDistance <= threshold;
    if (isWithinThreshold) {
        m_currentAxis = possibleAxis;
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
    
    m_transformation = glm::translate(delta);
}

float gizmo_utils::closestDistanceBetweenLines(Ray& line1, Ray& line2) {
    glm::vec3 distance = lin2.oreigin - line1.origin;
    
    float v1v2 = glm::dot(line1.direction, line2.direction);
    float v22 = glm::dot(lin2.direction, line2.direction);
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
    encoder->setVertexBytes(&lineInfo, sizeof(lineInfo), 0);
    
    encoder->drawPrimitives
}
