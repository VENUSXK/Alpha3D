#version 330 core
#define PI 3.14159265358979323846

uniform bool useSurfaceShadow;
uniform mat4 shadowViewProjection;
uniform sampler2DShadow surfaceShadowMap;

// aerial perspective
uniform mat3 environmentRotation;
uniform bool useAerialPerspective;
uniform sampler3D aerialPerspectiveVolume;
uniform vec3 aerialPerspectiveViewportSize;
uniform float aerialPerspectiveMaxDistance;
uniform int aerialPerspectiveSliceCount;

// material

// light
struct Light {
    vec3 position;
    vec3 ambient;
    vec3 intensity;
};

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;  

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
uniform bool useDirectionalLight;
uniform vec3 sunDirection;
uniform vec3 viewPos;

float calculateAttenuation(float distance){
    return 1.0 / (distance * distance);
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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  



void main()
{
    vec3 albedo = baseColor;
    vec3 normal = normalize(Normal);

    vec3 light_dir = useDirectionalLight
        ? normalize(sunDirection)
        : normalize(lightPos - FragPos);
    vec3 view_dir    = normalize(viewPos - FragPos);
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

    vec3 radiance = light.intensity;
    if (!useDirectionalLight) {
        float distance = length(lightPos - FragPos);
        radiance *= calculateAttenuation(distance);
    }

    // vec3 L0 = (BRDF_specular + BRDF_diffuse) * radiance * NdotL;
    vec4 shadowPosition = shadowViewProjection * vec4(FragPos, 1.0);
    vec3 shadowCoordinates = shadowPosition.xyz / shadowPosition.w * 0.5 + 0.5;
    bool insideShadowMap = all(greaterThanEqual(shadowCoordinates, vec3(0.0))) && all(lessThanEqual(shadowCoordinates, vec3(1.0)));
    float sunVisibility = useSurfaceShadow && insideShadowMap ? texture(surfaceShadowMap, vec3(shadowCoordinates.xy, shadowCoordinates.z)) : 1.0;
    vec3 L0 = (BRDF_specular + BRDF_diffuse) * radiance * NdotL * sunVisibility;

    // ibl diffuse
    vec3 kS_ibl = fresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kD = 1.0 - kS_ibl;
    kD *= 1.0 - metallic;

    vec3 reflect_dir = reflect(-view_dir, normal);   

    const float MAX_REFLECTION_LOD = 4.0;

    vec3 environmentNormal = normalize(environmentRotation * normal);
    vec3 environmentReflect = normalize(environmentRotation * reflect_dir);

    vec3 irradiance = texture(irradianceMap, environmentNormal).rgb;
    vec3 prefilteredColor = textureLod(prefilterMap, environmentReflect, roughness * MAX_REFLECTION_LOD).rgb;

    vec3 diffuse    = irradiance * albedo;

    vec2 envBRDF = textureLod(brdfLUT, vec2(NdotV, roughness), 0.0).rg;
    vec3 specular = prefilteredColor * (kS_ibl * envBRDF.x + envBRDF.y);

    vec3 ambient = (kD * diffuse + specular);

    vec3 color = L0 + ambient;
    if (useAerialPerspective) {
        float normalizedDistance = clamp(length(FragPos - viewPos) / max(aerialPerspectiveMaxDistance, 1e-6), 0.0, 1.0);
        float firstSlice = 0.5 / float(max(aerialPerspectiveSliceCount, 1));
        vec4 scatteredLight = texture(aerialPerspectiveVolume, vec3(gl_FragCoord.xy / aerialPerspectiveViewportSize.xy, clamp(sqrt(normalizedDistance), firstSlice, 1.0 - firstSlice)));
        scatteredLight *= clamp(normalizedDistance / (firstSlice * firstSlice), 0.0, 1.0);
        color = scatteredLight.rgb + color * (1.0 - scatteredLight.a);
    }
    color = color / (color + vec3(1.0));
    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
