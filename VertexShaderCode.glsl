#version 430

in layout(location=0) vec3 position;
in layout(location=1) vec2 vertexUV;
in layout(location=2) vec3 normal;

uniform mat4 modelTransformMatrix;
uniform mat4 modelRotationMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform mat4 biasMatrix;

out vec2 UV;
out vec3 normalWorld;
out vec3 vertexPositionWorld;
void main()
{
	UV = vertexUV;
	vec4 v = vec4(position, 1.0);
	vec4 newPosition = modelTransformMatrix * modelRotationMatrix * v;
	vertexPositionWorld = newPosition.xyz;
	gl_Position = projectionMatrix * viewMatrix * newPosition;
	vec4 normal_temp = modelTransformMatrix * vec4(normal, 0);
	normalWorld = normal_temp.xyz;

	
}
