#include "CompOctant.ih"

#if 0
void findOthers(const uniform Chombo *uniform self,
                CompactOctant &o,
                const float cw)
{
  // if (o.width[0][0][0] == 0.f) 
  //   findOthers(self,o,make_vec3f(o.P0.x,o.P0.y,o.P0.z),cw);
  if (o.width[0][0][1] == 0.f) 
    findOthers(self,o,make_vec3f(o.P1.x,o.P0.y,o.P0.z),cw);

  if (o.width[0][1][0] == 0.f) 
    findOthers(self,o,make_vec3f(o.P0.x,o.P1.y,o.P0.z),cw);
  if (o.width[0][1][1] == 0.f) 
    findOthers(self,o,make_vec3f(o.P1.x,o.P1.y,o.P0.z),cw);


  if (o.width[1][0][0] == 0.f) 
    findOthers(self,o,make_vec3f(o.P0.x,o.P0.y,o.P1.z),cw);
  if (o.width[1][0][1] == 0.f)
    findOthers(self,o,make_vec3f(o.P1.x,o.P0.y,o.P1.z),cw);

  if (o.width[1][1][0] == 0.f) 
    findOthers(self,o,make_vec3f(o.P0.x,o.P1.y,o.P1.z),cw);
  if (o.width[1][1][1] == 0.f) 
    findOthers(self,o,make_vec3f(o.P1.x,o.P1.y,o.P1.z),cw);
}
#else
inline void findOthers(const uniform Chombo *uniform self,
                       CompactOctant &o,
                       const float cw)
{
  // if (o.width[0][0][0] == 0.f) {
  //   CellRef cr = findCell(self,make_vec3f(o.P0.x,o.P0.y,o.P0.z),cw);
  //   o.width[0][0][0] = cr.width;
  //   o.value[0][0][0] = cr.value;
  // }
  if (o.width[0][0][1] == 0.f) {
    CellRef cr = findCell(self,make_vec3f(o.P1.x,o.P0.y,o.P0.z),cw);
    o.width[0][0][1] = cr.width;
    o.value[0][0][1] = cr.value;
  }

  if (o.width[0][1][0] == 0.f) {
    CellRef cr = findCell(self,make_vec3f(o.P0.x,o.P1.y,o.P0.z),cw);
    o.width[0][1][0] = cr.width;
    o.value[0][1][0] = cr.value;
  }
  if (o.width[0][1][1] == 0.f) {
    CellRef cr = findCell(self,make_vec3f(o.P1.x,o.P1.y,o.P0.z),cw);
    o.width[0][1][1] = cr.width;
    o.value[0][1][1] = cr.value;
  }



  if (o.width[1][0][0] == 0.f) {
    CellRef cr = findCell(self,make_vec3f(o.P0.x,o.P0.y,o.P1.z),cw);
    o.width[1][0][0] = cr.width;
    o.value[1][0][0] = cr.value;
  }
  if (o.width[1][0][1] == 0.f) {
    CellRef cr = findCell(self,make_vec3f(o.P1.x,o.P0.y,o.P1.z),cw);
    o.width[1][0][1] = cr.width;
    o.value[1][0][1] = cr.value;
  }

  if (o.width[1][1][0] == 0.f) {
    CellRef cr = findCell(self,make_vec3f(o.P0.x,o.P1.y,o.P1.z),cw);
    o.width[1][1][0] = cr.width;
    o.value[1][1][0] = cr.value;
  }
  if (o.width[1][1][1] == 0.f) {
    CellRef cr = findCell(self,make_vec3f(o.P1.x,o.P1.y,o.P1.z),cw);
    o.width[1][1][1] = cr.width;
    o.value[1][1][1] = cr.value;
  }
}
#endif
                                  
