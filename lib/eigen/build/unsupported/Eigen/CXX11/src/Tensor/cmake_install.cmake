# Install script for directory: /home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor

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
   "/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorGenerator.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorChipping.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorBroadcasting.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorBase.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorStriding.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorDimensions.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorContractionCuda.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorConvolution.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorFunctors.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorInitializer.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorExecutor.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorTraits.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorInflation.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorDimensionList.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorIndexList.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorDevice.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorIO.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorEvaluator.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorShuffling.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorAssign.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorLayoutSwap.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorEvalTo.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorFixedSize.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorCustomOp.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorConversion.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorStorage.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorForwardDeclarations.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorIntDiv.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorForcedEval.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorReduction.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorMorphing.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorMap.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorVolumePatch.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorConcatenation.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorMeta.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorContraction.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/Tensor.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorImagePatch.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorContractionThreadPool.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorPadding.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorRef.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorExpr.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorReverse.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorPatch.h;/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor/TensorDeviceType.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/include/eigen3/unsupported/Eigen/CXX11/src/Tensor" TYPE FILE FILES
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorGenerator.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorChipping.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorBroadcasting.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorBase.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorStriding.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorDimensions.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorContractionCuda.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorConvolution.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorFunctors.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorInitializer.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorExecutor.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorTraits.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorInflation.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorDimensionList.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorIndexList.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorDevice.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorIO.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorEvaluator.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorShuffling.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorAssign.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorLayoutSwap.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorEvalTo.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorFixedSize.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorCustomOp.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorConversion.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorStorage.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorForwardDeclarations.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorIntDiv.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorForcedEval.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorReduction.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorMorphing.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorMap.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorVolumePatch.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorConcatenation.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorMeta.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorContraction.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/Tensor.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorImagePatch.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorContractionThreadPool.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorPadding.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorRef.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorExpr.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorReverse.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorPatch.h"
    "/home/zen/scratch/githubs/audio-systems/mixes/mixxx-mp3-rewrite/lib/eigen/unsupported/Eigen/CXX11/src/Tensor/TensorDeviceType.h"
    )
endif()

