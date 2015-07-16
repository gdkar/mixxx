# Install script for directory: /home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Devel")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/include/eigen3/unsupported/Eigen/AdolcForward;/usr/local/include/eigen3/unsupported/Eigen/BVH;/usr/local/include/eigen3/unsupported/Eigen/IterativeSolvers;/usr/local/include/eigen3/unsupported/Eigen/MatrixFunctions;/usr/local/include/eigen3/unsupported/Eigen/MoreVectorization;/usr/local/include/eigen3/unsupported/Eigen/AutoDiff;/usr/local/include/eigen3/unsupported/Eigen/AlignedVector3;/usr/local/include/eigen3/unsupported/Eigen/Polynomials;/usr/local/include/eigen3/unsupported/Eigen/FFT;/usr/local/include/eigen3/unsupported/Eigen/NonLinearOptimization;/usr/local/include/eigen3/unsupported/Eigen/SparseExtra;/usr/local/include/eigen3/unsupported/Eigen/IterativeSolvers;/usr/local/include/eigen3/unsupported/Eigen/NumericalDiff;/usr/local/include/eigen3/unsupported/Eigen/Skyline;/usr/local/include/eigen3/unsupported/Eigen/MPRealSupport;/usr/local/include/eigen3/unsupported/Eigen/OpenGLSupport;/usr/local/include/eigen3/unsupported/Eigen/KroneckerProduct;/usr/local/include/eigen3/unsupported/Eigen/Splines;/usr/local/include/eigen3/unsupported/Eigen/LevenbergMarquardt")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/include/eigen3/unsupported/Eigen" TYPE FILE FILES
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/AdolcForward"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/BVH"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/IterativeSolvers"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/MatrixFunctions"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/MoreVectorization"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/AutoDiff"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/AlignedVector3"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/Polynomials"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/FFT"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/NonLinearOptimization"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/SparseExtra"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/IterativeSolvers"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/NumericalDiff"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/Skyline"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/MPRealSupport"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/OpenGLSupport"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/KroneckerProduct"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/Splines"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/LevenbergMarquardt"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/build/unsupported/Eigen/src/cmake_install.cmake")
  include("/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/build/unsupported/Eigen/CXX11/cmake_install.cmake")

endif()

