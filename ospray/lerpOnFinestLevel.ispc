#include "CompOctant.ih"

/*! interpolate on the finest level */
extern varying float Chombo_lerpOnFinestLevel(void *uniform _self, 
                                              const varying vec3f &worldSpacePos)
{
  ChomboVolume *uniform self = (ChomboVolume *)_self;
  Chombo *uniform chom = &self->chombo;
  
  // for now, let's assume the world-space position already IS in grid space
  const vec3f pos = worldSpacePos;

  const float cw = chom->finestLevelCellWidth;
  const float hcw = 0.5f * cw;

  // translate to grid space on finest grid level
  const float fx = (pos.x-hcw)/cw;
  const float fy = (pos.y-hcw)/cw;
  const float fz = (pos.z-hcw)/cw;

  const float x0 = (pos.x <= hcw) ? -1.f : (float)(int)(fx);
  const float y0 = (pos.y <= hcw) ? -1.f : (float)(int)(fy);
  const float z0 = (pos.z <= hcw) ? -1.f : (float)(int)(fz);

#if 1
  CompactOctant co;
  co.P0 = (make_vec3f(x0,y0,z0)+make_vec3f(0.5f,0.5f,0.5f))*cw;
  co.P1 = co.P0 + make_vec3f(cw);
  findEightCornerValuesOrdered(chom,co);

  const float f000 = co.value[0][0][0];
  const float f001 = co.value[0][0][1];
  const float f010 = co.value[0][1][0];
  const float f011 = co.value[0][1][1];
  const float f100 = co.value[1][0][0];
  const float f101 = co.value[1][0][1];
  const float f110 = co.value[1][1][0];
  const float f111 = co.value[1][1][1];
  // for (uniform int iz=0;iz<2;iz++)
  //   for (uniform int iy=0;iy<2;iy++)
  //     for (uniform int ix=0;ix<2;ix++)
  //       if (co.width[iz][iy][ix] == 0.f)
  //         print("invalid query!\n");
#else
  const CellRef c000 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(0.5f,0.5f,0.5f))*cw);
  const CellRef c001 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(1.5f,0.5f,0.5f))*cw);
  const CellRef c010 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(0.5f,1.5f,0.5f))*cw);
  const CellRef c011 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(1.5f,1.5f,0.5f))*cw);
  const CellRef c100 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(0.5f,0.5f,1.5f))*cw);
  const CellRef c101 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(1.5f,0.5f,1.5f))*cw);
  const CellRef c110 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(0.5f,1.5f,1.5f))*cw);
  const CellRef c111 = findLeafCell(chom,(make_vec3f(x0,y0,z0)+make_vec3f(1.5f,1.5f,1.5f))*cw);

  const float f000 = c000.value;
  const float f001 = c001.value;
  const float f010 = c010.value;
  const float f011 = c011.value;
  const float f100 = c100.value;
  const float f101 = c101.value;
  const float f110 = c110.value;
  const float f111 = c111.value;
  
#endif

  const float dx = fx - x0;
  const float dy = fy - y0;
  const float dz = fz - z0;

  const float f00 = (1.f-dx)*f000 + dx*f001;
  const float f01 = (1.f-dx)*f010 + dx*f011;
  const float f10 = (1.f-dx)*f100 + dx*f101;
  const float f11 = (1.f-dx)*f110 + dx*f111;

  const float f0 = (1.f-dy)*f00+dy*f01;
  const float f1 = (1.f-dy)*f10+dy*f11;

  const float f = (1.f-dz)*f0+dz*f1;
  return f00;
}


