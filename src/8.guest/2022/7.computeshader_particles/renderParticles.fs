#version 430 core

// ----------------------------------------------------------------------------
//
// attributes
//
// ----------------------------------------------------------------------------

out vec4 fragColor;		/**< color of the fragment */

in VS_OUT {
	vec2 vel;
	vec2 pos;
} fs_in;

// ----------------------------------------------------------------------------
//
// uniforms
//
// ----------------------------------------------------------------------------

layout (binding = 0) uniform sampler1D colorLookUp;

// ----------------------------------------------------------------------------
//
// functions
//
// ----------------------------------------------------------------------------

float rand(vec2 xi){
    return fract(sin(dot(xi.xy, vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
	float texCoord = smoothstep(0,200,length(fs_in.vel));

	vec4 color = texture(colorLookUp, rand(fs_in.vel)*pow(texCoord,2));

	float a = 1 - length(2*gl_PointCoord - 1);
	color.a *= pow(a, 3);

	fragColor = color;
}
