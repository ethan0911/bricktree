// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#include "viewer.h"
#include "camera.h"
#include "common.h"
#include "framebuffer.h"

#include <atomic>
#include <thread>

#include <imgui.h>
#include <imgui_glfw_impi.h>

// ======================================================================== //
static affine3f Identity(vec3f(1, 0, 0),
                         vec3f(0, 1, 0),
                         vec3f(0, 0, 1),
                         vec3f(0, 0, 0));
using namespace viewer;
// ======================================================================== //
static std::vector<GLFWwindow *> windowmap;
static Camera camera;
static Framebuffer framebuffer;
static OSPCamera ospCam;
static OSPModel ospMod;
static OSPRenderer ospRen;
// ======================================================================== //
struct GeoProp
{
  OSPGeometry geo;
  float &isoValue;
  float vmin, vmax;
  bool hasMinMax;
  std::string name;
  GeoProp(OSPGeometry g,
          float &v,
          const float v0,
          const float v1,
          const std::string n)
      : geo(g), isoValue(v), vmin(v0), vmax(v1), name(n)
  {
    hasMinMax = vmin < vmax;
  }
  void Draw()
  {
    ImGui::Text(name.c_str());
    if (hasMinMax) {
      ImGui::SliderFloat(("IsoValue##" + name).c_str(), &isoValue, vmin, vmax);
    } else {
      ImGui::InputFloat(("IsoValue##" + name).c_str(), &isoValue);
    }
  }
  void Commit()
  {
    ospSet1f(geo, "isoValue", isoValue);
    ospCommit(geo);
  }
};
static std::vector<GeoProp> geoPropList;
// ======================================================================== //
struct btVolumeProp
{
  OSPVolume volume;

  btVolumeProp(OSPVolume v) : volume(v) {}

  void Commit()
  {
    ospCommit(volume);
  }
};
static std::vector<btVolumeProp> btVolumeList;
// ======================================================================== //
static std::vector<OSPLight> lightList;
struct LightProp
{
  OSPLight L;
  float I = 0.25f;
  vec3f D = vec3f(-1.f, 0.679f, -0.754f);
  vec3f C = vec3f(1.f, 1.f, 1.f);
  std::string type, name;
  LightProp(std::string str) : type(str)
  {
    L = ospNewLight(ospRen, str.c_str());
    ospSet1f(L, "angularDiameter", 0.53f);
    Commit();
    lightList.push_back(L);
    name = std::to_string(lightList.size());
  }
  void Draw()
  {
    ImGui::Text((type + "-" + name).c_str());
    ImGui::SliderFloat3(("direction##" + name).c_str(), &D.x, -1.f, 1.f);
    ImGui::SliderFloat3(("color##" + name).c_str(), &C.x, 0.f, 1.f);
    ImGui::SliderFloat(
        ("intensity##" + name).c_str(), &I, 0.f, 100000.f, "%.3f", 5.0f);
  }
  void Commit()
  {
    ospSet1f(L, "intensity", I);
    ospSetVec3f(L, "color", (osp::vec3f &)C);
    ospSetVec3f(L, "direction", (osp::vec3f &)D);
    ospCommit(L);
  }
};
static OSPData lights;
static std::vector<LightProp> lightPropList;
// ======================================================================== //
struct RendererProp
{
  int aoSamples              = 1;
  float aoDistance           = 100.f;
  bool shadowsEnabled        = true;
  int maxDepth               = 100;
  bool aoTransparencyEnabled = false;
  void Draw()
  {
    ImGui::SliderInt("maxDepth", &maxDepth, 0, 100, "%.0f");
    ImGui::SliderInt("aoSamples", &aoSamples, 0, 100, "%.0f");
    ImGui::SliderFloat("aoDistance", &aoDistance, 0.f, 1e20, "%.3f", 5.0f);
    ImGui::Checkbox("shadowsEnabled", &shadowsEnabled);
    ImGui::Checkbox("aoTransparencyEnabled", &aoTransparencyEnabled);
  }
  void Commit()
  {
    ospSet1i(ospRen, "aoSamples", aoSamples);
    ospSet1f(ospRen, "aoDistance", aoDistance);
    ospSet1i(ospRen, "shadowsEnabled", shadowsEnabled);
    ospSet1i(ospRen, "maxDepth", maxDepth);
    ospSet1i(ospRen, "aoTransparencyEnabled", aoTransparencyEnabled);
    ospSet1i(ospRen, "spp", 1);
    ospSet1i(ospRen, "autoEpsilon", 1);
    ospSet1f(ospRen, "epsilon", 0.001f);
    ospSet1f(ospRen, "minContribution", 0.001f);
    ospCommit(ospRen);
  }
};
static RendererProp renderProp;

