//
//  gizmo.hpp
//  metal_engine
//
//  Created by Juan Perez on 12/16/22.
//
#pragma once

#include <stdio.h>
#include <glm/glm/glm.hpp>
#include <vector>

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    float t;
    
    Ray(glm::vec3 org, glm::vec3 dir) {
        origin = org;
        direction = dir;
    }
    
    glm::vec3 calculatePoint() {
        return origin + direction * t;
    }
};

struct Circle {
    glm::vec3 origin;
};

enum ChosenAxis {
    X = 0,
    Y,
    Z,
    NONE
};

class Gizmo {
public:
    Gizmo();
    void manipulateGizmo(Ray& line);
    void findIntersection(Ray& line);
    
    void draw(MTL::RenderCommandEncoder* encoder);
    
private:
    std::vector<Ray> m_axes;
    glm::mat4 m_transformation;
    float m_treshold;
    glm::vec3 m_startPosition;
    ChosenAxis m_currentAxis;
};

namespace gizmo_utils {
    float closestDistanceBetweenLines(Ray& line1, Ray& line2);

}
