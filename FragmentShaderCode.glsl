#version 430
 
in vec2 UV;
in vec3 normalWorld;
in vec3 vertexPositionWorld;

struct Material {
	float shininess;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct DirLight {
	vec3 intensity;
	vec3 direction;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PtLight {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float constant;
	float linear;
	float quadratic;
};

struct StLight {
	vec3 position;  
    vec3 direction;
    float cutOff;
    float outerCutOff;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform sampler2D myTextureSampler;
uniform sampler2D NM;
uniform vec3 lightPositionWorld;
uniform vec3 eyePositionWorld;
uniform Material material;
uniform DirLight dirlt;
uniform PtLight ptlt[4];
uniform StLight stlt[5];
uniform bool normalMap_flag;
out vec3 daColor;



vec3 CalDirLight(Material material, DirLight light, vec3 normal, vec3 viewDir) 
{
	vec3 lightDir = normalize(-light.direction);
	// diffuse
	float diff = max(dot(normal, lightDir), 0.0);
	// specular
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 ambient = light.intensity * light.ambient * material.ambient;
	vec3 diffuse = light.intensity * light.diffuse * diff * material.diffuse;
	vec3 specular = light.intensity  * light.specular * spec * material.specular;
	return (ambient + diffuse + specular);
}

vec3 CalPtLight(Material material, PtLight light, vec3 normal, vec3 fragPos, vec3 viewPos) 
{	
	vec3 ambient = light.ambient * material.diffuse;

	vec3 lightDir = normalize(light.position - fragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.diffuse * diff * material.diffuse;
	
	vec3 viewDir = normalize(viewPos - fragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * spec * material.specular;

	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

	return (attenuation * (ambient + diffuse + specular));
}

vec3 CalStLight(Material material, StLight light, vec3 normal, vec3 fragPos, vec3 viewPos)
{
	vec3 ambient = light.ambient * material.diffuse;
    
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * material.diffuse;  
   
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular;  
    
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    diffuse  *= intensity;
    specular *= intensity;
    
    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

	return (attenuation * (ambient + diffuse + specular));
}
void main()
{	

	// texture
	vec3 normal =normalize(normalWorld);
	if(normalMap_flag)
	{
		normal=texture(NM, UV).rgb;
		normal=normalize(normal*2.0-1.0);
	}
	vec3 Texture = texture(myTextureSampler, UV).rgb;
	vec3 eyeVectorWorld = normalize(eyePositionWorld - vertexPositionWorld);
	vec3 dlt = CalDirLight(material, dirlt, normal, eyeVectorWorld);
	vec3 plt;
	for (int i = 0; i < 4; i++) {
		plt += CalPtLight(material, ptlt[i], normal, vertexPositionWorld, eyePositionWorld);
	}
	vec3 slt;
	for (int i = 0; i < 5;i++) {
		slt += CalStLight(material, stlt[i], normal, vertexPositionWorld, eyePositionWorld);
	}
	daColor = (dlt+plt+slt)*Texture;
}
