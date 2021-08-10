#include "sphere.h"

Sphere::Sphere(float radius, int squares_per_row, bool project) : radius(radius), squares_per_row(squares_per_row), is_project(project) {
    int hori_verts = (squares_per_row + 1) * squares_per_row * 4;
    int cover_verts = (squares_per_row - 1) * (squares_per_row - 1) * 2;
    total_verts = hori_verts + cover_verts;

    set_colour(colour);

    build_vertices();
}

glm::mat3 Sphere::get_tinv() {
    return tinv_model;
}

void Sphere::set_radius(float radius) {
    model[0][0] = radius;
    model[1][1] = radius;
    model[2][2] = radius;

    tinv_model = glm::transpose(glm::inverse(glm::mat3(model)));
}

void Sphere::set_squares(int squares_per_row) {
    this->squares_per_row = squares_per_row;

    int hori_verts = (squares_per_row + 1) * squares_per_row * 4;
    int cover_verts = (squares_per_row - 1) * (squares_per_row - 1) * 2;
    total_verts = hori_verts + cover_verts;

    build_vertices();
}

void Sphere::set_position(glm::vec3 position) {
    model[3] = glm::vec4(position, 1.0f);

    this->position = position;
}

void Sphere::set_colour(glm::vec3 colour) {
    this->colour = colour;
}

void Sphere::project(bool project) {
    is_project = project;
}

