// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#include "engine.h"
#include "scene/properties.h"
void viewer::Engine::Validate()
{
  if (fbState == ExecState::INVALID)
    fbState = ExecState::STOPPED;
}
void viewer::Engine::Start() {
  // avoid multiple start
  if (fbState == ExecState::RUNNING)
    return;
  // start the thread
  fbState = ExecState::RUNNING;
  fbThread = std::make_unique<std::thread>([&] {
      while (fbState != ExecState::STOPPED) {
        // check if we need to resize
        if (fbSize.update()) {
          // resize buffer
          ospcommon::vec2i size = fbSize.get();
          fbNumPixels = size.x * size.y;
          fbBuffers.front().resize(fbNumPixels);
          fbBuffers.back().resize(fbNumPixels);
          // resize ospray framebuffer
          if (ospFB != nullptr) {
            ospUnmapFrameBuffer(ospFBPtr, ospFB);
            ospFreeFrameBuffer(ospFB);
            ospFB = nullptr;
          }
          ospFB = ospNewFrameBuffer((osp::vec2i&)size, 
                                    OSP_FB_SRGBA, 
                                    OSP_FB_COLOR | OSP_FB_ACCUM);
          ospFrameBufferClear(ospFB, OSP_FB_COLOR | OSP_FB_ACCUM);
          ospFBPtr = (uint32_t *) ospMapFrameBuffer(ospFB, OSP_FB_COLOR);
        }
        // clear a frame
        if (fbClear || viewer::widgets::Commit()) {
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
void viewer::Engine::Stop() {
  if (fbState != ExecState::RUNNING)
    return;      
  fbState = ExecState::STOPPED;
  fbThread->join();
  fbThread.reset();
}
bool viewer::Engine::HasNewFrame() { return fbHasNewFrame; };
const std::vector<uint32_t>& viewer::Engine::MapFramebuffer()
{
  fbMutex.lock();
  fbHasNewFrame = false;
  return fbBuffers.front();
}
void viewer::Engine::UnmapFramebuffer()
{
  fbMutex.unlock();
}
void viewer::Engine::Resize(size_t width, size_t height) 
{
  fbSize = ospcommon::vec2i(width, height);
}
void viewer::Engine::Init(size_t width, size_t height, OSPRenderer ren) 
{ 
  Resize(width, height); 
  ospRen = ren;
}
void viewer::Engine::Clear() 
{
  fbClear = true;
}
void viewer::Engine::Delete() 
{
  if (ospFB != nullptr) {
    ospUnmapFrameBuffer(ospFBPtr, ospFB); 
    ospFreeFrameBuffer(ospFB);
    ospFB = nullptr;
  }
}
