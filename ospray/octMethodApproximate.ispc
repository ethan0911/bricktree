/*! the simplest variant of an octant method - determines the octant
    of the leaf cell the poitn is in, then finds the 8 neighboring
    dual octants of same or coarser level. If NOT at a level boundary
    that is the same as interpolating at the given level; across a
    cell boundary it is NOT smooth, but close. */
extern varying float Chombo_octantMethodApproximate(void *uniform _self, 
                                                    const varying vec3f &worldSpacePos)
{
  CellRef cell = findCell(self,pos,0.f);
  const vec3f cc = centerOf(cell);
  const vec3f sign = make_vec3f(pos.x < cc.x ? -1.f : +1.f,
                                pos.y < cc.y ? -1.f : +1.f,
                                pos.z < cc.z ? -1.f : +1.f);
  const vec3f sideID = make_vec3f(pos.x < cc.x ? 0.f : 1.f,
                                  pos.y < cc.y ? 0.f : 1.f,
                                  pos.z < cc.z ? 0.f : 1.f);
  const vec3f sidePos = cell.pos + sideID*cell.width;
  CompactOctant co;
  co.P0 = pos;
  co.P1 = pos + sign * make_vec3f(cell.width);

  findEightCellsNotOrdered(self,co,cell.width);

  const float rcpWidth = 1.f/oct.c[C000].width;
  return lerp(oct,abs(pos-cc)*rcpWidth);
}
