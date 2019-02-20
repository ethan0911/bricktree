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
#include <chrono>

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
  private:
    std::chrono::high_resolution_clock::time_point metric_time;
    const size_t metric_frame_step = 10;
    size_t       metric_frames     = 0;
    double       metric_fps        = 0.;
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
    void ResetFPS() 
    {
      metric_frames = 0;
      metric_time = std::chrono::high_resolution_clock::now();
    }
    void CountFPS()
    {
      ++ metric_frames;
      if ((metric_frames % metric_frame_step) == 0) {
        const auto t = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<double> et =
          std::chrono::duration_cast<std::chrono::duration<double>>
          (t - metric_time);
        metric_fps = metric_frame_step / et.count();
        ResetFPS();
      }      
    }
    double GetFPS() const { return metric_fps; }
  };
};
#endif //OSPRAY_ENGINE_H
