#include "core/Graph.h"
#include "route/Router.h"
#include "io/JsonImporter.h"
#include "ui/UiOverlay.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <iostream>

using namespace olsr;

static void glfwErrorCallback(int error, const char* desc) {
    std::cerr << "GLFW Error " << error << ": " << desc << "\n";
}

int run_gui(int argc, char** argv) {
    std::string topoPath;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--topo" && i + 1 < argc) topoPath = argv[++i];
        if (arg == "--no-gui") return 0; // if explicitly disabled, just skip
    }

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1280, 800, "OLSR-lite", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 2; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to load OpenGL via GLAD\n";
        return 3;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Graph g;
    if (!topoPath.empty()) {
        JsonImporter imp; std::string err;
        if (!imp.loadTopology(topoPath, g, &err)) {
            std::cerr << "Error loading topology: " << err << "\n";
        }
    } else {
        auto n1 = g.addNode("R1", 200, 200);
        auto n2 = g.addNode("R2", 400, 200);
        g.addLink(n1, n2, 1.0);
    }
    Router router; router.recomputeAll(g);
    UiOverlay ui(g, router);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Keyboard shortcuts
        ImGuiIO& io2 = ImGui::GetIO();
        if (io2.WantCaptureKeyboard == false) {
            // R = recompute
            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_R))) {
                router.recomputeAll(g);
            }
            // E = export using default GUI path
            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_E))) {
                JsonExporter exp; exp.exportRoutes(g, router, "build/routes_gui.json");
            }
            // J = Jam/Unjam selected link
            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_J))) {
                ui.toggleSelectedLinkJam();
            }
        }

        ui.draw();

        ImGui::Render();
        int display_w, display_h; glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


