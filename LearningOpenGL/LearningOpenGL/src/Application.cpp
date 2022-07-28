// Include GLEW
#include <GL/glew.h>
//gili is meowzer!
// Include GLFW
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
//By defining STB_IMAGE_IMPLEMENTATION the preprocessor modifies the header file such that it only contains the relevant definition source code,
//effectively turning the header file into a .cpp file, and that's about it. 
#include "stb_image.h"
#include "Application.h"

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void CameraTransformations(GLFWwindow* window, glm::vec3& cameraPos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::mat4 view;
bool firstMouse = true;
float yaw = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

glm::vec3 lightPos(1.0f, 0.0f,0.0f);



struct ShaderProgramSource
{
    std::string VertexSource;
    std::string FragmentSource;
};

static struct ShaderProgramSource ParseShader(const std::string& filepath)
{
    enum class ShaderType
    {
        NONE = -1, VERTEX = 0, FRAGMENT = 1
    };

    std::ifstream stream(filepath); //opens the file
    std::string line;
    std::stringstream ss[2];
    ShaderType type = ShaderType::NONE;

    while (getline(stream, line)) //as long as theres still lines
    {
        if (line.find("#shader") != std::string::npos) //if it found #shader
        {
            if (line.find("vertex") != std::string::npos)
                type = ShaderType::VERTEX;
            else if (line.find("fragment") != std::string::npos)
                type = ShaderType::FRAGMENT;
        }
        else
        {
            ss[(int)type] << line << '\n';
        }
    }

    return { ss[0].str(), ss[1].str() };
}

static unsigned int CompileShader(unsigned int type, const std::string& source)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    // Error handling
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    std::cout << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << " shader compile status: " << result << std::endl;
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        char* message = (char*)alloca(length * sizeof(char));
        glGetShaderInfoLog(id, length, &length, message);
        std::cout
            << "Failed to compile "
            << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
            << "shader"
            << std::endl;
        std::cout << message << std::endl;
        glDeleteShader(id);
        return 0;
    }

    return id;
}

static unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    // create a shader program
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

