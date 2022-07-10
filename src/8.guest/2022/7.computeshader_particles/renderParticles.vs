#version 430 core

// ----------------------------------------------------------------------------
//
// attributes
//
// ----------------------------------------------------------------------------

layout (location = 0) in vec4 position;		/**< position of the particle */
layout (location = 1) in vec2 velocity;		/**< velocity of the particle */

out VS_OUT {
    vec2 vel;
    vec2 pos;
} vs_out;

// ----------------------------------------------------------------------------
//
// uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform mat4 projectionMatrix;	/**< Projection Matrix*/

// ----------------------------------------------------------------------------
//
// functions
//
// ----------------------------------------------------------------------------

void main()
{
    vs_out.vel = velocity;
    vs_out.pos = position.xy;

    gl_Position = projectionMatrix * position;
}
