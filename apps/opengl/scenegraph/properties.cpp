// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#include "properties.h"

#include <imgui.h>
#include <imgui_glfw_impi.h>
#include "widgets/TransferFunctionWidget.h"

// ======================================================================== //
//
// ======================================================================== //
using namespace ospcommon;

// ======================================================================== //
//
// ======================================================================== //
viewer::CameraProp::CameraProp(const CameraProp::Type& t)
  :
  type(t),
  /* camera */
  pos(vec3f(0.f)),
  dir(vec3f(0.f, 0.f, 1.f)),
  up(vec3f(0.f, 1.f, 0.f)),
  nearClip(1e-6f), 
  imageStart(vec2f(0.f)),
  imageEnd(vec2f(1.f)),
  shutterOpen(0.f),
  shutterClose(0.f),
  aspect(1.f),
  /* perspective camera */
  fovy(60.f),
  apertureRadius(0.f),
  focusDistance(1.f),
  architectural(false),
  stereoMode(0),
  interpupillaryDistance(0.0635f),
  /* orthographic camera */
  height(1.f)
{
  /* camera */
  pos.update();
  dir.update();
  up.update();
  nearClip.update();
  imageStart.update();
  imageEnd.update();
  shutterOpen.update();
  shutterClose.update();
  aspect.update();
  /* perspective camera */
  fovy.update();
  apertureRadius.update();
  focusDistance.update();
  architectural.update();
  stereoMode.update();
  interpupillaryDistance.update();
  /* orthographic camera */
  height.update();
}
void viewer::CameraProp::Init(OSPCamera camera) 
{
  if (camera == nullptr) { throw std::runtime_error("empty camera found"); }
  self = camera;
}
void viewer::CameraProp::Draw()
{
}
bool viewer::CameraProp::Commit()
{
  bool update = false;
  if (pos.update()) {
    ospSetVec3f(self, "pos", (osp::vec3f &) pos.ref()); 
    update = true;
  }
  if (dir.update()) {
    ospSetVec3f(self, "dir", (osp::vec3f &) dir.ref());
    update = true;
  }
  if (up.update()) {
    ospSetVec3f(self, "up", (osp::vec3f &) up.ref());
    update = true;
  }
  if (aspect.update()) {
    ospSet1f(self, "aspect", aspect.ref());
    update = true;
  }
  if (type == Perspective) {
    if (fovy.update()) {
      ospSet1f(self, "fovy", fovy.ref());
      update = true;
    }
  }
  else if (type == Orthographic) {
    if (height.update()) {
      ospSet1f(self, "height", height.ref());
      update = true;
    }
  }
  ospCommit(self);
  return update;
}

// ======================================================================== //
//
// ======================================================================== //
viewer::RendererProp::RendererProp() 
  :
  /* ospray */
  autoEpsilon(true),
  epsilon(1e-6f),
  spp(1),
  maxDepth(20),
  minContribution(0.001f),
  varianceThreshold(0.f),
  bgColor(vec4f(0.f)),
  shadowsEnabled(false),
  aoSamples(0),
  aoDistance(1e20f),
  aoWeight(0.f),
  aoTransparencyEnabled(false),
  oneSidedLighting(true)
{
  autoEpsilon.update();
  epsilon.update();
  spp.update();
  maxDepth.update();
  minContribution.update();
  varianceThreshold.update();
  bgColor.update();
  shadowsEnabled.update();
  aoSamples.update();
  aoDistance.update();
  aoWeight.update();
  aoTransparencyEnabled.update();
  oneSidedLighting.update();
  /* imgui */
  imgui_autoEpsilon = autoEpsilon.ref();
  imgui_epsilon = epsilon.ref();
  imgui_spp = spp.ref();
  imgui_maxDepth = maxDepth.ref();
  imgui_minContribution = minContribution.ref();
  imgui_varianceThreshold = varianceThreshold.ref();
  imgui_bgColor = bgColor.ref();
  imgui_shadowsEnabled = shadowsEnabled.ref();
  imgui_aoSamples = aoSamples.ref();
  imgui_aoDistance = aoDistance.ref();
  imgui_aoWeight = aoWeight.ref();
  imgui_aoTransparencyEnabled = aoTransparencyEnabled.ref();
  imgui_oneSidedLighting = oneSidedLighting.ref();
}
void viewer::RendererProp::Init(OSPRenderer renderer) 
{
  self = renderer;
}
void viewer::RendererProp::Draw()
{
  if (ImGui::Checkbox("shadowsEnabled", &imgui_autoEpsilon)) {
    autoEpsilon = imgui_autoEpsilon;
  }
  if (ImGui::SliderInt("maxDepth", &imgui_maxDepth, 0, 100, "%.0f")) {
    maxDepth = imgui_maxDepth;
  }
  if (ImGui::Checkbox("shadowsEnabled", &imgui_shadowsEnabled)) {
    shadowsEnabled = imgui_shadowsEnabled;
  }

  if (ImGui::SliderInt("aoSamples", &imgui_aoSamples, 0, 100, "%.0f")) {
    aoSamples = imgui_aoSamples;
  }
  if (ImGui::SliderFloat("aoDistance", &imgui_aoDistance, 
                         0.f, 1e20, "%.3f", 5.0f)) {
    aoDistance = imgui_aoDistance;
  }
  if (ImGui::Checkbox("aoTransparencyEnabled", 
                      &imgui_aoTransparencyEnabled)) {
    aoTransparencyEnabled = imgui_aoTransparencyEnabled;
  }
  if (ImGui::Checkbox("oneSidedLighting", 
                      &imgui_oneSidedLighting)) {
    oneSidedLighting = imgui_oneSidedLighting;
  }
}
bool viewer::RendererProp::Commit()
{
  bool update = false;
  if (aoSamples.update()) {
    ospSet1i(self, "aoSamples", aoSamples.ref());
    update = true;
  }
  if (aoDistance.update()) {
    ospSet1f(self, "aoDistance", aoDistance.ref());
    update = true;
  }
  if (shadowsEnabled.update()) {
    ospSet1i(self, "shadowsEnabled", shadowsEnabled.ref());
    update = true;
  }
  if (maxDepth.update()) {
    ospSet1i(self, "maxDepth", maxDepth.ref());
    update = true;
  }
  if (aoTransparencyEnabled.update()) {
    ospSet1i(self, "aoTransparencyEnabled", aoTransparencyEnabled.ref());
    update = true;
  }
  ospSet1i(self, "spp", 1);
  ospSet1i(self, "autoEpsilon", 1);
  ospSet1f(self, "epsilon", 0.001f);
  ospSet1f(self, "minContribution", 0.001f);
  ospCommit(self);
  return update;
}

