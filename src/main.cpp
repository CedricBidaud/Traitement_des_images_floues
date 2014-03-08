#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>

#include "glew/glew.h"
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "GL/glfw.h"
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/imguiRenderGL3.h"

#include "glm/glm.hpp"
#include "glm/vec3.hpp" // glm::vec3
#include "glm/vec4.hpp" // glm::vec4, glm::ivec4
#include "glm/mat4x4.hpp" // glm::mat4
#include "glm/gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "glm/gtc/type_ptr.hpp" // glm::value_ptr

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 1
#endif

#if DEBUG_PRINT == 0
#define debug_print(FORMAT, ...) ((void)0)
#else
#ifdef _MSC_VER
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __func__, __FILE__, __LINE__, __VA_ARGS__)
#endif
#endif

// Font buffers
extern const unsigned char DroidSans_ttf[];
extern const unsigned int DroidSans_ttf_len;    

struct ShaderGLSL
{
    enum ShaderType
    {
        VERTEX_SHADER = 1,
        FRAGMENT_SHADER = 2,
        GEOMETRY_SHADER = 4
    };
    GLuint program;
};

int compile_and_link_shader(ShaderGLSL & shader, int typeMask, const char * sourceBuffer, int bufferSize);
int destroy_shader(ShaderGLSL & shader);
int load_shader_from_file(ShaderGLSL & shader, const char * path, int typemask);
    
struct Camera
{
    float radius;
    float theta;
    float phi;
    glm::vec3 o;
    glm::vec3 eye;
    glm::vec3 up;
};

void camera_defaults(Camera & c);
void camera_zoom(Camera & c, float factor);
void camera_turn(Camera & c, float phi, float theta);
void camera_pan(Camera & c, float x, float y);

struct GUIStates
{
    bool panLock;
    bool turnLock;
    bool zoomLock;
    int lockPositionX;
    int lockPositionY;
    int camera;
    double time;
    bool playing;
    static const float MOUSE_PAN_SPEED;
    static const float MOUSE_ZOOM_SPEED;
    static const float MOUSE_TURN_SPEED;
};
const float GUIStates::MOUSE_PAN_SPEED = 0.001f;
const float GUIStates::MOUSE_ZOOM_SPEED = 0.05f;
const float GUIStates::MOUSE_TURN_SPEED = 0.005f;


void init_gui_states(GUIStates & guiStates)
{
    guiStates.panLock = false;
    guiStates.turnLock = false;
    guiStates.zoomLock = false;
    guiStates.lockPositionX = 0;
    guiStates.lockPositionY = 0;
    guiStates.camera = 0;
    guiStates.time = 0.0;
    guiStates.playing = false;
}

int main( int argc, char **argv )
{
    int width = 800, height= 600;
    float widthf = (float) width, heightf = (float) height;
    double t;
    float fps = 0.f;

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }

    // Force core profile on Mac OSX
#ifdef __APPLE__
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
    glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
    glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    // Open a window and create its OpenGL context
    if( !glfwOpenWindow( width, height, 0,0,0,0, 24, 0, GLFW_WINDOW ) )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );

        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    glfwSetWindowTitle( "001_a" );

    // Core profile is flagged as experimental in glew
#ifdef __APPLE__
    glewExperimental = GL_TRUE;
