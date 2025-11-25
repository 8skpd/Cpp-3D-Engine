#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include "renderer.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow* window = glfwCreateWindow(1000, 700, "3D Engine by Michael Velikanov", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    Renderer renderer(1000, 700);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        float fps = 1.0f / deltaTime;

        // Обновляем заголовок окна
        std::string title = "3D Engine by Michael Velikanov - FPS: " + std::to_string((int)fps);
        glfwSetWindowTitle(window, title.c_str());

        glfwPollEvents();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Settings");
        ImGui::SliderFloat("Ambient Intensity", &Renderer::ambientIntensity, 0.0f, 1.0f);
        ImGui::Checkbox("Enable Back Face Culling", &Renderer::enableCulling);
        ImGui::Checkbox("Auto Rotate", &Renderer::autoRotate);
        const char* lightModels[] = { "Lambert", "Phong" };
        static int currentLightModelItem = 0;
        
        // Выбор модели освещения
        if (ImGui::Combo("Light Model", &currentLightModelItem, lightModels, IM_ARRAYSIZE(lightModels))) {
            switch (currentLightModelItem) {
                case 0: Renderer::currentLightModel = LightModel::Lambert; break;
                case 1: Renderer::currentLightModel = LightModel::Phong; break;
            }
        }
        
        // Выбор фигуры
        const char* items[] = { "Cube", "Sphere", "Torus", "Pyramid", "Mobius Strip", "Custom OBJ" };
        static int current_item = 0;
        if (ImGui::Combo("Shape", &current_item, items, IM_ARRAYSIZE(items))) {
            switch (current_item) {
                case 0: Renderer::currentShape = Cube; break;
                case 1: Renderer::currentShape = Sphere; break;
                case 2: Renderer::currentShape = Torus; break;
                case 3: Renderer::currentShape = Pyramid; break;
                case 4: Renderer::currentShape = MobiusStrip; break;
                case 5: Renderer::currentShape = CustomOBJ; break;
            }
        }

        if (Renderer::currentShape == CustomOBJ) {
            static char path[512] = "assets/model.obj"; // путь по умолчанию
            ImGui::InputText("OBJ Path", path, sizeof(path));
            if (ImGui::Button("Load OBJ")) {
                Renderer::objFilePath = std::string(path);
            }
        }

        ImGui::Text("FPS: %.1f", fps);

        // Управление камерой
        ImGui::SliderFloat("Camera Distance", &Renderer::cameraDistance, 1.0f, 20.0f);
        ImGui::SliderFloat("Camera Angle X", &Renderer::cameraAngleX, -1.57f, 1.57f);
        ImGui::SliderFloat("Camera Angle Y", &Renderer::cameraAngleY, -3.14f, 3.14f);

        ImGui::End();

        renderer.renderScene();
        renderer.present();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
