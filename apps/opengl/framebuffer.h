#pragma once
#ifndef OSPRAY_FRAMEBUFFER_H
#define OSPRAY_FRAMEBUFFER_H

#include "common.h"
// ospcommon
#include "ospray/ospray.h"
#include "ospcommon/vec.h"
#include "ospcommon/box.h"
#include "ospcommon/AsyncLoop.h"
#include "ospcommon/utility/CodeTimer.h"
#include "ospcommon/utility/DoubleBufferedValue.h"
#include "ospcommon/utility/TransactionalValue.h"
// std
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <memory>

namespace viewer {
  class AsyncFramebuffer {
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
    void Validate()
    {
      if (fbState == ExecState::INVALID)
        fbState = ExecState::STOPPED;
    }
    void Start() {
      // avoid multiple start
      if (fbState == ExecState::RUNNING)
        return;
      // start the thread
      fbState = ExecState::RUNNING;
      fbThread = std::make_unique<std::thread>([&] {
          while (fbState != ExecState::STOPPED) {
            // check if we need to resize
            if (fbSize.update()) {
              //std::cout << "resize" << std::endl;
              // resize buffer
              ospcommon::vec2i size = fbSize.get();
              fbNumPixels = size.x * size.y;
              fbBuffers.front().resize(fbNumPixels);
              fbBuffers.back().resize(fbNumPixels);
              //std::cout << "new size " << fbNumPixels << std::endl;
              // resize ospray framebuffer
              if (ospFB != nullptr) {
                ospUnmapFrameBuffer(ospFBPtr, ospFB);
                ospFreeFrameBuffer(ospFB);
                ospFB = nullptr;
              }
              //std::cout << "clean" << std::endl;
              ospFB = ospNewFrameBuffer((osp::vec2i&)size, 
                                        OSP_FB_SRGBA, 
                                        OSP_FB_COLOR | OSP_FB_ACCUM);
              //std::cout << "create" << std::endl;
              ospFrameBufferClear(ospFB, OSP_FB_COLOR | OSP_FB_ACCUM);
              ospFBPtr = (uint32_t *) ospMapFrameBuffer(ospFB, OSP_FB_COLOR);
            }
            viewer::widgets::Commit();
            // clear a frame
            if (fbClear) {
              fbClear = false;
              ospFrameBufferClear(ospFB, OSP_FB_COLOR | OSP_FB_ACCUM);
            }
            // render a frame
            ospRenderFrame(ospFB, ospRen, OSP_FB_COLOR | OSP_FB_ACCUM);
            // map
            auto *srcPB = (uint32_t*)ospFBPtr;
            auto *dstPB = (uint32_t*)fbBuffers.back().data();
            memcpy(dstPB, srcPB, fbNumPixels * sizeof(uint32_t));
            // swap
            if (fbMutex.try_lock()) {
              fbBuffers.swap();
              fbHasNewFrame = true;
              fbMutex.unlock();
            }
          }
        });
    }
    void Stop() {
      if (fbState != ExecState::RUNNING)
        return;      
      fbState = ExecState::STOPPED;
      fbThread->join();
      fbThread.reset();
    }
    bool HasNewFrame() { return fbHasNewFrame; };
    const std::vector<uint32_t> &MapFramebuffer()
    {
      fbMutex.lock();
      fbHasNewFrame = false;
      return fbBuffers.front();
    }
    void UnmapFramebuffer()
    {
      fbMutex.unlock();
    }
    void Resize(size_t width, size_t height) 
    {
      fbSize = ospcommon::vec2i(width, height);
    }
    void Init(size_t width, size_t height, OSPRenderer ren) 
    { 
      Resize(width, height); 
      ospRen = ren;
    }
    void Clear() 
    {
      fbClear = true;
    }
    void Delete() 
    {
      if (ospFB != nullptr) {
        ospUnmapFrameBuffer(ospFBPtr, ospFB); 
        ospFreeFrameBuffer(ospFB);
        ospFB = nullptr;
      }
    }

  };
};
#endif //OSPRAY_FRAMEBUFFER_H
