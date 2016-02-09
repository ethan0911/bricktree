// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// stl
#include <algorithm>

namespace ospray {
  using std::min;
  using std::max;
  
  template<typename T>
  struct Range {
    Range(const T &t) : lo(t), hi(t) {};

    T lo, hi;

    T size() const { return hi - lo; }
    T center() const { return .5f*(lo+hi); }
    void extend(const T &t) { lo = min(lo,t); hi = max(hi,t); }
  };

  template<typename T>
  inline std::ostream &operator<<(std::ostream &o, const Range<T> &r)
  { o << "[" << r.lo << "," << r.hi << "]"; return o; }

} // ::ospray

