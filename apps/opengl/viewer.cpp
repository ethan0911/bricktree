// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
# undef APIENTRY
# define GLFW_EXPOSE_NATIVE_WIN32
# define GLFW_EXPOSE_NATIVE_WGL
# include <GLFW/glfw3native.h>
#endif
#include <iostream>
//! @name error check helper from EPFL ICG class
static inline const char *ErrorString(GLenum error) {
  const char *msg;
  switch (error) {
#define Case(Token)  case Token: msg = #Token; break;
    Case(GL_INVALID_ENUM);
    Case(GL_INVALID_VALUE);
    Case(GL_INVALID_OPERATION);
    Case(GL_INVALID_FRAMEBUFFER_OPERATION);
    Case(GL_NO_ERROR);
    Case(GL_OUT_OF_MEMORY);
#undef Case
  }
  return msg;
}

//! @name check error
static inline void _glCheckError
  (const char *file, int line, const char *comment) {
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "ERROR: %s (file %s, line %i: %s).\n", 
            comment, file, line, ErrorString(error));
  }
}
#ifndef NDEBUG
# define check_error_gl(x) _glCheckError(__FILE__, __LINE__, x)
#else
# define check_error_gl(x) ((void)0)
#endif

// ======================================================================== //
#include "viewer.h"
#include "common/constants.h"
#include "scene/properties.h"
#include "camera.h"
#include "engine.h"
#include "ospcommon/vec.h"
// ======================================================================== //
using namespace viewer;
static affine3f Identity(vec3f(1, 0, 0),
                         vec3f(0, 1, 0),
                         vec3f(0, 0, 1),
                         vec3f(0, 0, 0));
static std::vector<GLFWwindow *> windowmap;
static std::vector<uint32_t> displaybuffer;

// ======================================================================== //
static OSPModel              ospMod;
static OSPRenderer           ospRen;

static CameraProp               camProp;
static LightListProp            litProp;
static RendererProp             renProp(camProp, litProp);
static TransferFunctionProp     tfnProp;

static Engine engine;
static Camera camera(camProp);
bool viewer::widgets::Commit() {
  bool update = false;
  if (camProp.Commit()) { update = true; }
  if (litProp.Commit()) { update = true; }
  if (renProp.Commit()) { update = true; }
  if (tfnProp.Commit()) { update = true; }
  return update;
}

// ======================================================================== //
#include <imgui.h>
#include <imgui_glfw_impi.h>
void WidgetInit(GLFWwindow *window) {
  ImGui_Impi_Init(window, false);
  tfnProp.Init();
}
void WidgetStop() { 
  ImGui_Impi_Shutdown();
}
void WidgetDraw() {
  ImGui_Impi_NewFrame();

  // -- Main Menu
  // parameters
  static bool show_app_camera = false;
  static bool show_app_lights = false;
  static bool show_app_renderer = false;
  static bool show_app_tfn = true;
  // debug
  static bool show_app_metrics = true;
  static bool show_app_imgui_test_win = false;
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Parameters"))
    {
      ImGui::MenuItem("Camera", NULL, &show_app_camera);
      ImGui::MenuItem("Lights", NULL, &show_app_lights);
      ImGui::MenuItem("Renderer", NULL, &show_app_renderer);
      ImGui::MenuItem("Transfer Function", NULL, &show_app_tfn);
      ImGui::EndMenu();                 
    }
    if (ImGui::BeginMenu("Debug"))
    {
      ImGui::MenuItem("Show Rendering Metrics", NULL, &show_app_metrics);
      ImGui::MenuItem("Show ImGui Test Window", NULL, &show_app_imgui_test_win);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  if (show_app_camera) {
    if (ImGui::Begin("Camera Parameters", &show_app_camera, 0))
    { camProp.Draw(); }
    ImGui::End();
  }
  if (show_app_lights) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(250,200),ImVec2(FLT_MAX,FLT_MAX));
    if (ImGui::Begin("Light Parameters", &show_app_lights, 0))
    { litProp.Draw(); }
    ImGui::End();
  }
  if (show_app_renderer) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(250,400),ImVec2(FLT_MAX,FLT_MAX));
    if (ImGui::Begin("Renderer Parameters", &show_app_renderer, 0))
    { renProp.Draw(); }
    ImGui::End();
  }
  if (show_app_tfn) tfnProp.Draw(&show_app_tfn);

  // -- draw Metrics
  if (show_app_metrics) {
    static const float DISTANCE = 10.0f;
    static int corner = 0;
    ImVec2 window_pos = ImVec2((corner & 1) ? 
                               ImGui::GetIO().DisplaySize.x - DISTANCE : 
                               DISTANCE, 
                               (corner & 2) ? 
                               ImGui::GetIO().DisplaySize.y - DISTANCE :
                               DISTANCE + 18.f);
    ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, 
                                     (corner & 2) ? 1.0f : 0.0f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    if (ImGui::Begin("Metrics", &show_app_tfn, 
                     ImGuiWindowFlags_NoTitleBar|
                     ImGuiWindowFlags_NoResize|
                     ImGuiWindowFlags_AlwaysAutoResize|
                     ImGuiWindowFlags_NoMove|
                     ImGuiWindowFlags_NoSavedSettings))
    {
      ImGui::Text("FPS: %s", std::to_string(engine.GetFPS()).c_str());
    }
    ImGui::End();
    ImGui::PopStyleColor();
  }

  // -- ImGUI Example:  
  if (show_app_imgui_test_win) {
    ImGui::ShowTestWindow(&show_app_imgui_test_win);
  }

  ImGui::Render();
}

