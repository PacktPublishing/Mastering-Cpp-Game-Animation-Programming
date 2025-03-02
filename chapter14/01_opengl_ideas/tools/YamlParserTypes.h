#pragma once
#include <string>
#include <vector>
#include <set>
#include <glm/glm.hpp>

#include "InstanceSettings.h"
#include "ModelSettings.h"
#include "CameraSettings.h"
#include "BehaviorData.h"
#include "LevelSettings.h"
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
        Logger::log(1, "%s error: glm::vec4 must be a sequence and have 4 elements\n", __FUNCTION__);
        return false;
      }

      rhs.x = node[0].as<float>();
      rhs.y = node[1].as<float>();
      rhs.z = node[2].as<float>();
      rhs.w = node[3].as<float>();
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
      try {
        rhs.aaClipNr = node["clip"].as<int>();
        rhs.aaClipSpeed = node["clip-speed"].as<float>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse action animation mapping, using defaults\n", __FUNCTION__);
        rhs = ActionAnimation{};
      }
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
      try {
        rhs = static_cast<moveState>(node.as<int>());
      } catch (...) {
        Logger::log(1, "%s warning: could not parse move state, using default 'idle'\n", __FUNCTION__);
        rhs = moveState::idle;
      }
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
      try {
        rhs = static_cast<moveDirection>(node.as<int>());
      } catch (...) {
        Logger::log(1, "%s warning: could not parse move direction, using default 'none'\n", __FUNCTION__);
        rhs = moveDirection::none;
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
      try {
        rhs = static_cast<collisionChecks>(node.as<int>());
      } catch (...) {
        Logger::log(1, "%s warning: could not parse collision checks, using default 'none'\n", __FUNCTION__);
        rhs = collisionChecks::none;
      }
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
      try {
        rhs = static_cast<graphNodeType>(node.as<int>());
      } catch (...) {
        Logger::log(1, "%s warning: could not parse graph node type, using default 'none'\n", __FUNCTION__);
        rhs = graphNodeType::none;
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
      try {
        rhs = static_cast<faceAnimation>(node.as<int>());
      } catch (...) {
        Logger::log(1, "%s warning: could not parse face animation type, using default 'none'\n", __FUNCTION__);
        rhs = faceAnimation::none;
      }
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
      try {
        rhs = static_cast<headMoveDirection>(node.as<int>());
      } catch (...) {
        Logger::log(1, "%s warning: could not parse head move direction type, using default 'left'\n", __FUNCTION__);
        rhs = headMoveDirection::left;
      }
      return true;
    }
  };

  /* read and write timeOfDay */
  template<>
  struct convert<timeOfDay> {
    static Node encode(const timeOfDay& rhs) {
      Node node;
      node = static_cast<int>(rhs);
      return node;
    }

    static bool decode(const Node& node, timeOfDay& rhs) {
      try {
        rhs = static_cast<timeOfDay>(node.as<int>());
      } catch (...) {
        Logger::log(1, "%s warning: could not parse head move direction type, using default 'noon'\n", __FUNCTION__);
        rhs = timeOfDay::noon;
      }
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
      if (rhs.isFaceAnimType != faceAnimation::none) {
        node["face-anim"] = rhs.isFaceAnimType;
        node["face-anim-weight"] = rhs.isFaceAnimWeight;
      }
      if (rhs.isHeadLeftRightMove != 0.0f) {
        node["head-anim-left-right-timestamp"] = rhs.isHeadLeftRightMove;
      }
      if (rhs.isHeadUpDownMove != 0.0f) {
        node["head-anim-up-down-timestamp"] = rhs.isHeadUpDownMove;
      }
      node["enable-navigation"] = rhs.isNavigationEnabled;
      node["path-target-instance"] = rhs.isPathTargetInstance;
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
        Logger::log(1, "%s warning: could not parse Y-Z axis swapping oof an instance f model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
        rhs.isSwapYZAxis = defaultSettings.isSwapYZAxis;
      }
      /* migrate from old config */
      if (node["anim-clip-number"]) {
        Logger::log(1, "%s: found old (single) anim clip number, using it as first and second clip\n", __FUNCTION__);
        try {
          rhs.isFirstAnimClipNr = node["anim-clip-number"].as<int>();
          rhs.isSecondAnimClipNr = node["anim-clip-number"].as<int>();
          rhs.isAnimBlendFactor = 0.0f;
        } catch (...) {
          Logger::log(1, "%s warning: could not parse old anim clip number of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
          rhs.isFirstAnimClipNr = defaultSettings.isFirstAnimClipNr;
          rhs.isSecondAnimClipNr = defaultSettings.isSecondAnimClipNr;
          rhs.isAnimBlendFactor = defaultSettings.isAnimBlendFactor;
        }
      } else {
        try {
          rhs.isFirstAnimClipNr = node["1st-anim-clip-number"].as<int>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse first anim clip number of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
          rhs.isFirstAnimClipNr = defaultSettings.isFirstAnimClipNr;
        }
        try {
          rhs.isSecondAnimClipNr = node["2nd-anim-clip-number"].as<int>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse anim clip number of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
          rhs.isSecondAnimClipNr = defaultSettings.isSecondAnimClipNr;
        }
      }
      try {
        rhs.isAnimSpeedFactor = node["anim-clip-speed"].as<float>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse anim clip speed of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
        rhs.isAnimSpeedFactor = defaultSettings.isAnimSpeedFactor;
      }
      try {
        rhs.isAnimBlendFactor = node["anim-blend-factor"].as<float>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse anim blend factor of an instance of model '%s', init with a default value\n", __FUNCTION__, rhs.isModelFile.c_str());
        rhs.isAnimBlendFactor = defaultSettings.isAnimBlendFactor;
      }
      if (node["target-of-cameras"]) {
        try {
          rhs.eisCameraNames = node["target-of-cameras"].as<std::vector<std::string>>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse target camera of an instance of model '%s', ignoring\n", __FUNCTION__, rhs.isModelFile.c_str());
        }
      }
      if (node["node-tree"]) {
        try {
          rhs.isNodeTreeName = node["node-tree"].as<std::string>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse mode tree name of an instance of model '%s', ignoring\n", __FUNCTION__, rhs.isModelFile.c_str());
        }
      }
      if (node["face-anim"]) {
        try {
          rhs.isFaceAnimType = node["face-anim"].as<faceAnimation>();
          rhs.isFaceAnimWeight = node["face-anim-weight"].as<float>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse face anim settings of an instance of model '%s', ignoring\n", __FUNCTION__, rhs.isModelFile.c_str());
          rhs.isFaceAnimType = faceAnimation::none;
          rhs.isFaceAnimWeight = defaultSettings.isFaceAnimWeight;
        }
      }
      if (node["head-anim-left-right-timestamp"]) {
        try {
          rhs.isHeadLeftRightMove = node["head-anim-left-right-timestamp"].as<float>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse head left/right anim settings of an instance of model '%s', ignoring\n", __FUNCTION__, rhs.isModelFile.c_str());
          rhs.isHeadLeftRightMove = defaultSettings.isHeadLeftRightMove;
        }
      }
      if (node["head-anim-up-down-timestamp"]) {
        try {
          rhs.isHeadUpDownMove = node["head-anim-up-down-timestamp"].as<float>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse head up/down anim settings of an instance of model '%s', ignoring\n", __FUNCTION__, rhs.isModelFile.c_str());
          rhs.isHeadUpDownMove = defaultSettings.isHeadUpDownMove;
        }
      }
      if (node["enable-navigation"]) {
        try {
          rhs.isNavigationEnabled = node["enable-navigation"].as<bool>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse navigation status of an instance of model '%s', ignoring\n", __FUNCTION__, rhs.isModelFile.c_str());
          rhs.isNavigationEnabled = false;
        }
      }
      if (node["path-target-instance"]) {
        try {
          rhs.isPathTargetInstance = node["path-target-instance"].as<int>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse navigation target of an instance of model '%s', ignoring\n", __FUNCTION__, rhs.isModelFile.c_str());
          rhs.isPathTargetInstance = defaultSettings.isPathTargetInstance;
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
      if (rhs.csCamType == cameraType::stationaryFollowing) {
        node["follow-cam-height-offset"] = rhs.csFollowCamHeightOffset;
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
      if (rhs.csCamType == cameraType::stationaryFollowing) {
        if (node["follow-cam-height-offset"]) {
          try {
            rhs.csFollowCamHeightOffset = node["follow-cam-height-offset"].as<float>();
          } catch (...) {
            Logger::log(1, "%s warning: could not parse follow cam height offset of camera '%s', init with a default value\n", __FUNCTION__, rhs.csCamName.c_str());
            rhs.csFollowCamHeightOffset = defaultSettings.csFollowCamHeightOffset;
          }
        }
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
      try {
        rhs.iwrbIdleClipNr = node["idle-clip"].as<int>();
        rhs.iwrbIdleClipSpeed = node["idle-clip-speed"].as<float>();
        rhs.iwrbWalkClipNr = node["walk-clip"].as<int>();
        rhs.iwrbWalkClipSpeed = node["walk-clip-speed"].as<float>();
        rhs.iwrbRunClipNr = node["run-clip"].as<int>();
        rhs.iwrbRunClipSpeed = node["run-clip-speed"].as<float>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse idle/walk/run blendings, using defaults\n", __FUNCTION__);
        rhs = IdleWalkRunBlending{};
      }
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
      node["is-nav-target"] = rhs.msUseAsNavigationTarget;
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
      node["forward-speed-factor"] = rhs.msForwardSpeedFactor;
      node["bounding-sphere-adjustment"] = rhs.msBoundingSphereAdjustments;
      clips = node["head-movement-mappings"];
      for (const auto& state : rhs.msHeadMoveClipMappings) {
        clips[state.first] = state.second;
      }
      return node;
    }

    static bool decode(const Node& node, ModelSettings& rhs) {
      ModelSettings defaultSettings{};
      try {
        rhs.msModelFilenamePath = node["model-file"].as<std::string>();
        rhs.msModelFilename = node["model-name"].as<std::string>();
      } catch (...) {
        Logger::log(1, "%s error: could not parse model file or model name\n", __FUNCTION__);
        return false;
      }
      if (Node clipNode = node["idle-walk-run-clips"]) {
        try {
          for (size_t i = 0; i < clipNode.size(); ++i) {
            std::map<moveDirection, IdleWalkRunBlending> entry = clipNode[i].as<std::map<moveDirection, IdleWalkRunBlending>>();
            rhs.msIWRBlendings.insert(entry.begin(), entry.end());
          }
        } catch (...) {
          Logger::log(1, "%s warning: could not parse idle/walk/run blendings of model '%s', using empty defaults'\n", __FUNCTION__, rhs.msModelFilename.c_str());
          rhs.msIWRBlendings = defaultSettings.msIWRBlendings;
        }
      }
      if (Node clipNode = node["action-clips"]) {
        try {
          for (size_t i = 0; i < clipNode.size(); ++i) {
            std::map<moveState, ActionAnimation> entry = clipNode[i].as<std::map<moveState, ActionAnimation>>();
            rhs.msActionClipMappings.insert(entry.begin(), entry.end());
          }
        } catch(...) {
          Logger::log(1, "%s warning: could not parse action clip mappings of model '%s', using empty defaults'\n", __FUNCTION__, rhs.msModelFilename.c_str());
          rhs.msActionClipMappings = defaultSettings.msActionClipMappings;
        }
      }
      if (Node clipNode = node["action-sequences"]) {
        try {
          for (size_t i = 0; i < clipNode.size(); ++i) {
            /* std::set does not work, but std::map does... */
            std::map<moveState, moveState> entry = clipNode[i].as<std::map<moveState, moveState>>();
            std::pair<moveState, moveState> state = std::make_pair(entry.begin()->first, entry.begin()->second);
            rhs.msAllowedStateOrder.insert(state);
          }
        } catch (...) {
          Logger::log(1, "%s warning: could not parse allowed clip order of model '%s', using empty defaults'\n", __FUNCTION__, rhs.msModelFilename.c_str());
          rhs.msAllowedStateOrder = defaultSettings.msAllowedStateOrder;
        }
      }
      if (Node clipNode = node["bounding-sphere-adjustments"]) {
        try {
          for (size_t i = 0; i < clipNode.size(); ++i) {
            glm::vec4 entry = clipNode[i].as<glm::vec4>();
            rhs.msBoundingSphereAdjustments.emplace_back(entry);
          }
        } catch (...) {
          Logger::log(1, "%s warning: could not parse bounding sphere adjustment of model '%s', using empty defaults'\n", __FUNCTION__, rhs.msModelFilename.c_str());
          rhs.msBoundingSphereAdjustments = defaultSettings.msBoundingSphereAdjustments;
        }
      }
      if (Node clipNode = node["forward-speed-factor"]) {
        try {
          rhs.msForwardSpeedFactor = node["forward-speed-factor"].as<float>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse forward speed factor of model '%s', using empty defaults'\n", __FUNCTION__, rhs.msModelFilename.c_str());
          rhs.msForwardSpeedFactor = defaultSettings.msForwardSpeedFactor;
        }
      }
      if (Node clipNode = node["head-movement-mappings"]) {
        try {
          for (size_t i = 0; i < clipNode.size(); ++i) {
            std::map<headMoveDirection, int> entry = clipNode[i].as<std::map<headMoveDirection, int>>();
            rhs.msHeadMoveClipMappings.insert(entry.begin(), entry.end());
          }
        } catch (...) {
          Logger::log(1, "%s warning: could not parse head move clip mappings of model '%s', using empty defaults'\n", __FUNCTION__, rhs.msModelFilename.c_str());
          rhs.msHeadMoveClipMappings = defaultSettings.msHeadMoveClipMappings;
        }
      }
      if (Node clipNode = node["left-foot-ik-chain"]) {
        try {
          for (size_t i = 0; i < clipNode.size(); ++i) {
            int entry = clipNode[i].as<int>();
            rhs.msFootIKChainNodes.at(0).emplace_back(entry);
          }
          rhs.msFootIKChainPair.at(0).first = rhs.msFootIKChainNodes.at(0).at(0);
          rhs.msFootIKChainPair.at(0).second = rhs.msFootIKChainNodes.at(0).at(rhs.msFootIKChainNodes.at(0).size() - 1);
        } catch (...) {
          Logger::log(1, "%s warning: could not parse left foot ik chain of model '%s', ignoring\n", __FUNCTION__, rhs.msModelFilename.c_str());
          rhs.msFootIKChainNodes.at(0).clear();
        }
      }
      if (Node clipNode = node["right-foot-ik-chain"]) {
        try {
          for (size_t i = 0; i < clipNode.size(); ++i) {
            int entry = clipNode[i].as<int>();
            rhs.msFootIKChainNodes.at(1).emplace_back(entry);
          }
          rhs.msFootIKChainPair.at(1).first = rhs.msFootIKChainNodes.at(1).at(0);
          rhs.msFootIKChainPair.at(1).second = rhs.msFootIKChainNodes.at(1).at(rhs.msFootIKChainNodes.at(1).size() - 1);
        } catch (...) {
          Logger::log(1, "%s warning: could not parse right foot ik chain of model '%s', ignoring\n", __FUNCTION__, rhs.msModelFilename.c_str());
          rhs.msFootIKChainNodes.at(1).clear();
        }
      }
      if (node["is-nav-target"]) {
        try {
          rhs.msUseAsNavigationTarget = node["is-nav-target"].as<bool>();
        } catch (...) {
          Logger::log(1, "%s warning: could not parse nav target status of model '%s', disabling\n", __FUNCTION__, rhs.msModelFilename.c_str());
          rhs.msUseAsNavigationTarget = false;
        }
      }
      return true;
    }
  };

  /* read and write Behavior data */
  template<>
  struct convert<ExtendedBehaviorData> {
    static Node encode(const ExtendedBehaviorData& rhs) {
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

    static bool decode(const Node& node, ExtendedBehaviorData& rhs) {
      rhs.bdName = node["node-tree-name"].as<std::string>();
      rhs.bdEditorSettings = node["editor-settings"].as<std::string>();
      if (Node nodesNode = node["nodes"]) {
        for (size_t i = 0; i < nodesNode.size(); ++i) {
          PerNodeImportData nodeData;
          nodeData.nodeType = nodesNode[i]["node-type"].as<graphNodeType>();
          nodeData.nodeId = nodesNode[i]["node-id"].as<int>();

          if (Node nodeDataNode = nodesNode[i]["node-data"]) {
            std::vector<std::map<std::string, std::string>> entry = nodeDataNode.as<std::vector<std::map<std::string, std::string>>>();
            if (!entry.empty()) {
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


  /* read and write LevelSettings */
  template<>
  struct convert<LevelSettings> {
    static Node encode(const LevelSettings& rhs) {
      Node node;
      node["level-file"] = rhs.lsLevelFilenamePath;
      node["level-name"] = rhs.lsLevelFilename;
      node["position"] = rhs.lsWorldPosition;
      node["rotation"] = rhs.lsWorldRotation;
      node["scale"] = rhs.lsWorldRotation;
      node["swap-axes"] = rhs.lsSwapYZAxis;
      return node;
    }

    static bool decode(const Node& node, LevelSettings& rhs) {
      LevelSettings defaultSettings{};
      rhs.lsLevelFilenamePath = node["level-file"].as<std::string>();
      rhs.lsLevelFilename = node["level-name"].as<std::string>();
      try {
        rhs.lsWorldPosition = node["position"].as<glm::vec3>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse position of level '%s', init with a default value\n", __FUNCTION__, rhs.lsLevelFilename.c_str());
        rhs.lsWorldPosition = defaultSettings.lsWorldPosition;
      }
      try {
        rhs.lsWorldRotation = node["rotation"].as<glm::vec3>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse rotation of level '%s', init with a default value\n", __FUNCTION__, rhs.lsLevelFilename.c_str());
        rhs.lsWorldRotation = defaultSettings.lsWorldRotation;
      }
      try {
        rhs.lsScale = node["scale"].as<float>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parses caling of level '%s', init with a default value\n", __FUNCTION__, rhs.lsLevelFilename.c_str());
        rhs.lsScale = defaultSettings.lsScale;
      }
      try {
        rhs.lsSwapYZAxis = node["swap-axes"].as<bool>();
      } catch (...) {
        Logger::log(1, "%s warning: could not parse Y-Z axis swappin of level '%s', init with a default value\n", __FUNCTION__, rhs.lsLevelFilename.c_str());
        rhs.lsSwapYZAxis = defaultSettings.lsSwapYZAxis;
      }
      return true;
    }
  };
}
