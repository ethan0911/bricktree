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

using ospcommon::utility::TransactionalValue;
using namespace ospcommon;

namespace viewer {

  // ====================================================================== //
  struct Prop { virtual void Commit() = 0; };
  
  // ====================================================================== //
  class CameraProp : public Prop
  {
  public:
    enum Type { Perspective, Orthographic, Panoramic } type;
  private:
    OSPCamera self = nullptr;
    // ==== camera ===== //
    TransactionalValue<vec3f> pos, dir, up;
    TransactionalValue<float> aspect;
    TransactionalValue<float> nearClip;               /* OPT */
    TransactionalValue<vec3f> imageStart, imageEnd;   /* OPT */
    // ==== perspective camera ===== //
    TransactionalValue<float> fovy;
    TransactionalValue<float> apertureRadius;         /* OPT */
    TransactionalValue<float> focusDistance;          /* OPT */
    TransactionalValue<bool> architectural;           /* OPT */
    TransactionalValue<float> stereoMode;             /* OPT */
    TransactionalValue<float> interpupillaryDistance; /* OPT */
    // ==== orthographic camera ===== //
    TransactionalValue<float> height;
  public:
    // ==== Constructor ===== //
    CameraProp(const Type& t);
    OSPCamera& operator*() { return self; }
    // ==== Init ===== //
    void Init(OSPCamera camera);
    // ==== Set ===== //
    void SetPos(const vec3f& v) { pos = v; }
    void SetDir(const vec3f& v) { dir = v; }
    void SetUp(const vec3f& v) { up = v; }
    void SetAspect(const float& v) { aspect = v; }
    void SetNearClip(const float& v) { nearClip = v; }
    void SetImageStart(const vec3f& v) { imageStart = v; }
    void SetImageEnd(const vec3f& v) { imageEnd = v; }
    void SetFovy(const float& v) { fovy = v; }
    void SetApertureRadius(const float& v) { apertureRadius = v; }
    void SetFocusDistance(const float& v) { focusDistance = v; }
    void SetArchitectural(const bool& v) { architectural = v; }
    void SetStereoMode(const float& v) { stereoMode = v; }
    void SetInterpupillaryDistance(const float& v)
    { interpupillaryDistance = v; }
    void SetHeight(const float& v) { height = v; }
    // ==== Commit ===== //
    void Commit();
  };

  // ====================================================================== //
  struct IsoGeoProp
  {
    OSPGeometry geo;
    float &isoValue;
    float vmin, vmax;
    bool  hasMinMax;
    std::string name;
    IsoGeoProp(OSPGeometry g, float &v,
               const float v0, const float v1,
               const std::string n);
    void Draw();
    void Commit();
  };

  // ====================================================================== //
  struct VolumeProp
  {
    OSPVolume volume;
    VolumeProp(OSPVolume v);
    void Commit();
  };

  // ====================================================================== //
  struct LightProp
  {
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
  // ====================================================================== //
  struct RendererProp
  {
    OSPRenderer& ospRen;
    int aoSamples              = 1;
    float aoDistance           = 100.f;
    bool shadowsEnabled        = true;
    int maxDepth               = 100;
    bool aoTransparencyEnabled = false;
    RendererProp(OSPRenderer& r);
    void Draw();
    void Commit();
  };
  // ====================================================================== //
  struct TfnProp
  {
    OSPTransferFunction ospTfn;
    std::shared_ptr<tfn::tfn_widget::TransferFunctionWidget> tfnWidget;
    float tfnValueRange[2] = {0.f, 1.f};
    std::vector<float> cptr;
    std::vector<float> aptr;
    bool print = false;
    void Create(OSPTransferFunction o, 
                const float a = 0.f, 
                const float b = 1.f)
    {
      ospTfn           = o;
      tfnValueRange[0] = a;
      tfnValueRange[1] = b;
    }
    void Init();
    void Print();
    void Draw();
  };
};
#endif//OSPRAY_PROPERTIES_H
