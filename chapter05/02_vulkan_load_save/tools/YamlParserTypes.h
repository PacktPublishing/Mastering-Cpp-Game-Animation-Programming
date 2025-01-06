#pragma once
#include <glm/glm.hpp>

#include "InstanceSettings.h"

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
      rhs.isModelFile = node["model-file"].as<std::string>();
      rhs.isWorldPosition = node["position"].as<glm::vec3>();
      rhs.isWorldRotation = node["rotation"].as<glm::vec3>();
      rhs.isScale = node["scale"].as<float>();
      rhs.isSwapYZAxis = node["swap-axes"].as<bool>();
      rhs.isAnimClipNr = node["anim-clip-number"].as<int>();
      rhs.isAnimSpeedFactor = node["anim-clip-speed"].as<float>();
      return true;
    }
  };
}
