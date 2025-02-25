#include <fstream>
#include <tuple>

#include "YamlParser.h"
#include "AssimpModel.h"
#include "AssimpInstance.h"
#include "Camera.h"
#include "GraphNodeBase.h"
#include "BehaviorManager.h"
#include "AssimpLevel.h"
#include "Logger.h"

/* overloads */
YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& vec) {
  out << YAML::Flow;
  out << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& vec) {
  out << YAML::Flow;
  out << YAML::BeginSeq << vec.x << vec.y << vec.z << vec.w << YAML::EndSeq;
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const moveState& state) {
  out << YAML::Flow;
  out << static_cast<int>(state);
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const moveDirection& dir) {
  out << YAML::Flow;
  out << static_cast<int>(dir);
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const std::pair<int, int> data) {
  out << YAML::Flow;
  out << YAML::BeginSeq << data.first << data.second << YAML::EndSeq;
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const collisionChecks& checkCollisions) {
  out << YAML::Flow;
  out << static_cast<int>(checkCollisions);
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const graphNodeType& nodeType) {
  out << YAML::Flow;
  out << static_cast<int>(nodeType);
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const faceAnimation& faceAnim) {
  out << YAML::Flow;
  out << static_cast<int>(faceAnim);
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const headMoveDirection& moveDir) {
  out << YAML::Flow;
  out << static_cast<int>(moveDir);
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const timeOfDay& tofd) {
  out << YAML::Flow;
  out << static_cast<int>(tofd);
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const InstanceSettings& settings) {
  out << YAML::Key << "model-file";
  out << YAML::Value << settings.isModelFile;
  out << YAML::Key << "position";
  out << YAML::Value << settings.isWorldPosition;
  out << YAML::Key << "rotation";
  out << YAML::Value << settings.isWorldRotation;
  out << YAML::Key << "scale";
  out << YAML::Value << settings.isScale;
  out << YAML::Key << "swap-axes";
  out << YAML::Value << settings.isSwapYZAxis;
  out << YAML::Key << "1st-anim-clip-number";
  out << YAML::Value << settings.isFirstAnimClipNr;
  out << YAML::Key << "2nd-anim-clip-number";
  out << YAML::Value << settings.isSecondAnimClipNr;
  out << YAML::Key << "anim-clip-speed";
  out << YAML::Value << settings.isAnimSpeedFactor;
  out << YAML::Key << "anim-blend-factor";
  out << YAML::Value << settings.isAnimBlendFactor;
  if (!settings.isNodeTreeName.empty()) {
    out << YAML::Key << "node-tree";
    out << YAML::Value << settings.isNodeTreeName;
  }
  if (settings.isFaceAnimType != faceAnimation::none) {
    out << YAML::Key << "face-anim-index";
    out << YAML::Value << settings.isFaceAnimType;
    out << YAML::Key << "face-anim-weight";
    out << YAML::Value << settings.isFaceAnimWeight;
  }
  if (settings.isHeadLeftRightMove != 0.0f) {
    out << YAML::Key << "head-anim-left-right-timestamp";
    out << YAML::Value << settings.isHeadLeftRightMove;
  }
  if (settings.isHeadUpDownMove != 0.0f) {
    out << YAML::Key << "head-anim-up-down-timestamp";
    out << YAML::Value << settings.isHeadUpDownMove;
  }
  out << YAML::Key << "enable-navigation";
  out << YAML::Value << settings.isNavigationEnabled;
  out << YAML::Key << "path-target-instance";
  out << YAML::Value << settings.isPathTargetInstance;
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const CameraSettings& settings) {
  out << YAML::Key << "camera-name";
  out << YAML::Value << settings.csCamName;
  out << YAML::Key << "position";
  out << YAML::Value << settings.csWorldPosition;
  out << YAML::Key << "view-azimuth";
  out << YAML::Value << settings.csViewAzimuth;
  out << YAML::Key << "view-elevation";
  out << YAML::Value << settings.csViewElevation;
  out << YAML::Key << "camera-type";
  out << YAML::Value << static_cast<int>(settings.csCamType);
  out << YAML::Key << "camera-projection";
  out << YAML::Value << static_cast<int>(settings.csCamProjection);
  if (settings.csCamProjection == cameraProjection::perspective) {
    out << YAML::Key << "field-of-view";
    out << YAML::Value << settings.csFieldOfView;
  }
  if (settings.csCamProjection == cameraProjection::orthogonal) {
    out << YAML::Key << "ortho-scale";
    out << YAML::Value << settings.csOrthoScale;
  }
  if (settings.csCamType == cameraType::firstPerson) {
    out << YAML::Key << "1st-person-view-lock";
    out << YAML::Value << settings.csFirstPersonLockView;
    out << YAML::Key << "1st-person-bone-to-follow";
    out << YAML::Value << settings.csFirstPersonBoneToFollow;
    out << YAML::Key << "1st-person-view-offsets";
    out << YAML::Value << settings.csFirstPersonOffsets;
  }
  if (settings.csCamType == cameraType::thirdPerson) {
    out << YAML::Key << "3rd-person-view-distance";
    out << YAML::Value << settings.csThirdPersonDistance ;
    out << YAML::Key << "3rd-person-height-offset";
    out << YAML::Value << settings.csThirdPersonHeightOffset;
  }
  if (settings.csCamType == cameraType::stationaryFollowing) {
    out << YAML::Key << "follow-cam-height-offset";
    out << YAML::Value << settings.csFollowCamHeightOffset ;
  }
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const IdleWalkRunBlending& settings) {
  out << YAML::BeginMap;
  out << YAML::Key << "idle-clip";
  out << YAML::Value << settings.iwrbIdleClipNr;
  out << YAML::Key << "idle-clip-speed";
  out << YAML::Value << settings.iwrbIdleClipSpeed;
  out << YAML::Key << "walk-clip";
  out << YAML::Value << settings.iwrbWalkClipNr;
  out << YAML::Key << "walk-clip-speed";
  out << YAML::Value << settings.iwrbWalkClipSpeed;
  out << YAML::Key << "run-clip";
  out << YAML::Value << settings.iwrbRunClipNr;
  out << YAML::Key << "run-clip-speed";
  out << YAML::Value << settings.iwrbRunClipSpeed;
  out << YAML::EndMap;
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const ActionAnimation& settings) {
  out << YAML::BeginMap;
  out << YAML::Key << "clip";
  out << YAML::Value << settings.aaClipNr;
  out << YAML::Key << "clip-speed";
  out << YAML::Value << settings.aaClipSpeed;
  out << YAML::EndMap;
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const ModelSettings& settings) {
  out << YAML::Key << "model-name";
  out << YAML::Value << settings.msModelFilename;
  out << YAML::Key << "model-file";
  out << YAML::Value << settings.msModelFilenamePath;
  out << YAML::Key << "is-nav-target";
  out << YAML::Value << settings.msUseAsNavigationTarget;
  if (!settings.msIWRBlendings.empty()) {
    out << YAML::Key << "idle-walk-run-clips";
    out << YAML::Value;
    out << YAML::BeginSeq;;
    for (auto& setting : settings.msIWRBlendings) {
      out << YAML::BeginMap;
      out << YAML::Key << setting.first;
      out << YAML::Value << setting.second;
      out << YAML::EndMap;
    }
    out << YAML::EndSeq;
  }
  if (!settings.msActionClipMappings.empty()) {
    out << YAML::Key << "action-clips";
    out << YAML::Value;
    out << YAML::BeginSeq;;
    for (auto& setting : settings.msActionClipMappings) {
      out << YAML::BeginMap;
      out << YAML::Key << setting.first;
      out << YAML::Value << setting.second;
      out << YAML::EndMap;
    }
    out << YAML::EndSeq;
  }
  if (!settings.msAllowedStateOrder.empty()) {
    out << YAML::Key << "action-sequences";
    out << YAML::Value;
    out << YAML::BeginSeq;;
    for (auto& setting : settings.msAllowedStateOrder) {
      out << YAML::BeginMap;
      out << YAML::Key << setting.first;
      out << YAML::Value << setting.second;
      out << YAML::EndMap;
    }
    out << YAML::EndSeq;
  }
  if (!settings.msBoundingSphereAdjustments.empty()) {
    out << YAML::Key << "bounding-sphere-adjustments";
    out << YAML::Value;
    out << YAML::BeginSeq;
    for (auto& setting : settings.msBoundingSphereAdjustments) {
      out << YAML::Key << setting;
    }
    out << YAML::EndSeq;
  }
  out << YAML::Key << "forward-speed-factor";
  out << YAML::Value << settings.msForwardSpeedFactor;
  if (!settings.msHeadMoveClipMappings.empty() &&
    settings.msHeadMoveClipMappings.at(headMoveDirection::left) >= 0 &&
    settings.msHeadMoveClipMappings.at(headMoveDirection::right) >= 0 &&
    settings.msHeadMoveClipMappings.at(headMoveDirection::up) >= 0 &&
    settings.msHeadMoveClipMappings.at(headMoveDirection::down) >= 0) {
    out << YAML::Key << "head-movement-mappings";
    out << YAML::Value;
    out << YAML::BeginSeq;
    for (auto& setting : settings.msHeadMoveClipMappings) {
      out << YAML::BeginMap;
      out << YAML::Key << setting.first;
      out << YAML::Value << setting.second;
      out << YAML::EndMap;
    }
    out << YAML::EndSeq;
  }
  if (!settings.msFootIKChainNodes.at(0).empty() &&
      !settings.msFootIKChainNodes.at(1).empty()) {
    out << YAML::Key << "left-foot-ik-chain";
    out << YAML::Value;
    out << YAML::BeginSeq;
    for (auto& setting : settings.msFootIKChainNodes.at(0)) {
      out << YAML::Value << setting;
    }
    out << YAML::EndSeq;

    out << YAML::Key << "right-foot-ik-chain";
    out << YAML::Value;
    out << YAML::BeginSeq;
    for (auto& setting : settings.msFootIKChainNodes.at(1)) {
      out << YAML::Value << setting;
    }
    out << YAML::EndSeq;

  }
  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const BehaviorData& behavior) {
  out << YAML::Key << "node-tree-name";
  out << YAML::Value << behavior.bdName;
  out << YAML::Key << "editor-settings";
  out << YAML::Value << behavior.bdEditorSettings;

  /* should be always > 0 due to the root node */
  if (!behavior.bdGraphNodes.empty()) {
    out << YAML::Key << "nodes";
    out << YAML::Value;
    out << YAML::BeginSeq;;
    for (auto& node : behavior.bdGraphNodes) {
      out << YAML::BeginMap;
      out << YAML::Key << "node-type";
      out << YAML::Value << node->getNodeType();
      out << YAML::Key << "node-id";
      out << YAML::Value << node->getNodeId();
      if (node->exportData().has_value()) {
        std::map<std::string, std::string> exportData = node->exportData().value();
        out << YAML::Key << "node-data";
        out << YAML::Value;
        out << YAML::BeginSeq;
        for (auto& value : exportData) {
          out << YAML::BeginMap;
          out << YAML::Key << value.first;
          out << YAML::Value << value.second;
          out << YAML::EndMap;
        }
        out << YAML::EndSeq;
      }
      out << YAML::EndMap;
    }
    out << YAML::EndSeq;
  }

  if (!behavior.bdGraphLinks.empty()) {
    out << YAML::Key << "links";
    out << YAML::Value;
    out << YAML::BeginSeq;;
    for (auto& link : behavior.bdGraphLinks) {
      out << YAML::BeginMap;
      out << YAML::Key << link.first;
      out << YAML::Value << link.second;
      out << YAML::EndMap;
    }
    out << YAML::EndSeq;
  }

  return out;
}

