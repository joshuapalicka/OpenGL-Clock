//TextureMapping.cpp - Load and display a .obj file with texture-mapping
//Orig. created by Prof. Jules Bloomenthal & modified by Joshua Palicka

#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include "GLXtras.h"
#include <vector>
#include <Mesh.h>
#include <Camera.h>
#include "Misc.h"
#include <gl.h>
#include <Draw.h>
#include <time.h>

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

vec2 mouseDown(0, 0);
vec2 rotOld(0, 0), rotNew(0, 0);
float rotSpeed = 1; //amt of rotation

//textures
const char* clockfaceTexture = "E:/GraphicsDir/Assignments/ClockProject/obj_tx/clockface.png";
const char* clocklinesTexture = "E:/GraphicsDir/Assignments/ClockProject/obj_tx/clocklines.png";
const char* secondhandTexture = "E:/GraphicsDir/Assignments/ClockProject/obj_tx/secondhand.png";
const char* minutehandTexture = "E:/GraphicsDir/Assignments/ClockProject/obj_tx/minutehand.png";
const char* hourhandTexture = "E:/GraphicsDir/Assignments/ClockProject/obj_tx/hourhand.png";

//objects
const char* clockface = "E:/GraphicsDir/Assignments/ClockProject/obj_tx/clockface.obj";
const char* clocklines = "E:/GraphicsDir/Assignments/ClockProject/obj_tx/clocklines.obj";
const char* hourhand = "E:/GraphicsDir/Assignments/ClockProject/obj_tx/hourhand.obj";
const char* minutehand = "E:/GraphicsDir/Assignments/ClockProject/obj_tx/minutehand.obj";
const char* secondhand = "E:/GraphicsDir/Assignments/ClockProject/obj_tx/secondhand.obj";

const char* names[] = { clockface, clocklines, hourhand, minutehand, secondhand };
const char* textureNames[] = { clockfaceTexture, clocklinesTexture, hourhandTexture, minutehandTexture, secondhandTexture };

float last_hour = 0;
float last_minute = 0;
float last_second = 0;

//global time obj & struct for use in getTime function
time_t current_time;
struct tm* current_time_struct;

const int nmeshes = sizeof(names) / sizeof(char*);
Mesh meshes[nmeshes];

int winW = 400, winH = 400;
CameraAB camera(0, 0, winW, winH, vec3(0, -90, -90), vec3(0, 0, -5));

float getTime(string hand) {
    //originally I created current_time and current_time_struct every call but performance-wise, this should be the better way to do it.
    current_time = time(0);
    current_time_struct = localtime(&current_time);

    //casts parts of the struct to floats and assigns them to second, minute, and hour - hour is modded by 12 as ctime is a 24hr clock
    float second = static_cast<float>(current_time_struct->tm_sec);
    float minute = static_cast<float>(current_time_struct->tm_min);
    float hour = static_cast<float>((current_time_struct->tm_hour) % 12);

    //these just give the angle that each clock hand should be at, depending on input string

    if (hand == "hour") {
        return (hour + (minute / 60) + (second / 3600)) * (360 / 12);
    }

    else if (hand == "minute") {
        return (minute + (second / 60)) * (360 / 60);
    }

    else if (hand == "second") {
        return (second * (360 / 60));
    }
}

void MouseButton(GLFWwindow* w, int butn, int action, int mods) {
    if (action == GLFW_PRESS) {
        double x, y;
        glfwGetCursorPos(w, &x, &y);
        camera.MouseDown(x, y);
        
    }
    if (action == GLFW_RELEASE)
        rotOld = rotNew;
}

void MouseMove(GLFWwindow* w, double x, double y) {
    if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        camera.MouseDrag(x, y);
        //vec2 dif((float)x - mouseDown.x, (float)y - mouseDown.y);
        //rotNew = rotOld + rotSpeed * dif;
    }
}

void Resize(GLFWwindow* window, int width, int height) {
    camera.Resize(winW = width, winH = height);
    glViewport(0, 0, winW, winH);
}

void Display() {
    // clear background
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // access GPU vertex buffer
    glUseProgram(program);
    glBindBuffer(GL_ARRAY_BUFFER, vBuffer);

    // associate position input to shader with position array in vertex buffer
    UseMeshShader();

    // get current second, minute, hour
    float cur_second = getTime("second");
    float cur_minute = getTime("minute");
    float cur_hour = getTime("hour");

    //set the new transform matrix of each hand to the difference between the last position and this position & multiply by old transform matrices
    meshes[2].transform = RotateY((-1) * cur_hour + last_hour)* meshes[2].transform;
    meshes[3].transform = RotateY((-1) * cur_minute + last_minute)* meshes[3].transform;
    meshes[4].transform = RotateY((-1) * cur_second + last_second)* meshes[4].transform;

    for (int i = 0; i < nmeshes; i++) {
        meshes[i].Display(camera);
    }

    //set last_second/minute/hour for next frame
    last_second = cur_second;
    last_minute = cur_minute;
    last_hour = cur_hour;

    glDisable(GL_DEPTH_TEST);
    glFlush();
}

// application
void Keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // Seemed like the simplest way to implement easy-to-use zoom keys
    if (action == GLFW_PRESS)
        switch (key) {
        case 'W': camera.MouseWheel(5, true); break;
        case 'S': camera.MouseWheel(-5, true); break;
       }
}

void ErrorGFLW(int id, const char *reason) {
    printf("GFLW error %i: %s\n", id, reason);
}

void Close() {
    // unbind vertex buffer and free GPU memory
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vBuffer);
}


int main() {
    glfwSetErrorCallback(ErrorGFLW);
    if (!glfwInit())
        return 1;

    //supersampling to cut down on aliasing
    glfwWindowHint(GLFW_SAMPLES, 32);

    GLFWwindow *w = glfwCreateWindow(winW, winH, "Moving Clock", NULL, NULL);
    if (!w) {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(w);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSetWindowSizeCallback(w, Resize);

    //reads in meshes and textures
    for (int i = 0; i < nmeshes; i++)
        meshes[i].Read(names[i], textureNames[i], 1 + i);
    
    //initial transforms to get the meshes where I want them
    meshes[0].transform = Translate(0, -0.15f, 0); //clockface
    meshes[1].transform = Scale(.99f, .5f, .99f) * Translate(0, 0, 0);
    meshes[2].transform = Scale(.3f, .6f, .5f) * Translate(-.5f, .03f, 0); //hourhand
    meshes[3].transform = Scale(.6f, .3f, 1) * Translate(-.5f, .04f, 0); //minutehand
    meshes[4].transform = Scale(.5f, 1, .5f) * Translate(-.5f, .04f, 0); //secondhand

    glfwSetKeyCallback(w, Keyboard);
    glfwSwapInterval(1); // ensure no generated frame backlog
    glfwSetMouseButtonCallback(w, MouseButton);
    glfwSetCursorPosCallback(w, MouseMove);

    // event loop
    while (!glfwWindowShouldClose(w)) {
        Display();
        glfwSwapBuffers(w);
        glfwPollEvents();
    }

    Close();
    glfwDestroyWindow(w);
    glfwTerminate();
}