int main(void)
{
    // Initialise GLFW
    if (!glfwInit())
    {
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    // Open a window and create its OpenGL context
    GLFWwindow* window = glfwCreateWindow(1600, 1800, "first screen", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_callback); //interface with mouse 
    glfwSetScrollCallback(window, scroll_callback); //interface with scroll

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        glfwTerminate();
        return -1;
    }

    std::cout << "Using GL Version: " << glGetString(GL_VERSION) << std::endl;

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    float vertices[] = {
        //     COORDINATES     /        NORMAL VECTOR      /   TexCoord  //
        -0.5f, -0.5f, -0.5f,  0.0f,0.0f,-1.0f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,0.0f,-1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,0.0f,-1.0f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,0.0f,-1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,0.0f,-1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,0.0f,-1.0f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,0.0f,1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,0.0f,1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,0.0f,1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,0.0f,1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,0.0f,1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,0.0f,1.0f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  -1.0f,0.0f,0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  -1.0f,0.0f,0.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f,0.0f,0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  -1.0f,0.0f,0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  -1.0f,0.0f,0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  -1.0f,0.0f,0.0f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,0.0f,0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,0.0f,0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,0.0f,0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,0.0f,0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,0.0f,0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,0.0f,0.0f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f,-1.0f,0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,-1.0f,0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,-1.0f,0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,-1.0f,0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,-1.0f,0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,-1.0f,0.0f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,1.0f,0.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,1.0f,0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,1.0f,0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,1.0f,0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,1.0f,0.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,1.0f,0.0f,  0.0f, 1.0f

    };

    //Unoptimized indexes
    unsigned int indices[] = {
         0, 1, 2, 3, 4, 5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35
    };

    float texCoords[] = {
        0.0f, 0.0f,  // lower-left corner  
        1.0f, 0.0f,  // lower-right corner
        0.5f, 1.0f   // top-center corner
    };

    glm::vec3 cubePositions[] = {
    glm::vec3(0.0f,  0.0f,  0.0f),
    glm::vec3(2.0f,  5.0f, -15.0f),
    glm::vec3(-1.5f, -2.2f, -2.5f),
    glm::vec3(-3.8f, -2.0f, -12.3f),
    glm::vec3(2.4f, -0.4f, -3.5f),
    glm::vec3(-1.7f,  3.0f, -7.5f),
    glm::vec3(1.3f, -2.0f, -2.5f),
    glm::vec3(1.5f,  2.0f, -2.5f),
    glm::vec3(1.5f,  0.2f, -1.5f),
    glm::vec3(-1.3f,  1.0f, -1.5f)
    };


    // ----------------- VERTEX BUFFER, VERTEX ARRAY, AND INDEX BUFFER OBJECTS ---------------

    unsigned int VBO, CubeVAO;
    glGenVertexArrays(1, &CubeVAO);
    glGenBuffers(1, &VBO); 

    glBindVertexArray(CubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);


    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
    unsigned int lightCubeVAO;
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);

    // we only need to bind to the VBO (to link it with glVertexAttribPointer), no need to fill it; the VBO's data already contains all we need (it's already bound, but we do it again for educational purposes)
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ----------------- SHADER ---------------

    std::string lightingShader_fragments =
        "#version 330 core\n"
        "\n"
        "layout(location = 0) out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "       FragColor = vec4(1.0);\n"
        "}\n";

    ShaderProgramSource source = ParseShader("res/shaders/Basic.shader");
    std::cout << "VERTEX" << std::endl << source.VertexSource << std::endl;
    std::cout << "FRAGMENT" << std::endl << source.FragmentSource << std::endl;

    unsigned int lightingShader = CreateShader(source.VertexSource, source.FragmentSource);
    unsigned int lightCubeShader = CreateShader(source.VertexSource,lightingShader_fragments);

    // ----------------- TEXTURES ---------------
    unsigned int texture1, texture2;
    glUseProgram(lightingShader); //for some reason we need to do this for awsomeface.jpg, the container doesnt need this

    
    //---ASSIGN TEXTURE 1---

    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);


    //---ASSIGN TEXTURE 2---
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    data = stbi_load("awesomeface.png", &width, &height, &nrChannels, 0);
    if (data)
    {
        // note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        //glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);
    
   
  
    // INITIALIZING UNIFORMS
    glUniform1i(glGetUniformLocation(lightingShader, "texture1"), 0); //TEXTURE UNIFORMS
    glUniform1i(glGetUniformLocation(lightingShader, "texture2"), 1);


    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);

    // RESETTING ALL BINDINGS
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
    int r = 2;
    // ------------ MAIN LOOP -------------
    do {
      //  r += 0.1f;
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // -------------------------------------ORANGE CUBE SHADER -------------------------------
        glUseProgram(lightingShader);
       
        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);
        //------- SET LIGHT AND CAMERA POSITION
        unsigned int LightPos = glGetUniformLocation(lightingShader, "lightPos");
        glUniform3f(LightPos, lightPos.x,lightPos.y,lightPos.z);

        unsigned int ViewPos = glGetUniformLocation(lightingShader, "viewPos");
        glUniform3f(ViewPos, cameraPos.x,cameraPos.x,cameraPos.z);


        //------- SET COLOR UNIFORMS

        unsigned int objColor = glGetUniformLocation(lightingShader, "objectColor");
        glUniform3f(objColor, 1.0f, 0.5f, 0.31f);
 
        //the view matrix is calculated in CameraTransformations();

        unsigned int ligColor = glGetUniformLocation(lightingShader, "lightColor");
        glUniform3f(ligColor, 1.0f, 1.0f, 1.0f);

        //------- SET TRANSFORMATION UNIFORMS
        unsigned int viewLoc = glGetUniformLocation(lightingShader, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glm::mat4 proj = glm::mat4(1.0f);
        proj = glm::perspective(glm::radians(fov), float(800 / 800), 0.1f, 100.0f); //FOV, aspect ratio, closest and fartest point to see
        glUniformMatrix4fv(glGetUniformLocation(lightingShader, "proj"), 1, GL_FALSE, glm::value_ptr(proj));

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0, 0, 0));
        glUniformMatrix4fv(glGetUniformLocation(lightingShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        glBindVertexArray(CubeVAO);
    
        for (unsigned int i = 0; i < 10; i++)
        {
            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            glUniformMatrix4fv(glGetUniformLocation(lightingShader, "model"), 1, GL_FALSE, glm::value_ptr(model));

            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
       

        // -------------------------------------LIGHTING CUBE SHADER -------------------------------
        glUseProgram(lightCubeShader);

        //------- SET TRANSFORMATION UNIFORMS
        glUniformMatrix4fv(glGetUniformLocation(lightCubeShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lightCubeShader, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glm::mat4 lightModel = glm::mat4(1.0f);
        lightModel = glm::mat4(1.0f);
        lightModel = glm::translate(lightModel, lightPos);
        lightModel = glm::scale(lightModel, glm::vec3(0.2f)); // a smaller cube
        glUniformMatrix4fv(glGetUniformLocation(lightCubeShader, "model"), 1, GL_FALSE, glm::value_ptr(lightModel));
      
        glBindVertexArray(lightCubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
        CameraTransformations(window, cameraPos);
    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

    // Cleanup VBO
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &CubeVAO);
    glDeleteProgram(lightingShader);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}
void CameraTransformations(GLFWwindow* window, glm::vec3& cameraPos)
{
    float cameraSpeed = 0.02f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
}
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f; // change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}


