// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#pragma once
#ifndef OSPRAY_WIDGET_H
#define OSPRAY_WIDGET_H
#include "viewer.h"
#include "common.h"
#include "scenegraph/properties.h"
#include <imgui.h>
#include <imgui_glfw_impi.h>
#include "widgets/TransferFunctionWidget.h"

// ======================================================================== //
// This is the place where to involve ospray commits
// ======================================================================== //
using namespace ospcommon;
namespace viewer {
  namespace widgets { bool Commit(); };
};
#endif//OSPRAY_WIDGET_H