YAML::Emitter& operator<<(YAML::Emitter& out, const LevelSettings& settings) {
  out << YAML::Key << "level-name";
  out << YAML::Value << settings.lsLevelFilename;
  out << YAML::Key << "level-file";
  out << YAML::Value << settings.lsLevelFilenamePath;
  out << YAML::Key << "position";
  out << YAML::Value << settings.lsWorldPosition;
  out << YAML::Key << "rotation";
  out << YAML::Value << settings.lsWorldRotation;
  out << YAML::Key << "scale";
  out << YAML::Value << settings.lsScale;
  out << YAML::Key << "swap-axes";
  out << YAML::Value << settings.lsSwapYZAxis;

  return out;
}

bool YamlParser::loadYamlFile(const std::string fileName) {
  try {
    mYamlNode = YAML::LoadFile(fileName);
  } catch(...) {
    Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__, fileName.c_str());
    return false;
  }

  Logger::log(2, "%s: successfully loaded and parsed file '%s'\n", __FUNCTION__, fileName.c_str());

  mYamlFileName = fileName;
  return true;
}

std::string YamlParser::getFileName() {
  return mYamlFileName;
}

std::string YamlParser::getFileVersion() {
  const std::string versionString = "version";
  if (!hasKey(versionString)) {
    Logger::log(1, "%s error: cold not find version string in YAML config file '%s'\n", __FUNCTION__, getFileName().c_str());
    return std::string();
  }

  if (!getValue(versionString, mYamlFileVersion)) {
    Logger::log(1, "%s error: could not get version number from YAML config file '%s'\n", __FUNCTION__, getFileName().c_str());
    return std::string();
  }

  Logger::log(1, "%s: found config version %s\n", __FUNCTION__, mYamlFileVersion.c_str());
  return mYamlFileVersion;
}

