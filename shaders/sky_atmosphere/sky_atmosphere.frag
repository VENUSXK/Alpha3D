#version 330 core

out vec4 FragColor;

in vec2 screenUV;

uniform bool outputLinear;

uniform vec3 cameraPos;
uniform mat4 invProjection;
uniform mat4 invView;

uniform bool useRayleigh;
uniform bool useMie;
uniform bool useAbsorption;

uniform bool useTransmittanceLUT;
uniform bool useSkyViewLUT;

uniform vec3 lightDirection;
uniform float lightIntensity;


uniform float cameraHeight;
uniform float planetRadius;
uniform float atmosphereRadius;

uniform vec3 rayleighBeta;
uniform vec3 mieBeta;
uniform vec3 absorptionBeta;

uniform float rayleighHeight;
uniform float mieHeight;
uniform float absorptionHeight;

uniform float absorptionFalloff;

uniform float mieG;
uniform float exposure;
uniform float gamma;

uniform int primarySteps;
uniform int lightSteps;
uniform int transmittanceLUTSteps;

uniform int MODE;
uniform sampler2D transmittanceLUT;
uniform sampler2D skyViewLUT;


vec3 GetViewDir() {
    vec2 screenNDC = screenUV * 2.0 - 1.0;
    vec4 viewPos = invProjection * vec4(screenNDC, 1.0, 1.0);
    vec3 viewDir = normalize(viewPos.xyz / viewPos.w);
    vec3 worldDir = normalize((invView * vec4(viewDir, 0.0)).xyz);
    return worldDir;
}

vec3 AtmosphereDensity(float height) {
    vec2 particleDensity = exp(-height / vec2(rayleighHeight, mieHeight));
    float ozoneOffset = (absorptionHeight - height) / absorptionFalloff;
    float ozoneDensity = particleDensity.x / (ozoneOffset * ozoneOffset + 1.0);
    return vec3(useRayleigh ? particleDensity.x : 0.0, useMie ? particleDensity.y : 0.0, useAbsorption ? ozoneDensity : 0.0);
}

vec3 Extinction(vec3 opticalDepth) { return rayleighBeta * opticalDepth.x + mieBeta * opticalDepth.y + absorptionBeta * opticalDepth.z; }

vec2 RaySphereIntersect(vec3 origin, vec3 direction, float radius) {
    float b = 2.0 * dot(direction, origin);
    float c = dot(origin, origin) - radius * radius;
    float discriminant = b * b - 4.0 * c;
    if (discriminant < 0.0) return vec2(1e20, -1e20);
    float root = sqrt(discriminant);
    return vec2((-b - root) * 0.5, (-b + root) * 0.5);
}

float fromSubUvsToUnit(float value, float resolution)
{
    return clamp((value - 0.5 / resolution) * resolution / (resolution - 1.0), 0.0, 1.0);
}

float fromUnitToSubUvs(float value, float resolution)
{
    return (value * (resolution - 1.0) + 0.5) / resolution;
}

void uvToSkyViewParams(
    out float viewZenithCos,
    out float lightViewCos,
    float viewHeight,
    vec2 uv)
{
    const float PI = 3.14159265359;
    uv = vec2(
        fromSubUvsToUnit(uv.x, 192.0),
        fromSubUvsToUnit(uv.y, 108.0));
    uv.y = 1.0 - uv.y;

    float horizonDistance = sqrt(max(
        viewHeight * viewHeight - planetRadius * planetRadius, 0.0));
    float cosBeta = clamp(horizonDistance / viewHeight, 0.0, 1.0);
    float beta = acos(cosBeta);
    float zenithHorizonAngle = PI - beta;

    if (uv.y < 0.5) {
        float coord = 1.0 - 2.0 * uv.y;
        coord *= coord;
        coord = 1.0 - coord;
        viewZenithCos = cos(zenithHorizonAngle * coord);
    } else {
        float coord = uv.y * 2.0 - 1.0;
        coord *= coord;
        viewZenithCos = cos(zenithHorizonAngle + beta * coord);
    }

    float coord = uv.x;
    coord *= coord;
    lightViewCos = -(coord * 2.0 - 1.0);
}


