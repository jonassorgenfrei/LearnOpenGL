#version 430 core

layout (local_size_x = 1000, local_size_y = 1, local_size_z = 1) in;

// ----------------------------------------------------------------------------
//
// uniforms
//
// ----------------------------------------------------------------------------

layout (location = 0) uniform float dt;                 /** Time Difference to last call */
layout (location = 1) uniform vec2 frameBufferSize;		/** Size of the Framebuffer/Rendertarget */
layout (location = 2) uniform vec2 attractorPosition;	/** Attractor position */
layout (location = 3) uniform float attractorForce;     /** Multiplicator for attraction force */

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

float rand(vec2 xi)
{
	return fract(sin(dot(xi.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
	// Position und Geschwindigkeit aus Shader Storage Buffer lesen
	// gl_GlobalInvocationID - built-in Variable: einmaliger index
	// des aktuellen Aufrugs in der Globalen Work Group
	vec2 position = positions[gl_GlobalInvocationID.x];
	vec2 velocity = velocities[gl_GlobalInvocationID.x];

	// Geschwindikeit zum Anziehungspunkt berechnen
	vec2 attractorVector = attractorPosition - position;
	float attractorDistance = length(attractorVector);
	vec2 attractorVelocity = 50000 * normalize(attractorVector) / max(0.5, attractorDistance);

	// Zufaellige Geschwindigkeit berechnen
	vec2 randomVelocity = 2 * vec2(rand(attractorVector + gl_GlobalInvocationID.xy)) - vec2(1);

	// Drehgeschwindigkeit berechnen
	vec2 rotationalVelocity = cross(vec3(attractorVector, 0), vec3(0, 0, 1)).xy;
	rotationalVelocity = smoothstep(100, 0, attractorDistance) * rotationalVelocity;

	// Traegheit der Partikel simulieren
	velocity = mix(velocity, attractorForce * (attractorVelocity + rotationalVelocity + 5 * randomVelocity), 0.05);

	// Randbehandlung
	{
		if (position.x < 0)
		{
			velocity.x *= -1;
			position.x = 0;
		}
		else if (position.x > frameBufferSize.x)
		{
			velocity.x *= -1;
			position.x = frameBufferSize.x;
		}

		if (position.y < 0)
		{
			velocity.y *= -1;
			position.y = 0;
		}
		else if (position.y > frameBufferSize.y)
		{
			velocity.y *= -1;
			position.y = frameBufferSize.y;
		}
	}

	// Neue Position berechnen
	position = position + dt * velocity;

	// Position und Geschwindigkeit in Shader Storage Buffer schreiben
	positions[gl_GlobalInvocationID.x] = position;
	velocities[gl_GlobalInvocationID.x] = velocity;
}