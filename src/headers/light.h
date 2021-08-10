#ifndef LIGHT_H
#define LIGHT_H

#include "sphere.h"

const float PI = 3.141592654f;

class Light : public Sphere {
public:
    Light(float radius = 1, int squaresPerRow = 2, glm::vec3 pos = glm::vec3(0, 0, 0)) : Sphere(radius, squaresPerRow, true) {
        position = pos;
        set_position(position);
    }

    glm::vec3 ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    glm::vec3 diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 specular = glm::vec3(1, 1, 1);

    glm::vec3 orbit_around = glm::vec3(0, 0, 0);
    float orbit_distance = 30.0f;
    float orbit_period = 30.0f;
    bool orbiting = false;

    void update(float ct) {
        if (orbiting) {
            float trigo_freq = 2 * PI / orbit_period;

            glm::vec3 orbit_pos = orbit_distance * glm::vec3(cos(ct * trigo_freq), sin(ct * trigo_freq), 0);
            position = orbit_pos + orbit_around;
            set_position(position);
        }
    }
};

#endif