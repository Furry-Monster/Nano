#version 330 core

#define PI 3.1415926535897932384626433832795

layout(location = 0) out vec4 FragColor;

in vec3 worldCoordinates;
in vec2 textureCoordinates;
in vec3 tangent;
in vec3 bitangent;
in vec3 normal;

struct Material
{
    bool useTextureAlbedo;
    bool useTextureMetallicRoughness;
    bool useTextureNormal;
    bool useTextureAmbientOcclusion;
    bool useTextureEmissive;

    vec3  albedo;
    float metallic;
    float roughness;
    float ambientOcclusion;
    vec3  emissive;

    sampler2D textureAlbedo;
    sampler2D textureMetallicRoughness;
    sampler2D textureNormal;
    sampler2D textureAmbientOcclusion;
    sampler2D textureEmissive;
};

struct LightData
{
    vec4 position;    // xyz = position, w = type
    vec4 direction;   // xyz = direction, w = intensity
    vec4 color;       // rgb = color, w = constant
    vec4 attenuation; // x = linear, y = quadratic, z = range, w = inner_cone_angle
    vec4 spot_area;   // x = outer_cone_angle, y = width, z = height, w = padding
};

// PBR uniforms
uniform Material material;
uniform vec3     cameraPosition;

layout(std140) uniform LightBlock
{
    int       lightCount;
    LightData lights[16];
};

// Simple ambient color (replacing IBL)
uniform vec3 ambientColor;

// Fresnel function (Fresnel-Schlick approximation)
vec3 fresnelSchlick(float cosTheta, vec3 f0) { return f0 + (1.0 - f0) * pow(max(1 - cosTheta, 0.0), 5.0); }

// Normal distribution function (Trowbridge-Reitz GGX)
float ndfTrowbridgeReitzGGX(vec3 n, vec3 h, float roughness)
{
    float alpha        = roughness * roughness;
    float alphaSquared = alpha * alpha;

    float nDotH        = max(dot(n, h), 0.0);
    float nDotHSquared = nDotH * nDotH;
    float innerTerms   = nDotHSquared * (alphaSquared - 1.0) + 1.0;

    float numerator   = alphaSquared;
    float denomenator = PI * innerTerms * innerTerms;
    denomenator       = max(denomenator, 0.0001);

    return numerator / denomenator;
}

// Geometry function
float geometrySchlickGGX(vec3 n, vec3 v, float k)
{
    float nDotV       = max(dot(n, v), 0.0);
    float numerator   = nDotV;
    float denomenator = nDotV * (1.0 - k) + k;
    return numerator / denomenator;
}

// Geometry function - smiths method
float geometrySmith(vec3 n, vec3 v, vec3 l, float roughness)
{
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return geometrySchlickGGX(n, v, k) * geometrySchlickGGX(n, l, k);
}

// Tangent space to world
vec3 calculateNormal(vec3 tangentNormal)
{
    vec3 norm = normalize(tangentNormal * 2.0 - 1.0);
    mat3 TBN  = mat3(tangent, bitangent, normal);
    return normalize(TBN * norm);
}

// Cook-Torrance specular BRDF term
vec3 discreteMonteCarloContribution(vec3  l,
                                    vec3  radiance,
                                    vec3  n,
                                    vec3  v,
                                    vec3  albedo,
                                    float metallic,
                                    float roughness,
                                    vec3  f0)
{
    vec3 h = normalize(v + l);

    float D = ndfTrowbridgeReitzGGX(n, h, roughness);
    vec3  F = fresnelSchlick(max(dot(h, v), 0.0), f0);
    float G = geometrySmith(n, v, l, roughness);

    vec3  numerator   = D * F * G;
    float denominator = 4.0 * max(dot(v, n), 0.0) * max(dot(l, n), 0.0);

    vec3 specular = numerator / max(denominator, 0.001);

    vec3 kSpecular = F;
    vec3 kDiffuse  = vec3(1.0) - kSpecular;
    kDiffuse *= 1.0 - metallic;

    vec3  diffuse          = kDiffuse * albedo / PI;
    vec3  cookTorranceBrdf = diffuse + specular;
    float nDotL            = max(dot(n, l), 0.0);

    return cookTorranceBrdf * radiance * nDotL;
}

