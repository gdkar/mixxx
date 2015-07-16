# Install script for directory: /home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products

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
   "/usr/local/include/eigen3/Eigen/src/Core/products/TriangularMatrixMatrix_MKL.h;/usr/local/include/eigen3/Eigen/src/Core/products/GeneralMatrixMatrixTriangular_MKL.h;/usr/local/include/eigen3/Eigen/src/Core/products/TriangularSolverMatrix_MKL.h;/usr/local/include/eigen3/Eigen/src/Core/products/GeneralMatrixMatrix_MKL.h;/usr/local/include/eigen3/Eigen/src/Core/products/TriangularMatrixVector.h;/usr/local/include/eigen3/Eigen/src/Core/products/SelfadjointMatrixVector.h;/usr/local/include/eigen3/Eigen/src/Core/products/TriangularSolverMatrix.h;/usr/local/include/eigen3/Eigen/src/Core/products/SelfadjointMatrixMatrix.h;/usr/local/include/eigen3/Eigen/src/Core/products/TriangularMatrixMatrix.h;/usr/local/include/eigen3/Eigen/src/Core/products/GeneralMatrixMatrix.h;/usr/local/include/eigen3/Eigen/src/Core/products/GeneralMatrixVector_MKL.h;/usr/local/include/eigen3/Eigen/src/Core/products/SelfadjointMatrixMatrix_MKL.h;/usr/local/include/eigen3/Eigen/src/Core/products/Parallelizer.h;/usr/local/include/eigen3/Eigen/src/Core/products/TriangularSolverVector.h;/usr/local/include/eigen3/Eigen/src/Core/products/SelfadjointProduct.h;/usr/local/include/eigen3/Eigen/src/Core/products/GeneralBlockPanelKernel.h;/usr/local/include/eigen3/Eigen/src/Core/products/SelfadjointRank2Update.h;/usr/local/include/eigen3/Eigen/src/Core/products/GeneralMatrixMatrixTriangular.h;/usr/local/include/eigen3/Eigen/src/Core/products/GeneralMatrixVector.h;/usr/local/include/eigen3/Eigen/src/Core/products/SelfadjointMatrixVector_MKL.h;/usr/local/include/eigen3/Eigen/src/Core/products/TriangularMatrixVector_MKL.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/include/eigen3/Eigen/src/Core/products" TYPE FILE FILES
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/TriangularMatrixMatrix_MKL.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/GeneralMatrixMatrixTriangular_MKL.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/TriangularSolverMatrix_MKL.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/GeneralMatrixMatrix_MKL.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/TriangularMatrixVector.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/SelfadjointMatrixVector.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/TriangularSolverMatrix.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/SelfadjointMatrixMatrix.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/TriangularMatrixMatrix.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/GeneralMatrixMatrix.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/GeneralMatrixVector_MKL.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/SelfadjointMatrixMatrix_MKL.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/Parallelizer.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/TriangularSolverVector.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/SelfadjointProduct.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/GeneralBlockPanelKernel.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/SelfadjointRank2Update.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/GeneralMatrixMatrixTriangular.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/GeneralMatrixVector.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/SelfadjointMatrixVector_MKL.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/Eigen/src/Core/products/TriangularMatrixVector_MKL.h"
    )
endif()