// ======================================================================== //
#include "navsphere.h"
Sphere sphere;

// ======================================================================== //
#include "widgets/TransferFunctionWidget.h"
struct TfnProp
{
  OSPTransferFunction ospTfn;
  std::shared_ptr<tfn::tfn_widget::TransferFunctionWidget> tfnWidget;
  float tfnValueRange[2] = {0.f, 1.f};
  std::vector<float> cptr;
  std::vector<float> aptr;
  bool print = false;
  void Create(OSPTransferFunction o, const float a = 0.f, const float b = 1.f)
  {
    ospTfn           = o;
    tfnValueRange[0] = a;
    tfnValueRange[1] = b;
  }
  void Init()
  {
    if (ospTfn != nullptr) {
      tfnWidget = std::make_shared<tfn::tfn_widget::TransferFunctionWidget>(
          [&](const std::vector<float> &c,
              const std::vector<float> &a,
              const std::array<float, 2> &r) {
            cptr               = std::vector<float>(c);
            aptr               = std::vector<float>(a);
            OSPData colorsData = ospNewData(c.size() / 3, OSP_FLOAT3, c.data());
            ospCommit(colorsData);
            std::vector<float> o(a.size() / 2);
            for (int i = 0; i < a.size() / 2; ++i) { o[i] = a[2 * i + 1]; }
            OSPData opacitiesData = ospNewData(o.size(), OSP_FLOAT, o.data());

            ospCommit(opacitiesData);
            ospSetData(ospTfn, "colors", colorsData);
            ospSetData(ospTfn, "opacities", opacitiesData);
            ospSetVec2f(ospTfn, "valueRange", osp::vec2f{r[0], r[1]});
            ospCommit(ospTfn);
            ospRelease(colorsData);
            ospRelease(opacitiesData);
            ClearOSPRay();
          });
      tfnWidget->setDefaultRange(tfnValueRange[0], tfnValueRange[1]);
    }
  }
  void Print()
  {
    if ((!cptr.empty()) && !(aptr.empty())) {
      const std::vector<float> &c = cptr;
      const std::vector<float> &a = aptr;
      std::cout << std::endl
                << "static std::vector<float> colors = {" << std::endl;
      for (int i = 0; i < c.size() / 3; ++i) {
        std::cout << "    " << c[3 * i] << ", " << c[3 * i + 1] << ", "
                  << c[3 * i + 2] << "," << std::endl;
      }
      std::cout << "};" << std::endl;
      std::cout << "static std::vector<float> opacities = {" << std::endl;
      for (int i = 0; i < a.size() / 2; ++i) {
        std::cout << "    " << a[2 * i + 1] << ", " << std::endl;
      }
      std::cout << "};" << std::endl << std::endl;
    }
  }
  void Draw()
  {
    if (ospTfn != nullptr) {
      if (tfnWidget->drawUI()) {
        tfnWidget->render(128);
      };
    }
  }
};
struct TfnProp transferFcn;

