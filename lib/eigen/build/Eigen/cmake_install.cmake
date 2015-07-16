# Install script for directory: /home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen

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
   "/usr/local/include/eigen3/Eigen/Eigenvalues;/usr/local/include/eigen3/Eigen/SparseCore;/usr/local/include/eigen3/Eigen/QR;/usr/local/include/eigen3/Eigen/Householder;/usr/local/include/eigen3/Eigen/MetisSupport;/usr/local/include/eigen3/Eigen/SuperLUSupport;/usr/local/include/eigen3/Eigen/Eigen;/usr/local/include/eigen3/Eigen/SparseCholesky;/usr/local/include/eigen3/Eigen/StdDeque;/usr/local/include/eigen3/Eigen/Geometry;/usr/local/include/eigen3/Eigen/SVD;/usr/local/include/eigen3/Eigen/CholmodSupport;/usr/local/include/eigen3/Eigen/Cholesky;/usr/local/include/eigen3/Eigen/LU;/usr/local/include/eigen3/Eigen/UmfPackSupport;/usr/local/include/eigen3/Eigen/IterativeLinearSolvers;/usr/local/include/eigen3/Eigen/OrderingMethods;/usr/local/include/eigen3/Eigen/SparseLU;/usr/local/include/eigen3/Eigen/Core;/usr/local/include/eigen3/Eigen/PardisoSupport;/usr/local/include/eigen3/Eigen/SparseQR;/usr/local/include/eigen3/Eigen/Sparse;/usr/local/include/eigen3/Eigen/StdVector;/usr/local/include/eigen3/Eigen/Dense;/usr/local/include/eigen3/Eigen/Jacobi;/usr/local/include/eigen3/Eigen/SPQRSupport;/usr/local/include/eigen3/Eigen/StdList;/usr/local/include/eigen3/Eigen/PaStiXSupport;/usr/local/include/eigen3/Eigen/QtAlignedMalloc")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/include/eigen3/Eigen" TYPE FILE FILES
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/Eigenvalues"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/SparseCore"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/QR"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/Householder"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/MetisSupport"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/SuperLUSupport"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/Eigen"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/SparseCholesky"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/StdDeque"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/Geometry"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/SVD"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/CholmodSupport"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/Cholesky"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/LU"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/UmfPackSupport"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/IterativeLinearSolvers"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/OrderingMethods"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/SparseLU"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/Core"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/PardisoSupport"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/SparseQR"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/Sparse"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/StdVector"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/Dense"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/Jacobi"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/SPQRSupport"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/StdList"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/PaStiXSupport"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/QtAlignedMalloc"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/build/Eigen/src/cmake_install.cmake")

endif()

