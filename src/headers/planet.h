#ifndef PLANET_H
#define PLANET_H

#include "sphere.h"
#include "light.h"

class Planet : public Sphere {
public:
    Planet(float radius = 1, int squaresPerRow = 2);

    void draw(const glm::mat4 &vp, const glm::vec3 &cam_pos, const Light light);

    glm::vec3 get_position();
    glm::vec3 get_radii();
    glm::vec3 get_scatter();

    // noise parameters
    // float noise_mult = 0.0f;
    float noise_mult = 0.23f;
    glm::vec3 offset = glm::vec3(0);
    int octaves = 11;
    glm::vec4 noise_params = glm::vec4(1, 0.5f, 2, 0.01f);
    float normal_map_str = 1;

    // ocean floor
    glm::vec3 ocean_params = glm::vec3(0.5f, 0.05f, 3.2f);

    // ocean
    float ocean_radius = 0.993f;
    glm::vec3 ocean_shallow_colour = glm::vec3(0.22f, 0.34f, 0.44f);
    glm::vec3 ocean_deep_colour = glm::vec3(0.001f, 0.026f, 0.27f);
    glm::vec2 ocean_blends = glm::vec2(0.7f, 50);
    glm::vec2 ocean_wave_speed = glm::vec2(0.3f, 0.3f);
    float ocean_wave_strength = 0.5f;
    float ocean_shininess = 256;

    // terrain colours
    glm::vec3 grass_colour = glm::vec3(0.31f, 0.34f, 0);
    glm::vec3 rock_colour = glm::vec3(0.23f, 0.11f, 0.07f);
    glm::vec3 snow_colour = glm::vec3(0.9f, 0.9f, 0.9f);
    glm::vec3 shore_colour = glm::vec3(0.62f, 0.55f, 0.28f);
    glm::vec3 seafloor_colour = glm::vec3(0.28f, 0.27f, 0.24f);
    glm::vec4 colour_params = glm::vec4(0.16f, 0.77f, 0.45f, 0.14f);
    glm::vec4 colour_params2 = glm::vec4(0.38f, 0.34f, 0.14f, 0.75f);
    glm::vec2 seafloor_params = glm::vec2(0.05f, 0.9f);

    // lighting
    float shininess = 1;
    float spec_str = 1;

    // atmosphere
    float atmosphere_radius = 2.5f;
    int num_inscatter_pts = 10;
    int num_od_pts = 10;
    float density_falloff = 10;
    glm::vec3 rgb_wavelengths = glm::vec3(700, 530, 440);
    float scatter_str = 20;

    // clouds
    glm::vec2 cloud_radii = glm::vec2(1.2f, 1.5f);
    int num_cloud_pts = 4;
    int cloud_noise_octaves = 6;
    glm::vec3 cloud_speed = glm::vec3(1);
    glm::vec3 cloud_noise = glm::vec3(1.2, 0.5f, 2);
    int num_cloud_light_pts = 4;
    float cloud_transmittance = 1.5f;
    float hg_g = 0.85f;
    float extinction = -3.3f;

private:
    Shader planet_shader = Shader("data/shaders/planet.vert", "data/shaders/planet.frag");
    Shader cube_shader = Shader("data/shaders/default.vert", "data/shaders/default.frag");

    unsigned int normal_tex;
};

#endif