std::vector<ModelSettings> YamlParser::getModelConfigs() {
  std::vector<ModelSettings> modSettings;

  if (!hasKey("models")) {
    Logger::log(1, "%s error: no model file names found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return modSettings;
  }

  YAML::Node modelsNode = mYamlNode["models"];
  ModelSettings settings{};
  for (size_t i = 0; i < modelsNode.size(); ++i) {
    try {
      settings = modelsNode[i].as<ModelSettings>();
    } catch (const YAML::Exception& e) {
      Logger::log(1, "%s error: could not parse file '%s' (%s)\n", __FUNCTION__, mYamlFileName.c_str(), e.what());
      return std::vector<ModelSettings>{};
    }
    Logger::log(1, "%s: found model name: %s\n", __FUNCTION__, settings.msModelFilename.c_str());
    modSettings.emplace_back(settings);
  }

  return modSettings;
}

int YamlParser::getSelectedModelNum() {
  int selectedModelNum = 0;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return 0;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "selected-model") {
        selectedModelNum = it->second.as<int>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return 0;
  }

  return selectedModelNum;
}

std::vector<ExtendedInstanceSettings> YamlParser::getInstanceConfigs() {
  std::vector<ExtendedInstanceSettings> instSettings;
  if (!hasKey("instances")) {
    Logger::log(1, "%s error: no instances found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return instSettings;
  }

  YAML::Node instanceNode = mYamlNode["instances"];
  ExtendedInstanceSettings settings{};
  for (size_t i = 0; i < instanceNode.size(); ++i) {
    try {
      settings = instanceNode[i].as<ExtendedInstanceSettings>();
    } catch (...) {
      Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
      continue;
    }
    instSettings.emplace_back(settings);
  }

  return instSettings;
}

std::vector<CameraSettings> YamlParser::getCameraConfigs() {
  std::vector<CameraSettings> camSettings;
  if (mYamlFileVersion == "1.0") {
    Logger::log(1, "%s: found version 1.0 camera settings, migrating\n", __FUNCTION__);
    /* get old settings */
    glm::vec3 camPos = getCameraPosition();
    float camAzimuth = getCameraAzimuth();
    float camElevation = getCameraElevation();

    /* and re-create the default FreeCam */
    CameraSettings settings{};
    settings.csCamName = "FreeCam";
    settings.csWorldPosition = camPos;
    settings.csViewAzimuth = camAzimuth;
    settings.csViewElevation = camElevation;

    camSettings.emplace_back(settings);
  } else {
    if (!hasKey("cameras")) {
      Logger::log(1, "%s warning: no cameras found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
      return camSettings;
    }

    YAML::Node camNode = mYamlNode["cameras"];
    CameraSettings settings{};
    for (size_t i = 0; i < camNode.size(); ++i) {
      try {
        settings = camNode[i].as<CameraSettings>();
      } catch (...) {
        Logger::log(1, "%s error: could not parse file '%s', skipping camera entry %i\n", __FUNCTION__, mYamlFileName.c_str(), i);
        continue;
      }
      camSettings.emplace_back(settings);
    }
  }
  return camSettings;
}

std::vector<ExtendedBehaviorData> YamlParser::getBehaviorData() {
  std::vector<ExtendedBehaviorData> behaviorData;
  if (!hasKey("node-trees")) {
    Logger::log(1, "%s warning: no node-trees found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return behaviorData;
  }

  YAML::Node behaviorNode = mYamlNode["node-trees"];
  ExtendedBehaviorData data;
  for (size_t i = 0; i < behaviorNode.size(); ++i) {
    try {
      data = behaviorNode[i].as<ExtendedBehaviorData>();
    } catch (...) {
      Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
      continue;
    }
    behaviorData.emplace_back(data);
  }

  return behaviorData;
}

std::vector<LevelSettings> YamlParser::getLevelConfigs() {
  std::vector<LevelSettings> levelSettings;
  if (!hasKey("levels")) {
    Logger::log(1, "%s warning: no levels found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return levelSettings;
  }

  YAML::Node levelNode = mYamlNode["levels"];
  LevelSettings settings;
  for (size_t i = 0; i < levelNode.size(); ++i) {
    try {
      settings = levelNode[i].as<LevelSettings>();
    } catch (...) {
      Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
      continue;
    }
    levelSettings.emplace_back(settings);
  }

  return levelSettings;
}


int YamlParser::getSelectedInstanceNum() {
  int selectedInstanceNum = 0;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return 0;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "selected-instance") {
        selectedInstanceNum = it->second.as<int>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return 0;
  }

  return selectedInstanceNum;
}

int YamlParser::getSelectedCameraNum() {
  int selectedCameraNum = 0;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return 0;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "selected-camera") {
        selectedCameraNum = it->second.as<int>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return 0;
  }

  return selectedCameraNum;
}

int YamlParser::getSelectedLevelNum() {
  int selectedLevelNum = 0;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return 0;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "selected-level") {
        selectedLevelNum = it->second.as<int>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return 0;
  }

  return selectedLevelNum;
}

bool YamlParser::getHighlightActivated() {
  bool highlightActive = false;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return highlightActive;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "highlight-selection") {
        highlightActive = it->second.as<bool>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return highlightActive;
  }

  return highlightActive;
}