// ======================================================================== //
static std::thread *osprayThread = nullptr;
static std::atomic<bool> osprayStop(false);
static std::atomic<bool> osprayClear(false);
static std::atomic<bool> osprayCommit(true);
namespace viewer {
  void StartOSPRay()
  {
    osprayStop   = false;
    osprayThread = new std::thread([=] {
      while (!osprayStop) {
        // commit ospray changes
        if (osprayCommit) {
          std::cout << "commit" << std::endl;
          for (auto &g : geoPropList) {
            g.Commit();
          }
          for (auto &v : btVolumeList) {
            v.Commit();
          }
          ospCommit(ospMod);
          for (auto &l : lightPropList) {
            l.Commit();
          }
          ospSetData(ospRen, "lights", lights);
          renderProp.Commit();          
          osprayCommit = false;
          osprayClear  = true;
        }
        // render
        if (osprayClear) {
          framebuffer.Clear();
          osprayClear = false;
        }
        sphere.Update(camera.CameraFocus(), ospMod);
        framebuffer.Render();
      }
    });
  }
  void StopOSPRay()
  {
    osprayStop = true;
    osprayThread->join();
    delete osprayThread;
  }
  void ClearOSPRay()
  {
    osprayClear = true;
  }
  void CommitOSPRay()
  {
    osprayCommit = true;
  }
  void ResizeOSPRay(int width, int height)
  {
    StopOSPRay();
    framebuffer.Resize((size_t)width, (size_t)height);
    StartOSPRay();
  }
  void UploadOSPRay()
  {
    framebuffer.Display();
  }
};  // namespace viewer
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
    lightPropList.emplace_back("DirectionalLight");
    lightPropList.emplace_back("DirectionalLight");
    lightPropList.emplace_back("AmbientLight");
    lights = ospNewData(
        lightList.size(), OSP_OBJECT, lightList.data(), OSP_DATA_SHARED_BUFFER);
    ospCommit(lights);
    ospSetData(ospRen, "lights", lights);
    ospCommit(ospRen);
    framebuffer.Init(camera.CameraWidth(), camera.CameraHeight(), ospRen);
    RenderWindow(windowmap[id]);
  };
  void Handler(OSPCamera c,
               const osp::vec3f &vp,
               const osp::vec3f &vu,
               const osp::vec3f &vi)
  {
    ospCam = c;
    camera.SetViewPort(vec3f(vp.x, vp.y, vp.z),
                       vec3f(vu.x, vu.y, vu.z),
                       vec3f(vi.x, vi.y, vi.z),
                       60.f);
    camera.Init(ospCam);
  };
  void Handler(OSPTransferFunction t, const float &a, const float &b)
  {
    transferFcn.Create(t, a, b);
  };
  void Handler(OSPModel m, OSPRenderer r)
  {
    ospMod = m;
    ospRen = r;
  };
  void Handler(OSPGeometry g,
               float &v,
               const char *n,
               const float vmin,
               const float vmax)
  {
    geoPropList.emplace_back(g, v, vmin, vmax, n);
  };

  void Handler(OSPVolume v)
  {
    btVolumeList.emplace_back(v);
  };
};  // namespace viewer

