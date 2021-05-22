cmake_minimum_required(VERSION 2.8)
#####################################################################################
# offer the choice of having shared_sources as a sub-folder... good for packaging a sample
#
# if BASE_DIRECTORY not defined, it means this cmake file was called as the first entry point and not included
if(NOT BASE_DIRECTORY) # if not defined, it means this cmake file was called as the first entry point and not included
  set(PROJNAME shared_sources)
  Project(${PROJNAME})
  Message(STATUS "-------------------------------")
  Message(STATUS "Processing Project ${PROJNAME}:")
  SET(BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)
  INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists_include.txt)
else()
  Message(STATUS "-------------------------------")
  Message(STATUS "Processing shared_sources lib")
endif()

set(MAIN_SOURCE ${BASE_DIRECTORY}/shared_sources/main.h )
if(GLFW_FOUND AND USE_GLFW)
  SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_glfw.cpp)
else()
  if(UNIX) 
    SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_x11.cpp ) #TODO
  else(UNIX)
    if(APPLE)
      SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_osx.cpp ) #TODO
    else(APPLE)
      if(WIN32)
        SET(MAIN_FILE ${BASE_DIRECTORY}/shared_sources/main_win32.cpp )
        # Add the OpenGL code: some project in the group of stuff to build need it
        if(USING_OPENGL)
          SET(WININTERNAL_FILE ${BASE_DIRECTORY}/shared_sources/wininternal_win32_gl.cpp)
        endif()
        # Add the Vulkan code: some project in the group of stuff to build need it
        if(USING_VULKANSDK)
          LIST(APPEND WININTERNAL_FILE ${BASE_DIRECTORY}/shared_sources/wininternal_win32_vk.cpp)
        endif()
      endif(WIN32)
    endif(APPLE)
  endif(UNIX)
endif()
LIST(APPEND MAIN_SOURCE
    ${MAIN_FILE}
    ${WININTERNAL_FILE}
)

# Eventually add the Vulkan package: some project in the group of stuff to build need it
_optional_package_VulkanSDK()
_optional_package_OpenGL()
#eventually add the package for socket communication
_optional_package_Sockets()
_optional_package_CThread()
# process the rest of some cmake code that needs to be done after the packages
_process_shared_cmake_code()

file(GLOB NVMATH_SOURCE nv_math/*.cpp nv_math/*.hpp nv_math/*.inl nv_math/*.h)
file(GLOB NVHELPERS_SOURCE nv_helpers/*.cpp nv_helpers/*.hpp nv_helpers/*.inl nv_helpers/*.h)
file(GLOB NOISE_SOURCE noise/*.cpp noise/*.hpp noise/*.inl noise/*.h)
if(USING_OPENGL)
  file(GLOB NVHELPERSGL_SOURCE nv_helpers_gl/*.cpp nv_helpers_gl/*.hpp nv_helpers_gl/*.inl nv_helpers_gl/*.h)
  file(GLOB OPENGLTEXT nv_helpers_gl/OpenGLText/*.cpp nv_helpers_gl/OpenGLText/*.hpp nv_helpers_gl/OpenGLText/*.inl nv_helpers_gl/OpenGLText/*.h)
  file(GLOB TRACEDISPLAY nv_helpers_gl/traceDisplay/*.cpp nv_helpers_gl/traceDisplay/*.hpp nv_helpers_gl/traceDisplay/*.inl nv_helpers_gl/traceDisplay/*.h)
  source_group("OpenGLText" FILES ${OPENGLTEXT})
  source_group("traceDisplay" FILES ${TRACEDISPLAY})
  source_group("nv_helpers_gl" FILES ${NVHELPERSGL_SOURCE})
endif()

if(USING_VULKANSDK)
  file(GLOB NVHELPERSVK_SOURCE nv_helpers_vk/*.cpp nv_helpers_vk/*.hpp nv_helpers_vk/*.inl nv_helpers_vk/*.h)
  source_group("nv_helpers_vk" FILES ${NVHELPERSVK_SOURCE})
endif()

if(GLEW_SOURCE)
  Message(STATUS "Note: directly adding glew.c in the shared_sources lib")
  file(GLOB GLEW_FILES glew/include/GL/*.h glew/src/glew.c)
  source_group("glew" FILES ${GLEW_FILES})
endif()

source_group("nv_math" FILES ${NVMATH_SOURCE})
source_group("nv_helpers" FILES ${NVHELPERS_SOURCE})
source_group("noise" FILES ${NOISE_SOURCE})
source_group("main" FILES ${MAIN_SOURCE})

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)
if(SUPPORT_NVTOOLSEXT)
  include_directories(
      ${BASE_DIRECTORY}/shared_external/NSight
  )
  LIST(APPEND PACKAGE_SOURCE_FILES 
    ${BASE_DIRECTORY}/shared_external/NSight/NSightEvents.h
    ${BASE_DIRECTORY}/shared_external/NSight/nvToolsExt.h
  )
  add_definitions(-DSUPPORT_NVTOOLSEXT)
endif()

if(USING_OPENGL)
  SET(OPENGL_FILES
    ${OPENGLTEXT}
    ${TRACEDISPLAY}
    ${NVHELPERSGL_SOURCE} 
    ${GLEW_FILES} 
  )
endif()

add_library(shared_sources STATIC 
  ${NVHELPERS_SOURCE} 
  ${NVMATH_SOURCE} 
  ${NOISE_SOURCE} 
  ${MAIN_SOURCE}
  ${OPENGL_FILES}
  ${NVHELPERSVK_SOURCE}
  ${PACKAGE_SOURCE_FILES}
)

target_link_libraries(shared_sources ${PLATFORM_LIBRARIES})