// ======================================================================== //
#include "common/navsphere.h"
static Sphere sphere;

// ======================================================================== //
//
// ======================================================================== //
void RenderWindow(GLFWwindow *window);
GLFWwindow *CreateWindow();
namespace viewer {
  int Init(const int ac, const char **av, const size_t &w, const size_t &h)
  {
    camera.SetSize(w, h);
    windowmap.push_back(CreateWindow());
    return windowmap.size() - 1;
  }
  void Render(int id)
  {
    sphere.Init();
    litProp.Append("ambient", "scivis");
    litProp[0].SetIntensity(0.025f);
    litProp[0].SetColor(vec3f(-0.75f,0.70f,-0.75f));
    litProp.Append("distant", "scivis");
    litProp[1].SetIntensity(2.500f);
    litProp[1].SetColor(vec3f(1.00f,1.00f,1.00f));
    litProp[1].SetDirection(vec3f(-1.00f,-1.00f,0.65f));
    litProp.Append("distant", "scivis");
    litProp[2].SetIntensity(2.500f);
    litProp[2].SetColor(vec3f(1.00f,1.00f,1.00f));
    litProp[2].SetDirection(vec3f(1.00f,0.65f,0.65f));
    litProp.Finalize();
    ospSetData(ospRen, "lights", *litProp);
    ospCommit(ospRen);
    engine.Init(camera.CameraWidth(), camera.CameraHeight(), ospRen);
    RenderWindow(windowmap[id]);
  };
  void Handler(OSPCamera c, const std::string& type,
               const osp::vec3f &vp,
               const osp::vec3f &vu,
               const osp::vec3f &vi)
  {
    camera.SetViewPort(vec3f(vp.x, vp.y, vp.z),
                       vec3f(vu.x, vu.y, vu.z),
                       vec3f(vi.x, vi.y, vi.z),
                       60.f);
    camera.Init(c, type);
  };
  void Handler(OSPModel m, OSPRenderer r)
  {
    ospMod = m;
    ospRen = r;
    renProp.Init(r, viewer::RendererProp::Scivis);
  };
  void Handler(OSPTransferFunction t, const float &a, const float &b)
  {
    tfnProp.Create(t, a, b);
  };
}; // namespace viewer

