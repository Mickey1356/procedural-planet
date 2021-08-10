#version 460 core

layout (location = 0) in vec3 aPos;

out float localHt;
out vec3 position;
out vec3 localUp;
out vec3 normal;
out vec3 spherePos;

uniform mat4 vp;
uniform mat4 model;
uniform float radius;

uniform float noise_mult;
uniform vec3 offset;
uniform int octaves;
uniform vec4 noise_params;

uniform vec3 ocean_params;

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

float fnoise(vec3 pos) {
    float frequency = noise_params.x;
    float persistence = noise_params.y;
    float lacunarity = noise_params.z;

    float nsum = 0.0;
    float amplitude = 1.0;
    float total_amp = 0.0;

    for (int i = 0; i < octaves; i++) {
        nsum += snoise(pos * frequency + offset) * amplitude;
        total_amp += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    // range from -1 to 1
    return nsum / total_amp;
}

float smooth_max(float a, float b, float k) {
    k = min(0.0, -k);
    float h = max(0, min(1.0, (b - a + k) / (2.0 * k)));
    return a * h + b * (1.0 - h) - k * h * (1.0 - h);
}

vec3 npos(vec3 pos) {
    localHt = fnoise(pos);
    float new_height = localHt * noise_mult;

    // determine if this is considered "ocean"
    float ocean_depth = ocean_params.x;
    float ocean_smooth = ocean_params.y;
    float ocean_mult = ocean_params.z;
    new_height = smooth_max(new_height, -(ocean_depth * noise_mult), ocean_smooth);

    if (new_height < 0.0) {
        new_height *= ocean_mult;
    }

    return normalize(pos) * (radius + new_height);
}

void main() {
    vec3 sphere_pos = npos(aPos);

    // calculate sphere normal
    vec3 sphere_normal = normalize(aPos);
    // calculate sphere tangent
    float phi = atan(aPos.z, aPos.x);
    vec3 tangent = vec3(-sin(phi), 0.0, cos(phi));
    vec3 bitangent = cross(sphere_normal, tangent);

    float delta = noise_params.w;
    vec3 tangent_sample = npos(aPos + delta * normalize(tangent));
    vec3 bitangent_sample = npos(aPos + delta * normalize(bitangent));

    position = vec3(model * vec4(sphere_pos, 1.0));
    localUp = sphere_normal;
    normal = normalize(cross(tangent_sample - sphere_pos, bitangent_sample - sphere_pos));
    spherePos = vec3(model * vec4(normalize(aPos) * radius, 1.0));

    gl_Position = vp * vec4(position, 1.0);
    gl_PointSize = 10.0;
}