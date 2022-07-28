#shader vertex
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 viewPos;


void main()
{
    gl_Position = proj * view * model * vec4(aPos,1.0f);
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  // the matrix fixes an issue where the scaled normal vectors are no longer perpendicular to the surface
;
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
} 




#shader fragment
#version 330 core
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

in vec3 ourColor;
in vec3 Normal;
in vec3 FragPos;

in vec2 TexCoord;
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    //---- AMBIENT
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    //----DIFFUSE
    vec3 norm = normalize(Normal); //normal vector as specified in the vertex attribute (1)
    vec3 lightDir = normalize(lightPos - FragPos); //the light direction
    float diff = max(dot(norm, lightDir), 0.0); //the angle between them, max returns the highest of its parameters so we never get a negative result
    vec3 diffuse = diff * lightColor;
    //---SPECULAR
    float specularStrength = 0.65;
    vec3 viewDir = normalize(viewPos - FragPos); //vector from pixel to player
    vec3 reflectDir = reflect(-lightDir, norm); //reflex the vector from the light to the pixel

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 36); //calculating the angle between our view vector and the reflexion vector, the more close the angle is the more shiny stuff we should see
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular);
    FragColor = vec4(result, 1.0) * mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
    ;

}