#endif
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
          /* Problem: glewInit failed, something is seriously wrong. */
          fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
          exit( EXIT_FAILURE );
    }

    // Ensure we can capture the escape key being pressed below
    glfwEnable( GLFW_STICKY_KEYS );

    // Enable vertical sync (on cards that support it)
    glfwSwapInterval( 1 );
    GLenum glerr = GL_NO_ERROR;
    glerr = glGetError();

    if (!imguiRenderGLInit(DroidSans_ttf, DroidSans_ttf_len))
    {
        fprintf(stderr, "Could not init GUI renderer.\n");
        exit(EXIT_FAILURE);
    }

    // Init viewer structures
    Camera camera;
    camera_defaults(camera);
    GUIStates guiStates;
    init_gui_states(guiStates);
    float dummySlider = 0.f;

    // Load images and upload textures
    GLuint textures[2];
    glGenTextures(2, textures);
    int x;
    int y;
    int comp;

    unsigned char * diffuse = stbi_load("textures/mountain-07.jpg", &x, &y, &comp, 3);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fprintf(stderr, "Diffuse %dx%d:%d\n", x, y, comp);

    unsigned char * spec = stbi_load("textures/spnza_bricks_a_spec.tga", &x, &y, &comp, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, x, y, 0, GL_RED, GL_UNSIGNED_BYTE, spec);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    fprintf(stderr, "Spec %dx%d:%d\n", x, y, comp);
    glerr = glGetError();
    if(glerr != GL_NO_ERROR)
        fprintf(stderr, "2nd OpenGL Error : %s\n", gluErrorString(glerr));

    // Try to load and compile shader
    ShaderGLSL baseShader;
    const char * shaderFile = "src/base.glsl";
	int status = load_shader_from_file(baseShader, shaderFile, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFile);
        exit( EXIT_FAILURE );
    }

    // Uniform locations
    GLuint base_projectionLocation = glGetUniformLocation(baseShader.program, "Projection");
    GLuint base_viewLocation = glGetUniformLocation(baseShader.program, "View");
    GLuint base_objectLocation = glGetUniformLocation(baseShader.program, "Object");
    GLuint base_timeLocation = glGetUniformLocation(baseShader.program, "Time");
    GLuint base_textureLocation = glGetUniformLocation(baseShader.program, "Texture");
    GLuint base_cameraPositionLocation = glGetUniformLocation(baseShader.program, "CameraPosition");
    
    
    // Load blur shader
    ShaderGLSL blur_shader;
    const char * shaderFileblur = "src/blur.glsl";
    status = load_shader_from_file(blur_shader, shaderFileblur, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFileblur);
        exit( EXIT_FAILURE );
    }    
    // Compute locations for blur_shader
    GLuint blur_tex1Location = glGetUniformLocation(blur_shader.program, "Texture1");
    GLuint blur_isBluredLocation = glGetUniformLocation(blur_shader.program, "IsBlured");
    
    // Load sobel shader
    ShaderGLSL sobel_shader;
    const char * shaderFilesobel = "src/sobel.glsl";
    status = load_shader_from_file(sobel_shader, shaderFilesobel, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFilesobel);
        exit( EXIT_FAILURE );
    }    
    // Compute locations for sobel_shader
    GLuint sobel_tex1Location = glGetUniformLocation(sobel_shader.program, "Texture1");
    GLuint sobel_sobelLocation = glGetUniformLocation(sobel_shader.program, "SobelCoef");
    
    // Load gamma shader
    ShaderGLSL gamma_shader;
    const char * shaderFilegamma = "src/gamma.glsl";
    status = load_shader_from_file(gamma_shader, shaderFilegamma, ShaderGLSL::VERTEX_SHADER | ShaderGLSL::FRAGMENT_SHADER);
    if ( status == -1 )
    {
        fprintf(stderr, "Error on loading  %s\n", shaderFilegamma);
        exit( EXIT_FAILURE );
    }    
    // Compute locations for gamma_shader
    GLuint gamma_tex1Location = glGetUniformLocation(gamma_shader.program, "Texture1");
    GLuint gamma_gammaLocation = glGetUniformLocation(gamma_shader.program, "Gamma");
    
    
    GLuint fxFbo;
    glGenFramebuffers(1, &fxFbo);
    
    GLuint fxTextures[2];
    glGenTextures(2, fxTextures);
    
    glBindTexture(GL_TEXTURE_2D, fxTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, fxTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Load geometry
    int plane_triangleCount = 2;
    int plane_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float plane_uvs[] = {0.f, 1.f, 1.f, 1.f, 0.f, 0.f, 1.f, 0.f};
    float plane_vertices[] = {-5.0, -5.0, 0.0, 5.0, -5.0, 0.0, -5.0, 5.0, 0.0, 5.0, 5.0, 0.0};
    float plane_normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};
    
    int   quad_triangleCount = 2;
    int   quad_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float quad_vertices[] =  {-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0};

    // Vertex Array Object
    GLuint vao[2];
    glGenVertexArrays(2, vao);

    // Vertex Buffer Objects
    GLuint vbo[6];
    glGenBuffers(6, vbo);

    // Plane
    glBindVertexArray(vao[0]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_triangleList), plane_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);
    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_normals), plane_normals, GL_STATIC_DRAW);
    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_uvs), plane_uvs, GL_STATIC_DRAW);
    
    // Quad
    glBindVertexArray(vao[1]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_triangleList), quad_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    // Unbind everything. Potentially illegal on some implementations
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Viewport 
    glViewport( 0, 0, width, height  );


	// settings
	float gamma = 1.0f;
	float isBlured = 1.0f;
	float sobelCoef = 1.0f;

    do
    {
        t = glfwGetTime();

        // Mouse states
        int leftButton = glfwGetMouseButton( GLFW_MOUSE_BUTTON_LEFT );
        int rightButton = glfwGetMouseButton( GLFW_MOUSE_BUTTON_RIGHT );
        int middleButton = glfwGetMouseButton( GLFW_MOUSE_BUTTON_MIDDLE );

        if( leftButton == GLFW_PRESS )
            guiStates.turnLock = true;
        else
            guiStates.turnLock = false;

        if( rightButton == GLFW_PRESS )
            guiStates.zoomLock = true;
        else
            guiStates.zoomLock = false;

        if( middleButton == GLFW_PRESS )
            guiStates.panLock = true;
        else
            guiStates.panLock = false;

        // Camera movements
        int altPressed = glfwGetKey(GLFW_KEY_LSHIFT);
        if (!altPressed && (leftButton == GLFW_PRESS || rightButton == GLFW_PRESS || middleButton == GLFW_PRESS))
        {
            int x; int y;
            glfwGetMousePos(&x, &y);
            guiStates.lockPositionX = x;
            guiStates.lockPositionY = y;
        }
        if (altPressed == GLFW_PRESS)
        {
            int mousex; int mousey;
            glfwGetMousePos(&mousex, &mousey);
            int diffLockPositionX = mousex - guiStates.lockPositionX;
            int diffLockPositionY = mousey - guiStates.lockPositionY;
            if (guiStates.zoomLock)
            {
                float zoomDir = 0.0;
                if (diffLockPositionX > 0)
                    zoomDir = -1.f;
                else if (diffLockPositionX < 0 )
                    zoomDir = 1.f;
                camera_zoom(camera, zoomDir * GUIStates::MOUSE_ZOOM_SPEED);
            }
            else if (guiStates.turnLock)
            {
                camera_turn(camera, diffLockPositionY * GUIStates::MOUSE_TURN_SPEED,
                            diffLockPositionX * GUIStates::MOUSE_TURN_SPEED);

            }
            else if (guiStates.panLock)
            {
                camera_pan(camera, diffLockPositionX * GUIStates::MOUSE_PAN_SPEED,
                            diffLockPositionY * GUIStates::MOUSE_PAN_SPEED);
            }
            guiStates.lockPositionX = mousex;
            guiStates.lockPositionY = mousey;
        }

        // Default states
        glEnable(GL_DEPTH_TEST);

        // Clear the front buffer
        //~ glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Get camera matrices
        glm::mat4 projection = glm::perspective(45.0f, widthf / heightf, 0.1f, 100.f); 
        glm::mat4 worldToView = glm::lookAt(camera.eye, camera.o, camera.up);
        glm::mat4 objectToWorld;

        // Select textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        //~ glActiveTexture(GL_TEXTURE1);
        //~ glBindTexture(GL_TEXTURE_2D, textures[1]);

        // Select shader
        glUseProgram(baseShader.program);

        // Upload uniforms
        glUniformMatrix4fv(base_projectionLocation, 1, 0, glm::value_ptr(projection));
        glUniformMatrix4fv(base_viewLocation, 1, 0, glm::value_ptr(worldToView));
        glUniformMatrix4fv(base_objectLocation, 1, 0, glm::value_ptr(objectToWorld));
        glUniform1f(base_timeLocation, t);
        glUniform3fv(base_cameraPositionLocation, 1, glm::value_ptr(camera.eye));
        glUniform1i(base_textureLocation, 0);

        // Render vaos
        
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        
        // fx fbo
        glBindFramebuffer(GL_FRAMEBUFFER, fxFbo);
        
        // Viewport 
        glViewport( 0, 0, width, height  );
		// link texture
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[0], 0);
        //~ glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        
        
        glBindVertexArray(vao[0]);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        
        // ----------
        // -- blur --
        // ----------
        glUseProgram(blur_shader.program);
        
        glActiveTexture(GL_TEXTURE0);        
        glUniform1i(blur_tex1Location, 0);
        glUniform1i(blur_isBluredLocation, (int)isBlured);
        
        glBindTexture(GL_TEXTURE_2D, fxTextures[0]);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, fxTextures[1], 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glBindVertexArray(vao[1]);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        
        
        // -----------
        // -- sobel --
        // -----------
        glUseProgram(sobel_shader.program);
        
        glActiveTexture(GL_TEXTURE0);
        glUniform1i(sobel_tex1Location, 0);
        glUniform1f(sobel_sobelLocation, sobelCoef);
        
        glBindTexture(GL_TEXTURE_2D, fxTextures[1]);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fxTextures[0], 0);
        
        glBindVertexArray(vao[1]);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        
        
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        
        // result display
        // bind default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // Viewport 
        glViewport( 0, 0, width, height);

        // Clear the front buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Bind gamma shader
        glUseProgram(gamma_shader.program);
        // Upload uniforms
		glUniform1i(gamma_tex1Location, 0);
		
		// bind last fx texture used
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fxTextures[0]);
        
		glUniform1f(gamma_gammaLocation, gamma);
		
		//render full screen quad
		glBindVertexArray(vao[1]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        

#if 1
        // Draw UI
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, width, height);

        unsigned char mbut = 0;
        int mscroll = 0;
        int mousex; int mousey;
        glfwGetMousePos(&mousex, &mousey);
        mousey = height - mousey;

        if( leftButton == GLFW_PRESS )
            mbut |= IMGUI_MBUT_LEFT;

        imguiBeginFrame(mousex, mousey, mbut, mscroll);
        int logScroll = 0;
        char lineBuffer[512];
        imguiBeginScrollArea("src", width - 210, height - 310, 200, 300, &logScroll);
        sprintf(lineBuffer, "FPS %f", fps);
        imguiLabel(lineBuffer);
        imguiSlider("Gamma", &gamma, 0.0, 2.0, 0.1);
        imguiSlider("Blured", &isBlured, 0.0, 1.0, 1.0);
        imguiSlider("Sobel", &sobelCoef, 0.0, 2.0, 0.1);

        imguiEndScrollArea();
        imguiEndFrame();
        imguiRenderGLDraw(width, height);

        glDisable(GL_BLEND);
