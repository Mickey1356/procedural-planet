#ifndef SPHERE_H
#define SPHERE_H

#include <glm/glm.hpp>

#include <vector>

#include "shader.h"

class Sphere {
public:
    Sphere(float radius = 1, int squaresPerRow = 2, bool project = true);

    glm::mat3 get_tinv();

    void set_radius(float radius);
    void set_squares(int squares_per_row);
    void set_position(glm::vec3 position);
    void set_colour(glm::vec3 colour);

    void project(bool project);

    // void draw(const glm::mat4 &vp, const glm::vec3 &cam_pos, const glm::vec3 &light_pos);
    void draw(const glm::mat4 &vp);

    int get_verts();
    bool get_project();
    float get_radius();
    int get_segments();

    glm::vec3 position = glm::vec3(0);

protected:
    bool is_project = false;
    float radius;
    int total_indices = 0;

    unsigned int vao, vbo, ebo;

    glm::mat4 model = glm::mat4(1);
    glm::mat3 tinv_model = glm::mat3(1);

private:
    void build_vertices();

    void add_vertex(glm::vec3 point);
    void add_triangle(int i1, int i2, int i3);
    void clear_arrays();

    int squares_per_row, total_verts;

    glm::vec3 colour = glm::vec3(0.5f, 0.5f, 0.5f);

    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned int> indices;

    Shader sphere_shader = Shader("data/shaders/sphere.vert", "data/shaders/sphere.frag");
};

#endif