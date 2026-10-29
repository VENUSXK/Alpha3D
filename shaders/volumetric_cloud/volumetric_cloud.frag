#version 330 core

in vec3 worldNormal;
in vec3 localPosition;
in vec3 worldPosition;
noperspective in vec2 screenUV;

out vec4 FragColor;

// transform and camera options
uniform mat4 model;
uniform mat4 inverseModel;
uniform mat4 invProjection;
uniform mat4 invView;
uniform vec3 cameraPos;
uniform bool outputLinear;
uniform float exposure;
uniform float gamma;

// light options
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 ambientLight;
uniform float ambientLightIntensity;
uniform float lightIntensity;

// density options
uniform float densityScale;
uniform float extinction;
uniform float cloudOpacity;
uniform float cloudTopOffset;
uniform float anvilBias;
uniform float cloudCoverageBlend;
uniform bool animateCoverageBlend;
uniform float coverageBlendAmplitude;
uniform float coverageBlendSpeed;

// noise options
uniform bool useLowFreqNoise;
uniform bool useHighFreqNoise;
uniform bool useHeightProfile;
uniform bool useBaseReduction;
uniform vec4 stratusProfile;
uniform vec4 stratocumulusProfile;
uniform vec4 cumulusProfile;
uniform float lowFreqNoiseScale;
uniform float highFreqNoiseScale;
uniform float erosionStrength;

uniform float cloudMapScale;

// animation options
uniform vec3 windDirection;
uniform float windSpeed;
uniform float noiseSpeed;
uniform float time;

// ray marching options
uniform int primarySteps;
uniform int lightSteps;
uniform float rayJitterStrength;
uniform float transmittanceCutoff;
uniform float phaseG;
uniform float silverIntensity;
uniform float silverSpread;
uniform float heightExponentTop;
uniform float lightingDensityLodScale;

uniform bool useAerialPerspective;
uniform sampler3D aerialPerspectiveVolume;
uniform float aerialPerspectiveMaxDistance;
uniform float aerialPerspectiveDistanceScale;
uniform int aerialPerspectiveSliceCount;

// volume textures
// R: perlin-worley noise
// G: worley noise fmb 1
// B: worley noise fmb 2
// A: worley noise fmb 3
uniform sampler3D lowFreqNoiseTex;
uniform sampler3D highFreqNoiseTex;

// cloud map
// R/G: cloud coverage probabilities
// B: cloud type probability
uniform sampler2D cloudMapTex;

// cloud height map
uniform sampler2D heightTex;


const float PI = 3.14159265359;
const float EPSILON = 0.0001;

float lerp(float value0, float value1, float amount) { return mix(value0, value1, amount); }
float remap(float value, float inMin, float inMax, float outMin, float outMax) { return outMin + (value - inMin) * (outMax - outMin) / (inMax - inMin); }
float saturate(float value) { return clamp(value, 0.0, 1.0); }

// sample texture

float sampleCloudHeight(float height, float cloudType) {
    float type = saturate(cloudType);
    vec4 heightProfile = type < 0.5 ? mix(stratusProfile, stratocumulusProfile, type * 2.0)
        : mix(stratocumulusProfile, cumulusProfile, (type - 0.5) * 2.0);
    float bottom = saturate(remap(height, heightProfile.x, heightProfile.y, 0.0, 1.0));
    float top = saturate(remap(height, heightProfile.z, heightProfile.w, 1.0, 0.0));
    return bottom * top;
}

float sampleBaseReduction(float height)
{
    return texture(heightTex, vec2(0.5, 1.0 - height)).a;
}

float getNoiseLod(float stepSize, float noiseScale, float textureSize)
{
    float texelFootprint = max(stepSize * textureSize / noiseScale, 1.0);
    return clamp(log2(texelFootprint), 0.0, log2(textureSize));
}

vec3 getNoiseWorldPosition(vec3 uvw)
{
    return (model * vec4(uvw - 0.5, 1.0)).xyz;
}

vec3 GetAnimatedWorldPosition(vec3 uvw, float speed) {
    vec3 noiseWorldPosition = getNoiseWorldPosition(uvw);
    vec3 normalizedWindDirection = length(windDirection) > EPSILON ? normalize(windDirection) : vec3(1.0, 0.0, 0.0);
    noiseWorldPosition += uvw.y * normalizedWindDirection * cloudTopOffset;
    noiseWorldPosition += (normalizedWindDirection + vec3(0.0, 0.1, 0.0)) * time * speed;
    return noiseWorldPosition;
}

float interleavedGradientNoise(vec2 pixelPosition)
{
    return fract(52.9829189 * fract(dot(pixelPosition, vec2(0.06711056, 0.00583715))));
}

