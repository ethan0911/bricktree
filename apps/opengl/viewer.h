// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
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
};

#endif//OSPRAY_VIEWER_H