inline float compOctMethodOnLevel(const uniform Chombo *uniform chom,
                                  const float desiredLevel, 
                                  const vec3f &pos)
{
  CompactOctant o;
  initialFind(chom,o,pos,desiredLevel);
  const float leafWidth = o.width[0][0][0];
  const vec3f leafCenter = centerOf(o);

  // Delta to octant corners
  const float DX = (pos.x >= leafCenter.x ? +.5f : -.5f)*leafWidth;
  const float DY = (pos.y >= leafCenter.y ? +.5f : -.5f)*leafWidth;
  const float DZ = (pos.z >= leafCenter.z ? +.5f : -.5f)*leafWidth;

  // neighbor cell _locator_ - ie, a position somewhere _in_ that
  // cell, but neither the boundary nor the cell center
  const vec3f loc = leafCenter + 2.0f*make_vec3f(DX,DY,DZ);
  findOthers(chom,o,leafWidth);
  
  // if (o.cr[0][0][1].width == 0.f)
  //   o.cr[0][0][1] = findCell(chom,make_vec3f(loc.x,pos.y,pos.z),leafWidth);
  // if (o.cr[0][1][0].width == 0.f)
  //   o.cr[0][1][0] = findCell(chom,make_vec3f(pos.x,loc.y,pos.z),leafWidth);
  // if (o.cr[0][1][1].width == 0.f)
  //   o.cr[0][1][1] = findCell(chom,make_vec3f(loc.x,loc.y,pos.z),leafWidth);
  // if (o.cr[1][0][0].width == 0.f)
  //   o.cr[1][0][0] = findCell(chom,make_vec3f(pos.x,pos.y,loc.z),leafWidth);
  // if (o.cr[1][0][1].width == 0.f)
  //   o.cr[1][0][1] = findCell(chom,make_vec3f(loc.x,pos.y,loc.z),leafWidth);
  // if (o.cr[1][1][0].width == 0.f)
  //   o.cr[1][1][0] = findCell(chom,make_vec3f(pos.x,loc.y,loc.z),leafWidth);
  // if (o.cr[1][1][1].width == 0.f)
  //   o.cr[1][1][1] = findCell(chom,make_vec3f(loc.x,loc.y,loc.z),leafWidth);

  // interpolation levels (actually, cell width) in given directions:
  // it's always the maximum cell width of any neighbor cell that
  // abuts on this/these side(s)
  const float l_dx  = o.width[0][0][1];
  const float l_dy  = o.width[0][1][0];
  const float l_dz  = o.width[1][0][0];
  const float l_dxy = max(max(l_dx,l_dy),o.width[0][1][1]);
  const float l_dyz = max(max(l_dy,l_dz),o.width[1][1][0]);
  const float l_dxz = max(max(l_dx,l_dz),o.width[1][0][1]);
  const float l_dxyz = max(max(max(l_dxy,l_dxz),l_dyz),o.width[1][1][1]);
  // "leaf" width
  const float leaf_l = leafWidth;

  // -------------------------------------------------------
  // compute octant corners
  // -------------------------------------------------------
  // us:
  const float oc000 = o.value[0][0][0];
  // face neighbors
  const float oc001 = (l_dx == leaf_l) 
    ? 0.5f*(o.value[0][0][1]+o.value[0][0][0])
    : compOctMethodOnLevel(chom,l_dx,leafCenter+make_vec3f(0,0,DX));
  const float oc010 = (l_dy == leaf_l)
    ? 0.5f*(o.value[0][1][0]+o.value[0][0][0])
    : compOctMethodOnLevel(chom,l_dy,leafCenter+make_vec3f(0,DY,0));
  const float oc100 = (l_dz == leaf_l)
    ? 0.5f*(o.value[1][0][0]+o.value[0][0][0])
    : compOctMethodOnLevel(chom,l_dz,leafCenter+make_vec3f(DZ,0,0));

  const float oc011 = (l_dxy == leaf_l)
    ? 0.25f*(o.value[0][0][0]+o.value[0][0][1]+
             o.value[0][1][0]+o.value[0][1][1])
    : compOctMethodOnLevel(chom,l_dxy,leafCenter+make_vec3f(0,DY,DX));
  const float oc101 = (l_dxz == leaf_l)
    ? 0.25f*(o.value[0][0][0]+o.value[0][0][1]+
             o.value[1][0][0]+o.value[0][0][1])
    : compOctMethodOnLevel(chom,l_dxz,leafCenter+make_vec3f(DZ,0,DX));
  const float oc110 = (l_dyz == leaf_l)
    ? 0.25f*(o.value[0][0][0]+o.value[1][0][0]+
             o.value[0][1][0]+o.value[1][1][0])
    : compOctMethodOnLevel(chom,l_dyz,leafCenter+make_vec3f(DZ,DY,0));
  
  const float oc111 = (l_dxyz == leaf_l)
    ? 0.125f*(o.value[0][0][1]+o.value[0][0][0]+
              o.value[0][0][1]+o.value[0][0][0]+
              o.value[0][1][1]+o.value[0][1][0]+
              o.value[0][1][1]+o.value[0][1][0])
    : compOctMethodOnLevel(chom,l_dxyz,leafCenter+make_vec3f(DZ,DY,DZ));
  
  // -------------------------------------------------------
  // interpolation
  // -------------------------------------------------------
  // interpolation weights inside octant (mind the "*2" for the octant)
  const float dx = 2.f*abs(pos.x-leafCenter.x)/leafWidth;
  const float dy = 2.f*abs(pos.y-leafCenter.y)/leafWidth;
  const float dz = 2.f*abs(pos.z-leafCenter.z)/leafWidth;
  
  // interpolate in x
  const float f00 = (1.f-dx)*oc000 + dx*oc001;
  const float f01 = (1.f-dx)*oc010 + dx*oc011;
  const float f10 = (1.f-dx)*oc100 + dx*oc101;
  const float f11 = (1.f-dx)*oc110 + dx*oc111;
  
  // interpolate result in y
  const float f0 = (1.f-dy)*f00+dy*f01;
  const float f1 = (1.f-dy)*f10+dy*f11;
  
  // interpolate result in z
  const float f = (1.f-dz)*f0+dz*f1;
  return f;
}