void Sphere::draw(const glm::mat4 &vp) {
    glBindVertexArray(vao);
    sphere_shader.use();
    sphere_shader.set_matrix4("vp", vp);
    sphere_shader.set_float("radius", radius);
    sphere_shader.set_matrix4("model", model);
    sphere_shader.set_vector3("colour", colour);
    glDrawElements(GL_TRIANGLES, (int)indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

int Sphere::get_verts() {
    return total_verts;
}

bool Sphere::get_project() {
    return is_project;
}

float Sphere::get_radius() {
    return radius;
}

int Sphere::get_segments() {
    return squares_per_row;
}

void Sphere::build_vertices() {
    // ensure that the arrays are empty
    clear_arrays();

    // assume (0, 0, 0) is the centre of the cube/sphere
    // the corner of the cube is at a distance <radius> away from the centre
    // cube is a box [-side_length / 2, side_length / 2]
    float side_length = (2 * radius) / sqrt(3.0f);

    // we build the cube starting from the 4 vertical faces, then ending with the top and bottom
    // ie. if the cube is a box [-1, 1],
    // then we go (1, 1, 1) -> (1, 1, -1) -> (-1, 1, -1) -> (-1, 1, 1)
    // shift down along the y-axis, then repeat until we hit y=-1
    // then, build the inner verts on the top and bottom faces (y-axis = 1 and -1)

    // first get positions of the vertices in the order as described above
    // repeat until you reach the bottom of the cube
    glm::vec3 cur_pos(side_length / 2, side_length / 2, side_length / 2);
    float step = side_length / squares_per_row;
    for (int y = 0; y < squares_per_row + 1; y++) {
        // face from (1, y, 1)  to (1, y, -1)
        for (int p = 0; p < squares_per_row; p++) {
            add_vertex(cur_pos);
            cur_pos.z -= step;
        }
        // face from (1, y, -1) to (-1, y, -1)
        for (int p = 0; p < squares_per_row; p++) {
            add_vertex(cur_pos);
            cur_pos.x -= step;
        }
        // face from (-1, y, -1) to (-1, y, 1)
        for (int p = 0; p < squares_per_row; p++) {
            add_vertex(cur_pos);
            cur_pos.z += step;
        }
        // face from (-1, y, 1) to (1, y, 1)
        for (int p = 0; p < squares_per_row; p++) {
            add_vertex(cur_pos);
            cur_pos.x += step;
        }
        // move down
        cur_pos.y -= step;
    }

    // calculate the positions of the top vertices
    // we will go from the corner closest to (1, y, 1) to (1, y, -1)
    // and decrease x from 1 to -1
    // first find the corner closest to (1, y, 1)
    cur_pos = glm::vec3(side_length / 2 - step, side_length / 2, side_length / 2 - step);
    for (int x = 0; x < squares_per_row - 1; x++) {
        for (int y = 0; y < squares_per_row - 1; y++) {
            add_vertex(cur_pos);
            cur_pos.z -= step;
        }
        cur_pos.x -= step;
        cur_pos.z = side_length / 2 - step;
    }

    // do the same but for the bottom
    cur_pos = glm::vec3(side_length / 2 - step, -side_length / 2, side_length / 2 - step);
    for (int x = 0; x < squares_per_row - 1; x++) {
        for (int y = 0; y < squares_per_row - 1; y++) {
            add_vertex(cur_pos);
            cur_pos.z -= step;
        }
        cur_pos.x -= step;
        cur_pos.z = side_length / 2 - step;
    }

    // now we have to order them into triangles
    int hori_verts = squares_per_row * 4;
    int cover_verts = (squares_per_row - 1) * (squares_per_row - 1) * 2;
    // the sides are quite straightfoward, each square is made of two triangles
    // the first is (i, i + hori_verts, i + hori_verts + 1)
    // the second is (i, i + hori_verts + 1, i + 1)
    // we need to check for wrapping
    for (int i = 0; i < squares_per_row; i++) {
        int v_off = i * hori_verts;
        for (int j = 0; j < hori_verts; j++) {
            int i1 = j + v_off;
            int i2 = j + hori_verts + v_off;
            int i3 = (j + 1) % hori_verts + hori_verts + v_off;
            int i4 = (j + 1) % hori_verts + v_off;
            add_triangle(i1, i2, i3);
            add_triangle(i1, i3, i4);
        }
    }

    // now we do the covers
    // we can do this by first assuming that we have the indices laid out row by row in order
    // and then create a map to map these "fake" indices back to their original index
    int first_top_index = 4 * squares_per_row * (1 + squares_per_row);
    std::vector<int> top_map((squares_per_row + 1) * (squares_per_row + 1));
    // the first row is going to be the same
    for (int i = 0; i <= squares_per_row; i++) {
        top_map[i] = i;
    }
    // we sort of manually do the middle squares
    // because we know the first top index, we can just continuously add 1 everytime we reference it
    // the sides can be calculated directly
    for (int i = 0; i < squares_per_row - 1; i++) {
        int fake_index = (i + 1) * (squares_per_row + 1);
        top_map[fake_index] = 4 * squares_per_row - (i + 1);
        for (int j = 0; j < squares_per_row - 1; j++) {
            top_map[fake_index + 1 + j] = first_top_index;
            first_top_index++;
        }
        top_map[fake_index + squares_per_row] = squares_per_row + i + 1;
    }
    // the final row
    for (int i = 0; i <= squares_per_row; i++) {
        top_map[(squares_per_row + 1) * squares_per_row + i] = 3 * squares_per_row - i;
    }

    // now we can create the triangles for the top plane
    // the triangles are (i, i + 1, i + 1 + squares_per_row)
    // and (i + 1, i + 2 + squares_per_row, i + 1 + squares_per_row)
    for (int r = 0; r < squares_per_row; r++) {
        for (int i = 0; i < squares_per_row; i++) {
            int i1 = r * (squares_per_row + 1) + i;
            int i2 = i1 + 1;
            int i3 = i1 + (squares_per_row + 1);
            int i4 = i3 + 1;
            add_triangle(top_map[i1], top_map[i2], top_map[i3]);
            add_triangle(top_map[i2], top_map[i4], top_map[i3]);
        }
    }

    // now we do the bottom plane, with the same strategy
    int first_btm_index = 4 * squares_per_row * (1 + squares_per_row) + (squares_per_row - 1) * (squares_per_row - 1);
    int edge_index = 4 * squares_per_row * squares_per_row;
    std::vector<int> btm_map((squares_per_row + 1) * (squares_per_row + 1));

    // the first row
    for (int i = 0; i <= squares_per_row; i++) {
        btm_map[i] = edge_index + i;
    }
    // the middle rows
    for (int i = 0; i < squares_per_row - 1; i++) {
        int fake_index = (i + 1) * (squares_per_row + 1);
        btm_map[fake_index] = 4 * squares_per_row * (squares_per_row + 1) - (i + 1);
        for (int j = 0; j < squares_per_row - 1; j++) {
            btm_map[fake_index + 1 + j] = first_btm_index;
            first_btm_index++;
        }
        btm_map[fake_index + squares_per_row] = 4 * squares_per_row * squares_per_row + squares_per_row + i + 1;
    }
    // final row
    for (int i = 0; i <= squares_per_row; i++) {
        btm_map[(squares_per_row + 1) * squares_per_row + i] = 4 * (squares_per_row + 1) * squares_per_row - squares_per_row - i;
    }

    // finally, we create the triangles
    // the triangles are the same as the top, except in reversed order
    for (int r = 0; r < squares_per_row; r++) {
        for (int i = 0; i < squares_per_row; i++) {
            int i1 = r * (squares_per_row + 1) + i;
            int i2 = i1 + 1;
            int i3 = i1 + (squares_per_row + 1);
            int i4 = i3 + 1;
            add_triangle(btm_map[i3], btm_map[i2], btm_map[i1]);
            add_triangle(btm_map[i3], btm_map[i4], btm_map[i2]);
        }
    }

    // set the total number of indices to draw
    total_indices = (int)indices.size();

    // build the opengl buffers
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * indices.size(), indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
}

void Sphere::add_vertex(glm::vec3 point) {
    vertices.push_back(point.x);
    vertices.push_back(point.y);
    vertices.push_back(point.z);
}

void Sphere::add_triangle(int i1, int i2, int i3) {
    indices.push_back(i1);
    indices.push_back(i2);
    indices.push_back(i3);
}

void Sphere::clear_arrays() {
    std::vector<float>().swap(vertices);
    std::vector<float>().swap(normals);
    std::vector<unsigned int>().swap(indices);
}