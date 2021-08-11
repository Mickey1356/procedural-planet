#version 460 core

in float localHt;
in vec3 position;
in vec3 localUp;
in vec3 normal;
in vec3 spherePos;

out vec4 FragColour;

uniform vec3 camera_pos;
uniform mat3 tinv_mdl;

uniform vec3 grass_colour;
uniform vec3 rock_colour;
uniform vec3 snow_colour;
uniform vec3 shore_colour;
uniform vec3 seafloor_colour;
uniform vec4 colour_params;
uniform vec4 colour_params2;
uniform vec2 seafloor_params;

uniform float normal_map_str;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Light light;
uniform float shininess;
uniform float spec_str;

uniform sampler2D terrain_normal_map;

const float pi = 3.141592654;

float saturate(float x) {
    return min(1.0, max(0.0, x));
}

vec3 get_normal_from_texture(vec2 uv) {
    vec3 rgb = texture(terrain_normal_map, uv).rgb;
    return normalize(rgb * 2.0 - 1.0);
}

vec3 rnm(vec3 a, vec3 b) {
    a += vec3(0, 0, 1);
    b *= vec3(-1, -1, 1);
    return a * dot(a, b) / a.z - b;
}

void main() {
    float steepness = 1.0 - dot(normal, localUp);
    // steepness = min(1.0, max(0.0, steepness / 0.6));

    float slope_threshold = colour_params.x;
    float slope_blend = colour_params.y;
    float snow_threshold = colour_params.z;
    float snow_blend = colour_params.w;
    float grass_threshold = colour_params2.x;
    float grass_blend = colour_params2.y;
    float shore_threshold = colour_params2.z;
    float shore_blend = colour_params2.w;
    float seafloor_threshold = seafloor_params.x;
    float seafloor_blend = seafloor_params.y;

    // grass only when the terrain is relatively flat
    float slope_blend_ht = slope_threshold * (1.0 - slope_blend);
    float slope_weight = saturate((steepness - slope_blend_ht) / (slope_threshold - slope_blend_ht));
    vec3 landCol = mix(grass_colour, rock_colour, slope_weight);

    // grass only below a certain elevation
    float grass_blend_ht = grass_threshold * (1.0 - grass_blend);
    float grass_weight = saturate((localHt - grass_blend_ht) / (grass_threshold - grass_blend_ht));
    landCol = mix(landCol, rock_colour, grass_weight);

    // shore only below a certain elevation
    float shore_blend_ht = shore_threshold * (1.0 - shore_blend);
    float shore_weight = saturate((localHt - shore_blend_ht) / (shore_threshold - shore_blend_ht));
    landCol = mix(shore_colour, landCol, shore_weight);
 
    // seafloor when below 0
    float seafloor_blend_ht = seafloor_threshold * (1.0 - seafloor_blend);
    float seafloor_weight = saturate((localHt - seafloor_blend_ht) / (seafloor_threshold - seafloor_blend_ht));
    landCol = mix(seafloor_colour, landCol, seafloor_weight);

    // snow past a certain elevation
    float snow_blend_ht = snow_threshold * (1.0 - snow_blend);
    float snow_weight = saturate((localHt - snow_blend_ht) / (snow_threshold - snow_blend_ht));
    landCol = mix(landCol, snow_colour, snow_weight);

    // compute triplanar mapping and compute normal from normal map
    vec3 nn = normalize(tinv_mdl * normal);

    vec3 blend = abs(nn);
    blend = max(blend - 0.2, 0);
    blend /= dot(blend, vec3(1.0));

    vec2 uvx = position.zy;
    vec2 uvy = position.xz;
    vec2 uvz = position.xy;

    vec3 tnx = get_normal_from_texture(uvx);
    vec3 tny = get_normal_from_texture(uvy);
    vec3 tnz = get_normal_from_texture(uvz);

    vec3 avn = abs(nn);
    tnx = rnm(vec3(nn.zy, avn.x), tnx);
    tny = rnm(vec3(nn.xz, avn.y), tny);
    tnz = rnm(vec3(nn.xy, avn.z), tnz);

    vec3 asign = sign(nn);
    tnx.z *= asign.x;
    tny.z *= asign.y;
    tnz.z *= asign.z;

    vec3 norm = normalize(tnx * blend.x + tny * blend.y + tnz * blend.z + nn);
    // norm = norm * 0.5 + 0.5;

    // compute spherical mapping (assignment 3)
    // float theta = atan(spherePos.y, spherePos.x);
    // float phi = 0.5 + atan(spherePos.y / sin(theta), spherePos.z) / (2 * pi);
    // theta = 0.5 + theta / (2 * pi);
    // vec3 norm = normalize(tinv_mdl * get_normal_from_texture(vec2(theta, phi)));
    
    // use the vertex normal
    // vec3 norm = normalize(tinv_mdl * normal);
    norm = mix(nn, norm, normal_map_str);

    // phong lighting
    vec3 ambient = light.ambient;

    vec3 lightDir = normalize(light.position - position);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff;

    vec3 viewDir = normalize(camera_pos - position);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = light.specular * spec * spec_str;

    vec3 result = (ambient + diffuse + specular) * landCol;
    FragColour = vec4(result, 1.0);
}