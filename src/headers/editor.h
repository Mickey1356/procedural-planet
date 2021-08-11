#ifndef EDITOR_H
#define EDITOR_H

#include <glm/glm.hpp>
#include <imgui.h>

#include "light.h"
#include "planet.h"

namespace Editor {

void show_editor(Planet &planet, Camera &camera, Light &light, float dt, bool &postprocessing, bool &moving) {
    ImGui::Begin("Parameter Editor");

    ImGui::Text("FPS: %.0f, %.2f ms per frame", 1.0f / dt, dt * 1000);

    if (postprocessing) {
        ImGui::Text("Postprocessing on");
    } else {
        ImGui::Text("Postprocessing off");
    }
    ImGui::SameLine();
    if (ImGui::Button("Toggle postprocessing")) {
        postprocessing = !postprocessing;
    }

    if (ImGui::CollapsingHeader("Camera settings")) {
        static float speed = 10.0f, sens = 0.1f, scroll_sens = 1.0f, near = 0.01f, far = 500;
        ImGui::SliderFloat("Speed", &speed, 1, 20);
        ImGui::SliderFloat("Sensitivity", &sens, 0.1f, 5);
        ImGui::SliderFloat("Scroll sensitivity", &scroll_sens, 1, 10);
        ImGui::SliderFloat("Near plane", &near, 0, 1);
        ImGui::SliderFloat("Far plane", &far, 100, 1000);

        camera.set(speed, sens, scroll_sens, near, far);
    }

    if (ImGui::CollapsingHeader("Lighting")) {
        ImGui::Text("Light position");

        if (light.orbiting) {
            ImGui::Text("Time of day on");
        } else {
            ImGui::Text("Time of day off");
        }
        ImGui::SameLine();
        if (ImGui::Button("Toggle time of day")) {
            light.orbiting = !light.orbiting;
        }

        if (ImGui::SliderFloat3("Light position", (float *)&light.position, -100, 100)) {
            light.set_position(light.position);
        }
        ImGui::SliderFloat3("Orbit around", (float *)&light.orbit_around, -100, 100);
        ImGui::SliderFloat("Orbit distance", &light.orbit_distance, 0, 100);
        ImGui::SliderFloat("Orbit period", &light.orbit_period, 0, 60);

        ImGui::Text("Light colour");
        ImGui::ColorEdit3("Light ambient", (float *)&light.ambient);
        if (ImGui::ColorEdit3("Light diffuse", (float *)&light.diffuse)) {
            light.set_colour(light.diffuse);
        }
        ImGui::ColorEdit3("Light specular", (float *)&light.specular);
    }

    if (ImGui::CollapsingHeader("Cubesphere")) {
        static int n_segments = planet.get_segments();
        static bool project = planet.get_project();
        static float radius = planet.get_radius();
        if (ImGui::SliderInt("Segments", &n_segments, 1, 512)) {
            planet.set_squares(n_segments);
        }
        if (ImGui::SliderFloat("Radius", &radius, 1, 100)) {
            planet.set_radius(radius);
        }
        if (ImGui::Checkbox("Project to sphere", &project)) {
            planet.project(project);
        }

        ImGui::Text("Num verts: %i", planet.get_verts());
    }

    if (ImGui::CollapsingHeader("Terrain")) {
        ImGui::SliderFloat("Noise multiplier", &planet.noise_mult, 0, 1);

        ImGui::Text("Noise parameters");
        ImGui::SliderFloat3("Offset", (float *)&planet.offset, -100, 100);
        ImGui::SliderInt("Octaves", &planet.octaves, 0, 16);
        ImGui::SliderFloat("Scale", &planet.noise_params.x, 0, 2);
        ImGui::SliderFloat("Persistence", &planet.noise_params.y, 0, 2);
        ImGui::SliderFloat("Lacunarity", &planet.noise_params.z, 0, 5);

        ImGui::Text("Normals");
        ImGui::SliderFloat("Normal delta", &planet.noise_params.w, 0.00001f, 0.1f, "%.5f");
        ImGui::SliderFloat("Normal strength", &planet.normal_map_str, 0, 1);

        ImGui::Text("Ocean floor parameters");
        ImGui::SliderFloat("Ocean depth", &planet.ocean_params.x, 0, 1);
        ImGui::SliderFloat("Ocean smoothing", &planet.ocean_params.y, 0, 2);
        ImGui::SliderFloat("Ocean multiplier", &planet.ocean_params.z, 0, 10);

        ImGui::Text("Terrain colours");
        ImGui::ColorEdit3("Grass colour", (float *)&planet.grass_colour);
        ImGui::ColorEdit3("Rock colour", (float *)&planet.rock_colour);
        ImGui::ColorEdit3("Snow colour", (float *)&planet.snow_colour);
        ImGui::ColorEdit3("Shore colour", (float *)&planet.shore_colour);
        ImGui::ColorEdit3("Sea floor colour", (float *)&planet.seafloor_colour);

        ImGui::Text("Blending parameters");
        ImGui::SliderFloat("Slope threshold", &planet.colour_params.x, 0, 1);
        ImGui::SliderFloat("Slope blend", &planet.colour_params.y, 0, 1);
        ImGui::SliderFloat("Snow height", &planet.colour_params.z, 0, 1);
        ImGui::SliderFloat("Snow blend", &planet.colour_params.w, 0, 1);
        ImGui::SliderFloat("Grass height", &planet.colour_params2.x, 0, 1);
        ImGui::SliderFloat("Grass blend", &planet.colour_params2.y, 0, 1);
        ImGui::SliderFloat("Shore height", &planet.colour_params2.z, 0, 1);
        ImGui::SliderFloat("Shore blend", &planet.colour_params2.w, 0, 1);
        ImGui::SliderFloat("Seafloor height", &planet.seafloor_params.x, -1, 1, "%.4f");
        ImGui::SliderFloat("Seafloor blend", &planet.seafloor_params.y, 0, 1);

        ImGui::Text("Terrain lighting");
        ImGui::SliderFloat("Shininess", &planet.shininess, 0, 256);
        ImGui::SliderFloat("Specular strength", &planet.spec_str, 0, 1);
    }

    if (ImGui::CollapsingHeader("Ocean")) {
        ImGui::Text("Ocean parameters");
        ImGui::SliderFloat("Ocean radius", &planet.ocean_radius, 0, 50, "%.4f");
        ImGui::ColorEdit3("Ocean shallow colour", (float *)&planet.ocean_shallow_colour);
        ImGui::ColorEdit3("Ocean deep colour", (float *)&planet.ocean_deep_colour);
        ImGui::SliderFloat("Ocean depth multiplier", &planet.ocean_blends.x, 0, 1);
        ImGui::SliderFloat("Ocean alpha multiplier", &planet.ocean_blends.y, 0, 100);
        ImGui::SliderFloat("Ocean shininess", &planet.ocean_shininess, 0, 256);

        ImGui::Text("Wave parameters");
        ImGui::SliderFloat2("Wave speed", (float *)&planet.ocean_wave_speed, -5, 5);
        ImGui::SliderFloat("Wave strength", &planet.ocean_wave_strength, 0, 1);
    }

    if (ImGui::CollapsingHeader("Atmosphere")) {
        ImGui::SliderFloat("Atmosphere radius", &planet.atmosphere_radius, 0, 50, "%.4f");
        ImGui::SliderInt("In-scatter points", &planet.num_inscatter_pts, 2, 16);
        ImGui::SliderInt("Optical depth points", &planet.num_od_pts, 2, 16);
        ImGui::SliderFloat("Density falloff", &planet.density_falloff, -20, 20);
        ImGui::SliderFloat("Scatter strength", &planet.scatter_str, -20, 50);

        ImGui::Text("Wavelengths");
        ImGui::SliderFloat("Red wavelength (nm)", &planet.rgb_wavelengths.x, 0, 1000);
        ImGui::SliderFloat("Green wavelength (nm)", &planet.rgb_wavelengths.y, 0, 1000);
        ImGui::SliderFloat("Blue wavelength (nm)", &planet.rgb_wavelengths.z, 0, 1000);
    }

    if (ImGui::CollapsingHeader("Clouds")) {
        ImGui::SliderFloat("Min cloud radius", &planet.cloud_radii.x, 0, 50, "%.4f");
        ImGui::SliderFloat("Max cloud radius", &planet.cloud_radii.y, 0, 50, "%.4f");
        ImGui::SliderInt("Cloud density points", &planet.num_cloud_pts, 2, 16);
        ImGui::SliderFloat3("Cloud speed", (float *)&planet.cloud_speed, -5, 5);

        ImGui::Text("Cloud Noise");
        ImGui::SliderInt("Octaves", &planet.cloud_noise_octaves, 0, 16);
        ImGui::SliderFloat("Scale", &planet.cloud_noise.x, 0, 2);
        ImGui::SliderFloat("Persistence", &planet.cloud_noise.y, 0, 2);
        ImGui::SliderFloat("Lacunarity", &planet.cloud_noise.z, 0, 5);

        ImGui::Text("Cloud Lighting");
        ImGui::SliderInt("Cloud light points", &planet.num_cloud_light_pts, 2, 16);
        ImGui::SliderFloat("Cloud transmittance", &planet.cloud_transmittance, -20, 20);
        ImGui::SliderFloat("HG parameter", &planet.hg_g, -1, 1);
        ImGui::SliderFloat("Extinction factor", &planet.extinction, -20, 20);
    }

    ImGui::End();
}

} // namespace Editor

#endif