// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#pragma once
#ifndef OSPRAY_VIEWER_H
#define OSPRAY_VIEWER_H

#include "ospray/ospray.h"
#include <string>

namespace viewer {
  int  Init(const int ac, const char** av,
            const size_t& w, const size_t& h);
  void Render(int window_id);
  void Handler(OSPModel model, OSPRenderer renderer);

  void Handler(OSPCamera c, const std::string& type,
	       const osp::vec3f& vp,
	       const osp::vec3f& vu,
	       const osp::vec3f& vi);
  void Handler(OSPTransferFunction tfn, 
               const float& min, const float& max);

};

#endif//OSPRAY_VIEWER_H