// ======================================================================== //
void viewer::TfnProp::Init()
{
  if (self != nullptr) {
    tfnWidget = std::make_shared<tfn::tfn_widget::TransferFunctionWidget>
      ([&](const std::vector<float> &c,
           const std::vector<float> &a,
           const std::array<float, 2> &r) {
        tfnMutex.lock();
        cptr = std::vector<float>(c);
        aptr = std::vector<float>(a);
        valueRange.x = r[0];
        valueRange.y = r[1];
        hasNewValue = true;
        tfnMutex.unlock();
      });
    tfnWidget->setDefaultRange(tfnValueRange[0], tfnValueRange[1]);
  }
}
void viewer::TfnProp::Print()
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
void viewer::TfnProp::Draw()
{
  if (self != nullptr) {
    if (tfnWidget->drawUI()) {
      tfnWidget->render(128);
    };
  }
}
bool viewer::TfnProp::Commit()
{
  bool update = false;
  if (hasNewValue && tfnMutex.try_lock()) {
    hasNewValue = false;
    OSPData colorsData = ospNewData(cptr.size() / 3, 
                                    OSP_FLOAT3, 
                                    cptr.data());
    ospCommit(colorsData);
    std::vector<float> o(aptr.size() / 2);
    for (int i = 0; i < aptr.size() / 2; ++i) { o[i] = aptr[2 * i + 1]; }
    OSPData opacitiesData = ospNewData(o.size(), OSP_FLOAT, o.data());   
    ospCommit(opacitiesData);
    ospSetData(self, "colors", colorsData);
    ospSetData(self, "opacities", opacitiesData);
    ospSetVec2f(self, "valueRange", (osp::vec2f&)valueRange);
    ospCommit(self);
    ospRelease(colorsData);
    ospRelease(opacitiesData);
    update= true;
    tfnMutex.unlock();
  }
  return update;
}

// ======================================================================== //
//
// ======================================================================== //
viewer::LightProp::LightProp(std::string str, OSPRenderer& r, 
                             std::vector<OSPLight>& l) 
  : type(str), ospRen(r), lightList(l)
{
  L = ospNewLight(ospRen, str.c_str());
  ospSet1f(L, "angularDiameter", 0.53f);
  Commit();
  lightList.push_back(L);
  name = std::to_string(lightList.size());
}
void viewer::LightProp::Draw()
{
  ImGui::Text((type + "-" + name).c_str());
  ImGui::SliderFloat3(("direction##" + name).c_str(), &D.x, -1.f, 1.f);
  ImGui::SliderFloat3(("color##" + name).c_str(), &C.x, 0.f, 1.f);
  ImGui::SliderFloat(("intensity##" + name).c_str(), &I, 
                     0.f, 100000.f, "%.3f", 5.0f);
}
void viewer::LightProp::Commit()
{
  ospSet1f(L, "intensity", I);
  ospSetVec3f(L, "color", (osp::vec3f &)C);
  ospSetVec3f(L, "direction", (osp::vec3f &)D);
  ospCommit(L);
}

