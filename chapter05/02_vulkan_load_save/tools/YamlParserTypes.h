#pragma once
#include <glm/glm.hpp>

#include "InstanceSettings.h"
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
  struct convert<InstanceSettings> {
    static Node encode(const InstanceSettings& rhs) {
      Node node;
      node["model-file"] = rhs.isModelFile;
      node["position"] = rhs.isWorldPosition;
      node["rotation"] = rhs.isWorldRotation;
      node["scale"] = rhs.isWorldRotation;
      node["swap-axes"] = rhs.isSwapYZAxis;
      node["anim-clip-number"] = rhs.isAnimClipNr;
      node["anim-clip-speed"] = rhs.isAnimSpeedFactor;
      return node;
    }

    static bool decode(const Node& node, InstanceSettings& rhs) {
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
      return true;
    }
  };
}