glm::vec3 YamlParser::getCameraPosition() {
  glm::vec3 cameraPos = glm::vec3(5.0f);
  if (!hasKey("camera")) {
    Logger::log(1, "%s error: no camera settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return cameraPos;
  }

  YAML::Node cameraNode = mYamlNode["camera"];
  try {
    for(auto it = cameraNode.begin(); it != cameraNode.end(); ++it) {
      if (it->first.as<std::string>() == "camera-position") {
        cameraPos = it->second.as<glm::vec3>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return cameraPos;
  }

  return cameraPos;
}

float YamlParser::getCameraElevation() {
  float cameraElevation = -15.0f;
  if (!hasKey("camera")) {
    Logger::log(1, "%s error: no camera settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return cameraElevation;
  }

  YAML::Node cameraNode = mYamlNode["camera"];
  try {
    for(auto it = cameraNode.begin(); it != cameraNode.end(); ++it) {
      if (it->first.as<std::string>() == "camera-elevation") {
        cameraElevation = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return cameraElevation;
  }

  return cameraElevation;
}

float YamlParser::getCameraAzimuth() {
  float cameraAzimuthh = 310.0f;
  if (!hasKey("camera")) {
    Logger::log(1, "%s error: no camera settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return cameraAzimuthh;
  }

  YAML::Node cameraNode = mYamlNode["camera"];
  try {
    for(auto it = cameraNode.begin(); it != cameraNode.end(); ++it) {
      if (it->first.as<std::string>() == "camera-azimuth") {
        cameraAzimuthh = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return cameraAzimuthh;
  }

  return cameraAzimuthh;
}

collisionChecks YamlParser::getCollisionChecksEnabled() {
  collisionChecks collisionValue = collisionChecks::none;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return collisionValue;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "collision-enabled") {
        collisionValue = it->second.as<collisionChecks>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return collisionValue;
  }

  return collisionValue;
}

bool YamlParser::getInteractionEnabled() {
  bool interactionEnabled = false;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return interactionEnabled;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "interaction-enabled") {
        interactionEnabled = it->second.as<bool>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return interactionEnabled;
  }

  return interactionEnabled;
}

float YamlParser::getInteractionMinRange() {
  float interactionMinRange = 1.5f;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return interactionMinRange;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "interaction-min-range") {
        interactionMinRange = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return interactionMinRange;
  }

  return interactionMinRange;
}

float YamlParser::getInteractionMaxRange() {
  float interactionMinRange = 10.0f;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return interactionMinRange;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "interaction-max-range") {
        interactionMinRange = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return interactionMinRange;
  }

  return interactionMinRange;
}

float YamlParser::getInteractionFOV() {
  float interactionFov = 45.0f;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return interactionFov;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "interaction-fov") {
        interactionFov = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return interactionFov;
  }

  return interactionFov;
}

bool YamlParser::getGravityEnabled() {
  bool gravityEnabled = false;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return gravityEnabled;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "gravity-enabled") {
        gravityEnabled = it->second.as<bool>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return gravityEnabled;
  }

  return gravityEnabled;
}

float YamlParser::getMaxGroundSlopeAngle() {
  float maxSlope = 90.0f;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return maxSlope;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "level-collision-ground-slope") {
        maxSlope = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return maxSlope;
  }

  return maxSlope;
}

float YamlParser::getMaxStairStepHeight() {
  float maxStepHeight = 2.0f;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return maxStepHeight;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "level-collision-max-stairstep-height") {
        maxStepHeight = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return maxStepHeight;
  }

  return maxStepHeight;
}

bool YamlParser::getIKEnabled() {
  bool ikEnabled = false;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return ikEnabled;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "inverse-kinematics-enabled") {
        ikEnabled = it->second.as<bool>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return ikEnabled;
  }

  return ikEnabled;
}