#endif
        // Check for errors
        GLenum err = glGetError();
        if(err != GL_NO_ERROR)
        {
            fprintf(stderr, "OpenGL Error : %s\n", gluErrorString(err));
        }

        // Swap buffers
        glfwSwapBuffers();

        double newTime = glfwGetTime();
        fps = 1.f/ (newTime - t);
    } // Check if the ESC key was pressed or the window was closed
    while( glfwGetKey( GLFW_KEY_ESC ) != GLFW_PRESS &&
           glfwGetWindowParam( GLFW_OPENED ) );

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    exit( EXIT_SUCCESS );
}


int  compile_and_link_shader(ShaderGLSL & shader, int typeMask, const char * sourceBuffer, int bufferSize)
{
    // Create program object
    shader.program = glCreateProgram();
    
    //Handle Vertex Shader
    GLuint vertexShaderObject ;
    if (typeMask & ShaderGLSL::VERTEX_SHADER)
    {
        // Create shader object for vertex shader
        vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);
        // Add #define VERTEX to buffer
        const char * sc[3] = { "#version 150\n", "#define VERTEX\n", sourceBuffer};
        glShaderSource(vertexShaderObject, 
                       3, 
                       sc,
                       NULL);
        // Compile shader
        glCompileShader(vertexShaderObject);

        // Get error log size and print it eventually
        int logLength;
        glGetShaderiv(vertexShaderObject, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1)
        {
            char * log = new char[logLength];
            glGetShaderInfoLog(vertexShaderObject, logLength, &logLength, log);
            fprintf(stderr, "Error in compiling vertex shader : %s", log);
            fprintf(stderr, "%s\n%s\n%s", sc[0], sc[1], sc[2]);
            delete[] log;
        }
        // If an error happend quit
        int status;
        glGetShaderiv(vertexShaderObject, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
            return -1;          

        //Attach shader to program
        glAttachShader(shader.program, vertexShaderObject);
    }

    // Handle Geometry shader
    GLuint geometryShaderObject ;
    if (typeMask & ShaderGLSL::GEOMETRY_SHADER)
    {
        // Create shader object for Geometry shader
        geometryShaderObject = glCreateShader(GL_GEOMETRY_SHADER);
        // Add #define Geometry to buffer
        const char * sc[3] = { "#version 150\n", "#define GEOMETRY\n", sourceBuffer};
        glShaderSource(geometryShaderObject, 
                       3, 
                       sc,
                       NULL);
        // Compile shader
        glCompileShader(geometryShaderObject);

        // Get error log size and print it eventually
        int logLength;
        glGetShaderiv(geometryShaderObject, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1)
        {
            char * log = new char[logLength];
            glGetShaderInfoLog(geometryShaderObject, logLength, &logLength, log);
            fprintf(stderr, "Error in compiling Geometry shader : %s \n", log);
            fprintf(stderr, "%s\n%s\n%s", sc[0], sc[1], sc[2]);
            delete[] log;
        }
        // If an error happend quit
        int status;
        glGetShaderiv(geometryShaderObject, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
            return -1;          

        //Attach shader to program
        glAttachShader(shader.program, geometryShaderObject);
    }


    // Handle Fragment shader
    GLuint fragmentShaderObject ;
    if (typeMask && ShaderGLSL::FRAGMENT_SHADER)
    {
        // Create shader object for fragment shader
        fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);
        // Add #define fragment to buffer
        const char * sc[3] = { "#version 150\n", "#define FRAGMENT\n", sourceBuffer};
        glShaderSource(fragmentShaderObject, 
                       3, 
                       sc,
                       NULL);
        // Compile shader
        glCompileShader(fragmentShaderObject);

        // Get error log size and print it eventually
        int logLength;
        glGetShaderiv(fragmentShaderObject, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 1)
        {
            char * log = new char[logLength];
            glGetShaderInfoLog(fragmentShaderObject, logLength, &logLength, log);
            fprintf(stderr, "Error in compiling fragment shader : %s \n", log);
            fprintf(stderr, "%s\n%s\n%s", sc[0], sc[1], sc[2]);
            delete[] log;
        }
        // If an error happend quit
        int status;
        glGetShaderiv(fragmentShaderObject, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE)
            return -1;          

        //Attach shader to program
        glAttachShader(shader.program, fragmentShaderObject);
    }


    // Bind attribute location
    glBindAttribLocation(shader.program,  0,  "VertexPosition");
    glBindAttribLocation(shader.program,  1,  "VertexNormal");
    glBindAttribLocation(shader.program,  2,  "VertexTexCoord");
    glBindFragDataLocation(shader.program, 0, "Color");
    glBindFragDataLocation(shader.program, 1, "Normal");

    // Link attached shaders
    glLinkProgram(shader.program);

    // Clean
    if (typeMask & ShaderGLSL::VERTEX_SHADER)
    {
        glDeleteShader(vertexShaderObject);
    }
    if (typeMask && ShaderGLSL::GEOMETRY_SHADER)
    {
        glDeleteShader(fragmentShaderObject);
    }
    if (typeMask && ShaderGLSL::FRAGMENT_SHADER)
    {
        glDeleteShader(fragmentShaderObject);
    }

    // Get link error log size and print it eventually
    int logLength;
    glGetProgramiv(shader.program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetProgramInfoLog(shader.program, logLength, &logLength, log);
        fprintf(stderr, "Error in linking shaders : %s \n", log);
        delete[] log;
    }
    int status;
    glGetProgramiv(shader.program, GL_LINK_STATUS, &status);        
    if (status == GL_FALSE)
        return -1;


    return 0;
}

