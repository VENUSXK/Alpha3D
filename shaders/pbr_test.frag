#version 330 core
#define PI 3.14159265358979323846
// material

// light
struct Light {
    vec3 position;
    vec3 ambient;
    vec3 intensity;
};

uniform Light light;

out vec4 FragColor;

uniform vec3 baseColor;
in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform float roughness;
uniform float metallic;

in mat3 TBN;

uniform vec3 lightPos;
uniform vec3 viewPos;

float calculateAttenuation(float distance){
    float constant  = 1.0;
    float linear    = 0.09;
    float quadratic = 0.032;
    return 1.0 / (constant + linear * distance + quadratic * distance * distance);
}

float NDF_GGX(float roughness, vec3 normal, vec3 half_dir){
    float roughness2 = roughness * roughness;
    
    float cosTheta = max(0, dot(normal, half_dir));
    float cosTheta2 = cosTheta * cosTheta;

    float denom = (cosTheta2 * (roughness2 - 1) + 1);
    return roughness2 / (PI * denom * denom);
}

float GEO_GGX_sub(vec3 normal, vec3 ray_dir, float k){
    float NdotV = max(dot(normal, ray_dir), 0.0);
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GEO_GGX(vec3 normal, vec3 view_dir, vec3 light_dir, float k){
    return GEO_GGX_sub(normal, view_dir, k) * GEO_GGX_sub(normal, light_dir, k);
}

vec3 BRDF_CookTorrance(float ndf, vec3 fresnel, float geo, vec3 view_dir, vec3 light_dir, vec3 normal){
    float NdotV = max(dot(normal, view_dir), 0.001);
    float NdotL = max(dot(normal, light_dir), 0.001);
    return ndf * fresnel * geo / (4.0 * NdotV * NdotL);
}


void main()
{
    vec3 albedo = baseColor;
    vec3 normal = normalize(Normal);

    float shininess = mix(256.0, 2.0, roughness);

    vec3 light_dir = normalize(lightPos - FragPos);
    vec3 view_dir    = normalize(viewPos - FragPos);
    vec3 reflect_dir = reflect(-light_dir, normal);
    vec3 half_dir = normalize(light_dir + view_dir);

    // pbr specular
    float k_direct = (roughness + 1) * (roughness + 1) / 8;

    float NdotV = max(dot(normal, view_dir), 0.0);
    float NdotL = max(dot(normal, light_dir), 0.0);
    float HdotV = max(dot(half_dir, view_dir), 0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 fresnel = F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);

    float ndf_ggx = NDF_GGX(roughness, normal, half_dir);
    float geo_ggx = GEO_GGX(normal, view_dir, light_dir, k_direct);

    vec3 BRDF_specular = BRDF_CookTorrance(ndf_ggx, fresnel, geo_ggx, view_dir, light_dir, normal);

    // pbr diffuse
    vec3 BRDF_diffuse = albedo / PI * (1 - fresnel);

    float distance = length(lightPos - FragPos);
    float attenuation = calculateAttenuation(distance);

    vec3 radiance = light.intensity * attenuation;

    vec3 L0 = (BRDF_specular + BRDF_diffuse) * radiance * NdotL;

    vec3 ambient = light.ambient * albedo;

    vec3 color = L0 + ambient;

    FragColor = vec4(color, 1.0);
}
