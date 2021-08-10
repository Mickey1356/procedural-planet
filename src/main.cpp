
#include <glad/glad.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>

#include "stb_image.h"

#include "camera.h"
#include "editor.h"
#include "light.h"
#include "planet.h"
#include "sphere.h"

#include <glm/gtx/string_cast.hpp>

const int SCR_WIDTH = 1600;
const int SCR_HEIGHT = 900;
const float ASPECT_RATIO = (float)SCR_WIDTH / (float)SCR_HEIGHT;

// create a camera
Camera camera(glm::vec3(0, 0, 10), glm::vec3(0, 1, 0), glm::vec3(0, 0, -1));

// timers for delta time
float dt = 0, lt = 0;

// mouse variables
bool first_mouse = true, capturing = false;
float last_x, last_y;

// whether to render postprocessing or not
bool wireframe = false;
bool postprocessing = false;

// whether to move the sun or not
bool moving = false;

// forward declarations for inputs
void process_input(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoff, double yoff);

int main() {
    // glfw/opengl setup
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Procedural Planet", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialise GLAD" << std::endl;
        glfwTerminate();
        return 1;
    }

    // imgui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460 core");
    ImGui::StyleColorsDark();

    // create the window and capture inputs
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // enable some rendering options
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);

    // create a framebuffer for the post processing effects
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // generate a texture to which we would render the scene to
    unsigned int texColourBuffer;
    glGenTextures(1, &texColourBuffer);
    // glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texColourBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach the texture to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texColourBuffer, 0);

    // create a depth texture
    unsigned int texDepthBuffer;
    glGenTextures(1, &texDepthBuffer);
    // glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texDepthBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // attach depth texture to framebuffer
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texDepthBuffer, 0);

    // check to make sure framebuffer is ok
    auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is not complete: " << fboStatus << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // load the quad on which we will render the texture on
    float quad_verts[] = {
        -1, 1, 0, 1,
        -1, -1, 0, 0,
        1, -1, 1, 0,
        -1, 1, 0, 1,
        1, -1, 1, 0,
        1, 1, 1, 1};
    unsigned int quad_vao, quad_vbo;
    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);
    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), &quad_verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindVertexArray(0);

    // load the framebuffer shader
    Shader screen_shader("data/shaders/framebuffer.vert", "data/shaders/framebuffer.frag");
    screen_shader.use();
    screen_shader.set_int("screenTex", 0);
    screen_shader.set_int("depthTex", 1);
    screen_shader.set_int("water_normal_map", 2);

    // load normal map texture for ocean
    unsigned int water_normal_tex;
    glGenTextures(1, &water_normal_tex);
    glBindTexture(GL_TEXTURE_2D, water_normal_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int w, h, c;
    unsigned char *data = stbi_load("data/textures/water_normal_map.png", &w, &h, &c, 0);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    } else {
        std::cout << "Failed to load water normal map" << std::endl;
        return 0;
    }
    stbi_image_free(data);

    // create the sphere for the planet
    Planet planet(1, 512);

    // create a sphere for the light (sun)
    Light sun(0.5f, 8, glm::vec3(30, 0, 0));

    while (!glfwWindowShouldClose(window)) {
        float ct = (float)glfwGetTime();
        dt = ct - lt;
        lt = ct;

        process_input(window);
        glfwPollEvents();

        // calcualte camera matrices
        glm::mat4 vp, ivp, ip, iv;
        // projection = camera.get_projection(ASPECT_RATIO);
        // view = camera.get_view();
        vp = camera.get_projection(ASPECT_RATIO) * camera.get_view();
        iv = glm::inverse(camera.get_view());
        ip = glm::inverse(camera.get_projection(ASPECT_RATIO));
        ivp = glm::inverse(vp);

        // std::cout << glm::to_string(vp) << std::endl;

        // first pass (render to texture)
        if (postprocessing) {
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        }
        // allow for wireframe mode even when post-processing is turned on
        if (wireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        // glEnable(GL_CULL_FACE);
        planet.draw(vp, camera.get_position(), sun);

        sun.update(ct);
        sun.draw(vp);

        // second pass (render from texture)
        if (postprocessing) {
            // turn this back into fill (so we dont draw triangles of the quad)
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(1, 1, 1, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // set the framebuffer shader parameters
            screen_shader.use();
            // general parameters
            screen_shader.set_vector3("cam_pos", camera.get_position());
            screen_shader.set_vector4("near_far", camera.get_props());
            screen_shader.set_vector3("planet_pos", planet.get_position());
            screen_shader.set_vector3("radii", planet.get_radii());

            // ocean colours
            screen_shader.set_vector3("ocean_shallow", planet.ocean_shallow_colour);
            screen_shader.set_vector3("ocean_deep", planet.ocean_deep_colour);
            screen_shader.set_vector2("ocean_blends", planet.ocean_blends);
            screen_shader.set_float("ocean_shininess", planet.ocean_shininess);

            // wave
            screen_shader.set_vector2("ocean_wave_speed", planet.ocean_wave_speed);
            screen_shader.set_float("ocean_wave_strength", planet.ocean_wave_strength);

            screen_shader.set_float("time", ct);

            screen_shader.set_matrix4("ip", ip);
            screen_shader.set_matrix4("iv", iv);

            // lights (for ocean lighting)
            screen_shader.set_vector3("light.position", sun.position);
            screen_shader.set_vector3("light.ambient", sun.ambient);
            screen_shader.set_vector3("light.diffuse", sun.diffuse);
            screen_shader.set_vector3("light.specular", sun.specular);
            screen_shader.set_matrix3("tinv", planet.get_tinv());

            // atmosphere
            screen_shader.set_int("num_inscatter_pts", planet.num_inscatter_pts);
            screen_shader.set_int("num_od_pts", planet.num_od_pts);
            screen_shader.set_float("density_falloff", planet.density_falloff);
            screen_shader.set_vector3("rgb_scatter", planet.get_scatter());

            // clouds
            screen_shader.set_vector2("cloud_radii", planet.cloud_radii);
            screen_shader.set_int("num_cloud_pts", planet.num_cloud_pts);

            screen_shader.set_int("cloud_noise_octaves", planet.cloud_noise_octaves);
            screen_shader.set_vector3("cloud_speed", planet.cloud_speed);
            screen_shader.set_vector3("cloud_noise", planet.cloud_noise);

            screen_shader.set_int("num_cloud_light_pts", planet.num_cloud_light_pts);
            screen_shader.set_float("cloud_transmittance", planet.cloud_transmittance);
            screen_shader.set_float("hg_g", planet.hg_g);
            screen_shader.set_float("extinction", planet.extinction);

            glBindVertexArray(quad_vao);

            glDisable(GL_DEPTH_TEST);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texColourBuffer);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texDepthBuffer);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, water_normal_tex);

            glDrawArrays(GL_TRIANGLES, 0, 6);
            glBindVertexArray(0);
            glDisable(GL_BLEND);
        }

        // imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // show editor
        Editor::show_editor(planet, camera, sun, dt, postprocessing, moving);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // swap buffers and draw
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();

    return 0;
}