float sampleCloudDensity(vec3 localPos, float stepSize) {
    vec3 uvw = localPos + 0.5;
    if (any(lessThan(uvw, vec3(0.0))) || any(greaterThan(uvw, vec3(1.0)))) return 0.0;

    vec3 cloudMapWorldPosition = GetAnimatedWorldPosition(uvw, windSpeed);
    vec3 noiseWorldPosition = GetAnimatedWorldPosition(uvw, noiseSpeed);
    vec2 cloudMapUV = cloudMapWorldPosition.xz / cloudMapScale;
    vec4 cloudMap = texture(cloudMapTex, cloudMapUV + 0.5);

    float cloud = cloudMap.r;
    if (useHeightProfile) cloud *= sampleCloudHeight(uvw.y, cloudMap.b);
    if (useBaseReduction) cloud *= sampleBaseReduction(uvw.y);

    if (useLowFreqNoise) {
        float shapeLod = getNoiseLod(stepSize, lowFreqNoiseScale, 128.0);
        vec3 noiseUVW = noiseWorldPosition / lowFreqNoiseScale;
        vec4 lowFrequencyNoises = textureLod(lowFreqNoiseTex, noiseUVW, shapeLod).rgba;
        float lowFrequencyFBM = dot(lowFrequencyNoises.gba, vec3(0.625, 0.250, 0.125));
        float lowFrequencyShape = clamp(remap(lowFrequencyNoises.r, -(1.0 - lowFrequencyFBM), 1.0, 0.0, 1.0), 0.0, 1.0);
        cloud *= lowFrequencyShape;
    }


    float anvilAmount = saturate(remap(uvw.y, 0.7, 0.8, 0.0, 1.0) + anvilBias);
    float anvilExponent = mix(1.0, 0.5, anvilAmount);
    float animatedCoverageBlend = cloudCoverageBlend;
    if (animateCoverageBlend) {
        float maxAmplitude = min(cloudCoverageBlend, 1.0 - cloudCoverageBlend);
        animatedCoverageBlend += sin(time * coverageBlendSpeed) * min(coverageBlendAmplitude, maxAmplitude);
    }
    float cloudCoverage = pow(clamp(mix(cloudMap.r, cloudMap.g, animatedCoverageBlend), 0.0, 1.0), anvilExponent);
    float baseCloudWithCoverage = clamp(remap(cloud, 1.0 - cloudCoverage, 1.0, 0.0, 1.0), 0.0, 1.0);
    cloud = baseCloudWithCoverage * cloudCoverage;


    if (useHighFreqNoise) {
        vec3 detailUVW = noiseWorldPosition / highFreqNoiseScale;
        float detailLod = getNoiseLod(stepSize, highFreqNoiseScale, 32.0);
        vec3 highFrequencyNoises = textureLod(highFreqNoiseTex, detailUVW, detailLod).rgb;
        float highFrequencyFBM = dot(highFrequencyNoises, vec3(0.625, 0.250, 0.125));
        float highFrequencyNoiseModifier = mix(highFrequencyFBM, 1.0 - highFrequencyFBM, clamp(uvw.y * 10.0, 0.0, 1.0));
        cloud = clamp(remap(cloud, highFrequencyNoiseModifier * erosionStrength, 1.0, 0.0, 1.0), 0.0, 1.0);
    }
    
    return cloud;
}


vec2 calculateVolumeIntersection(vec3 rayOrigin, vec3 rayDirection)
{
    vec3 t0 = (-0.5 - rayOrigin) / rayDirection;
    vec3 t1 = ( 0.5 - rayOrigin) / rayDirection;
    vec3 nearValues = min(t0, t1);
    vec3 farValues = max(t0, t1);
    return vec2(max(max(nearValues.x, nearValues.y), nearValues.z), min(min(farValues.x, farValues.y), farValues.z));
}

float traceLightDensity(vec3 position, out float lightStepSize)
{
    vec3 lightDir_world = normalize(lightDirection);
    vec3 lightDir_local = (inverseModel * vec4(lightDir_world, 0.0)).xyz;

    vec2 hit = calculateVolumeIntersection(position, lightDir_local);
    float t0 = max(hit.x, 0.0);
    float t1 = hit.y;
    if (t1 <= t0) {
        lightStepSize = 1.0;
        return 0.0;
    }

    lightStepSize = (t1 - t0) / float(lightSteps);
    vec3 p = position + lightDir_local * (t0 + lightStepSize * 0.5);
    float densityAlongLightRay = 0.0;

    for (int i = 0; i < lightSteps; ++i)
    {
        densityAlongLightRay += sampleCloudDensity(p, lightStepSize) * lightStepSize;
        p += lightDir_local * lightStepSize;
    }

    return densityAlongLightRay;
}

float getScatteringMu(vec3 position)
{
    vec3 sampleWorldPosition = (model * vec4(position, 1.0)).xyz;
    vec3 directionToCamera = normalize(cameraPos - sampleWorldPosition);
    vec3 incomingLightDirection = -normalize(lightDirection);
    return clamp(dot(incomingLightDirection, directionToCamera), -1.0, 1.0);
}


float henyeyGreenstein(float g, float mu) { float g2 = g * g; return (1.0 - g2) / (4.0 * PI * pow(max(1.0 + g2 - 2.0 * g * mu, EPSILON), 1.5)); }

vec3 ApplyToneMapping(vec3 color) {
    color = 1.0 - exp(-color * exposure);
    return pow(max(color, vec3(0.0)), vec3(1.0 / gamma));
}