vec2 skyViewLutParamsToUv(
    float viewZenithCos,
    float lightViewCos,
    float viewHeight,
    bool intersectGround)
{
    const float PI = 3.14159265359;
    float horizonDistance = sqrt(max(
        viewHeight * viewHeight - planetRadius * planetRadius, 0.0));
    float cosBeta = clamp(horizonDistance / viewHeight, 0.0, 1.0);
    float beta = acos(cosBeta);
    float zenithHorizonAngle = PI - beta;
    float viewZenithAngle = acos(clamp(viewZenithCos, -1.0, 1.0));

    float logicalV;
    if (intersectGround || viewZenithAngle >= zenithHorizonAngle) {
        float coord = sqrt(clamp(
            (viewZenithAngle - zenithHorizonAngle) / max(beta, 1e-6),
            0.0, 1.0));
        logicalV = 0.5 + 0.5 * coord;
    } else {
        float coord = sqrt(clamp(
            1.0 - viewZenithAngle / max(zenithHorizonAngle, 1e-6),
            0.0, 1.0));
        logicalV = 0.5 * (1.0 - coord);
    }

    float logicalU = sqrt(clamp((1.0 - lightViewCos) * 0.5, 0.0, 1.0));

    // The LUT is written with its V direction flipped.
    return vec2(
        fromUnitToSubUvs(logicalU, 192.0),
        fromUnitToSubUvs(1.0 - logicalV, 108.0));
}

void uvToTransmittanceParams(vec2 uv, out float viewHeight, out float viewZenithCos)
{
    float H = sqrt(max(atmosphereRadius * atmosphereRadius - planetRadius * planetRadius, 0.0));
    float rho = H * clamp(uv.y, 0.0, 1.0);
    viewHeight = sqrt(rho * rho + planetRadius * planetRadius);
    float dMin = atmosphereRadius - viewHeight;
    float dMax = rho + H;
    float d = mix(dMin, dMax, clamp(uv.x, 0.0, 1.0));
    if (d <= 1e-6) { viewZenithCos = 1.0; return; }
    viewZenithCos = (H * H - rho * rho - d * d) / (2.0 * viewHeight * d);
    viewZenithCos = clamp(viewZenithCos, -1.0, 1.0);
}

vec2 transmittanceParamsToUv(float viewRadius, float viewZenithCos)
{
    float H = sqrt(max(atmosphereRadius * atmosphereRadius - planetRadius * planetRadius, 0.0));
    float rho = sqrt(max(viewRadius * viewRadius - planetRadius * planetRadius, 0.0));
    float discriminant = viewRadius * viewRadius * (viewZenithCos * viewZenithCos - 1.0) + atmosphereRadius * atmosphereRadius;
    float distanceToTop = -viewRadius * viewZenithCos + sqrt(max(discriminant, 0.0));
    float dMin = atmosphereRadius - viewRadius;
    float dMax = rho + H;
    float xMu = (distanceToTop - dMin) / max(dMax - dMin, 1e-6);
    float xR = rho / max(H, 1e-6);
    return clamp(vec2(xMu, xR), vec2(0.0), vec2(1.0));
}


vec3 transmittance(float viewRadius, float viewZenithCos, int sampleCount)
{
    float rayLength = -viewRadius * viewZenithCos 
        + sqrt(max(viewRadius * viewRadius * (viewZenithCos * viewZenithCos - 1.0) 
        + atmosphereRadius * atmosphereRadius, 0.0));

    float stepLength = rayLength / float(sampleCount);
    vec3 opticalDepth = vec3(0.0);

    for (int i = 0; i < sampleCount; ++i) {
        float distanceAlongRay = (float(i) + 0.5) * stepLength;
        float sampleRadius = sqrt(max(
            viewRadius * viewRadius
            + distanceAlongRay * distanceAlongRay
            + 2.0 * viewRadius * viewZenithCos * distanceAlongRay,
            planetRadius * planetRadius));
        opticalDepth += AtmosphereDensity(sampleRadius - planetRadius) * stepLength;
    }

    return exp(-max(Extinction(opticalDepth), vec3(0.0)));
}

