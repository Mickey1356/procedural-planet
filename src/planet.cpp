#include "planet.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Planet::Planet(float radius, int squaresPerRow) : Sphere(radius, squaresPerRow) {
    glGenTextures(1, &normal_tex);
    glBindTexture(GL_TEXTURE_2D, normal_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int w, h, c;
    // unsigned char *data = stbi_load("data/textures/flat.png", &w, &h, &c, 0);
    unsigned char *data = stbi_load("data/textures/terrain_normal_map.jpg", &w, &h, &c, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else {
        std::cout << "Failed to load terrain normal map" << std::endl;
    }
    stbi_image_free(data);
}

void Planet::draw(const glm::mat4 &vp, const glm::vec3 &cam_pos, const Light light) {
    glBindVertexArray(vao);
    if (is_project) {
        planet_shader.use();
        planet_shader.set_matrix4("vp", vp);
        planet_shader.set_float("radius", radius);
        planet_shader.set_matrix4("model", model);

        // noise parameters
        planet_shader.set_float("noise_mult", noise_mult);
        planet_shader.set_vector3("offset", offset);
        planet_shader.set_int("octaves", octaves);
        planet_shader.set_vector4("noise_params", noise_params);
        planet_shader.set_vector3("ocean_params", ocean_params);
        planet_shader.set_float("normal_map_str", normal_map_str);

        // terrain colours
        planet_shader.set_vector3("grass_colour", grass_colour);
        planet_shader.set_vector3("rock_colour", rock_colour);
        planet_shader.set_vector3("snow_colour", snow_colour);
        planet_shader.set_vector3("shore_colour", shore_colour);
        planet_shader.set_vector3("seafloor_colour", seafloor_colour);
        planet_shader.set_vector4("colour_params", colour_params);
        planet_shader.set_vector4("colour_params2", colour_params2);
        planet_shader.set_vector2("seafloor_params", seafloor_params);

        // lighting parameters
        planet_shader.set_vector3("camera_pos", cam_pos);
        planet_shader.set_matrix3("tinv_mdl", tinv_model);
        planet_shader.set_vector3("light.position", light.position);
        planet_shader.set_vector3("light.ambient", light.ambient);
        planet_shader.set_vector3("light.diffuse", light.diffuse);
        planet_shader.set_vector3("light.specular", light.specular);
        planet_shader.set_float("shininess", shininess);
        planet_shader.set_float("spec_str", spec_str);

        // normal map
        planet_shader.set_int("terrain_normal_map", 0);
    } else {
        cube_shader.use();
        cube_shader.set_matrix4("vp", vp);
        cube_shader.set_matrix4("model", model);
    }
    // glDrawArrays(GL_POINTS, 0, total_verts);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, normal_tex);
    glDrawElements(GL_TRIANGLES, total_indices, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

glm::vec3 Planet::get_position() {
    return position;
}

glm::vec3 Planet::get_radii() {
    return glm::vec3(ocean_radius, atmosphere_radius, radius);
}

glm::vec3 Planet::get_scatter() {
    float r = (float) pow(400 / rgb_wavelengths.x, 4);
    float g = (float) pow(400 / rgb_wavelengths.y, 4);
    float b = (float) pow(400 / rgb_wavelengths.z, 4);
    return scatter_str * glm::vec3(r, g, b);
}