// ======================================================================== //
// Callback Functions
// ======================================================================== //
static GLuint texID;
static GLuint fboID;
void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}
void char_callback(GLFWwindow* window, unsigned int key)
{
  ImGui_Impi_CharCallback(window, key);
}
void key_onhold_callback(GLFWwindow *window)
{
  if (ImGui::GetIO().WantCaptureKeyboard) {
    return;
  }
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    /* UP: forward */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMoveNZ(0.5f * camera.CameraFocalLength());
    } else {
      camera.CameraMoveNZ(0.0001f * camera.CameraFocalLength());
    }
  } else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    /* DOWN: backward */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMoveNZ(-0.5f * camera.CameraFocalLength());
    } else {
      camera.CameraMoveNZ(-0.0001f * camera.CameraFocalLength());
    }
  } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    /* A: left */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMovePX(0.5f * camera.CameraFocalLength());
    } else {
      camera.CameraMovePX(0.0001f * camera.CameraFocalLength());
    }
  } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    /* D: right */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMovePX(-0.5f * camera.CameraFocalLength());
    } else {
      camera.CameraMovePX(-0.0001f * camera.CameraFocalLength());
    }
  } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    /* S: down */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMovePY(0.5f * camera.CameraFocalLength());
    } else {
      camera.CameraMovePY(0.01f * camera.CameraFocalLength());
    }
  } else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    /* W: up */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMovePY(-0.5f * camera.CameraFocalLength());
    } else {
      camera.CameraMovePY(-0.01f * camera.CameraFocalLength());
    }
  }
}
void key_callback(GLFWwindow *window, int key, 
                          int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
  if (!ImGui::GetIO().WantCaptureKeyboard) {
    if (key == GLFW_KEY_LEFT_ALT) {
      engine.Stop();
      if (action == GLFW_PRESS) {
        sphere.Add(ospMod);
      } else if (action == GLFW_RELEASE) {
        sphere.Remove(ospMod);
      }
      engine.Clear();
      engine.Start();
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
      tfnProp.Print();
    } else if (key == GLFW_KEY_V && action == GLFW_PRESS) {
      const auto vi = camera.CameraFocus();
      const auto vp = camera.CameraPos();
      const auto vu = camera.CameraUp();
      std::cout << std::endl
                << "-vp " << vp.x << " " << vp.y << " " << vp.z << " "
                << "-vi " << vi.x << " " << vi.y << " " << vi.z << " "
                << "-vu " << vu.x << " " << vu.y << " " << vu.z << " "
                << std::endl;
    }
  } else {
    ImGui_Impi_KeyCallback(window, key, scancode, action, mods);
  }
}
void mouse_button_callback(GLFWwindow* window, int button, 
                                  int action, int mods)
{
  ImGui_Impi_MouseButtonCallback(window, button, action, mods);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  ImGui_Impi_ScrollCallback(window, xoffset, yoffset);
}
void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
  if (!ImGui::GetIO().WantCaptureMouse) 
  {
    int left_state  = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    int right_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (left_state == GLFW_PRESS) {
      camera.CameraDrag((float)xpos, (float)ypos);
    } else {
      camera.CameraBeginDrag((float)xpos, (float)ypos);
    }
    if (right_state == GLFW_PRESS) {
      camera.CameraZoom((float)xpos, (float)ypos);
    } else {
      camera.CameraBeginZoom((float)xpos, (float)ypos);
    }
  }
}
void window_size_callback(GLFWwindow *window, int width, int height)
{
  // resize opengl
  glViewport(0, 0, width, height);
  glBindTexture(GL_TEXTURE_2D, texID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 
               0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  // resize ospray objects
  camera.CameraUpdateProj((size_t)width, (size_t)height);
  engine.Resize(width, height);
  // resize screen buffer
  displaybuffer.resize(camera.CameraWidth() * camera.CameraHeight());
}

// ======================================================================== //
//
// ======================================================================== //
void RenderWindow(GLFWwindow *window)
{
  // Init
  displaybuffer.resize(camera.CameraWidth() * camera.CameraHeight(), 0);
  WidgetInit(window);
  // Start
  engine.Start();
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    {
      key_onhold_callback(window);
      /* copy rendered buffer */
      if (engine.HasNewFrame()) {
        auto &mapped = engine.MapFramebuffer();
        if (mapped.size() == displaybuffer.size()) {
          auto *srcPixels = mapped.data();
          auto *dstPixels = displaybuffer.data();
          memcpy(dstPixels, srcPixels,
                 displaybuffer.size() * sizeof(uint32_t));
        }
        engine.UnmapFramebuffer();
      }
      /* display buffer*/
      glBindTexture(GL_TEXTURE_2D, texID);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
                      camera.CameraWidth(), camera.CameraHeight(), 
                      GL_RGBA, GL_UNSIGNED_BYTE,
                      displaybuffer.data());
      glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID);
      glBlitFramebuffer(0, 0, camera.CameraWidth(), camera.CameraHeight(), 
                        0, 0, camera.CameraWidth(), camera.CameraHeight(),
                        GL_COLOR_BUFFER_BIT, GL_NEAREST);
      glBindTexture(GL_TEXTURE_2D, 0);
      /* draw widgets */
      WidgetDraw();
    }
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  // ShutDown
  engine.Stop();
  WidgetStop();
  glfwDestroyWindow(window);
  glfwTerminate();
}
GLFWwindow *CreateWindow()
{
  // Initialize GLFW
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }
  // Provide Window Hints
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // Create Window
  GLFWwindow *window = glfwCreateWindow(camera.CameraWidth(),
                                        camera.CameraHeight(),
                                        "OSPRay Test Viewer",
                                        nullptr,
                                        nullptr);
  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }
  // Callback
  glfwSetWindowSizeCallback(window, window_size_callback);
  glfwSetCharCallback(window, char_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  // Ready
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSwapInterval(1);
  check_error_gl("Ready");
  // Setup OpenGL
  glEnable(GL_DEPTH_TEST);
  check_error_gl("Setup OpenGL Options");
  // Create OpenGL Texture
  glGenFramebuffers(1, &fboID);
  glGenTextures(1, &texID);
  glBindTexture(GL_TEXTURE_2D, texID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  glBindTexture(GL_TEXTURE_2D, texID);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
               camera.CameraWidth(), camera.CameraHeight(), 
               0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, fboID);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, texID, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glBindTexture(GL_TEXTURE_2D, 0);
  return window;
}
