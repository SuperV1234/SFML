#version 310 es

#ifdef GL_ES
precision mediump float;
#endif

in vec3       normal;
uniform float lightFactor;

layout(location = 2) uniform sampler2D sf_u_texture;

layout(location = 0) in vec4 sf_v_color;
layout(location = 1) in vec2 sf_v_texCoord;

layout(location = 0) out vec4 sf_fragColor;

void main()
{
    vec3  lightPosition = vec3(-1.0, 1.0, 1.0);
    vec3  eyePosition   = vec3(0.0, 0.0, 1.0);
    vec3  halfVector    = normalize(lightPosition + eyePosition);
    float intensity     = lightFactor + (1.0 - lightFactor) * dot(normalize(normal), normalize(halfVector));
    sf_fragColor        = sf_v_color * vec4(intensity, intensity, intensity, 1.0);
}
