inline float opt_octMethodOnLevel(const uniform Chombo *uniform chom,
                                  const float desiredLevel, const vec3f &pos)
{
  Octant o;
#if 1
  initialFind(chom,o,pos,desiredLevel);
  const CellRef mine = o.cr[0][0][0];
  const vec3f mineCenter = centerOf(mine);

  // Delta to octant corners
  const float DX = (pos.x >= mineCenter.x ? +.5f : -.5f)*mine.width;
  const float DY = (pos.y >= mineCenter.y ? +.5f : -.5f)*mine.width;
  const float DZ = (pos.z >= mineCenter.z ? +.5f : -.5f)*mine.width;

  // neighbor cell _locator_ - ie, a position somewhere _in_ that
  // cell, but neither the boundary nor the cell center
  const vec3f loc = mineCenter + 2.0f*make_vec3f(DX,DY,DZ);

  if (o.cr[0][0][1].width == 0.f)
    o.cr[0][0][1] = findCell(chom,make_vec3f(loc.x,pos.y,pos.z),mine.width);
  if (o.cr[0][1][0].width == 0.f)
    o.cr[0][1][0] = findCell(chom,make_vec3f(pos.x,loc.y,pos.z),mine.width);
  if (o.cr[0][1][1].width == 0.f)
    o.cr[0][1][1] = findCell(chom,make_vec3f(loc.x,loc.y,pos.z),mine.width);
  if (o.cr[1][0][0].width == 0.f)
    o.cr[1][0][0] = findCell(chom,make_vec3f(pos.x,pos.y,loc.z),mine.width);
  if (o.cr[1][0][1].width == 0.f)
    o.cr[1][0][1] = findCell(chom,make_vec3f(loc.x,pos.y,loc.z),mine.width);
  if (o.cr[1][1][0].width == 0.f)
    o.cr[1][1][0] = findCell(chom,make_vec3f(pos.x,loc.y,loc.z),mine.width);
  if (o.cr[1][1][1].width == 0.f)
    o.cr[1][1][1] = findCell(chom,make_vec3f(loc.x,loc.y,loc.z),mine.width);
#else
  const CellRef mine = findCell(chom,pos,desiredLevel);
  const vec3f mineCenter = centerOf(mine);
  // Delta to octant corners
  const float DX = (pos.x >= mineCenter.x ? +.5f : -.5f)*mine.width;
  const float DY = (pos.y >= mineCenter.y ? +.5f : -.5f)*mine.width;
  const float DZ = (pos.z >= mineCenter.z ? +.5f : -.5f)*mine.width;

  // neighbor cell _locator_ - ie, a position somewhere _in_ that
  // cell, but neither the boundary nor the cell center
  const vec3f loc = mineCenter + 2.0f*make_vec3f(DX,DY,DZ);

  o.cr[0][0][0] = mine; //findLeafCell(chom,make_vec3f(pos.x,pos.y,pos.z);
  o.cr[0][0][1] = findCell(chom,make_vec3f(loc.x,pos.y,pos.z),mine.width);
  o.cr[0][1][0] = findCell(chom,make_vec3f(pos.x,loc.y,pos.z),mine.width);
  o.cr[0][1][1] = findCell(chom,make_vec3f(loc.x,loc.y,pos.z),mine.width);
  o.cr[1][0][0] = findCell(chom,make_vec3f(pos.x,pos.y,loc.z),mine.width);
  o.cr[1][0][1] = findCell(chom,make_vec3f(loc.x,pos.y,loc.z),mine.width);
  o.cr[1][1][0] = findCell(chom,make_vec3f(pos.x,loc.y,loc.z),mine.width);
  o.cr[1][1][1] = findCell(chom,make_vec3f(loc.x,loc.y,loc.z),mine.width);
#endif

  // interpolation levels (actually, cell width) in given directions:
  // it's always the maximum cell width of any neighbor cell that
  // abuts on this/these side(s)
  const float l_dx  = o.cr[0][0][1].width;
  const float l_dy  = o.cr[0][1][0].width;
  const float l_dz  = o.cr[1][0][0].width;
  const float l_dxy = max(max(l_dx,l_dy),o.cr[0][1][1].width);
  const float l_dyz = max(max(l_dy,l_dz),o.cr[1][1][0].width);
  const float l_dxz = max(max(l_dx,l_dz),o.cr[1][0][1].width);
  const float l_dxyz = max(max(max(l_dxy,l_dxz),l_dyz),o.cr[1][1][1].width);
  // "my" width
  const float my_l = o.cr[0][0][0].width;

  // -------------------------------------------------------
  // compute octant corners
  // -------------------------------------------------------
  // us:
  const float oc000 = o.cr[0][0][0].value;
  // face neighbors
  const float oc001 = (l_dx == my_l) 
    ? 0.5f*(o.cr[0][0][1].value+o.cr[0][0][0].value)
    : opt_octMethodOnLevel(chom,l_dx,mineCenter+make_vec3f(0,0,DX));
  const float oc010 = (l_dy == my_l)
    ? 0.5f*(o.cr[0][1][0].value+o.cr[0][0][0].value)
    : opt_octMethodOnLevel(chom,l_dy,mineCenter+make_vec3f(0,DY,0));
  const float oc100 = (l_dz == my_l)
    ? 0.5f*(o.cr[1][0][0].value+o.cr[0][0][0].value)
    : opt_octMethodOnLevel(chom,l_dz,mineCenter+make_vec3f(DZ,0,0));

  const float oc011 = (l_dxy == my_l)
    ? 0.25f*(o.cr[0][0][0].value+o.cr[0][0][1].value+
             o.cr[0][1][0].value+o.cr[0][1][1].value)
    : opt_octMethodOnLevel(chom,l_dxy,mineCenter+make_vec3f(0,DY,DX));
  const float oc101 = (l_dxz == my_l)
    ? 0.25f*(o.cr[0][0][0].value+o.cr[0][0][1].value+
             o.cr[1][0][0].value+o.cr[0][0][1].value)
    : opt_octMethodOnLevel(chom,l_dxz,mineCenter+make_vec3f(DZ,0,DX));
  const float oc110 = (l_dyz == my_l)
    ? 0.25f*(o.cr[0][0][0].value+o.cr[1][0][0].value+
             o.cr[0][1][0].value+o.cr[1][1][0].value)
    : opt_octMethodOnLevel(chom,l_dyz,mineCenter+make_vec3f(DZ,DY,0));
  
  const float oc111 = (l_dxyz == my_l)
    ? 0.125f*(o.cr[0][0][1].value+o.cr[0][0][0].value+
              o.cr[0][0][1].value+o.cr[0][0][0].value+
              o.cr[0][1][1].value+o.cr[0][1][0].value+
              o.cr[0][1][1].value+o.cr[0][1][0].value)
    : opt_octMethodOnLevel(chom,l_dxyz,mineCenter+make_vec3f(DZ,DY,DZ));
  
  // -------------------------------------------------------
  // interpolation
  // -------------------------------------------------------
  // interpolation weights inside octant (mind the "*2" for the octant)
  const float dx = 2.f*abs(pos.x-mineCenter.x)/mine.width;
  const float dy = 2.f*abs(pos.y-mineCenter.y)/mine.width;
  const float dz = 2.f*abs(pos.z-mineCenter.z)/mine.width;
  
  // interpolant inputs
  const float f000 = oc000;
  const float f001 = oc001;
  const float f010 = oc010;
  const float f011 = oc011;
  const float f100 = oc100;
  const float f101 = oc101;
  const float f110 = oc110;
  const float f111 = oc111;
  
  // interpolate in x
  const float f00 = (1.f-dx)*f000 + dx*f001;
  const float f01 = (1.f-dx)*f010 + dx*f011;
  const float f10 = (1.f-dx)*f100 + dx*f101;
  const float f11 = (1.f-dx)*f110 + dx*f111;
  
  // interpolate result in y
  const float f0 = (1.f-dy)*f00+dy*f01;
  const float f1 = (1.f-dy)*f10+dy*f11;
  
  // interpolate result in z
  const float f = (1.f-dz)*f0+dz*f1;
  return f;
}

