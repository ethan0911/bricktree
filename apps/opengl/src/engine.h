// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#pragma once
#ifndef OSPRAY_ENGINE_H
#define OSPRAY_ENGINE_H

#include "common/constants.h"
// ospcommon
#include "ospray/ospray.h"
#include "ospcommon/vec.h"
#include "ospcommon/utility/DoubleBufferedValue.h"
#include "ospcommon/utility/TransactionalValue.h"
// std
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <memory>

using namespace ospcommon;
namespace viewer {  
  class Engine {
  public:
    enum ExecState {STOPPED, RUNNING, INVALID};
  private:
    std::unique_ptr<std::thread> fbThread;
    std::atomic<ExecState>       fbState{ExecState::INVALID};
    std::mutex                   fbMutex;
    std::atomic<bool>            fbHasNewFrame{false};
    std::atomic<bool>            fbClear{false};
    ospcommon::utility::TransactionalValue<vec2i>
      fbSize;
    ospcommon::utility::DoubleBufferedValue<std::vector<uint32_t>> 
      fbBuffers;
    int fbNumPixels{0};
  private:
    uint32_t      *ospFBPtr;
    OSPFrameBuffer ospFB  = nullptr;
    OSPRenderer    ospRen = nullptr;
  public:
    void Validate();
    void Start();
    void Stop();
    bool HasNewFrame();
    const std::vector<uint32_t> &MapFramebuffer();
    void UnmapFramebuffer();
    void Resize(size_t width, size_t height);
    void Init(size_t width, size_t height, OSPRenderer ren);
    void Clear();
    void Delete();
  };
};
#endif //OSPRAY_ENGINE_H
