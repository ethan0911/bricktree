// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //

#include "ospcommon/AffineSpace.h"
#include "ospcommon/LinearSpace.h"
#include "ospcommon/box.h"
#include "ospcommon/range.h"
#include "ospcommon/vec.h"

#include "ospray/ospray.h"
#include "ospray/BrickTree.h"

#include "common/helper.h"
#include "ospCommandLine.h"
#include "ospBrickTreeTools.h"

#if USE_VIEWER
#include "opengl/viewer.h"
#endif

#include "tfn_definition.h"  // where the TFN color and opacity are defined

#ifdef __unix__
# include <unistd.h>
#endif
#include <vector>

using namespace ospcommon;

int main(int ac, const char **av)
{
  //-----------------------------------------------------
  // Program Initialization
  //-----------------------------------------------------
  std::cout << std::endl;
#ifdef __unix__
  // check hostname
  char hname[200];
  gethostname(hname, 200);
  std::cout << "#osp: on host >> " << hname << " <<" << std::endl;
#endif
  if (ospInit(&ac, av) != OSP_NO_ERROR) {
    throw std::runtime_error("FATAL ERROR DURING INITIALIZATION!");
    return 1;
  }


  ospray::CommandLine args(ac,av);

  if (args.inputFiles.empty()) {
    throw std::runtime_error("missing input file");
  }
  if (args.inputFiles.size() > 1) {
    for (auto &s : args.inputFiles)
      std::cout << s << std::endl;
    throw std::runtime_error("too many input file");
  }

#if USE_VIEWER
  int window = viewer::Init(ac, av, args.imgSize.x, args.imgSize.y);
#endif

  //-----------------------------------------------------
  // Create ospray context
  //-----------------------------------------------------
  auto device = ospGetCurrentDevice();
  if (device == nullptr) {
    throw std::runtime_error("FATAL ERROR DURING GETTING CURRENT DEVICE!");
    return 1;
  }
  ospDeviceSetStatusFunc(device, [](const char *msg) { std::cout << msg; });
  ospDeviceSetErrorFunc(device, [](OSPError e, const char *msg) {
    std::cout << "OSPRAY ERROR [" << e << "]: " << msg << std::endl;
    std::exit(1);
  });
  ospDeviceCommit(device);
  if (ospLoadModule("bricktree") != OSP_NO_ERROR) {
    throw std::runtime_error("failed to initialize BrickTree module");
  }

  //-----------------------------------------------------
  // Create ospray objects
  //-----------------------------------------------------

  // create world and renderer
  OSPModel world       = ospNewModel();
  OSPRenderer renderer = ospNewRenderer(args.rendererName.c_str());
  std::cout << "#osp:bench: OSPRay renderer: " <<  args.rendererName 
            << std::endl;
  if (!renderer) {
    throw std::runtime_error("invalid renderer name: " +  args.rendererName);
  }

  // setup trasnfer function
  OSPData cData = ospNewData(colors.size() / 3, OSP_FLOAT3, colors.data());
  ospCommit(cData);
  OSPData oData = ospNewData(opacities.size(), OSP_FLOAT, opacities.data());
  ospCommit(oData);
  OSPTransferFunction transferFcn = ospNewTransferFunction("piecewise_linear");
  ospSetData(transferFcn, "colors", cData);
  ospSetData(transferFcn, "opacities", oData);
  if (args.valueRange.x > args.valueRange.y) {
    throw std::runtime_error("wrong valuerange set!");
  } else {
    ospSetVec2f(transferFcn, "valueRange", 
                (const osp::vec2f &)args.valueRange);
  }
  ospCommit(transferFcn);
  ospRelease(cData);
  ospRelease(oData);

  // create volume
  if (!args.use_hacked_vol) {
    std::cout << "\033[32;1m"
              << "#osp:bench using BrickTree volume"
              << "\033[0m" 
              << std::endl;
    std::shared_ptr<ospray::BrickTree> bricktreeVolume =
      std::make_shared<ospray::BrickTree>();
    bricktreeVolume->setFromXML(args.inputFiles[0]);
    bricktreeVolume->createBtVolume(transferFcn);
    ospAddVolume(world,bricktreeVolume->ospVolume);
    ospray::bt::BrickTreeVolume *btVolume = 
      (ospray::bt::BrickTreeVolume *)bricktreeVolume->ospVolume;  
    box3f worldBounds(vec3f(0), vec3f(btVolume->validSize) - vec3f(1));
  } else {
    std::cout << "\033[33;1m"
              << "#osp:bench using hacked volume"
              << "\033[0m" 
              << std::endl;
    const std::string hacked_volume_path = 
      "/home/sci/feng/Desktop/ws/data/"
      "magnetic-512-volume/magnetic-512-volume.raw";
    const std::string hacked_volume_type = "float";
    const size_t hacked_dtype_size = 4;
    const ospcommon::vec3i hacked_dims(512);
    FILE *f = fopen(hacked_volume_path.c_str(), "rb");
    std::vector<char> volume_data(hacked_dims.x * 
                                  hacked_dims.y * 
                                  hacked_dims.z * 
                                  hacked_dtype_size, 0);
    size_t voxelsRead = 
      fread(volume_data.data(), hacked_dtype_size, 
            hacked_dims.x * hacked_dims.y * hacked_dims.z, f);
    if (voxelsRead != hacked_dims.x * hacked_dims.y * hacked_dims.z) {
      throw std::runtime_error("Failed to read all voxles");
    }
    fclose(f);
    OSPVolume hacked_vol = ospNewVolume("block_bricked_volume");  
    ospSet2f(hacked_vol, "voxelRange", 0, 1.5);  
    ospSetString(hacked_vol, "voxelType", hacked_volume_type.c_str());
    ospSetVec3i(hacked_vol, "dimensions", (osp::vec3i&)hacked_dims);
    ospSet1i(hacked_vol, "gradientShadingEnabled", 1);
    ospSet1i(hacked_vol, "adaptiveSampling", 0);
    ospSetObject(hacked_vol, "transferFunction", transferFcn);
    ospSetRegion(hacked_vol, volume_data.data(), osp::vec3i{0,0,0}, 
                 (osp::vec3i&)hacked_dims);
    ospCommit(hacked_vol);
    ospAddVolume(world, hacked_vol);
  }

  // setup camera
  OSPCamera camera = ospNewCamera("perspective");
  vec3f vd         = args.vi - args.vp;
  ospSetVec3f(camera, "pos", (const osp::vec3f &)args.vp);
  ospSetVec3f(camera, "dir", (const osp::vec3f &)vd);
  ospSetVec3f(camera, "up", (const osp::vec3f &)args.vu);
  ospSet1f(camera, "aspect", args.imgSize.x / (float)args.imgSize.y);
  ospSet1f(camera, "fovy", 60.f);
  ospCommit(camera);

  // setup lighting
  OSPLight d_light = ospNewLight(renderer, "DirectionalLight");
  ospSet1f(d_light, "intensity", 0.25f);
  ospSet1f(d_light, "angularDiameter", 0.53f);
  ospSetVec3f(
      d_light,
      "color",
      osp::vec3f{255.f / 255.f,
                 255.f / 255.f,
                 255.f / 255.f});  // 127.f/255.f,178.f/255.f,255.f/255.f
  ospSetVec3f(d_light, "direction", (const osp::vec3f &)args.disDir);
  ospCommit(d_light);
  OSPLight s_light = ospNewLight(renderer, "DirectionalLight");
  ospSet1f(s_light, "intensity", 1.50f);
  ospSet1f(s_light, "angularDiameter", 0.53f);
  ospSetVec3f(s_light, "color", osp::vec3f{1.f, 1.f, 1.f});
  ospSetVec3f(s_light, "direction", (const osp::vec3f &)args.sunDir);
  ospCommit(s_light);
  OSPLight a_light = ospNewLight(renderer, "AmbientLight");
  ospSet1f(a_light, "intensity", 0.90f);
  ospSetVec3f(
      a_light,
      "color",
      osp::vec3f{255.f / 255.f,
                 255.f / 255.f,
                 255.f / 255.f});  // 174.f/255.f,218.f/255.f,255.f/255.f
  ospCommit(a_light);
  std::vector<OSPLight> light_list{a_light, d_light, s_light};
  OSPData lights = ospNewData(light_list.size(), OSP_OBJECT, light_list.data());
  ospCommit(lights);

  // setup world & renderer
  ospCommit(world);
  ospSetVec3f(renderer, "bgColor", osp::vec3f{1.f, 1.f, 1.f});
  ospSetData(renderer, "lights", lights);
  ospSetObject(renderer, "model", world);
  ospSetObject(renderer, "camera", camera);
  ospSet1i(renderer, "shadowsEnabled", 1);
  ospSet1i(renderer, "oneSidedLighting", 1);
  ospSet1i(renderer, "maxDepth", 5);
  ospSet1i(renderer, "spp", 1);
  ospSet1i(renderer, "autoEpsilon", 1);
  ospSet1i(renderer, "aoSamples", 1);
  ospSet1i(renderer, "aoTransparencyEnabled", 0);
  ospSet1f(renderer, "aoDistance", 1000.0f);
  ospSet1f(renderer, "epsilon", 0.001f);
  ospSet1f(renderer, "minContribution", 0.001f);
  ospCommit(renderer);
  
#if USE_VIEWER

  viewer::Handler(camera, "perspective",
                  (const osp::vec3f &)args.vp,
                  (const osp::vec3f &)args.vu,
                  (const osp::vec3f &)args.vi);

  viewer::Handler(transferFcn, args.valueRange.x, args.valueRange.y);
  viewer::Handler(world, renderer);
  viewer::Render(window);

#else

  // setup framebuffer
  std::cout << std::endl;
  OSPFrameBuffer fb = ospNewFrameBuffer(
      (const osp::vec2i &)args.imgSize, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
  ospFrameBufferClear(fb, OSP_FB_COLOR | OSP_FB_ACCUM);

  // render 10 more frames, which are accumulated to result in a better
  // converged image
  std::cout << "#osp:bench: warming-up for " << args.numFrames.x << " frames"
            << std::endl;
  for (int frames = 0; frames < args.numFrames.x;
       frames++) {  // skip frames to warmup
    ospRenderFrame(fb, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);
  }
  std::cout << "#osp:bench: benchmarking for " << args.numFrames.y << " frames"
            << std::endl;
  auto t = ospray::Time();
  for (int frames = 0; frames < args.numFrames.y; frames++) {
    ospRenderFrame(fb, renderer, OSP_FB_COLOR | OSP_FB_ACCUM);
  }
  auto et = ospray::Time(t);
  std::cout << "#osp:bench: average framerate " << args.numFrames.y / et
            << std::endl;

  // save frame
  const uint32_t *buffer = (uint32_t *)ospMapFrameBuffer(fb, OSP_FB_COLOR);
  ospray::writePPM(
      args.outputImageName + ".ppm", args.imgSize.x, args.imgSize.y, buffer);
  ospUnmapFrameBuffer(buffer, fb);
  std::cout << "#osp:bench: save image to " << args.outputImageName + ".ppm"
            << std::endl;

#endif

  return 0;
}
