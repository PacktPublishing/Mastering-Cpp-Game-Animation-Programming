#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "InstanceSettings.h"
#include "CameraSettings.h"
#include "Enums.h"
#include "Logger.h"

namespace YAML {
  /* support GLM types */
  template<>
  struct convert<glm::vec3> {
    static Node encode(const glm::vec3& rhs) {
      Node node;
      node.push_back(rhs.x);
      node.push_back(rhs.y);
      node.push_back(rhs.z);
      return node;
    }

    static bool decode(const Node& node, glm::vec3& rhs) {
      if(!node.IsSequence() || node.size() != 3) {
        Logger::log(1, "%s error: glm::vec3 must be a sequence and have 3 elements\n", __FUNCTION__);
        return false;
      }

      rhs.x = node[0].as<float>();
      rhs.y = node[1].as<float>();
      rhs.z = node[2].as<float>();
      return true;
    }
  };

  /* read and write InstanceSettings */
  template<>
  struct convert<ExtendedInstanceSettings> {
    static Node encode(const ExtendedInstanceSettings& rhs) {
      Node node;
      node["model-file"] = rhs.isModelFile;
      node["position"] = rhs.isWorldPosition;
      node["rotation"] = rhs.isWorldRotation;
      node["scale"] = rhs.isWorldRotation;
      node["swap-axes"] = rhs.isSwapYZAxis;
      node["anim-clip-number"] = rhs.isAnimClipNr;
      node["anim-clip-speed"] = rhs.isAnimSpeedFactor;
      node["target-of-cameras"] = rhs.eisCameraNames;
      return node;
    }

    static bool decode(const Node& node, ExtendedInstanceSettings& rhs) {
      InstanceSettings defaultSettings{};
      rhs.isModelFile = node["model-file"].as<std::string>();
      try {
        rhs.isWorldPosition = node["position"].as<glm::vec3>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse position of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
        rhs.isWorldPosition = defaultSettings.isWorldPosition;
      }
      try {
        rhs.isWorldRotation = node["rotation"].as<glm::vec3>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse rotation of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
        rhs.isWorldRotation = defaultSettings.isWorldRotation;
      }
      try {
        rhs.isScale = node["scale"].as<float>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse scaling of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
        rhs.isScale = defaultSettings.isScale;
      }
      try {
        rhs.isSwapYZAxis = node["swap-axes"].as<bool>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse Y-Z axis swapping of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
        rhs.isSwapYZAxis = defaultSettings.isSwapYZAxis;
      }
      try {
        rhs.isAnimClipNr = node["anim-clip-number"].as<int>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse anim clip number of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
        rhs.isAnimClipNr = defaultSettings.isAnimClipNr;
      }
      try {
        rhs.isAnimSpeedFactor = node["anim-clip-speed"].as<float>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse anim clip speed of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
        rhs.isAnimSpeedFactor = defaultSettings.isAnimSpeedFactor;
      }
      if (node["target-of-cameras"]) {
        try {
          rhs.eisCameraNames = node["target-of-cameras"].as<std::vector<std::string>>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse target camera of an instance of model '%s', ignoring\n", __FUNCTION__, rhs.isModelFile.c_str());
        }
      }
      return true;
    }
  };

  /* read and write CameraSettings */
  template<>
  struct convert<CameraSettings> {
    static Node encode(const CameraSettings& rhs) {
      Node node;
      node["camera-name"] = rhs.csCamName;
      node["position"] = rhs.csWorldPosition;
      node["view-azimuth"] = rhs.csViewAzimuth;
      node["view-elevation"] = rhs.csViewElevation;
      if (rhs.csCamProjection == cameraProjection::perspective) {
        node["field-of-view"] = rhs.csFieldOfView;
      }
      if (rhs.csCamProjection == cameraProjection::orthogonal) {
        node["ortho-scale"] = rhs.csOrthoScale;
      }
      node["camera-type"] = static_cast<int>(rhs.csCamType);
      node["camera-projection"] = static_cast<int>(rhs.csCamProjection);
      if (rhs.csCamType == cameraType::firstPerson) {
        node["1st-person-view-lock"] = rhs.csFirstPersonLockView;
        node["1st-person-bone-to-follow"] = rhs.csFirstPersonBoneToFollow;
        node["1st-person-view-offsets"] = rhs.csFirstPersonOffsets;
      }
      if (rhs.csCamType == cameraType::thirdPerson) {
        node["3rd-person-view-distance"] = rhs.csThirdPersonDistance ;
        node["3rd-person-height-offset"] = rhs.csThirdPersonHeightOffset;
      }
      return node;
    }