vec3 SampleTransmittanceLUT(float radius, float zenithCos) {
    vec2 uv = transmittanceParamsToUv(radius, zenithCos);
    return texture(transmittanceLUT, uv).rgb;
}

vec3 integrateAtmosphereScattering(vec3 rayOrigin, vec3 rayDirection, float maxDistance, vec3 background, vec3 lightDirection)
{
    vec2 atmosphereHit = RaySphereIntersect(rayOrigin, rayDirection, atmosphereRadius);
    if (atmosphereHit.x > atmosphereHit.y) return background;

    float viewHeight = length(rayOrigin);
    if (MODE == 0 && useSkyViewLUT && viewHeight < atmosphereRadius && maxDistance >= 1e20) {
        vec3 upVector = normalize(rayOrigin);
        float viewZenithCosAngle = dot(rayDirection, upVector);

        vec3 sideVector = normalize(cross(upVector, rayDirection));
        vec3 forwardVector = normalize(cross(sideVector, upVector));
        vec2 lightOnPlane = vec2(
            dot(lightDirection, forwardVector),
            dot(lightDirection, sideVector));
        lightOnPlane = normalize(lightOnPlane);
        float lightViewCosAngle = lightOnPlane.x;

        vec2 skyViewUV = skyViewLutParamsToUv(
            viewZenithCosAngle,
            lightViewCosAngle,
            viewHeight,
            false);
        vec3 viewTransmittance = useTransmittanceLUT
            ? SampleTransmittanceLUT(viewHeight, viewZenithCosAngle)
            : transmittance(viewHeight, viewZenithCosAngle, max(lightSteps, 1));
        return texture(skyViewLUT, skyViewUV).rgb + background * viewTransmittance;
    }

    float rayStart = max(atmosphereHit.x, 0.0);
    float rayEnd = min(atmosphereHit.y, maxDistance);
    if (rayStart >= rayEnd) return background;

    bool allowMie = maxDistance > atmosphereHit.y;
    int viewSampleCount = max(primarySteps, 1);
    int lightSampleCount = max(lightSteps, 1);
    float stepSize = (rayEnd - rayStart) / float(viewSampleCount);
    float rayPosition = rayStart + stepSize * 0.5;

    vec3 opticalDepth = vec3(0.0);
    vec3 totalRayleigh = vec3(0.0);
    vec3 totalMie = vec3(0.0);

    float PI = 3.14159265359;
    float mu = dot(rayDirection, lightDirection);
    float mu2 = mu * mu;
    float g2 = mieG * mieG;

    float rayleighPhase = 3.0 * (1.0 + mu2) / (16.0 * PI);
    float miePhase = allowMie ? 3.0 * (1.0 - g2) * (1.0 + mu2) /
          (8.0 * PI * (2.0 + g2) * pow(max(1.0 + g2 - 2.0 * mieG * mu, 1e-4), 1.5)) : 0.0;
    if (!allowMie)
        miePhase = 0.0;

    for (int i = 0; i < viewSampleCount; ++i) {
        float rayPosition = rayStart + (float(i) + 0.5) * stepSize;
        vec3 samplePosition = rayOrigin + rayDirection * rayPosition;
        float radius = length(samplePosition);
        vec3 localDensity = AtmosphereDensity(max(radius - planetRadius, 0.0)) * stepSize;

        opticalDepth += localDensity;

        float zenithCos = dot(samplePosition / radius, lightDirection);
        vec3 viewTransmittance = exp(-Extinction(opticalDepth));
        vec2 groundHit = RaySphereIntersect(samplePosition, lightDirection, planetRadius);
        vec3 lightTransmittance = vec3(0.0);
        if (groundHit.y <= 0.0) {
            lightTransmittance = useTransmittanceLUT
                ? SampleTransmittanceLUT(radius, zenithCos)
                : transmittance(radius, zenithCos, lightSampleCount);
        }
        vec3 attenuation = viewTransmittance * lightTransmittance;

        totalRayleigh += localDensity.x * attenuation;
        totalMie += localDensity.y * attenuation;
    }

    vec3 transmittance = exp(-Extinction(opticalDepth));
    vec3 scatteredLight = (rayleighPhase * rayleighBeta * totalRayleigh
                         + miePhase * mieBeta * totalMie) * lightIntensity;
    return scatteredLight + background * transmittance;
}



