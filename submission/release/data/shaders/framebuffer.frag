#version 460 core

out vec4 FragColour;

in vec2 TexCoords; // (0, 0) is btm-left

uniform vec4 near_far; // near-far (xy) aspect (z) zoom (w)
uniform vec3 cam_pos;
uniform vec3 planet_pos;
uniform vec3 radii; // ocean radius (x) atmosphere radius (y) planet radius (z)

uniform vec3 ocean_shallow;
uniform vec3 ocean_deep;
uniform vec2 ocean_blends;

uniform vec2 ocean_wave_speed;
uniform float ocean_wave_strength;
uniform float ocean_shininess;

uniform float time; // time in seconds since first frame

uniform mat4 ip;
uniform mat4 iv;

uniform sampler2D screenTex;
uniform sampler2D depthTex;

uniform sampler2D water_normal_map;

struct Light {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform Light light;
uniform mat3 tinv;

uniform int num_inscatter_pts;
uniform int num_od_pts;
uniform float density_falloff;
uniform vec3 rgb_scatter;

uniform vec2 cloud_radii;
uniform int num_cloud_pts;
uniform int num_cloud_light_pts;

uniform int cloud_noise_octaves;
uniform vec3 cloud_speed;
uniform vec3 cloud_noise;
uniform float cloud_transmittance;
uniform float hg_g;
uniform float extinction;

const float epsilon = 1e-3;
const float pi = 3.141592654;

// noise functions from https://github.com/ashima/webgl-noise
vec3 mod289(vec3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
    return mod289(((x*34.0)+10.0)*x);
}

vec4 taylorInvSqrt(vec4 r) {
    return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v) {
    const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
    const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

    // First corner
    vec3 i  = floor(v + dot(v, C.yyy) );
    vec3 x0 =   v - i + dot(i, C.xxx) ;

    // Other corners
    vec3 g = step(x0.yzx, x0.xyz);
    vec3 l = 1.0 - g;
    vec3 i1 = min( g.xyz, l.zxy );
    vec3 i2 = max( g.xyz, l.zxy );

    //   x0 = x0 - 0.0 + 0.0 * C.xxx;
    //   x1 = x0 - i1  + 1.0 * C.xxx;
    //   x2 = x0 - i2  + 2.0 * C.xxx;
    //   x3 = x0 - 1.0 + 3.0 * C.xxx;
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
    vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

    // Permutations
    i = mod289(i); 
    vec4 p = permute(permute(permute( 
              i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
            + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
            + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

    // Gradients: 7x7 points over a square, mapped onto an octahedron.
    // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
    float n_ = 0.142857142857; // 1.0/7.0
    vec3  ns = n_ * D.wyz - D.xzx;

    vec4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

    vec4 x_ = floor(j * ns.z);
    vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

    vec4 x = x_ *ns.x + ns.yyyy;
    vec4 y = y_ *ns.x + ns.yyyy;
    vec4 h = 1.0 - abs(x) - abs(y);

    vec4 b0 = vec4( x.xy, y.xy );
    vec4 b1 = vec4( x.zw, y.zw );

    //vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
    //vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
    vec4 s0 = floor(b0)*2.0 + 1.0;
    vec4 s1 = floor(b1)*2.0 + 1.0;
    vec4 sh = -step(h, vec4(0.0));

    vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
    vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

    vec3 p0 = vec3(a0.xy,h.x);
    vec3 p1 = vec3(a0.zw,h.y);
    vec3 p2 = vec3(a1.xy,h.z);
    vec3 p3 = vec3(a1.zw,h.w);

    //Normalise gradients
    vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    vec4 m = max(0.5 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
    m = m * m;
    return 105.0 * dot(m * m, vec4(dot(p0,x0), dot(p1,x1), dot(p2,x2), dot(p3,x3)));
}

float linear_depth(float depth) {
    float near = near_far.x;
    float far = near_far.y;
    float d = 2.0 * depth - 1.0;
    return 2.0 * near * far / (far + near - d * (far - near));
}

vec3 get_view_vector(vec2 pixel) {
    vec3 vv = vec3(ip * vec4(pixel * 2.0 - 1.0, 0.0, 1.0));
    return vec3(iv * vec4(vv, 0.0));
}

vec2 ray_sphere(vec3 centre, float radius, vec3 origin, vec3 direction) {
    vec3 off = origin - centre;
    float a = dot(direction, direction);
    float b = 2.0 * dot(off, direction);
    float c = dot(off, off) - radius * radius;
    float d = b * b - 4.0 * a * c;
    if (d > 0.0) {
        float s = sqrt(d);
        float near = max(0.0, (-b - s) / (2.0 * a));
        float far = (-b + s) / (2.0 * a);

        if (far >= 0) {
            return vec2(near, far - near);
        }
    }
    return vec2(1e9, 0.0);
}

float density_at_pt(vec3 pt) {
    float ht_above_surface = length(pt - planet_pos) - radii.z;
    float ht01 = ht_above_surface / (radii.y - radii.z);
    float density = exp(-ht01 * density_falloff) * (1 - ht01);
    return density;
}

float optical_depth(vec3 origin, vec3 dir, float ray_length) {
    vec3 pt = origin;
    float stepsize = ray_length / (num_od_pts - 1);
    float od = 0.0;
    for (int i = 0; i < num_od_pts; i++) {
        float density = density_at_pt(pt);
        od += density * stepsize;
        pt += dir * stepsize;
    }
    return od;
}

vec3 calculate_light(vec3 origin, vec3 dir, float ray_length, vec3 orig_colour) {
    vec3 inscatter_pt = origin;
    float stepsize = ray_length / (num_inscatter_pts - 1);
    vec3 in_light = vec3(0.0);
    float view_ray_od = 0.0;


    for (int i = 0; i < num_inscatter_pts; i++) {
        vec3 light_dir = normalize(light.position - inscatter_pt);
        float sun_ray_length = ray_sphere(planet_pos, radii.y, inscatter_pt, light_dir).y;
        float sun_ray_od = optical_depth(inscatter_pt, light_dir, sun_ray_length);
        view_ray_od = optical_depth(inscatter_pt, -dir, stepsize * i);
        vec3 transmittance = exp(-(sun_ray_od + view_ray_od) * rgb_scatter);
        float local_density = density_at_pt(inscatter_pt);

        in_light += local_density * transmittance * rgb_scatter * stepsize;
        inscatter_pt += dir * stepsize;
    }

    float orig_colour_transmittance = exp(-view_ray_od);

    return orig_colour * orig_colour_transmittance + in_light;
}

float fbm(vec3 pos) {
    float frequency = cloud_noise.x;
    float persistence = cloud_noise.y;
    float lacunarity = cloud_noise.z;

    float nsum = 0.0;
    float amplitude = 1.0;
    float total_amp = 0.0;

    vec3 offsets = time / 20.0 * cloud_speed;

    for (int i = 0; i < cloud_noise_octaves; i++) {
        nsum += snoise(pos * frequency + offsets) * amplitude;
        total_amp += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    return nsum / total_amp;
}

float cloud_density_at_pt(vec3 pt) {
    float ht = length(pt - planet_pos);
    // float h = step(cloud_radii.x, ht) - step(cloud_radii.y, ht);
    float h = 2.0 * clamp(ht - cloud_radii.x, 0.0, cloud_radii.y - cloud_radii.x) / (cloud_radii.y - cloud_radii.x) - 1.0;
    h = 1.0 - h * h;
    if (h > 0) {
        return h * fbm(pt);
    }
    return 0;
}

float hg(float a) {
    float g2 = hg_g * hg_g;
    return (1 - g2) / (4 * pi * pow(1 + g2 - 2 * hg_g * a, 1.5));
}

// calculate how much light travels along dir to reach origin
float lightmarch(vec3 pt) {
    vec3 light_dir = normalize(light.position - pt);
    float dst_thr = ray_sphere(planet_pos, cloud_radii.y, pt, light_dir).y;
    vec3 cloud_pt = pt;
    float total_density = 0;

    float stepsize = dst_thr / (num_cloud_light_pts - 1);
    for (int i = 0; i < num_cloud_light_pts; i++) {
        total_density += max(0, cloud_density_at_pt(cloud_pt)) * stepsize;
        cloud_pt += light_dir * stepsize;
    }
    return exp(-cloud_transmittance * total_density);
}

vec3 calculate_clouds(vec3 origin, vec3 dir, float ray_length, vec3 orig_colour) {
    vec3 cloud_pt = origin;
    float stepsize = ray_length / (num_cloud_pts - 1);
    vec3 in_light = vec3(0.0);
    float transmittance = 1;

    for (int i = 0; i < num_cloud_pts; i++) {
        float cos_angle = dot(dir, normalize(light.position - cloud_pt));
        float hg_factor = hg(cos_angle);
        float density = cloud_density_at_pt(cloud_pt);

        if (density > 0) {
            float lt = lightmarch(cloud_pt);
            in_light += density * stepsize * transmittance * lt * hg_factor;
            transmittance *= exp(-density * stepsize * extinction);

            if (transmittance < 0.01) {
                break;
            }
        }
        cloud_pt += dir * stepsize;
    }
    return orig_colour * transmittance + in_light;
}

vec3 get_normal_from_texture(vec2 uv) {
    vec3 rgb = texture(water_normal_map, uv).rgb;
    return normalize(rgb * 2.0 - 1.0);
}

vec3 rnm(vec3 a, vec3 b) {
    a += vec3(0.0, 0.0, 1.0);
    b *= vec3(-1.0, -1.0, 1.0);
    return a * dot(a, b) / a.z - b;
}

void main() {
    vec4 colour = texture(screenTex, TexCoords);
    float depth = texture(depthTex, TexCoords).r;

    vec3 view_vector = get_view_vector(TexCoords);
    vec3 cam_dir = normalize(view_vector);

    float scene_depth = linear_depth(depth);
    scene_depth *= length(view_vector);

    // render an ocean
    vec2 ocean_hit_info = ray_sphere(planet_pos, radii.x, cam_pos, cam_dir);
    float ocean_dst_to = ocean_hit_info.x;
    float ocean_dst_thr = ocean_hit_info.y;

    float ovd = min(ocean_dst_thr, scene_depth - ocean_dst_to);
    vec3 ocean_pt = cam_pos + cam_dir * ocean_dst_to;

    // calculate the colour of the ocean
    if (ovd > 0) {
        float t = 1.0 - exp(-ovd * ocean_blends.x);
        float alpha = 1.0 - exp(-ovd * ocean_blends.y);

        vec3 ocean_colour = mix(ocean_shallow, ocean_deep, t);

        vec3 ocean_normal = normalize(tinv * (ocean_pt - planet_pos));
        vec3 light_dir = normalize(light.position - planet_pos);

        // sample from normal map for diffuse/specular calculation
        // triplanar mapping again
        vec3 nn = normalize(tinv * ocean_normal);

        vec3 blend = abs(nn);
        blend = max(blend - 0.2, 0.0);
        blend /= dot(blend, vec3(1.0));

        // make the waves move
        vec2 offsets = time / 20.0 * ocean_wave_speed;

        vec2 uvx = ocean_pt.zy + offsets;
        vec2 uvy = ocean_pt.xz + offsets;
        vec2 uvz = ocean_pt.xy + offsets;

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
        norm = normalize(mix(ocean_normal, norm, ocean_wave_strength));

        // diffuse colour
        float diff = clamp(dot(norm, light_dir), 0.0, 1.0);
        vec3 diffuse = light.diffuse * diff;

        // specular highlights
        vec3 viewDir = normalize(cam_pos - ocean_pt);
        vec3 reflectDir = reflect(-light_dir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), ocean_shininess);
        vec3 specular = light.specular * spec;

        ocean_colour = (diffuse + specular) * ocean_colour;

        vec4 fcolour = mix(colour, vec4(ocean_colour, 1.0), alpha);
        colour = fcolour;
    }

    // distance from camera to the planet surface/ocean
    float surface_dst = min(scene_depth, ocean_dst_to);

    // render the cloud layer
    vec2 cloud_hit_info = ray_sphere(planet_pos, cloud_radii.y, cam_pos, cam_dir);
    float cloud_dst_to = cloud_hit_info.x;
    float cloud_dst_thr = cloud_hit_info.y;

    float cvd = min(cloud_dst_thr, surface_dst - cloud_dst_to);

    if (cvd > 0) {
        vec3 cloud_pt = cam_pos + cam_dir * (cloud_dst_to + epsilon);
        vec3 light = calculate_clouds(cloud_pt, cam_dir, cvd - 2.0 * epsilon, vec3(colour));
        colour = vec4(light, 1.0);
    }

    // render an atmosphere
    vec2 atmosphere_hit_info = ray_sphere(planet_pos, radii.y, cam_pos, cam_dir);
    float atmos_dst_to = atmosphere_hit_info.x;
    float atmos_dst_thr = atmosphere_hit_info.y;

    float avd = min(atmos_dst_thr, surface_dst - atmos_dst_to);

    if (avd > 0) {
        vec3 atmos_pt = cam_pos + cam_dir * (atmos_dst_to + epsilon);
        vec3 light = calculate_light(atmos_pt, cam_dir, avd - 2.0 * epsilon, vec3(colour));
        colour = vec4(light, 1.0);
    }

    FragColour = colour;
}