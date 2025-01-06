#pragma once

#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "YamlParserTypes.h"
#include "ModelAndInstanceData.h"
#include "InstanceSettings.h"
#include "OGLRenderData.h"

class YamlParser {
  public:
    /* loading */
    bool loadYamlFile(std::string fileName);
    std::string getFileName();
    bool checkFileVersion();

    std::vector<std::string> getModelFileNames();
    std::vector<InstanceSettings> getInstanceConfigs();
    int getSelectedModelNum();
    int getSelectedInstanceNum();
    bool getHighlightActivated();

    glm::vec3 getCameraPosition();
    float getCameraElevation();
    float getCameraAzimuth();

    /* saving */
    bool createConfigFile(OGLRenderData renderData, ModelAndInstanceData modInstData);
    bool writeYamlFile(std::string fileName);

    /* misc */
    bool hasKey(std::string key);
    bool getValue(std::string key, std::string& value);

  private:
    std::string mYamlFileName;
    YAML::Node mYamlNode{};
    YAML::Emitter mYamlEmit{};

    const std::string mYamlConfigFileVersion = "1.0";
    std::string mYamlFileVersion;
};