void main()
{
    // Preprocess:
    // albedo
    vec3 albedo = material.albedo;
    if (material.useTextureAlbedo)
    {
        albedo = texture(material.textureAlbedo, textureCoordinates).rgb;
    }

    // metallic/roughness
    float metallic  = material.metallic;
    float roughness = material.roughness;
    if (material.useTextureMetallicRoughness)
    {
        vec3 metallicRoughness = texture(material.textureMetallicRoughness, textureCoordinates).rgb;
        metallic               = metallicRoughness.b;
        roughness              = metallicRoughness.g;
    }

    // normal
    vec3 n = normal;
    if (material.useTextureNormal)
    {
        n = calculateNormal(texture(material.textureNormal, textureCoordinates).rgb);
    }

    // ambient occlusion
    float ao = material.ambientOcclusion;
    if (material.useTextureAmbientOcclusion)
    {
        ao = texture(material.textureAmbientOcclusion, textureCoordinates).r;
    }

    // emissive
    vec3 emissive = material.emissive;
    if (material.useTextureEmissive)
    {
        emissive = texture(material.textureEmissive, textureCoordinates).rgb;
    }

    vec3 v = normalize(cameraPosition - worldCoordinates);

    // for PBR-metallic we assume dialectrics all have 0.04
    vec3 f0 = vec3(0.04);
    f0      = mix(f0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    // Direct lighting
    for (int i = 0; i < lightCount; i++)
    {
        int  lightType = int(lights[i].position.w);
        vec3 radiance  = vec3(0.0);
        vec3 l         = vec3(0.0);

        vec3  lightPosition       = lights[i].position.xyz;
        vec3  lightDirection      = lights[i].direction.xyz;
        float lightIntensity      = lights[i].direction.w;
        vec3  lightColor          = lights[i].color.rgb;
        float lightConstant       = lights[i].color.w;
        float lightLinear         = lights[i].attenuation.x;
        float lightQuadratic      = lights[i].attenuation.y;
        float lightRange          = lights[i].attenuation.z;
        float lightInnerConeAngle = lights[i].attenuation.w;
        float lightOuterConeAngle = lights[i].spot_area.x;
        float lightWidth          = lights[i].spot_area.y;
        float lightHeight         = lights[i].spot_area.z;

        // Point Light (0)
        if (lightType == 0)
        {
            vec3  lightDir = lightPosition - worldCoordinates;
            float distance = length(lightDir);

            if (distance > lightRange)
                continue;

            l = normalize(lightDir);

            float attenuation = 1.0 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);

            radiance = lightColor * lightIntensity * attenuation;
        }
        // Directional Light (1)
        else if (lightType == 1)
        {
            l        = normalize(-lightDirection);
            radiance = lightColor * lightIntensity;
        }
        // Spot Light (2)
        else if (lightType == 2)
        {
            vec3  lightDir = lightPosition - worldCoordinates;
            float distance = length(lightDir);

            if (distance > lightRange)
                continue;

            l            = normalize(lightDir);
            vec3 spotDir = normalize(lightDirection);

            float theta    = dot(l, -spotDir);
            float innerCos = cos(radians(lightInnerConeAngle));
            float outerCos = cos(radians(lightOuterConeAngle));

            float epsilon    = innerCos - outerCos;
            float spotFactor = clamp((theta - outerCos) / epsilon, 0.0, 1.0);

            if (spotFactor <= 0.0)
                continue;

            float attenuation = 1.0 / (lightConstant + lightLinear * distance + lightQuadratic * distance * distance);

            radiance = lightColor * lightIntensity * attenuation * spotFactor;
        }
        // Area Light (3)
        else if (lightType == 3)
        {
            vec3  lightDir = lightPosition - worldCoordinates;
            float distance = length(lightDir);
            l              = normalize(lightDir);

            float attenuation  = 1.0 / (distance * distance);
            vec3  areaDir      = normalize(lightDirection);
            float facingFactor = max(dot(-areaDir, n), 0.0);

            radiance = lightColor * lightIntensity * attenuation * facingFactor;
        }

        if (length(radiance) > 0.0)
        {
            Lo += discreteMonteCarloContribution(l, radiance, n, v, albedo, metallic, roughness, f0);
        }
    }

    // Simple ambient lighting (replacing IBL)
    vec3 ambient = ambientColor * albedo * ao;

    // Outputs:
    // color = emissive + indirect + direct
    vec3 color = emissive + ambient + Lo;
    FragColor  = vec4(color, 1.0);
}