// ======================================================================== //
void error_callback(int error, const char *description)
{
  fprintf(stderr, "Error: %s\n", description);
}
void char_callback(GLFWwindow *window, unsigned int c)
{
  ImGuiIO &io = ImGui::GetIO();
  if (c > 0 && c < 0x10000) {
    io.AddInputCharacter((unsigned short)c);
  }
}
void key_onhold_callback(GLFWwindow *window)
{
  if (ImGui::GetIO().WantCaptureKeyboard) {
    return;
  }
  if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
    /* UP: forward */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMoveNZ(1.f * camera.CameraFocalLength());
    } else {
      camera.CameraMoveNZ(0.01f * camera.CameraFocalLength());
    }
    ClearOSPRay();
  } else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
    /* DOWN: backward */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMoveNZ(-0.5f * camera.CameraFocalLength());
    } else {
      camera.CameraMoveNZ(-0.01f * camera.CameraFocalLength());
    }
    ClearOSPRay();
  } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    /* A: left */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMovePX(1.f * camera.CameraFocalLength());
    } else {
      camera.CameraMovePX(0.01f * camera.CameraFocalLength());
    }
    ClearOSPRay();
  } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    /* D: right */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMovePX(-0.5f * camera.CameraFocalLength());
    } else {
      camera.CameraMovePX(-0.01f * camera.CameraFocalLength());
    }
    ClearOSPRay();
  } else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    /* S: down */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMovePY(1.f * camera.CameraFocalLength());
    } else {
      camera.CameraMovePY(0.01f * camera.CameraFocalLength());
    }
    ClearOSPRay();
  } else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    /* W: up */
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
      camera.CameraMovePY(-0.5f * camera.CameraFocalLength());
    } else {
      camera.CameraMovePY(-0.01f * camera.CameraFocalLength());
    }
    ClearOSPRay();
  }
}
void key_onpress_callback(GLFWwindow *window, int key, 
                          int scancode, int action, int mods)
{
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
    exit(0);
  }
  if (!ImGui::GetIO().WantCaptureKeyboard) {
    if (key == GLFW_KEY_LEFT_ALT) {
      StopOSPRay();
      if (action == GLFW_PRESS) {
        sphere.Add(ospMod);
      } else if (action == GLFW_RELEASE) {
        sphere.Remove(ospMod);
      }
      ClearOSPRay();
      StartOSPRay();
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
      transferFcn.Print();
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
void cursor_position_callback(GLFWwindow *window, double xpos, double ypos)
{
  if (!ImGui::GetIO().WantCaptureMouse) {
    int left_state  = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    int right_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (left_state == GLFW_PRESS) {
      camera.CameraDrag((float)xpos, (float)ypos);
      ClearOSPRay();
    } else {
      camera.CameraBeginDrag((float)xpos, (float)ypos);
    }
    if (right_state == GLFW_PRESS) {
      camera.CameraZoom((float)xpos, (float)ypos);
      ClearOSPRay();
    } else {
      camera.CameraBeginZoom((float)xpos, (float)ypos);
    }
  }
}
void window_size_callback(GLFWwindow *window, int width, int height)
{
  glViewport(0, 0, width, height);
  camera.CameraUpdateProj((size_t)width, (size_t)height);
  ResizeOSPRay(width, height);
}

// ======================================================================== //
void RenderWindow(GLFWwindow *window)
{
  // Init
  ImGui_Impi_Init(window, false);
  transferFcn.Init();
  // Start
  StartOSPRay();
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    {
      key_onhold_callback(window);
      UploadOSPRay();
      ImGui_Impi_NewFrame();
      transferFcn.Draw();
      ImGui::Begin("Rendering Properties");
      {
        renderProp.Draw();
        for (auto &g : geoPropList) {
          g.Draw();
        }
        for (auto &l : lightPropList) {
          l.Draw();
        }
        if (ImGui::Button("Commit")) {
          CommitOSPRay();
        };
      }
      ImGui::End();
      ImGui::Render();
    }
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  // ShutDown
  StopOSPRay();
  {
    ImGui_Impi_Shutdown();
  }
  glfwDestroyWindow(window);
  glfwTerminate();
}

// ======================================================================== //
GLFWwindow *CreateWindow()
{
  // Initialize GLFW
  glfwSetErrorCallback(error_callback);
  if (!glfwInit()) {
    exit(EXIT_FAILURE);
  }
  // Provide Window Hints
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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
  glfwSetKeyCallback(window, key_onpress_callback);
  glfwSetWindowSizeCallback(window, window_size_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glfwSetCharCallback(window, char_callback);
  // Ready
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSwapInterval(1);
  check_error_gl("Ready");
  // Setup OpenGL
  glEnable(GL_DEPTH_TEST);
  check_error_gl("Setup OpenGL Options");
  return window;
}
