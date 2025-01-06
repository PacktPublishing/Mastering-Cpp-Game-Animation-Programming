#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "YamlParserTypes.h"
#include "ModelInstanceCamData.h"
#include "InstanceSettings.h"
#include "CameraSettings.h"
#include "ModelSettings.h"
#include "BehaviorData.h"
#include "OGLRenderData.h"
#include "Enums.h"

class YamlParser {
  public:
    /* loading */
    bool loadYamlFile(std::string fileName);
    std::string getFileName();
    std::string getFileVersion();

    std::vector<ModelSettings> getModelConfigs();
    std::vector<ExtendedInstanceSettings> getInstanceConfigs();
    std::vector<CameraSettings> getCameraConfigs();
    std::vector<EnhancedBehaviorData> getBehaviorData();

    int getSelectedModelNum();
    int getSelectedInstanceNum();
    int getSelectedCameraNum();
    bool getHighlightActivated();

    glm::vec3 getCameraPosition();
    float getCameraElevation();
    float getCameraAzimuth();

    collisionChecks getCollisionChecksEnabled();
    bool getInteractionEnabled();
    float getInteractionFOV();
    float getInteractionMinRange();
    float getInteractionMaxRange();

    /* saving */
    bool createConfigFile(OGLRenderData renderData, ModelInstanceCamData modInstCamData);
    bool writeYamlFile(std::string fileName);

    /* misc */
    bool hasKey(std::string key);
    bool getValue(std::string key, std::string& value);

  private:
    void createInstanceToCamMap(ModelInstanceCamData modInstCamData);
    std::unordered_map<int, std::vector<std::string>> mInstanceToCamMap;

    std::string mYamlFileName;
    YAML::Node mYamlNode{};
    YAML::Emitter mYamlEmit{};

    const std::string mYamlConfigFileVersion = "6.0";
    std::string mYamlFileVersion;
};
