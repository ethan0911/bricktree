//!
//! This file contains headers of all external libraries
//!
#pragma once
#ifndef OSPRAY_COMMON_H
#define OSPRAY_COMMON_H
#define NOMINMAX

//
// include ospray
//
#include "ospray/ospray.h"
#include "ospcommon/vec.h"

//
// threading
//
#include <omp.h>
#include <tbb/tbb.h>
//
// GLFW
//
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

//! @name error check helper from EPFL ICG class
static inline const char *ErrorString(GLenum error) {
  const char *msg;
  switch (error) {
#define Case(Token)  case Token: msg = #Token; break;
    Case(GL_INVALID_ENUM);
    Case(GL_INVALID_VALUE);
    Case(GL_INVALID_OPERATION);
    Case(GL_INVALID_FRAMEBUFFER_OPERATION);
    Case(GL_NO_ERROR);
    Case(GL_OUT_OF_MEMORY);
#undef Case
  }
  return msg;
}

//! @name check error
static inline void _glCheckError
  (const char *file, int line, const char *comment) {
  GLenum error;
  while ((error = glGetError()) != GL_NO_ERROR) {
    fprintf(stderr, "ERROR: %s (file %s, line %i: %s).\n", comment, file, line, ErrorString(error));
  }
}

#ifndef NDEBUG
# define check_error_gl(x) _glCheckError(__FILE__, __LINE__, x)
#else
# define check_error_gl(x) ((void)0)
#endif

#endif //OSPRAY_COMMON_H
