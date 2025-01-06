#include <fstream>

#include "YamlParser.h"
#include "AssimpModel.h"
#include "AssimpInstance.h"
#include "Logger.h"

/* overloads */
YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& vec) {
  out << YAML::Flow;
  out << YAML::BeginSeq << vec.x << vec.y << vec.z << YAML::EndSeq;
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
  out << YAML::Key << "anim-clip-number";
  out << YAML::Value << settings.isAnimClipNr;
  out << YAML::Key << "anim-clip-speed";
  out << YAML::Value << settings.isAnimSpeedFactor;
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

std::vector<std::string> YamlParser::getModelFileNames() {
  std::vector<std::string> modelFileNames;

  if (!hasKey("models")) {
    Logger::log(1, "%s error: no model file names found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return modelFileNames;
  }

  YAML::Node modelsNode = mYamlNode["models"];
  try {
    for (size_t i = 0; i < modelsNode.size(); ++i) {
      Logger::log(1, "%s: found model name: %s\n", __FUNCTION__, modelsNode[i]["model-name"].as<std::string>().c_str());
      modelFileNames.emplace_back(modelsNode[i]["model-file"].as<std::string>());
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return std::vector<std::string>{};
  }

  return modelFileNames;
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

std::vector<InstanceSettings> YamlParser::getInstanceConfigs() {
  std::vector<InstanceSettings> instSettings;
  if (!hasKey("instances")) {
    Logger::log(1, "%s error: no instances found in config file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return instSettings;
  }

  YAML::Node instanceNode = mYamlNode["instances"];
  try {
    for (size_t i = 0; i < instanceNode.size(); ++i) {
      instSettings.emplace_back(instanceNode[i].as<InstanceSettings>());
    }
  } catch (...) {
    Logger::log(1, "%s error: could not parse file '%s'\n", __FUNCTION__, mYamlFileName.c_str());
    return std::vector<InstanceSettings>{};
  }

  return instSettings;
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

bool YamlParser::createConfigFile(VkRenderData renderData, ModelAndInstanceData modInstData) {
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
  mYamlEmit << YAML::Value << modInstData.miSelectedModel;
  mYamlEmit << YAML::Key << "selected-instance";
  mYamlEmit << YAML::Value << modInstData.miSelectedInstance;
  mYamlEmit << YAML::Key << "highlight-selection";
  mYamlEmit << YAML::Value << renderData.rdHighlightSelectedInstance;
  mYamlEmit << YAML::EndMap;
  mYamlEmit << YAML::EndMap;

  mYamlEmit << YAML::Newline;

  mYamlEmit << YAML::BeginMap;
  mYamlEmit << YAML::Key << "camera";
  mYamlEmit << YAML::Value;
  mYamlEmit << YAML::BeginMap;
  mYamlEmit << YAML::Key << "camera-position";
  mYamlEmit << YAML::Value << renderData.rdCameraWorldPosition;
  mYamlEmit << YAML::Key << "camera-elevation";
  mYamlEmit << YAML::Value << renderData.rdViewElevation;
  mYamlEmit << YAML::Key << "camera-azimuth";
  mYamlEmit << YAML::Value << renderData.rdViewAzimuth;
  mYamlEmit << YAML::EndMap;
  mYamlEmit << YAML::EndMap;

  mYamlEmit << YAML::Newline;

  /* models */
  mYamlEmit << YAML::BeginMap;
  mYamlEmit << YAML::Key << "models";
  mYamlEmit << YAML::Value;
  mYamlEmit << YAML::BeginSeq;

  for (const auto& model : modInstData.miModelList) {
    /* skip emtpy models (null model) */
    if (model->getTriangleCount() == 0) {
      continue;
    }
    mYamlEmit << YAML::BeginMap;
    mYamlEmit << YAML::Key << "model-name";
    mYamlEmit << YAML::Value << model->getModelFileName();
    mYamlEmit << YAML::Key << "model-file";
    mYamlEmit << YAML::Value <<  model->getModelFileNamePath();
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

  for (const auto& instance : modInstData.miAssimpInstances) {
    /* skip null instance */
    if (instance->getModel()->getTriangleCount() == 0) {
      continue;
    }

    mYamlEmit << YAML::BeginMap;
    mYamlEmit << instance->getInstanceSettings();
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
