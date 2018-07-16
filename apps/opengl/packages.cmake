#
#--- OPENGL
#
if (NOT TARGET OpenGL)
  find_package(OpenGL REQUIRED)
  add_library(OpenGL INTERFACE IMPORTED)
  set_target_properties(OpenGL PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${OPENGL_LIBRARIES}")
endif ()
#
#--- GLFW
#
if (NOT TARGET glfw)
  find_package(glfw3 REQUIRED)
  if (NOT TARGET glfw)
    add_library(glfw INTERFACE IMPORTED)
  endif ()
  set_target_properties(glfw PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GLFW_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${GLFW_LIBRARIES}")
endif ()
#
#--- GLAD
#
if (NOT TARGET glad)
  add_library(glad external/glad/glad.c)
  target_include_directories(glad PUBLIC
    "$<BUILD_INTERFACE:"
    "external/glad;"
    ">")
endif ()    
#
#--- ImGUI
#
if (NOT TARGET imgui)
  if (NOT TARGET ospray_imgui)
    message(FATAL_ERROR "cannot find imgui")
  endif ()
endif ()
