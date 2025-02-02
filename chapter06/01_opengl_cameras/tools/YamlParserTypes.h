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
      rhs.isModelFile = node["model-file"].as<std::string>();
      rhs.isWorldPosition = node["position"].as<glm::vec3>();
      rhs.isWorldRotation = node["rotation"].as<glm::vec3>();
      rhs.isScale = node["scale"].as<float>();
      rhs.isSwapYZAxis = node["swap-axes"].as<bool>();
      rhs.isAnimClipNr = node["anim-clip-number"].as<int>();
      rhs.isAnimSpeedFactor = node["anim-clip-speed"].as<float>();
      if (node["target-of-cameras"]) {
        rhs.eisCameraNames = node["target-of-cameras"].as<std::vector<std::string>>();
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
      CameraSettings defaultSettings = CameraSettings{};
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
        rhs.csOrthoScale = node["ortho-scale"].as<float>();
      }
      rhs.csCamType = static_cast<cameraType>(node["camera-type"].as<int>());
      rhs.csCamProjection = static_cast<cameraProjection>(node["camera-projection"].as<int>());
      if (node["1st-person-view-lock"]) {
        rhs.csFirstPersonLockView = node["1st-person-view-lock"].as<bool>();
      }
      if (node["1st-person-bone-to-follow"]) {
        rhs.csFirstPersonBoneToFollow = node["1st-person-bone-to-follow"].as<int>();
      }
      if (node["1st-person-view-offsets"]) {
        rhs.csFirstPersonOffsets = node["1st-person-view-offsets"].as<glm::vec3>();
      }
      if (node["3rd-person-view-distance"]) {
        rhs.csThirdPersonDistance = node["3rd-person-view-distance"].as<float>();
      }
      if (node["3rd-person-view-height-offset"]) {
        rhs.csThirdPersonHeightOffset = node["3rd-person-height-offset"].as<float>();
      }
      return true;
    }
  };
}
