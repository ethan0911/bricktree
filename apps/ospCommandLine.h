// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#pragma once

#include <ospcommon/vec.h>
#include <ospcommon/box.h>
#include <stdexcept>
#include <vector>

#ifdef __unix__
#include <unistd.h>
#endif

/*! _everything_ in the ospray core universe should _always_ be in the
  'ospray' namespace. */
namespace ospray {
  // use ospcommon for vec3f etc
  using namespace ospcommon;

  //template funtion for parsing command parameters
  template <class T1, class T2>
    T1 lexical_cast(const T2 &t2)
    {
      std::stringstream s;
      s << t2;
      T1 t1;
      if (s >> t1 && s.eof()) {
        return t1;
      } else {
        throw std::runtime_error("bad conversion " + s.str());
        return T1();
      }
    }
  // retrieve input arguments
  // because constexper cannot be used on gcc 4.8, a hack is used here
  template <typename T>
    T ParseScalar(const int ac, const char **av, int &i, T &v)
    {
      const int init = i;
      if (init + 1 < ac) {
        v = lexical_cast<double, const char *>(av[++i]);
      } else {
        throw std::runtime_error("value required for " + 
                                 std::string(av[init]));
      }
    }
  template <int N, typename T>
    T Parse(const int ac, const char **av, int &i, T &v)
    {
      const int init = i;
      if (init + N < ac) {
        for (int k = 0; k < N; ++k) {
          v[k] = lexical_cast<double, const char *>(av[++i]);
        }
      } else {
        throw std::runtime_error(std::to_string(N) + " values required for " +
            av[init]);
      }
    }
  template <>
    int Parse<1, int>(const int ac, const char **av, int &i, int &v)
    {
      return ParseScalar<int>(ac, av, i, v);
    }
  template <>
    float Parse<1, float>(const int ac, const char **av, int &i, float &v)
    {
      return ParseScalar<float>(ac, av, i, v);
    }
  template <>
    double Parse<1, double>(const int ac, const char **av, int &i, double &v)
    {
      return ParseScalar<double>(ac, av, i, v);
    }
  /*! helper class to parse command-line arguments */
  struct CommandLine {
    CommandLine() = default;
    CommandLine(int ac, const char **av) { Parse(ac, av); }
    void Parse(int ac, const char **av);
    std::vector<std::string> inputFiles;
    std::string outputImageName = "result";
    std::string rendererName    = "scivis";
    vec3f objTranslate{.0f, .0f, .0f};
    vec3f objScale{1.f, 1.f, 1.f};
    vec2i imgSize{1024, 768};
    vec3f vp{8.f, 8.f, 80.f};
    vec3f vu{0, 1, 0};
    vec3f vi{8, 8, 8};
    vec3f sunDir{-1.f, 0.679f, -0.754f};
    vec3f disDir{.372f, .416f, -0.605f};
    vec2f valueRange{0.f, -1.f};
    vec2i numFrames{1 /* skipped */, 100 /* measure */};
    bool use_hacked_vol{false};
    bool use_adaptive_sampling{false};
  };

  inline void CommandLine::Parse(int ac, const char **av)
  {
    //-----------------------------------------------------
    // parse the commandline;
    // complain about anything we do not recognize
    //-----------------------------------------------------
    for (int i = 1; i < ac; ++i) {
      std::string str(av[i]);
      if (str == "-o") {
        outputImageName = av[++i];
      } else if (str == "-renderer") {
        rendererName = av[++i];
      }else if (str == "-valueRange") {
        ospray::Parse<2>(ac, av, i, valueRange);
      }else if (str == "-translate") {
        ospray::Parse<3>(ac, av, i, objTranslate);
      } else if (str == "-scale") {
        ospray::Parse<3>(ac, av, i, objScale);
      } else if (str == "-fb") {
        ospray::Parse<2>(ac, av, i, imgSize);
      } else if (str == "-vp") {
        ospray::Parse<3>(ac, av, i, vp);
      } else if (str == "-vi") {
        ospray::Parse<3>(ac, av, i, vi);
      } else if (str == "-vu") {
        ospray::Parse<3>(ac, av, i, vu);
      } else if (str == "-sun") {
        ospray::Parse<3>(ac, av, i, sunDir);
      } else if (str == "-dis") {
        ospray::Parse<3>(ac, av, i, disDir);
      } else if (str == "-adaptive") {
        use_adaptive_sampling = true;
      } else if (str == "-frames") {
        try {
          ospray::Parse<2>(ac, av, i, numFrames);
        } catch (const std::runtime_error &e) {
          throw std::runtime_error(std::string(e.what()) +
              " usage: -frames "
              "<# of warmup frames> "
              "<# of benchmark frames>");
        }
      } else if (str == "-hacked-volume") {
        use_hacked_vol = true;
      } else if (str[0] == '-') {
        throw std::runtime_error("unknown argument: " + str);
      } else {
        inputFiles.push_back(av[i]);
      }
    }
  }
};//ospray