int YamlParser::getIKNumIterations() {
  int iterations = 10;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return iterations;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "inverse-kinematics-iterations") {
        iterations = it->second.as<int>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return iterations;
  }

  return iterations;
}

bool YamlParser::getNavEnabled() {
  bool navEnabled = false;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return navEnabled;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "navigation-enabled") {
        navEnabled = it->second.as<bool>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return navEnabled;
  }

  return navEnabled;
}


bool YamlParser::getSkyboxEnabled() {
  bool skyboxEnabled = false;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return skyboxEnabled;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "skybox-enabled") {
        skyboxEnabled = it->second.as<bool>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return skyboxEnabled;
  }

  return skyboxEnabled;
}

float YamlParser::getFogDensity() {
  float density = 0.0f;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return density;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "fog-density") {
        density = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return density;
  }

  return density;
}

float YamlParser::getLightSourceAngleEastWest() {
  float lightAngle = 40.0f;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return lightAngle;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "light-source-angle-east-west") {
        lightAngle = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return lightAngle;
  }

  return lightAngle;
}

float YamlParser::getLightSourceAngleNorthSouth() {
  float lightAngle = 40.0f;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return lightAngle;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "light-source-angle-north-south") {
        lightAngle = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return lightAngle;
  }

  return lightAngle;
}

