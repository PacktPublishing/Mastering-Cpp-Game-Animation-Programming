#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <tuple>
#include <set>
#include <variant>

#include <glm/glm.hpp>

#include "Enums.h"
#include "BoundingBox3D.h"

/* forward declarations*/
class AssimpModel;
class AssimpInstance;
class AssimpSettingsContainer;
class Camera;
class SingleInstanceBehavior;
struct MeshTriangle;


using getInstanceEditModeCallback = std::function<instanceEditMode(void)>;
using setInstanceEditModeCallback = std::function<void(instanceEditMode)>;

using getSelectedModelCallback = std::function<int(void)>;
using setSelectedModelCallback = std::function<void(int)>;

using modelCheckCallback = std::function<bool(std::string)>;
using modelAddCallback = std::function<bool(std::string, bool, bool)>;
using modelAddExistingCallback = std::function<void(std::shared_ptr<AssimpModel>, int)>;
using modelDeleteCallback = std::function<void(std::string, bool)>;

using getSelectedInstanceCallback = std::function<int(void)>;
using setSelectedInstanceCallback = std::function<void(int)>;
using getInstanceByIdCallback = std::function<std::shared_ptr<AssimpInstance>(int)>;

using instanceGetModelCallback= std::function<std::shared_ptr<AssimpModel>(std::string)>;
using instanceAddCallback = std::function<std::shared_ptr<AssimpInstance>(std::shared_ptr<AssimpModel>)>;
using instanceAddExistingCallback = std::function<void(std::shared_ptr<AssimpInstance>, int, int)>;
using instanceAddManyCallback = std::function<void(std::shared_ptr<AssimpModel>, int)>;
using instanceDeleteCallback = std::function<void(std::shared_ptr<AssimpInstance>, bool)>;
using instanceCloneCallback = std::function<void(std::shared_ptr<AssimpInstance>)>;
using instanceCloneManyCallback = std::function<void(std::shared_ptr<AssimpInstance>, int)>;

using instanceCenterCallback = std::function<void(std::shared_ptr<AssimpInstance>)>;

using instanceGetPositions = std::function<std::vector<glm::vec3>(void)>;
using instanceGetBoundingBox = std::function<BoundingBox3D(int)>;

using undoRedoCallback = std::function<void(void)>;
using newConfigCallback = std::function<void(void)>;
using loadSaveCallback = std::function<bool(std::string)>;
using setConfigDirtyCallbackFunction = std::function<void(bool)>;
using getConfigDirtyCallbackFunction = std::function<bool(void)>;

using cameraCloneCallback = std::function<void()>;
using cameraDeleteCallback = std::function<void()>;
using cameraNameCheckCallback = std::function<bool(std::string)>;

using octreeQueryBBoxCallback = std::function<std::set<int>(BoundingBox3D)>;
using octreeFindAllIntersectionsCallback = std::function<std::set<std::pair<int, int>>()>;
using octreeGetBoxesCallback = std::function<std::vector<BoundingBox3D>()>;
using worldGetBoundariesCallback = std::function<std::shared_ptr<BoundingBox3D>()>;
using triangleOctreeGetMeshesCallback = std::function<std::vector<MeshTriangle>(BoundingBox3D)>;
using triangleOctreeChangeCallback = std::function<void(void)>;

using fireNodeOutputCallback = std::function<void(int)>;
using editNodeGraphCallback = std::function<void(std::string)>;
using createEmptyNodeGraphCallback = std::function<std::shared_ptr<SingleInstanceBehavior>(void)>;

using nodeCallbackVariant = std::variant<float, glm::vec2, glm::vec3, moveState, moveDirection, faceAnimation>;
using nodeActionCallback = std::function<void(graphNodeType, instanceUpdateType, nodeCallbackVariant, bool)>;
using instanceNodeActionCallback = std::function<void(int, graphNodeType, instanceUpdateType, nodeCallbackVariant, bool)>;
using nodeEventCallback = std::function<void(int, nodeEvent)>;
using postNodeTreeDelBehaviorCallback = std::function<void(std::string)>;

using instanceAddBehaviorCallback = std::function<void(int, std::shared_ptr<SingleInstanceBehavior>)>;
using instanceDelBehaviorCallback = std::function<void(int)>;
using modelAddBehaviorCallback = std::function<void(std::string, std::shared_ptr<SingleInstanceBehavior>)>;
using modelDelBehaviorCallback = std::function<void(std::string)>;

using getSelectedLevelCallback = std::function<int(void)>;
using setSelectedLevelCallback = std::function<void(int)>;

using levelCheckCallback = std::function<bool(std::string)>;
using levelAddCallback = std::function<bool(std::string)>;
using levelDeleteCallback = std::function<void(std::string)>;
using levelGenerateAABBCallback = std::function<void(void)>;

using appExitCallback = std::function<void(void)>;