    static bool decode(const Node& node, CameraSettings& rhs) {
      CameraSettings defaultSettings{};
      rhs.csCamName = node["camera-name"].as<std::string>();
      try {
        rhs.csWorldPosition = node["position"].as<glm::vec3>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse position of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
        rhs.csWorldPosition = defaultSettings.csWorldPosition;
      }
      try {
        rhs.csViewAzimuth = node["view-azimuth"].as<float>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse azimuth of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
        rhs.csViewAzimuth = defaultSettings.csViewAzimuth;
      }
      try {
        rhs.csViewElevation = node["view-elevation"].as<float>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse elevation of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
        rhs.csViewElevation = defaultSettings.csViewElevation;
      }
      if (node["field-of-view"]) {
        try {
          rhs.csFieldOfView = node["field-of-view"].as<int>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse field of view of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
          rhs.csFieldOfView = defaultSettings.csFieldOfView;
        }
      }
      if (node["ortho-scale"]) {
        try {
          rhs.csOrthoScale = node["ortho-scale"].as<float>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse orthogonal scale of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
          rhs.csOrthoScale = defaultSettings.csOrthoScale;
        }
      }
      try {
        rhs.csCamType = static_cast<cameraType>(node["camera-type"].as<int>());
      } catch (...) {
        Logger::log(1, "%s warning: could not parse default type of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
        rhs.csCamType = defaultSettings.csCamType;
      }

      if (rhs.csCamType == cameraType::free || rhs.csCamType == cameraType::stationary || rhs.csCamType == cameraType::stationaryFollowing) {
        try {
          rhs.csCamProjection = static_cast<cameraProjection>(node["camera-projection"].as<int>());
        } catch (...) {
          Logger::log(1, "%s warning: could not parse projection mode of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
          rhs.csCamProjection = defaultSettings.csCamProjection;
        }
      }
      if (rhs.csCamType == cameraType::firstPerson) {
        if (node["1st-person-view-lock"]) {
          try {
            rhs.csFirstPersonLockView = node["1st-person-view-lock"].as<bool>();
          } catch (...) {
            Logger::log(1, "%s warning: could not parse first person view lock of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
            rhs.csFirstPersonLockView = defaultSettings.csFirstPersonLockView;
          }
        }
        if (node["1st-person-bone-to-follow"]) {
          try {
            rhs.csFirstPersonBoneToFollow = node["1st-person-bone-to-follow"].as<int>();
          } catch (...) {
            Logger::log(1, "%s warning: could not parse first person bone to follow of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
            rhs.csFirstPersonBoneToFollow = defaultSettings.csFirstPersonBoneToFollow;
          }
        }
        if (node["1st-person-view-offsets"]) {
          try {
            rhs.csFirstPersonOffsets = node["1st-person-view-offsets"].as<glm::vec3>();
          } catch (...) {
            Logger::log(1, "%s warning: could not parse first person view offset of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
            rhs.csFirstPersonOffsets = defaultSettings.csFirstPersonOffsets;
          }
        }
      }
      if (rhs.csCamType == cameraType::thirdPerson) {
        if (node["3rd-person-view-distance"]) {
          try {
            rhs.csThirdPersonDistance = node["3rd-person-view-distance"].as<float>();
          } catch (...) {
            Logger::log(1, "%s warning: could not parse third person view distance of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
            rhs.csThirdPersonDistance = defaultSettings.csThirdPersonDistance;
          }
        }
        if (node["3rd-person-view-height-offset"]) {
          try {
            rhs.csThirdPersonHeightOffset = node["3rd-person-height-offset"].as<float>();
          } catch (...) {
            Logger::log(1, "%s warning: could not parse third person view height offset of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
            rhs.csThirdPersonHeightOffset = defaultSettings.csThirdPersonHeightOffset;
          }
        }
      }
      return true;
    }
  };
}