vec3 evaluateNubisLighting(vec3 position, float stepSize) {
    float lightStepSize;

    float densityAlongLightRay = traceLightDensity(position, lightStepSize);
    
    // params
    float loddedDensity = sampleCloudDensity(position, stepSize * lightingDensityLodScale);
    float heightFraction = position.y + 0.5;
    float mu = getScatteringMu(position); // 1: same with light dir 
    float basePhase = henyeyGreenstein(phaseG, mu);
    float silverPhase = silverIntensity * henyeyGreenstein(0.99 - silverSpread, mu);
    float phaseProbability = max(basePhase, silverPhase);


    float scaledDensityAlongLightRay = extinction * densityScale * densityAlongLightRay;
    float primaryAttenuation = exp(-scaledDensityAlongLightRay);
    float secondaryAttenuation = exp(-scaledDensityAlongLightRay * 0.25) * 0.7;
    float attenuationProbability = max(primaryAttenuation, secondaryAttenuation);
    
    
    float heightExponent = mix(0.5, heightExponentTop, saturate(remap(heightFraction, 0.3, 0.85, 0.0, 1.0)));
    float depthProbability = 0.05 + pow(max(loddedDensity, 0.0), heightExponent);
    float verticalBase = saturate(remap(heightFraction, 0.07, 0.14, 0.1, 1.0));
    float verticalProbability = pow(verticalBase, 0.8);
    float inScatterProbability = depthProbability * verticalProbability;
    float lightEnergy = attenuationProbability * inScatterProbability * phaseProbability * lightIntensity;

    return ambientLight * ambientLightIntensity + lightColor * lightEnergy;
}

vec3 GetViewDir() {
    vec2 screenNDC = screenUV * 2.0 - 1.0;
    vec4 viewPos = invProjection * vec4(screenNDC, 0.0, 1.0);
    vec3 viewDir = normalize(viewPos.xyz / viewPos.w);
    vec3 worldDir = normalize((invView * vec4(viewDir, 0.0)).xyz);
    return worldDir;
}

void main() {
    vec3 rayOrigin = (inverseModel * vec4(cameraPos, 1.0)).xyz;
    vec3 worldRayDirection = GetViewDir();
    vec3 rayDirection = (inverseModel * vec4(worldRayDirection, 0.0)).xyz;

    vec2 hit = calculateVolumeIntersection(rayOrigin, rayDirection);
    float t0 = max(hit.x, 0.0);
    float t1 = hit.y;
    if (t1 <= t0) discard;

    float stepSize = (t1 - t0) / float(max(primarySteps, 1));
    float rayOffset = mix(0.5, interleavedGradientNoise(gl_FragCoord.xy), rayJitterStrength);
    vec3 position = rayOrigin + rayDirection * (t0 + stepSize * rayOffset);
    float transmittance = 1.0;
    vec3 radiance = vec3(0.0);
    float weightedDistance = 0.0;
    float distanceWeight = 0.0;

    for (int i = 0; i < primarySteps; ++i)
    {
        float density = sampleCloudDensity(position, stepSize);
        if (density >  0.0){
            float sampleSigmaE = max(extinction * densityScale * density, EPSILON);
            float sampleSigmaS = sampleSigmaE;
            float sampleT = exp(-sampleSigmaE * stepSize);
            vec3 luminance = evaluateNubisLighting(position, stepSize) * sampleSigmaS;
            float sampleWeight = transmittance * (1.0 - sampleT);
            vec3 sampleWorldPosition = (model * vec4(position, 1.0)).xyz;
            weightedDistance += length(sampleWorldPosition - cameraPos) * sampleWeight;
            distanceWeight += sampleWeight;
            radiance += transmittance * (luminance - luminance * sampleT) / sampleSigmaE;
            transmittance *= sampleT;
        }
        position += rayDirection * stepSize;
    }

    float alpha = clamp(1.0 - transmittance, 0.0, 1.0);
    if (useAerialPerspective && aerialPerspectiveMaxDistance > 0.0 && distanceWeight > EPSILON) {
        float effectiveDistance = weightedDistance / distanceWeight * max(aerialPerspectiveDistanceScale, 0.0);
        float sliceCount = float(max(aerialPerspectiveSliceCount, 1));
        float firstSlice = 0.5 / sliceCount;
        float slice = sqrt(clamp(effectiveDistance / aerialPerspectiveMaxDistance, 0.0, 1.0));
        vec4 aerialPerspective = texture(aerialPerspectiveVolume, vec3(screenUV, clamp(slice, firstSlice, 1.0 - firstSlice)));
        float firstSliceDepth = aerialPerspectiveMaxDistance * firstSlice * firstSlice;
        aerialPerspective *= clamp(effectiveDistance / max(firstSliceDepth, EPSILON), 0.0, 1.0);
        radiance = radiance * (1.0 - aerialPerspective.a) + aerialPerspective.rgb * alpha;
    }
    float opacity = clamp(cloudOpacity, 0.0, 1.0);
    vec3 outputRadiance = outputLinear ? radiance : ApplyToneMapping(radiance);
    FragColor = vec4(outputRadiance * opacity, alpha * opacity);
}
