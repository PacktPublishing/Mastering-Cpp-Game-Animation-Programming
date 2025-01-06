#pragma once
#include <string>
#include <vector>
#include <set>
#include <glm/glm.hpp>

#include "InstanceSettings.h"
#include "ModelSettings.h"
#include "CameraSettings.h"
#include "BehaviorData.h"
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

  template<>
  struct convert<glm::vec4> {
    static Node encode(const glm::vec4& rhs) {
      Node node;
      node.push_back(rhs.x);
      node.push_back(rhs.y);
      node.push_back(rhs.z);
      node.push_back(rhs.w);
      return node;
    }

    static bool decode(const Node& node, glm::vec4& rhs) {
      if(!node.IsSequence() || node.size() != 4) {
        return false;
      }

      rhs.x = node[0].as<float>();
      rhs.y = node[1].as<float>();
      rhs.z = node[2].as<float>();
      rhs.w = node[3].as<float>();
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
      node["1st-anim-clip-number"] = rhs.isFirstAnimClipNr;
      node["2nd-anim-clip-number"] = rhs.isSecondAnimClipNr;
      node["anim-clip-speed"] = rhs.isAnimSpeedFactor;
      node["anim-blend-factor"] = rhs.isAnimBlendFactor;
      node["target-of-cameras"] = rhs.eisCameraNames;
      if (!rhs.isNodeTreeName.empty()) {
        node["node-tree"] = rhs.isNodeTreeName;
      }
      if (rhs.isFaceAnim != faceAnimation::none) {
        node["face-anim"] = rhs.isFaceAnim;
        node["face-anim-weight"] = rhs.isFaceAnimWeight;
      }
      if (rhs.isHeadLeftRightMove != 0.0f) {
        node["head-anim-left-right-timestamp"] = rhs.isHeadLeftRightMove;
      }
      if (rhs.isHeadUpDownMove != 0.0f) {
        node["head-anim-up-down-timestamp"] = rhs.isHeadUpDownMove;
      }
      return node;
    }

    static bool decode(const Node& node, ExtendedInstanceSettings& rhs) {
      rhs.isModelFile = node["model-file"].as<std::string>();
      rhs.isWorldPosition = node["position"].as<glm::vec3>();
      rhs.isWorldRotation = node["rotation"].as<glm::vec3>();
      rhs.isScale = node["scale"].as<float>();
      rhs.isSwapYZAxis = node["swap-axes"].as<bool>();
      /* support reading back old instance data */
      if (node["anim-clip-number"]) {
        rhs.isFirstAnimClipNr = node["anim-clip-number"].as<int>();
        rhs.isSecondAnimClipNr = node["anim-clip-number"].as<int>();
        rhs.isAnimBlendFactor = 0.0f;
      }
      if (node["1st-anim-clip-number"]) {
        rhs.isFirstAnimClipNr = node["1st-anim-clip-number"].as<int>();
      }
      if (node["2nd-anim-clip-number"]) {
        rhs.isSecondAnimClipNr = node["2nd-anim-clip-number"].as<int>();
      }
      rhs.isAnimSpeedFactor = node["anim-clip-speed"].as<float>();
      if (node["anim-blend-factor"]) {
        rhs.isAnimBlendFactor = node["anim-blend-factor"].as<float>();
      }
      if (node["target-of-cameras"]) {
        rhs.eisCameraNames = node["target-of-cameras"].as<std::vector<std::string>>();
      }
      if (node["node-tree"]) {
        rhs.isNodeTreeName = node["node-tree"].as<std::string>();
      }
      if (node["face-anim"]) {
        rhs.isFaceAnim = node["face-anim"].as<faceAnimation>();
        rhs.isFaceAnimWeight = node["face-anim-weight"].as<float>();
      }
      if (node["head-anim-left-right-timestamp"]) {
        rhs.isHeadLeftRightMove = node["head-anim-left-right-timestamp"].as<float>();
      }
      if (node["head-anim-up-down-timestamp"]) {
        rhs.isHeadUpDownMove = node["head-anim-up-down-timestamp"].as<float>();
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
      if (rhs.csCamType == cameraType::stationaryFollowing) {
        node["follow-cam-height-offset"] = rhs.csFollowCamHeightOffset;
      }
      return node;
    }

    static bool decode(const Node& node, CameraSettings& rhs) {
      rhs.csCamName = node["camera-name"].as<std::string>();
      rhs.csWorldPosition = node["position"].as<glm::vec3>();
      rhs.csViewAzimuth = node["view-azimuth"].as<float>();
      rhs.csViewElevation = node["view-elevation"].as<float>();
      if (node["field-of-view"]) {
        rhs.csFieldOfView = node["field-of-view"].as<int>();
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
      if (node["follow-cam-height-offset"]) {
        rhs.csFollowCamHeightOffset = node["follow-cam-height-offset"].as<float>();
      }
      return true;
    }
  };

  /* read and write IdleWalkRunBlending */
  template<>
  struct convert<IdleWalkRunBlending> {
    static Node encode(const IdleWalkRunBlending& rhs) {
      Node node;
      node["idle-clip"] = rhs.iwrbIdleClipNr;
      node["idle-clip-speed"] = rhs.iwrbIdleClipSpeed;
      node["walk-clip"] = rhs.iwrbWalkClipNr;
      node["walk-clip-speed"] = rhs.iwrbWalkClipSpeed;
      node["run-clip"] = rhs.iwrbRunClipNr;
      node["run-clip-speed"] = rhs.iwrbRunClipSpeed;
      return node;
    }

    static bool decode(const Node& node, IdleWalkRunBlending& rhs) {
      rhs.iwrbIdleClipNr = node["idle-clip"].as<int>();
      rhs.iwrbIdleClipSpeed = node["idle-clip-speed"].as<float>();
      rhs.iwrbWalkClipNr = node["walk-clip"].as<int>();
      rhs.iwrbWalkClipSpeed = node["walk-clip-speed"].as<float>();
      rhs.iwrbRunClipNr = node["run-clip"].as<int>();
      rhs.iwrbRunClipSpeed = node["run-clip-speed"].as<float>();
      return true;
    }
  };

  /* read and write ActionAnimation */
  template<>
  struct convert<ActionAnimation> {
    static Node encode(const ActionAnimation& rhs) {
      Node node;
      node["clip"] = rhs.aaClipNr;;
      node["clip-speed"] = rhs.aaClipSpeed;
      return node;
    }

    static bool decode(const Node& node, ActionAnimation& rhs) {
      rhs.aaClipNr = node["clip"].as<int>();
      rhs.aaClipSpeed = node["clip-speed"].as<float>();
      return true;
    }
  };

  /* read and write moveState */
  template<>
  struct convert<moveState> {
    static Node encode(const moveState& rhs) {
      Node node;
      node = static_cast<int>(rhs);
      return node;
    }

    static bool decode(const Node& node, moveState& rhs) {
      rhs = static_cast<moveState>(node.as<int>());
      return true;
    }
  };

  /* read and write moveDirection */
  template<>
  struct convert<moveDirection> {
    static Node encode(const moveDirection& rhs) {
      Node node;
      node = static_cast<int>(rhs);
      return node;
    }

    static bool decode(const Node& node, moveDirection& rhs) {
      rhs = static_cast<moveDirection>(node.as<int>());
      return true;
    }
  };

  /* read and write ModelSettings */
  template<>
  struct convert<ModelSettings> {
    static Node encode(const ModelSettings& rhs) {
      Node node;
      node["model-file"] = rhs.msModelFilenamePath;
      node["model-name"] = rhs.msModelFilename;
      Node clips = node["idle-walk-run-clips"];
      for (const auto& blend : rhs.msIWRBlendings) {
        clips[blend.first] = blend.second;
      }
      clips = node["action-clips"];
      for (const auto& blend : rhs.msActionClipMappings) {
        clips[blend.first] = blend.second;
      }
      clips = node["action-sequences"];
      for (const auto& state : rhs.msAllowedStateOrder) {
        clips[state.first] = state.second;
      }
      node["bounding-sphere-adjustment"] = rhs.msBoundingSphereAdjustments;
      clips = node["head-movement-mappings"];
      for (const auto& state : rhs.msHeadMoveClipMappings) {
        clips[state.first] = state.second;
      }
      return node;
    }

    static bool decode(const Node& node, ModelSettings& rhs) {
      rhs.msModelFilenamePath = node["model-file"].as<std::string>();
      rhs.msModelFilename = node["model-name"].as<std::string>();
      if (Node clipNode = node["idle-walk-run-clips"]) {
        for (size_t i = 0; i < clipNode.size(); ++i) {
          std::map<moveDirection, IdleWalkRunBlending> entry = clipNode[i].as<std::map<moveDirection, IdleWalkRunBlending>>();
          rhs.msIWRBlendings.insert(entry.begin(), entry.end());
        }
      }
      if (Node clipNode = node["action-clips"]) {
        for (size_t i = 0; i < clipNode.size(); ++i) {
          std::map<moveState, ActionAnimation> entry = clipNode[i].as<std::map<moveState, ActionAnimation>>();
          rhs.msActionClipMappings.insert(entry.begin(), entry.end());
        }
      }
      if (Node clipNode = node["action-sequences"]) {
        for (size_t i = 0; i < clipNode.size(); ++i) {
          /* std::set does not work, but std::map does... */
          std::map<moveState, moveState> entry = clipNode[i].as<std::map<moveState, moveState>>();
          std::pair<moveState, moveState> state = std::make_pair(entry.begin()->first, entry.begin()->second);
          rhs.msAllowedStateOrder.insert(state);
        }
      }
      if (Node clipNode = node["bounding-sphere-adjustments"]) {
        for (size_t i = 0; i < clipNode.size(); ++i) {
          glm::vec4 entry = clipNode[i].as<glm::vec4>();
          rhs.msBoundingSphereAdjustments.emplace_back(entry);
        }
      }
      if (Node clipNode = node["head-movement-mappings"]) {
        for (size_t i = 0; i < clipNode.size(); ++i) {
          std::map<headMoveDirection, int> entry = clipNode[i].as<std::map<headMoveDirection, int>>();
          rhs.msHeadMoveClipMappings.insert(entry.begin(), entry.end());
        }
      }
      return true;
    }
  };

  /* read and write collisionChecks */
  template<>
  struct convert<collisionChecks> {
    static Node encode(const collisionChecks& rhs) {
      Node node;
      node = static_cast<int>(rhs);
      return node;
    }

    static bool decode(const Node& node, collisionChecks& rhs) {
      rhs = static_cast<collisionChecks>(node.as<int>());
      return true;
    }
  };

  /* read and write graphNodeType */
  template<>
  struct convert<graphNodeType> {
    static Node encode(const graphNodeType& rhs) {
      Node node;
      node = static_cast<int>(rhs);
      return node;
    }

    static bool decode(const Node& node, graphNodeType& rhs) {
      rhs = static_cast<graphNodeType>(node.as<int>());
      return true;
    }
  };

  /* read and write Behavior data */
  template<>
  struct convert<EnhancedBehaviorData> {
    static Node encode(const EnhancedBehaviorData& rhs) {
      Node node;
      node["node-tree-name"] = rhs.bdName;
      node["editor-settings"] = rhs.bdEditorSettings;
      Node links = node["links"];
      /* TODO: node creation for nodes */
      for (const auto& link : rhs.bdGraphLinks) {
        links[link.first] = link.second;
      }
      return node;
    }

    static bool decode(const Node& node, EnhancedBehaviorData& rhs) {
      rhs.bdName = node["node-tree-name"].as<std::string>();
      rhs.bdEditorSettings = node["editor-settings"].as<std::string>();
      if (Node nodesNode = node["nodes"]) {
        for (size_t i = 0; i < nodesNode.size(); ++i) {
          PerNodeImportData nodeData;
          nodeData.nodeType = nodesNode[i]["node-type"].as<graphNodeType>();
          nodeData.nodeId = nodesNode[i]["node-id"].as<int>();

          if (Node nodeDataNode = nodesNode[i]["node-data"]) {
            std::vector<std::map<std::string, std::string>> entry = nodeDataNode.as<std::vector<std::map<std::string, std::string>>>();
            if (entry.size() > 0) {
              for (const auto& mapEntry : entry) {
                nodeData.nodeProperties.insert(mapEntry.begin(), mapEntry.end());
              }
            }
          }
          rhs.nodeImportData.emplace_back(nodeData);
        }
      }
      if (Node linkNode = node["links"]) {
        for (size_t i = 0; i < linkNode.size(); ++i) {
          std::map<int, std::pair<int, int>> entry = linkNode[i].as<std::map<int, std::pair<int, int>>>();
          rhs.bdGraphLinks.insert(entry.begin(), entry.end());
        }
      }
      return true;
    }
  };

  /* read and write faceAnimations */
  template<>
  struct convert<faceAnimation> {
    static Node encode(const faceAnimation& rhs) {
      Node node;
      node = static_cast<int>(rhs);
      return node;
    }

    static bool decode(const Node& node, faceAnimation& rhs) {
      rhs = static_cast<faceAnimation>(node.as<int>());
      return true;
    }
  };

  /* read and write headMoveDirection */
  template<>
  struct convert<headMoveDirection> {
    static Node encode(const headMoveDirection& rhs) {
      Node node;
      node = static_cast<int>(rhs);
      return node;
    }

    static bool decode(const Node& node, headMoveDirection& rhs) {
      rhs = static_cast<headMoveDirection>(node.as<int>());
      return true;
    }
  };
}