void process_input(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // capture cursor or release it
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        capturing = true;
    }
    if (glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        capturing = false;
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        wireframe = true;
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        wireframe = false;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        postprocessing = true;
    }
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        postprocessing = false;
    }

    ImGuiIO &io = ImGui::GetIO();
    if (!capturing && (io.WantCaptureMouse || io.WantCaptureKeyboard)) return;

    // camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.process_keyboard(FORWARD, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.process_keyboard(BACKWARD, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.process_keyboard(LEFT, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.process_keyboard(RIGHT, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.process_keyboard(UP, dt);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        camera.process_keyboard(DOWN, dt);
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (first_mouse) {
        last_x = (float)xpos;
        last_y = (float)ypos;
        first_mouse = false;
    }

    ImGuiIO &io = ImGui::GetIO();
    if (!capturing && io.WantCaptureMouse) return;

    float xoff = (float)xpos - last_x;
    float yoff = last_y - (float)ypos;

    last_x = (float)xpos;
    last_y = (float)ypos;

    if (capturing) {
        camera.process_mouse(xoff, yoff);
    }
}

void scroll_callback(GLFWwindow *window, double xoff, double yoff) {
    ImGuiIO &io = ImGui::GetIO();
    if (capturing || !io.WantCaptureMouse)
        camera.process_scroll((float)yoff);
    else
        ImGui_ImplGlfw_ScrollCallback(window, xoff, yoff);
}