vec3 ApplyToneMapping(vec3 color)
{
    color = 1.0 - exp(-color * exposure);
    return pow(max(color, vec3(0.0)), vec3(1.0 / gamma));
}



vec4 SingleScattering() {

    vec3 viewDirection = GetViewDir();
    vec3 atmosphereCamera = vec3(cameraPos.x, planetRadius + cameraHeight + cameraPos.y, cameraPos.z);
    vec3 sunDirection = normalize(lightDirection);

    vec2 planetHit = RaySphereIntersect(atmosphereCamera, viewDirection, planetRadius);
    float sceneDepth = 1e20;
    vec3 sceneColor = vec3(0.0);

    if (planetHit.y > 0.0) {
        sceneDepth = max(planetHit.x, 0.0);
        vec3 surfacePosition = atmosphereCamera + viewDirection * sceneDepth;
        vec3 normal = normalize(surfacePosition);
        float diffuse = max(dot(normal, sunDirection), 0.0);
        sceneColor = vec3(0.008, 0.012, 0.006) * (0.08 + 0.25 * diffuse);
    }
    else if (dot(viewDirection, sunDirection) > 0.9997) {
        sceneColor = vec3(3.0);
    }

    vec3 color = integrateAtmosphereScattering(
        atmosphereCamera, viewDirection, sceneDepth, sceneColor, sunDirection);
    if (!outputLinear) color = ApplyToneMapping(color);
    return vec4(color, 1.0);
}


vec4 computeSkyViewLUT()
{
    float viewHeight = clamp(
        planetRadius + cameraHeight + cameraPos.y,
        planetRadius + 1.0,
        atmosphereRadius - 1.0);

    float viewZenithCos;
    float lightViewCos;
    uvToSkyViewParams(viewZenithCos, lightViewCos, viewHeight, screenUV);

    float sunZenithCos = clamp(normalize(lightDirection).y, -1.0, 1.0);
    vec3 sunDirection = normalize(vec3(sqrt(max(1.0 - sunZenithCos * sunZenithCos, 0.0)), sunZenithCos, 0.0));

    float viewZenithSin = sqrt(max(1.0 - viewZenithCos * viewZenithCos, 0.0));
    vec3 viewDirection = normalize(vec3(
        viewZenithSin * lightViewCos,
        viewZenithCos,
        viewZenithSin * sqrt(max(1.0 - lightViewCos * lightViewCos, 0.0))
    ));

    vec3 atmosphereCamera = vec3(0.0, viewHeight, 0.0);

    vec2 planetHit = RaySphereIntersect(atmosphereCamera, viewDirection, planetRadius);
    float sceneDepth = 1e20;
    if (planetHit.y > 0.0)
        sceneDepth = max(planetHit.x, 0.0);

    vec3 radiance = integrateAtmosphereScattering(atmosphereCamera, viewDirection, sceneDepth, vec3(0.0), sunDirection);

    return vec4(radiance, 1.0);
}


vec4 computeTransmittanceLUT() {
    int sampleCount = max(transmittanceLUTSteps, 1);
    float viewRadius;
    float viewZenithCos;
    uvToTransmittanceParams(clamp(screenUV, vec2(0.0), vec2(1.0)), viewRadius, viewZenithCos);
    return vec4(transmittance(viewRadius, viewZenithCos, sampleCount), 1.0);
}

void main() {
    switch (MODE) {
        case 0:
            FragColor = SingleScattering();
            break;
        case 1:
            FragColor = computeTransmittanceLUT();
            break;
        case 2:
            FragColor = computeSkyViewLUT();
            break;
    }
}
