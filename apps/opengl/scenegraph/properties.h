// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#pragma once
#ifndef OSPRAY_PROPERTIES_H
#define OSPRAY_PROPERTIES_H

#include "ospray/ospray.h"
#include "ospcommon/vec.h"
#include "ospcommon/utility/TransactionalValue.h"

#include <imgui.h>
#include <imgui_glfw_impi.h>
#include "widgets/TransferFunctionWidget.h"

#include <vector>

#define Setter(fvar, nvar, t) \
  void Set##nvar (const t& v) { this->fvar = v; }
#define TValue ospcommon::utility::TransactionalValue

namespace viewer {

  // ====================================================================== //
  struct Prop { 
    virtual void Draw() = 0; 
    virtual bool Commit() = 0; 
  };
  // ====================================================================== //
  class CameraProp : public Prop
  {
  public:
    enum Type { Perspective, Orthographic, Panoramic } type;
  private:
    OSPCamera self = nullptr;
    // ==== camera ===== //
    TValue<ospcommon::vec3f> pos, dir, up;
    TValue<float> aspect;
    TValue<float> nearClip;                        /* OPT */
    TValue<ospcommon::vec2f> imageStart, imageEnd; /* OPT */
    TValue<float> shutterOpen, shutterClose;
    // ==== perspective camera ===== //
    TValue<float> fovy;
    TValue<float> apertureRadius;                  /* OPT */
    TValue<float> focusDistance;                   /* OPT */
    TValue<bool>  architectural;                   /* OPT */
    TValue<int>   stereoMode;                      /* OPT */
    TValue<float> interpupillaryDistance;          /* OPT */
    // ==== orthographic camera ===== //
    TValue<float> height;
  public:
    // ==== Constructor ===== //
    CameraProp(const Type& t);
    OSPCamera& operator*() { return self; }
    // ==== Init ===== //
    void Init(OSPCamera camera);
    // ==== Set ===== //
    /* camera */
    Setter(pos, Pos, ospcommon::vec3f);
    Setter(dir, Dir, ospcommon::vec3f);
    Setter(up, Up, ospcommon::vec3f);
    Setter(nearClip, NearClip, float);
    Setter(imageStart, ImageStart, ospcommon::vec2f);
    Setter(imageEnd, ImageEnd, ospcommon::vec2f);
    Setter(shutterOpen, ShutterOpen, float);
    Setter(shutterClose, ShutterClose, float);
    Setter(aspect, Aspect, float);
    /* perspective camera */
    Setter(fovy, Fovy, float);
    Setter(apertureRadius, ApertureRadius, float);
    Setter(focusDistance, FocusDistance, float);
    Setter(architectural, Architectural, bool);
    Setter(stereoMode, StereoMode, int);
    Setter(interpupillaryDistance, InterpupillaryDistance, float);
    /* orthographic camera */
    Setter(height, Height, float);
    // ==== Draw & Commit ===== //
    void Draw();
    bool Commit();
  };

  // ====================================================================== //
  class RendererProp : public Prop
  {
  private:
    OSPRenderer self = nullptr;
    // ==== renderer ===== //
    TValue<bool> autoEpsilon;
    TValue<float> epsilon;
    TValue<int> spp;
    TValue<int> maxDepth;
    TValue<float> minContribution;
    TValue<float> varianceThreshold;
    TValue<ospcommon::vec4f> bgColor;    
    // ==== scivis renderer ===== //
    TValue<bool> shadowsEnabled;
    TValue<int> aoSamples;
    TValue<float> aoDistance;
    TValue<float> aoWeight;
    TValue<bool> aoTransparencyEnabled;
    TValue<bool> oneSidedLighting;
  private:
    // ==== renderer ===== //
    bool  imgui_autoEpsilon;
    float imgui_epsilon;
    int   imgui_spp;
    int   imgui_maxDepth;
    float imgui_minContribution;
    float imgui_varianceThreshold;
    ospcommon::vec4f imgui_bgColor;    
    // ==== scivis renderer ===== //
    bool  imgui_shadowsEnabled;
    int   imgui_aoSamples;
    float imgui_aoDistance;
    float imgui_aoWeight;
    bool  imgui_aoTransparencyEnabled;
    bool  imgui_oneSidedLighting;
  public:
    // ==== Constructor ===== //
    RendererProp();
    OSPRenderer& operator*() { return self; }
    // ==== Init ===== //
    void Init(OSPRenderer renderer);
    // ==== Draw & Commit ===== //
    void Draw();
    bool Commit();
  };

  // ====================================================================== //
  class TfnProp : public Prop
  {
  private:
    OSPTransferFunction self;
    std::shared_ptr<tfn::tfn_widget::TransferFunctionWidget> tfnWidget;
    float tfnValueRange[2] = {0.f, 1.f};
    ospcommon::vec2f valueRange;
    std::vector<float> cptr;
    std::vector<float> aptr;    
    bool hasNewValue = true;
    bool print = false;
    std::mutex tfnMutex;
  public:
    void Init(OSPTransferFunction o, 
              const float a = 0.f, 
              const float b = 1.f)
    {
      self             = o;
      tfnValueRange[0] = a;
      tfnValueRange[1] = b;
    }
    void Init();
    void Print();
    void Draw();
    bool Commit();
  };


  using namespace ospcommon;

  // ====================================================================== //
  class LightProp
  {
  public:
    OSPLight L;
    float I = 0.25f;
    vec3f D = vec3f(-1.f, 0.679f, -0.754f);
    vec3f C = vec3f(1.f, 1.f, 1.f);
    std::string type, name;
    OSPRenderer& ospRen;
    std::vector<OSPLight>&   lightList;
    LightProp(std::string str, OSPRenderer& r, std::vector<OSPLight>& l);
    void Draw();
    void Commit();
  };

};
#undef Setter
#undef TValue
#endif//OSPRAY_PROPERTIES_H
