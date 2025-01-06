#include <fstream>

#include "YamlParser.h"
#include "AssimpModel.h"
#include "AssimpInstance.h"
#include "Camera.h"
#include "Enums.h"
#include "Logger.h"

/* overloads */
YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& vec) {
  out << YAML::Flow;
  out << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
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
  if (settings.msIWRBlendings.size() > 0) {
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
  if (settings.msActionClipMappings.size() > 0) {
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
  if (settings.msAllowedStateOrder.size() > 0) {
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

bool YamlParser::checkFileVersion() {
  const std::string versionString = "version";
  if (!hasKey(versionString)) {
    Logger::log(1, "%s error: cold not find version string in YAML config file '%s'\n", __FUNCTION__, getFileName().c_str());
    return false;
  }

  if (!getValue(versionString, mYamlFileVersion)) {
    Logger::log(1, "%s error: could not get version number from YAML config file '%s'\n", __FUNCTION__, getFileName().c_str());
    return false;
  }

  Logger::log(1, "%s: found config version %s\n", __FUNCTION__, mYamlFileVersion.c_str());
  return true;
}

std::vector<ModelSettings> YamlParser::getModelConfigs() {
  std::vector<ModelSettings> modSettings;

  if (!hasKey("models")) {
    Logger::log(1, "%s error: no model file names found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return modSettings;
  }

  YAML::Node modelsNode = mYamlNode["models"];
  try {
    for (size_t i = 0; i < modelsNode.size(); ++i) {
      Logger::log(1, "%s: found model name: %s\n", __FUNCTION__, modelsNode[i]["model-name"].as<std::string>().c_str());
      modSettings.emplace_back(modelsNode[i].as<ModelSettings>());
    }
  } catch (const YAML::Exception& e) {
    Logger::log(1, "%s error: could not parse file '%s' (%s)\n", __FUNCTION__, mYamlFileName.c_str(), e.what());
    return std::vector<ModelSettings>{};
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
  try {
    for (size_t i = 0; i < instanceNode.size(); ++i) {
      instSettings.emplace_back(instanceNode[i].as<ExtendedInstanceSettings>());
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return std::vector<ExtendedInstanceSettings>{};
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
      Logger::log(1, "%s error: no cameras found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
      return camSettings;
    }

    YAML::Node camNode = mYamlNode["cameras"];
    try {
      for (size_t i = 0; i < camNode.size(); ++i) {
        camSettings.emplace_back(camNode[i].as<CameraSettings>());
      }
    } catch (...) {
      Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
      return std::vector<CameraSettings>{};
    }
  }
  return camSettings;
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

void YamlParser::createInstanceToCamMap(ModelInstanceCamData modInstCamData) {
  mInstanceToCamMap.clear();
  for (const auto& camera : modInstCamData.micCameras) {
    CameraSettings camSettings = camera->getCameraSettings();
    if (std::shared_ptr<AssimpInstance> instance = camera->getInstanceToFollow()) {
      mInstanceToCamMap[instance->getInstanceSettings().isInstanceIndexPosition].emplace_back(camSettings.csCamName);
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
  mYamlEmit << YAML::Key << "highlight-selection";
  mYamlEmit << YAML::Value << renderData.rdHighlightSelectedInstance;
  mYamlEmit << YAML::EndMap;
  mYamlEmit << YAML::EndMap;

  mYamlEmit << YAML::Newline;

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
