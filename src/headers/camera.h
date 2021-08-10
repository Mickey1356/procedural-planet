#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <algorithm>

enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
public:
    Camera(glm::vec3 position, glm::vec3 up, glm::vec3 front, float speed = 10.0f, float sensitivity = 0.1f, float scroll_sensitivity = 1.0f) : position(position), up(up), front(front), speed(speed), sensitivity(sensitivity), scroll_sensitivity(scroll_sensitivity) {
        update_vectors();
    }

    glm::mat4 get_projection(float aspect) {
        this->aspect = aspect;
        return glm::perspective(glm::radians(zoom), aspect, near, far);
    }

    glm::mat4 get_view() {
        return glm::lookAt(position, position + front, up);
    }

    glm::vec3 get_position() {
        return position;
    }

    float get_zoom() {
        return zoom;
    }

    glm::vec4 get_props() {
        return glm::vec4(near, far, aspect, glm::radians(zoom));
    }

    void process_keyboard(CameraMovement direction, float dt) {
        float velocity = speed * dt;
        switch (direction) {
        case FORWARD:
            position += front * velocity;
            break;
        case BACKWARD:
            position -= front * velocity;
            break;
        case LEFT:
            position -= right * velocity;
            break;
        case RIGHT:
            position += right * velocity;
            break;
        case UP:
            position += world_up * velocity;
            break;
        case DOWN:
            position -= world_up * velocity;
            break;
        }
    }

    void process_mouse(float xoff, float yoff) {
        xoff *= sensitivity;
        yoff *= sensitivity;

        yaw += xoff;
        pitch += yoff;

        pitch = std::min(89.0f, std::max(-89.0f, pitch));

        update_vectors();
    }

    void process_scroll(float off) {
        zoom -= off * scroll_sensitivity;
        zoom = std::min(90.0f, std::max(1.0f, zoom));
    }

    void set(float speed, float sensitivity, float scroll_sensitivity, float near, float far) {
        this->speed = speed;
        this->sensitivity = sensitivity;
        this->scroll_sensitivity = scroll_sensitivity;
        this->near = near;
        this->far = far;
    }

private:
    void update_vectors() {
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);

        right = glm::normalize(glm::cross(front, world_up));
        up = glm::normalize(glm::cross(right, front));
    }

    glm::vec3 position, front, up, right;
    glm::vec3 world_up = glm::vec3(0, 1, 0);
    float speed, sensitivity, scroll_sensitivity;
    float yaw = -90, pitch = 0, zoom = 45;
    float near = 0.1f, far = 500;
    float aspect;
};

#endif