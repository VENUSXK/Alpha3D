#version 330 core
#define PI 3.14159265358979323846
// material

uniform sampler2D terrainSplatMap;
uniform vec3 terrainTextureSizeMeters;
uniform vec3 grassColor = vec3(0.12, 0.22, 0.045);

uniform bool useAerialPerspective;

uniform sampler3D aerialPerspectiveVolume;

uniform vec3 aerialPerspectiveViewportSize;
uniform float aerialPerspectiveMaxDistance;
uniform int aerialPerspectiveSliceCount;

uniform sampler2D terrainColorMap;

struct Material {
    sampler2D albedo_1;
    sampler2D normal_1;
    sampler2D arm_1;
    sampler2D emissive_1;
}; 

uniform Material material;

// light
struct Light {
    vec3 position;
    vec3 intensity;
};

uniform float textureRepeat = 1.0;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;  

uniform Light light;

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

in mat3 TBN;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform bool isLight = false;
uniform bool lightOn = false;
uniform vec3 emissiveIntensity = vec3(0.0f, 0.0f, 0.0f);

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


vec3 ApplyAerialPerspective(vec3 surfaceColor)
{
    if (!useAerialPerspective || aerialPerspectiveMaxDistance <= 0.0) return surfaceColor;

    vec2 screenUV = gl_FragCoord.xy / aerialPerspectiveViewportSize.xy;
    float distanceToCamera = length(FragPos - viewPos);

    float sliceCount = float(max(aerialPerspectiveSliceCount, 1));
    float firstSlice = 0.5 / sliceCount;
    float lastSlice = 1.0 - firstSlice;

    float slice = sqrt(clamp(distanceToCamera / aerialPerspectiveMaxDistance, 0.0, 1.0));

    vec4 aerialPerspective = texture(aerialPerspectiveVolume, vec3(screenUV, clamp(slice, firstSlice, lastSlice)));

    float firstSliceDepth = aerialPerspectiveMaxDistance * firstSlice * firstSlice;
    float nearWeight = clamp(distanceToCamera / max(firstSliceDepth, 1e-6), 0.0, 1.0);

    aerialPerspective *= nearWeight;

    return aerialPerspective.rgb + surfaceColor * (1.0 - aerialPerspective.a);
}

struct TerrainSurface {
    vec3 albedo;
    vec3 worldNormal;
    vec3 arm;
};

TerrainSurface SampleTerrainTriplanar(sampler2DArray albedoMap, sampler2DArray normalMap, sampler2DArray armMap, int layerIndex, float textureSizeMeters) {
    vec3 surfaceNormal = normalize(Normal);
    vec3 projectionWeights = pow(abs(surfaceNormal), vec3(4.0));
    projectionWeights /= max(dot(projectionWeights, vec3(1.0)), 1e-6);
    vec3 projectionSign = vec3(surfaceNormal.x < 0.0 ? -1.0 : 1.0, surfaceNormal.y < 0.0 ? -1.0 : 1.0, surfaceNormal.z < 0.0 ? -1.0 : 1.0);
    vec3 scaledPosition = FragPos / max(textureSizeMeters, 0.001);

    vec2 projectionUV[3];
    projectionUV[0] = scaledPosition.yz * vec2(projectionSign.x, 1.0);
    projectionUV[1] = scaledPosition.zx * vec2(projectionSign.y, 1.0);
    projectionUV[2] = scaledPosition.xy * vec2(projectionSign.z, 1.0);

    TerrainSurface result;
    result.albedo = vec3(0.0);
    result.worldNormal = vec3(0.0);
    result.arm = vec3(0.0);
    for (int projectionIndex = 0; projectionIndex < 3; ++projectionIndex) {
        vec3 textureCoordinate = vec3(projectionUV[projectionIndex], float(layerIndex));
        float projectionWeight = projectionWeights[projectionIndex];
        result.albedo += texture(albedoMap, textureCoordinate).rgb * projectionWeight;
        result.arm += texture(armMap, textureCoordinate).rgb * projectionWeight;
        vec3 detailNormal = texture(normalMap, textureCoordinate).rgb * 2.0 - 1.0;
        vec3 projectedNormal;

        if (projectionIndex == 0) {
            projectedNormal = vec3(detailNormal.z * surfaceNormal.x, detailNormal.x * projectionSign.x + surfaceNormal.y, detailNormal.y + surfaceNormal.z);
        } else if (projectionIndex == 1) {
            projectedNormal = vec3(detailNormal.y + surfaceNormal.x, detailNormal.z * surfaceNormal.y, detailNormal.x * projectionSign.y + surfaceNormal.z);
        } else {
            projectedNormal = vec3(detailNormal.x * projectionSign.z + surfaceNormal.x, detailNormal.y + surfaceNormal.y, detailNormal.z * surfaceNormal.z);
        }
        result.worldNormal += projectedNormal * projectionWeight;
    }
    result.worldNormal = normalize(result.worldNormal);
    return result;
}


void main()
{
    vec2 terrainUV = clamp((FragPos.xz + vec2(4000.0)) / 8000.0, vec2(0.0), vec2(1.0));
    vec3 albedo = texture(terrainColorMap, terrainUV).rgb;
    vec3 normal = normalize(Normal);
    float ao = 1.0;
    float roughness = 0.8;
    float metallic = 0.0;

    // center grid
    vec2 gridCenter = vec2(0.0);
    float gridHalfSize = 4000.0;
    float gridCellSize = 10.0;
    vec2 gridPosition = FragPos.xz - gridCenter;

    vec2 gridCell = floor(gridPosition / gridCellSize);
    float checker = mod(gridCell.x + gridCell.y, 2.0);
    vec3 gridColor = mix(vec3(0.28, 0.38, 0.18), vec3(0.38, 0.49, 0.25), checker);
    float gridRadius = 1000.0;
    float gridBlendWidth = 300.0;
    float gridMask = 1.0 - smoothstep(gridRadius - gridBlendWidth, gridRadius, length(gridPosition));

    albedo = mix(albedo, gridColor, gridMask);
    roughness = mix(roughness, 0.3, gridMask);
    metallic = mix(metallic, 0.0, gridMask);
    normal = normalize(mix(normal, normalize(Normal), gridMask));


    vec3 light_dir = normalize(lightPos - FragPos);
    vec3 view_dir = normalize(viewPos - FragPos);
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


    // ibl diffuse
    vec3 kS_ibl = fresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kD = 1.0 - kS_ibl;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(irradianceMap, normal).rgb;
    vec3 diffuse    = irradiance * albedo;

    vec3 reflect_dir = reflect(-view_dir, normal);   

    // ibl specular

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, reflect_dir, roughness * MAX_REFLECTION_LOD).rgb;  

    vec2 envBRDF = textureLod(brdfLUT, vec2(NdotV, roughness), 0.0).rg;
    vec3 specular = prefilteredColor * (kS_ibl * envBRDF.x + envBRDF.y);

    vec3 ambient = (kD * diffuse + specular);

    vec3 color = L0 + ambient;

    if (isLight) {
        float emissive = texture(material.emissive_1, TexCoords).r;
        emissive = pow(emissive, 2.2);

        color += emissive * emissiveIntensity;
    }

    color = ApplyAerialPerspective(color);

    color = color / (color + vec3(1.0));
    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);

}