int  destroy_shader(ShaderGLSL & shader)
{
    glDeleteProgram(shader.program);
    shader.program = 0;
    return 0;
}

int load_shader_from_file(ShaderGLSL & shader, const char * path, int typemask)
{
    int status;
    FILE * shaderFileDesc = fopen( path, "rb" );
    if (!shaderFileDesc)
        return -1;
    fseek ( shaderFileDesc , 0 , SEEK_END );
    long fileSize = ftell ( shaderFileDesc );
    rewind ( shaderFileDesc );
    char * buffer = new char[fileSize + 1];
    fread( buffer, 1, fileSize, shaderFileDesc );
    buffer[fileSize] = '\0';
    status = compile_and_link_shader( shader, typemask, buffer, fileSize );
    delete[] buffer;
    return status;
}


void camera_compute(Camera & c)
{
    c.eye.x = cos(c.theta) * sin(c.phi) * c.radius + c.o.x;   
    c.eye.y = cos(c.phi) * c.radius + c.o.y ;
    c.eye.z = sin(c.theta) * sin(c.phi) * c.radius + c.o.z;   
    c.up = glm::vec3(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
}

void camera_defaults(Camera & c)
{
    c.phi = 3.14/2.f;
    c.theta = 3.14/2.f;
    c.radius = 10.f;
    camera_compute(c);
}

void camera_zoom(Camera & c, float factor)
{
    c.radius += factor * c.radius ;
    if (c.radius < 0.1)
    {
        c.radius = 10.f;
        c.o = c.eye + glm::normalize(c.o - c.eye) * c.radius;
    }
    camera_compute(c);
}

void camera_turn(Camera & c, float phi, float theta)
{
    c.theta += 1.f * theta;
    c.phi   -= 1.f * phi;
    if (c.phi >= (2 * M_PI) - 0.1 )
        c.phi = 0.00001;
    else if (c.phi <= 0 )
        c.phi = 2 * M_PI - 0.1;
    camera_compute(c);
}

void camera_pan(Camera & c, float x, float y)
{
    glm::vec3 up(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
    glm::vec3 fwd = glm::normalize(c.o - c.eye);
    glm::vec3 side = glm::normalize(glm::cross(fwd, up));
    c.up = glm::normalize(glm::cross(side, fwd));
    c.o[0] += up[0] * y * c.radius * 2;
    c.o[1] += up[1] * y * c.radius * 2;
    c.o[2] += up[2] * y * c.radius * 2;
    c.o[0] -= side[0] * x * c.radius * 2;
    c.o[1] -= side[1] * x * c.radius * 2;
    c.o[2] -= side[2] * x * c.radius * 2;       
    camera_compute(c);
}
