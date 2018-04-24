#pragma once
#ifndef OSPRAY_VIEWER_H
#define OSPRAY_VIEWER_H

#include "ospray/ospray.h"

namespace viewer {
  int Init(const int ac, const char** av, const size_t& w, const size_t& h);
  void Render(int);
  void Handler(OSPCamera c, 
	       const osp::vec3f& vp,
	       const osp::vec3f& vu,
	       const osp::vec3f& vi);
  void Handler(OSPTransferFunction, const float&, const float&);
  void Handler(OSPModel m, OSPRenderer r);
  void Handler(OSPGeometry g, float& v, const char* n, 
	       const float vmin = 0.f, const float vmax = -1.f);

  void Handler(OSPVolume v);
  void StartOSPRay();
  void StopOSPRay();
  void ClearOSPRay();
  void CommitOSPRay();
  void ResizeOSPRay(int width, int height);
  void UploadOSPRay();
};

#endif//OSPRAY_VIEWER_H
