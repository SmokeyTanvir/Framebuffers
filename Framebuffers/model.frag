#version 330 core

out vec4 FragColor;

in vec2 textureCoordinates;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main(){
	// ambient
	float ambientStrength = 0.2;
	vec3 ambient = texture(texture_diffuse1, textureCoordinates).rgb * ambientStrength;

	// diffuse 
	vec3 diffuse = texture(texture_diffuse1, textureCoordinates).rgb;

	// specular 
	float specularIntensity = 0.2;
	vec3 specular = texture(texture_specular1, textureCoordinates).rgb * specularIntensity;

	// final fragment
	vec3 result = ambient + diffuse + specular;
	FragColor = vec4(result, 1.0);
}