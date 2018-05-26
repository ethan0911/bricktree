// ======================================================================== //
// Copyright SCI Institute, University of Utah, 2018
// ======================================================================== //
#pragma once

#include "ospray/ospray.h"

struct Sphere {
  struct SphereInfo
  {
    vec3f org{0.f,0.f,0.f};
    int   colorID{0};
  };
  OSPData      data;
  OSPGeometry  sphere;
  //OSPGeometry  instance;
  //OSPModel     local;
  SphereInfo   info;
  bool added = false;
  void Init()
  {
    data = ospNewData(sizeof(SphereInfo), OSP_UCHAR, &info, OSP_DATA_SHARED_BUFFER);
    ospCommit(data);
    vec4f color(1.f, 0.f, 0.f, 1.f);
    OSPData cdata = ospNewData(1, OSP_FLOAT4, &color);
    ospCommit(cdata);
    sphere = ospNewGeometry("spheres");
    ospSetData(sphere, "spheres", data);
    ospSetData(sphere, "color", cdata);
    ospSet1i(sphere, "offset_center", 0);
    ospSet1i(sphere, "offset_colorID", int(sizeof(vec3f)));
    ospSet1f(sphere, "radius", 0.01f * camera.CameraFocalLength());
    ospCommit(sphere);
    ospRelease(cdata);
    /* local = ospNewModel(); */
    /* ospAddGeometry(local, sphere); */
    /* ospCommit(local); */
    /* instance = ospNewInstance(local,  */
    /* 			      osp::affine3f{ */
    /* 				  osp::vec3f{1.f,0.f,0.f},  */
    /* 				  osp::vec3f{0.f,1.f,0.f},  */
    /* 				  osp::vec3f{0.f,0.f,1.f},  */
    /* 				  osp::vec3f{0.f,0.f,0.f}}); */
    /* ospCommit(instance); */
  }
  void Update(const vec3f& center, OSPModel mod)
  {
    if (added) {
      if (info.org != center) {
	info.org = center;
	ospCommit(sphere);
	//ospCommit(local);
	ospCommit(mod);
      }
    }
  }
  void Add(OSPModel mod)
  {
    added = true;
    //ospAddGeometry(mod, instance);
    ospAddGeometry(mod, sphere);
    ospCommit(mod);
  }
  void Remove(OSPModel mod)
  {
    //ospRemoveGeometry(mod, instance);
    ospRemoveGeometry(mod, sphere);
    ospCommit(mod);
    added = false;
  }
};