float YamlParser::getLightSouceIntensity() {
  float intensity = 1.0f;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return intensity;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "light-source-intensity") {
        intensity = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return intensity;
  }

  return intensity;
}

glm::vec3 YamlParser::getLightSourceColor() {
  glm::vec3 lightCol = glm::vec3(1.0f);
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return lightCol;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "light-source-color") {
        lightCol = it->second.as<glm::vec3>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return lightCol;
  }

  return lightCol;
}

bool YamlParser::getTimeOfDayEnabled() {
  bool tofdEnabled = false;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return tofdEnabled;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "time-of-day-enabled") {
        tofdEnabled = it->second.as<bool>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return tofdEnabled;
  }

  return tofdEnabled;
}

float YamlParser::getTimeOfDayScaleFactor() {
  float scale = 10.0f;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return scale;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "time-of-day-scaling") {
        scale = it->second.as<float>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return scale;
  }

  return scale;
}

timeOfDay YamlParser::getTimeOfDayPreset() {
  timeOfDay preset = timeOfDay::fullLight;
  if (!hasKey("settings")) {
    Logger::log(1, "%s error: no settings found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return preset;
  }

  YAML::Node settingsNode = mYamlNode["settings"];
  try {
    for(auto it = settingsNode.begin(); it != settingsNode.end(); ++it) {
      if (it->first.as<std::string>() == "time-of-day-preset") {
        preset = it->second.as<timeOfDay>();
      }
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return preset;
  }

  return preset;
}

void YamlParser::createInstanceToCamMap(ModelInstanceCamData modInstCamData) {
  mInstanceToCamMap.clear();
  for (const auto& camera : modInstCamData.micCameras) {
    CameraSettings camSettings = camera->getCameraSettings();
    if (std::shared_ptr<AssimpInstance> instance = camera->getInstanceToFollow()) {
      mInstanceToCamMap[instance->getInstanceIndexPosition()].emplace_back(camSettings.csCamName);
    }
  }
}

bool YamlParser::createConfigFile(VkRenderData renderData, ModelInstanceCamData modInstCamData) {
  createInstanceToCamMap(modInstCamData);

  mYamlEmit << YAML::Comment("Application viewer config file");
  mYamlEmit << YAML::BeginMap;
  mYamlEmit << YAML::Key << "version";
  mYamlEmit << YAML::Value << mYamlConfigFileVersion;
  mYamlEmit << YAML::EndMap;

  mYamlEmit << YAML::Newline;

  mYamlEmit << YAML::BeginMap;
  mYamlEmit << YAML::Key << "settings";
  mYamlEmit << YAML::Value;
  mYamlEmit << YAML::BeginMap;
  mYamlEmit << YAML::Key << "selected-model";
  mYamlEmit << YAML::Value << modInstCamData.micSelectedModel;
  mYamlEmit << YAML::Key << "selected-instance";
  mYamlEmit << YAML::Value << modInstCamData.micSelectedInstance;
  mYamlEmit << YAML::Key << "selected-camera";
  mYamlEmit << YAML::Value << modInstCamData.micSelectedCamera;
  mYamlEmit << YAML::Key << "selected-level";
  mYamlEmit << YAML::Value << modInstCamData.micSelectedLevel;
  mYamlEmit << YAML::Key << "highlight-selection";
  mYamlEmit << YAML::Value << renderData.rdHighlightSelectedInstance;
  mYamlEmit << YAML::Key << "collision-enabled";
  mYamlEmit << YAML::Value << renderData.rdCheckCollisions;
  mYamlEmit << YAML::Key << "interaction-enabled";
  mYamlEmit << YAML::Value << renderData.rdInteraction;
  mYamlEmit << YAML::Key << "interaction-min-range";
  mYamlEmit << YAML::Value << renderData.rdInteractionMinRange;
  mYamlEmit << YAML::Key << "interaction-max-range";
  mYamlEmit << YAML::Value << renderData.rdInteractionMaxRange;
  mYamlEmit << YAML::Key << "interaction-fov";
  mYamlEmit << YAML::Value << renderData.rdInteractionFOV;
  mYamlEmit << YAML::Key << "gravity-enabled";
  mYamlEmit << YAML::Value << renderData.rdEnableSimpleGravity;
  mYamlEmit << YAML::Key << "level-collision-ground-slope";
  mYamlEmit << YAML::Value << renderData.rdMaxLevelGroundSlopeAngle;
  mYamlEmit << YAML::Key << "level-collision-max-stairstep-height";
  mYamlEmit << YAML::Value << renderData.rdMaxStairstepHeight;
  mYamlEmit << YAML::Key << "inverse-kinematics-enabled";
  mYamlEmit << YAML::Value << renderData.rdEnableFeetIK;
  mYamlEmit << YAML::Key << "inverse-kinematics-iterations";
  mYamlEmit << YAML::Value << renderData.rdNumberOfIkIteratons;
  mYamlEmit << YAML::Key << "navigation-enabled";
  mYamlEmit << YAML::Value << renderData.rdEnableNavigation;
  mYamlEmit << YAML::Key << "skybox-enabled";
  mYamlEmit << YAML::Value << renderData.rdDrawSkybox;
  mYamlEmit << YAML::Key << "fog-density";
  mYamlEmit << YAML::Value << renderData.rdFogDensity;
  mYamlEmit << YAML::Key << "light-source-angle-east-west";
  mYamlEmit << YAML::Value << renderData.rdLightSourceAngleEastWest;
  mYamlEmit << YAML::Key << "light-source-angle-north-south";
  mYamlEmit << YAML::Value << renderData.rdLightSourceAngleNorthSouth;
  mYamlEmit << YAML::Key << "light-source-intensity";
  mYamlEmit << YAML::Value << renderData.rdLightSourceIntensity;
  mYamlEmit << YAML::Key << "light-source-color";
  mYamlEmit << YAML::Value << renderData.rdLightSourceColor;
  mYamlEmit << YAML::Key << "time-of-day-enabled";
  mYamlEmit << YAML::Value << renderData.rdEnableTimeOfDay;
  mYamlEmit << YAML::Key << "time-of-day-scaling";
  mYamlEmit << YAML::Value << renderData.rdTimeScaleFactor;
  mYamlEmit << YAML::Key << "time-of-day-preset";
  mYamlEmit << YAML::Value << renderData.rdTimeOfDayPreset;
  mYamlEmit << YAML::EndMap;
  mYamlEmit << YAML::EndMap;

  mYamlEmit << YAML::Newline;

  if (modInstCamData.micLevels.size() > 1) {
    mYamlEmit << YAML::BeginMap;
    mYamlEmit << YAML::Key << "levels";
    mYamlEmit << YAML::Value;
    mYamlEmit << YAML::BeginSeq;

    for (const auto& level : modInstCamData.micLevels) {
      /* skip null level */
      if (level->getTriangleCount() == 0) {
        continue;
      }
      mYamlEmit << YAML::BeginMap;
      mYamlEmit << level->getLevelSettings();
      mYamlEmit << YAML::EndMap;
    }
    mYamlEmit << YAML::EndSeq;
    mYamlEmit << YAML::EndMap;

    mYamlEmit << YAML::Newline;
  }

  mYamlEmit << YAML::BeginMap;
  mYamlEmit << YAML::Key << "cameras";
  mYamlEmit << YAML::Value;
  mYamlEmit << YAML::BeginSeq;

  for (const auto& cam : modInstCamData.micCameras) {
    mYamlEmit << YAML::BeginMap;
    mYamlEmit << cam->getCameraSettings();
    mYamlEmit << YAML::EndMap;
  }
  mYamlEmit << YAML::EndSeq;
  mYamlEmit << YAML::EndMap;

  mYamlEmit << YAML::Newline;

  /* models */
  mYamlEmit << YAML::BeginMap;
  mYamlEmit << YAML::Key << "models";
  mYamlEmit << YAML::Value;
  mYamlEmit << YAML::BeginSeq;

  for (const auto& model : modInstCamData.micModelList) {
    /* skip emtpy models (null model) */
    if (model->getTriangleCount() == 0) {
      continue;
    }
    mYamlEmit << YAML::BeginMap;
    mYamlEmit << model->getModelSettings();
    mYamlEmit << YAML::EndMap;
  }
  mYamlEmit << YAML::EndSeq;
  mYamlEmit << YAML::EndMap;

  mYamlEmit << YAML::Newline;

  mYamlEmit << YAML::BeginMap;
  mYamlEmit << YAML::Key << "node-trees";
  mYamlEmit << YAML::Value;
  mYamlEmit << YAML::BeginSeq;

  for (const auto& behavior : modInstCamData.micBehaviorData) {
    mYamlEmit << YAML::BeginMap;
    mYamlEmit << *behavior.second->getBehaviorData();
    mYamlEmit << YAML::EndMap;
  }

  mYamlEmit << YAML::EndSeq;
  mYamlEmit << YAML::EndMap;

  mYamlEmit << YAML::Newline;

  /* instances */
  mYamlEmit << YAML::BeginMap;
  mYamlEmit << YAML::Key << "instances";
  mYamlEmit << YAML::Value;
  mYamlEmit << YAML::BeginSeq;

  for (const auto& instance : modInstCamData.micAssimpInstances) {
    /* skip null instance */
    if (instance->getModel()->getTriangleCount() == 0) {
      continue;
    }

    InstanceSettings instSettings = instance->getInstanceSettings();

    mYamlEmit << YAML::BeginMap;
    mYamlEmit << instSettings;
    if (mInstanceToCamMap.count(instSettings.isInstanceIndexPosition) > 0) {
      mYamlEmit << YAML::Key << "target-of-cameras";
      mYamlEmit << YAML::Value << mInstanceToCamMap[instSettings.isInstanceIndexPosition];
    }
    mYamlEmit << YAML::EndMap;
  }
  mYamlEmit << YAML::EndSeq;
  mYamlEmit << YAML::EndMap;

  mYamlEmit << YAML::Newline;

  Logger::log(2, "%s: --- emitter output ---\n", __FUNCTION__);
  Logger::log(2, "%s\n", mYamlEmit.c_str());
  Logger::log(2, "%s: --- emitter output ---\n", __FUNCTION__);

  return true;
}

bool YamlParser::writeYamlFile(std::string fileName) {
  std::ofstream fileToWrite(fileName);
  if (!fileToWrite.is_open()) {
    Logger::log(1, "%s error: could not open file '%s' for writing\n", __FUNCTION__, fileName.c_str());
    return false;
  }

  fileToWrite << mYamlEmit.c_str();
  if (fileToWrite.bad() || fileToWrite.fail()) {
    Logger::log(1, "%s error: failed to write file '%s'\n", __FUNCTION__, fileName.c_str());
    return false;
  }

  fileToWrite.close();
  return true;
}


bool YamlParser::hasKey(const std::string key) {
  try {
    if (mYamlNode[key]) {
      return true;
    }
  } catch(...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
  }
  return false;
}

bool YamlParser::getValue(const std::string key, std::string& value) {
  try {
    if (mYamlNode[key]) {
      value = mYamlNode[key].as<std::string>();
      return true;
    } else {
      Logger::log(1, "%s error: could not read key '%s' in file '%s'\n", __FUNCTION__, key.c_str(), mYamlFileName.c_str());
    }
  } catch(...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
  }

  return false;
}
