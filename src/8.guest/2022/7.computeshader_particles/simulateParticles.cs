#version 430 core

layout (local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;

// ----------------------------------------------------------------------------
//
// uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform float dt;                 /** time difference to the last call */
layout (location = 1) uniform vec2 frameBufferSize;		/** size of the framebuffer/rendertarget */
layout (location = 2) uniform vec2 attractorPosition;	/** attractor position */
layout (location = 3) uniform float attractorMult;     /** multiplicator for attraction force */

// ----------------------------------------------------------------------------
//
// buffers
//
// ----------------------------------------------------------------------------

// position buffer
layout(std430, binding = 0) buffer PositionBuffer
{
	vec2 positions [];
};

// velocity buffer
layout(std430, binding = 1) buffer VelocitiesBuffer
{
	vec2 velocities [];
};

// ----------------------------------------------------------------------------
//
// functions
//
// ----------------------------------------------------------------------------

/** 
 *  Pseudo random function
 *  @param xi - input vector2 as seed
 *  return a (pseudo) random float vector
 */
float rand(vec2 xi)
{
	return fract(sin(dot(xi.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
	// reading position and velocity from the shader storage buffer

	
	// gl_GlobalInvocationID - built-in variable: unique index
	// of the current invocation in the globalen work group
	vec2 position = positions[gl_GlobalInvocationID.x];
	vec2 velocity = velocities[gl_GlobalInvocationID.x];

	// calculate the attraction acceleration to the target point
	vec2 attractorVector = attractorPosition - position;
	float attractorDistance = length(attractorVector);
	vec2 attractorAcceleration = 50000 * normalize(attractorVector) / max(0.5, attractorDistance);

	// random acceleration based on attractor vector and globalinvocation id
	// (like a wind noise)
	vec2 randomAcceleration = 2 * vec2(rand(attractorVector + gl_GlobalInvocationID.xy)) - vec2(1);

	// calculate some rotational acceleration
	vec2 rotationalAcceleration = cross(vec3(attractorVector, 0), vec3(0, 0, 1)).xy;
	rotationalAcceleration = smoothstep(100, 0, attractorDistance) * rotationalAcceleration;

	// damping 
	vec2 dampingAcceleration = -velocity * 0.9;

	// summed acceleration
	vec2 acceleration = attractorMult * (attractorAcceleration + rotationalAcceleration + 5 * randomAcceleration) + dampingAcceleration;

	// border conditions
	{
		if (position.x < 0)
		{	
			// reverse x velocity
			velocity.x *= -1;
			// clamp x position
			position.x = 0;
		}
		else if (position.x > frameBufferSize.x)
		{
			// reverse x velocity
			velocity.x *= -1;
			// clamp x position
			position.x = frameBufferSize.x;
		}

		if (position.y < 0)
		{
			// reverse y velocity
			velocity.y *= -1;
			// clamp y position
			position.y = 0;
		}
		else if (position.y > frameBufferSize.y)
		{
			// reverse y velocity
			velocity.y *= -1;
			// clamp y position
			position.y = frameBufferSize.y;
		}
	}

	// euler integration to update position and velocity
	position = position + dt * velocity + 1/2 * acceleration * dt * dt;
	velocity = velocity + acceleration * dt;

	// write updated position und velocity in buffers
	positions[gl_GlobalInvocationID.x] = position;
	velocities[gl_GlobalInvocationID.x] = velocity;
}