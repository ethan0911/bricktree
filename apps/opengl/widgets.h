// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#pragma once
#ifndef OSPRAY_WIDGET_H
#define OSPRAY_WIDGET_H
#include "viewer.h"
#include "common.h"
#include <imgui.h>
#include <imgui_glfw_impi.h>
#include "widgets/TransferFunctionWidget.h"

// ======================================================================== //
namespace viewer {
struct IsoGeoProp
{
  OSPGeometry geo;
  float &isoValue;
  float vmin, vmax;
  bool  hasMinMax;
  std::string name;
  IsoGeoProp(OSPGeometry g,
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

// ======================================================================== //
struct VolumeProp
{
  OSPVolume volume;
  VolumeProp(OSPVolume v) : volume(v) {}
  void Commit()
  {
    ospCommit(volume);
  }
};

// ======================================================================== //
struct LightProp
{
  OSPLight L;
  float I = 0.25f;
  vec3f D = vec3f(-1.f, 0.679f, -0.754f);
  vec3f C = vec3f(1.f, 1.f, 1.f);
  std::string type, name;
  OSPRenderer& ospRen;
  std::vector<OSPLight>&   lightList;
  LightProp(std::string str, OSPRenderer& r, std::vector<OSPLight>& l) 
    : type(str), ospRen(r), lightList(l)
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
// ======================================================================== //
struct RendererProp
{
  OSPRenderer& ospRen;
  int aoSamples              = 1;
  float aoDistance           = 100.f;
  bool shadowsEnabled        = true;
  int maxDepth               = 100;
  bool aoTransparencyEnabled = false;
  RendererProp(OSPRenderer& r) : ospRen(r) {}
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
// ======================================================================== //
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
            OSPData colorsData = 
              ospNewData(c.size() / 3, OSP_FLOAT3, c.data());
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
            //ClearOSPRay();
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
};
#endif//OSPRAY_WIDGET_H
