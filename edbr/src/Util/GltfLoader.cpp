#include <edbr/Util/GltfLoader.h>

#include <cassert>
#include <iostream>
#include <span>

#include <edbr/Graphics/BaseRenderer.h>
#include <edbr/Graphics/CPUMesh.h>
#include <edbr/Graphics/Color.h>
#include <edbr/Graphics/GPUMesh.h>
#include <edbr/Graphics/GfxDevice.h>
#include <edbr/Graphics/Scene.h>
#include <edbr/Graphics/Skeleton.h>
#include <edbr/Math/Util.h>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

// tell tinygltf to not load images
namespace
{
bool LoadImageData(
    tinygltf::Image* image,
    const int image_idx,
    std::string* err,
    std::string* warn,
    int req_width,
    int req_height,
    const unsigned char* bytes,
    int size,
    void*)
{
    return true;
};

bool WriteImageData(
    const std::string* basepath,
    const std::string* filename,
    const tinygltf::Image* image,
    bool embedImages,
    const tinygltf::URICallbacks* uri_cb,
    std::string* out_uri,
    void*)
{
    return true;
};

void setEmptyImageLoaders(tinygltf::TinyGLTF& loader)
{
    loader.SetImageLoader(LoadImageData, nullptr);
    loader.SetImageWriter(WriteImageData, nullptr);
}

}

namespace
{
static const std::string GLTF_POSITIONS_ACCESSOR{"POSITION"};
static const std::string GLTF_NORMALS_ACCESSOR{"NORMAL"};
static const std::string GLTF_TANGENTS_ACCESSOR{"TANGENT"};
static const std::string GLTF_UVS_ACCESSOR{"TEXCOORD_0"};
static const std::string GLTF_JOINTS_ACCESSOR{"JOINTS_0"};
static const std::string GLTF_WEIGHTS_ACCESSOR{"WEIGHTS_0"};

static const std::string GLTF_SAMPLER_PATH_TRANSLATION{"translation"};
static const std::string GLTF_SAMPLER_PATH_ROTATION{"rotation"};
static const std::string GLTF_SAMPLER_PATH_SCALE{"scale"};

// extensions
static const std::string GLTF_LIGHTS_PUNCTUAL_KEY{"KHR_lights_punctual"};
static const std::string GLTF_LIGHTS_PUNCTUAL_POINT_NAME{"point"};
static const std::string GLTF_LIGHTS_PUNCTUAL_DIRECTIONAL_NAME{"directional"};
static const std::string GLTF_LIGHTS_PUNCTUAL_SPOT_NAME{"spot"};

static const std::string GLTF_EMISSIVE_STRENGTH_KHR_KEY{"KHR_materials_emissive_strength"};
static const std::string GLTF_EMISSIVE_STRENGTH_PARAM_NAME{"emissiveStrength"};

glm::vec3 tg2glm(const std::vector<double>& vec)
{
    assert(vec.size() == 3);
    return {vec[0], vec[1], vec[2]};
}

glm::vec4 tg2glmVec4(const std::vector<double>& vec)
{
    if (vec.size() == 4) {
        return {vec[0], vec[1], vec[2], vec[3]};
    }

    assert(vec.size() == 3);
    return {vec[0], vec[1], vec[2], 1.f};
}

LinearColor tg2LinearColor(const std::vector<double>& vec)
{
    if (vec.size() == 4) {
        return LinearColor{(float)vec[0], (float)vec[1], (float)vec[2], (float)vec[3]};
    }

    assert(vec.size() == 3);
    return LinearColor{(float)vec[0], (float)vec[1], (float)vec[2], 1.f};
}

glm::quat tg2glmQuat(const std::vector<double>& vec)
{
    return {
        static_cast<float>(vec[3]), // w
        static_cast<float>(vec[0]), // x
        static_cast<float>(vec[1]), // y
        static_cast<float>(vec[2]), // z
    };
}

template<typename T>
std::span<const T> getPackedBufferSpan(
    const tinygltf::Model& model,
    const tinygltf::Accessor& accessor)
{
    const auto& bv = model.bufferViews[accessor.bufferView];
    const int bs = accessor.ByteStride(bv);
    assert(bs == sizeof(T)); // check that it's packed
    const auto& buf = model.buffers[bv.buffer];
    const auto* data =
        reinterpret_cast<const T*>(&buf.data.at(0) + bv.byteOffset + accessor.byteOffset);
    return std::span{data, accessor.count};
}

int findAttributeAccessor(const tinygltf::Primitive& primitive, const std::string& attributeName)
{
    for (const auto& [accessorName, accessorID] : primitive.attributes) {
        if (accessorName == attributeName) {
            return accessorID;
        }
    }
    return -1;
}

bool hasAccessor(const tinygltf::Primitive& primitive, const std::string& attributeName)
{
    const auto accessorIndex = findAttributeAccessor(primitive, attributeName);
    return accessorIndex != -1;
}

template<typename T>
std::span<const T> getPackedBufferSpan(
    const tinygltf::Model& model,
    const tinygltf::Primitive& primitive,
    const std::string& attributeName)
{
    const auto accessorIndex = findAttributeAccessor(primitive, attributeName);
    assert(accessorIndex != -1 && "Accessor not found");
    const auto& accessor = model.accessors[accessorIndex];
    return getPackedBufferSpan<T>(model, accessor);
}

bool hasDiffuseTexture(const tinygltf::Material& material)
{
    const auto textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
    return textureIndex != -1;
}

bool hasNormalMapTexture(const tinygltf::Material& material)
{
    const auto textureIndex = material.normalTexture.index;
    return textureIndex != -1;
}

bool hasMetallicRoughnessTexture(const tinygltf::Material& material)
{
    const auto textureIndex = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
    return textureIndex != -1;
}

bool hasEmissiveTexture(const tinygltf::Material& material)
{
    const auto textureIndex = material.emissiveTexture.index;
    return textureIndex != -1;
}

glm::vec4 getDiffuseColor(const tinygltf::Material& material)
{
    const auto c = material.pbrMetallicRoughness.baseColorFactor;
    assert(c.size() == 4);
    return {(float)c[0], (float)c[1], (float)c[2], (float)c[3]};
}

std::filesystem::path getNormalMapTexturePath(
    const tinygltf::Model& model,
    const tinygltf::Material& material,
    const std::filesystem::path& fileDir)
{
    const auto textureIndex = material.normalTexture.index;
    const auto& textureId = model.textures[textureIndex];
    const auto& image = model.images[textureId.source];
    return fileDir / image.uri;
}

std::filesystem::path getMetallicRoughnessTexturePath(
    const tinygltf::Model& model,
    const tinygltf::Material& material,
    const std::filesystem::path& fileDir)
{
    const auto textureIndex = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
    const auto& textureId = model.textures[textureIndex];
    const auto& image = model.images[textureId.source];
    return fileDir / image.uri;
}

std::filesystem::path getEmissiveTexturePath(
    const tinygltf::Model& model,
    const tinygltf::Material& material,
    const std::filesystem::path& fileDir)
{
    const auto textureIndex = material.emissiveTexture.index;
    const auto& textureId = model.textures[textureIndex];
    const auto& image = model.images[textureId.source];
    return fileDir / image.uri;
}

float getEmissiveStrength(const tinygltf::Material& material)
{
    if (material.extensions.contains(GLTF_EMISSIVE_STRENGTH_KHR_KEY)) {
        return (float)material.extensions.at(GLTF_EMISSIVE_STRENGTH_KHR_KEY)
            .Get(GLTF_EMISSIVE_STRENGTH_PARAM_NAME)
            .GetNumberAsDouble();
    }
    return 1.f;
}

std::filesystem::path getDiffuseTexturePath(
    const tinygltf::Model& model,
    const tinygltf::Material& material,
    const std::filesystem::path& fileDir)
{
    const auto textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
    const auto& textureId = model.textures[textureIndex];
    const auto& image = model.images[textureId.source];
    return fileDir / image.uri;
}

CPUMesh loadPrimitive(
    const tinygltf::Model& model,
    const std::string& meshName,
    const tinygltf::Primitive& primitive)
{
    CPUMesh mesh{.name = meshName};

    if (primitive.indices != -1) { // load indices
        const auto& indexAccessor = model.accessors[primitive.indices];
        const auto indices = getPackedBufferSpan<std::uint16_t>(model, indexAccessor);
        mesh.indices.assign(indices.begin(), indices.end());
    }

    // load positions
    const auto positions =
        getPackedBufferSpan<glm::vec3>(model, primitive, GLTF_POSITIONS_ACCESSOR);
    mesh.vertices.resize(positions.size());
    for (std::size_t i = 0; i < positions.size(); ++i) {
        mesh.vertices[i].position = positions[i];
    }

    { // get min and max pos
        const auto posAccessorIndex = findAttributeAccessor(primitive, GLTF_POSITIONS_ACCESSOR);
        assert(posAccessorIndex != -1 && "Accessor not found");
        const auto& posAccessor = model.accessors[posAccessorIndex];
        mesh.minPos = tg2glm(posAccessor.minValues);
        mesh.maxPos = tg2glm(posAccessor.maxValues);
    }

    const auto numVertices = positions.size();

    // load normals
    if (hasAccessor(primitive, GLTF_NORMALS_ACCESSOR)) {
        const auto normals =
            getPackedBufferSpan<glm::vec3>(model, primitive, GLTF_NORMALS_ACCESSOR);
        assert(normals.size() == numVertices);
        for (std::size_t i = 0; i < normals.size(); ++i) {
            mesh.vertices[i].normal = normals[i]; // TEMP
        }
    }

    // load tangents
    if (hasAccessor(primitive, GLTF_TANGENTS_ACCESSOR)) {
        const auto tangents =
            getPackedBufferSpan<glm::vec4>(model, primitive, GLTF_TANGENTS_ACCESSOR);
        assert(tangents.size() == numVertices);
        for (std::size_t i = 0; i < tangents.size(); ++i) {
            mesh.vertices[i].tangent = tangents[i];
        }
    }

    // load uvs
    if (hasAccessor(primitive, GLTF_UVS_ACCESSOR)) {
        const auto uvs = getPackedBufferSpan<glm::vec2>(model, primitive, GLTF_UVS_ACCESSOR);
        assert(uvs.size() == numVertices);
        for (std::size_t i = 0; i < uvs.size(); ++i) {
            mesh.vertices[i].uv_x = uvs[i].x; // TEMP
            mesh.vertices[i].uv_y = uvs[i].y; // TEMP
        }
    }

    // load jointIds and weights
    if (hasAccessor(primitive, GLTF_JOINTS_ACCESSOR)) {
        const auto joints =
            getPackedBufferSpan<std::uint8_t[4]>(model, primitive, GLTF_JOINTS_ACCESSOR);
        const auto weights = getPackedBufferSpan<float[4]>(model, primitive, GLTF_WEIGHTS_ACCESSOR);

        assert(joints.size() == numVertices);
        assert(weights.size() == numVertices);

        mesh.hasSkeleton = true;
        mesh.skinningData.resize(numVertices);

        for (std::size_t i = 0; i < joints.size(); ++i) {
            mesh.skinningData[i].jointIds =
                glm::uvec4{joints[i][0], joints[i][1], joints[i][2], joints[i][3]};
        }

        for (std::size_t i = 0; i < weights.size(); ++i) {
            mesh.skinningData[i].weights =
                glm::vec4{weights[i][0], weights[i][1], weights[i][2], weights[i][3]};
        }
    }

    return mesh;
}

Material loadMaterial(
    BaseRenderer& renderer,
    const tinygltf::Model& gltfModel,
    const std::filesystem::path& fileDir,
    const tinygltf::Material& gltfMaterial)
{
    Material material{
        .baseColor = getDiffuseColor(gltfMaterial),
        .metallicFactor = (float)gltfMaterial.pbrMetallicRoughness.metallicFactor,
        .roughnessFactor = (float)gltfMaterial.pbrMetallicRoughness.roughnessFactor,
        .name = gltfMaterial.name,
    };

    if (hasDiffuseTexture(gltfMaterial)) {
        const auto diffusePath = getDiffuseTexturePath(gltfModel, gltfMaterial, fileDir);
        material.diffuseTexture = renderer.getGfxDevice().loadImageFromFile(
            diffusePath, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, true);
    }

    if (hasNormalMapTexture(gltfMaterial)) {
        const auto normalMapPath = getNormalMapTexturePath(gltfModel, gltfMaterial, fileDir);
        material.normalMapTexture = renderer.getGfxDevice().loadImageFromFile(
            normalMapPath, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
    }

    if (hasMetallicRoughnessTexture(gltfMaterial)) {
        const auto metalRoughnessPath =
            getMetallicRoughnessTexturePath(gltfModel, gltfMaterial, fileDir);
        material.metallicRoughnessTexture = renderer.getGfxDevice().loadImageFromFile(
            metalRoughnessPath, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
    }

    if (hasEmissiveTexture(gltfMaterial)) {
        material.emissiveFactor = getEmissiveStrength(gltfMaterial);

        const auto emissivePath = getEmissiveTexturePath(gltfModel, gltfMaterial, fileDir);
        material.emissiveTexture = renderer.getGfxDevice().loadImageFromFile(
            emissivePath, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, true);
    }

    return material;
}

Transform loadTransform(const tinygltf::Node& gltfNode)
{
    Transform transform;
    if (!gltfNode.translation.empty()) {
        transform.setPosition(tg2glm(gltfNode.translation));
    }
    if (!gltfNode.scale.empty()) {
        transform.setScale(tg2glm(gltfNode.scale));
    }
    if (!gltfNode.rotation.empty()) {
        transform.setHeading(tg2glmQuat(gltfNode.rotation));
    }
    return transform;
}

Skeleton loadSkeleton(
    std::unordered_map<int, JointId>& gltfNodeIdxToJointId,
    const tinygltf::Model& model,
    const tinygltf::Skin& skin)
{
    // load inverse bind matrices
    const auto& ibAccessor = model.accessors[skin.inverseBindMatrices];
    const auto ibs = getPackedBufferSpan<glm::mat4>(model, ibAccessor);
    std::vector<glm::mat4> ibMatrices(ibAccessor.count);
    ibMatrices.assign(ibs.begin(), ibs.end());

    const auto numJoints = skin.joints.size();
    Skeleton skeleton;
    skeleton.joints.reserve(numJoints);
    skeleton.inverseBindMatrices = ibMatrices;
    skeleton.jointNames.resize(numJoints);

    gltfNodeIdxToJointId.reserve(numJoints);
    { // load joints
        JointId jointId{0};
        for (const auto& nodeIdx : skin.joints) {
            gltfNodeIdxToJointId.emplace(nodeIdx, jointId);

            const auto& jointNode = model.nodes[nodeIdx];
            skeleton.jointNames[jointId] = jointNode.name;

            skeleton.joints.push_back(Joint{
                .id = jointId,
                .localTransform = loadTransform(jointNode),
            });

            ++jointId;
        }
    }

    { // build hierarchy
        skeleton.hierarchy.resize(numJoints);
        for (JointId jointId = 0; jointId < skeleton.joints.size(); ++jointId) {
            const auto& jointNode = model.nodes[skin.joints[jointId]];
            for (const auto& childIdx : jointNode.children) {
                const auto childJointId = gltfNodeIdxToJointId.at(childIdx);
                skeleton.hierarchy[jointId].children.push_back(childJointId);
            }
        }
    }

    return skeleton;
}

std::unordered_map<std::string, SkeletalAnimation> loadAnimations(
    const Skeleton& skeleton,
    const std::unordered_map<int, JointId>& gltfNodeIdxToJointId,
    const tinygltf::Model& gltfModel)
{
    std::unordered_map<std::string, SkeletalAnimation> animations(gltfModel.animations.size());
    for (const auto& gltfAnimation : gltfModel.animations) {
        auto& animation = animations[gltfAnimation.name];
        animation.name = gltfAnimation.name;

        const auto numJoints = skeleton.joints.size();

        animation.tracks.resize(numJoints);

        for (const auto& channel : gltfAnimation.channels) {
            const auto& sampler = gltfAnimation.samplers[channel.sampler];

            const auto& timesAccessor = gltfModel.accessors[sampler.input];
            const auto times = getPackedBufferSpan<float>(gltfModel, timesAccessor);

            animation.duration =
                static_cast<float>(timesAccessor.maxValues[0] - timesAccessor.minValues[0]);
            if (animation.duration == 0) {
                continue; // skip empty animations (e.g. keying sets)
            }

            if (channel.target_path == "weights") {
                // FIXME: find out why this channel exists
                // no idea what this is, but sometimes breaks stuff
                continue;
            }

            const auto nodeId = channel.target_node;
            const auto jointId = gltfNodeIdxToJointId.at(nodeId);

            const auto& outputAccessor = gltfModel.accessors[sampler.output];
            assert(outputAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
            if (channel.target_path == GLTF_SAMPLER_PATH_TRANSLATION) {
                const auto translationKeys =
                    getPackedBufferSpan<glm::vec3>(gltfModel, outputAccessor);

                auto& tc = animation.tracks[jointId].translations;
                if (translationKeys.size() == 2 && translationKeys[0] == translationKeys[1]) {
                    tc.push_back(translationKeys[0]);
                } else {
                    if (jointId == 0) {
                        assert(translationKeys.size() != 2);
                    }
                    tc.reserve(translationKeys.size());
                    for (const auto& key : translationKeys) {
                        tc.push_back(key);
                    }
                }

            } else if (channel.target_path == GLTF_SAMPLER_PATH_ROTATION) {
                const auto rotationKeys = getPackedBufferSpan<glm::vec4>(gltfModel, outputAccessor);
                auto& rc = animation.tracks[jointId].rotations;
                if (rotationKeys.size() == 2 && rotationKeys[0] == rotationKeys[1]) {
                    const auto& qv = rotationKeys[0];
                    const glm::quat q{qv.w, qv.x, qv.y, qv.z};
                    rc.push_back(q);
                } else {
                    rc.reserve(rotationKeys.size());
                    for (const auto& qv : rotationKeys) {
                        const glm::quat q{qv.w, qv.x, qv.y, qv.z};
                        rc.push_back(q);
                    }
                }
            } else if (channel.target_path == GLTF_SAMPLER_PATH_SCALE) {
                const auto scaleKeys =
                    getPackedBufferSpan<const glm::vec3>(gltfModel, outputAccessor);

                auto& sc = animation.tracks[jointId].scales;
                if (scaleKeys.size() == 2 && scaleKeys[0] == scaleKeys[1]) {
                    sc.push_back(scaleKeys[0]);
                } else {
                    sc.reserve(scaleKeys.size());
                    for (const auto& key : scaleKeys) {
                        sc.push_back(key);
                    }
                }

            } else {
                assert(false && "unexpected target_path");
            }
        }
    }

    return animations;
}

LightType fromGLTFLightType(const std::string& lt)
{
    if (lt == GLTF_LIGHTS_PUNCTUAL_POINT_NAME) {
        return LightType::Point;
    } else if (lt == GLTF_LIGHTS_PUNCTUAL_DIRECTIONAL_NAME) {
        return LightType::Directional;
    } else if (lt == GLTF_LIGHTS_PUNCTUAL_SPOT_NAME) {
        return LightType::Spot;
    }
    assert(false && "unexpected light type");
    return LightType::None;
}

float gltfIntensityConvert(float intensity)
{
    // not accurate conversion, but will do for now
    const auto PBR_WATTS_TO_LUMENS = 683.f;
    // should be 4*PI, but it's too much
    return intensity / (PBR_WATTS_TO_LUMENS * glm::pi<float>());
}

Light loadLight(const tinygltf::Light& tLight)
{
    Light light;
    light.name = tLight.name;
    light.type = fromGLTFLightType(tLight.type);
    light.color = tg2LinearColor(tLight.color);
    light.intensity = gltfIntensityConvert((float)tLight.intensity);
    light.range = (float)tLight.range;
    light.setConeAngles((float)tLight.spot.innerConeAngle, (float)tLight.spot.outerConeAngle);
    return light;
}

void loadNode(SceneNode& node, const tinygltf::Node& gltfNode, const tinygltf::Model& model)
{
    node.name = gltfNode.name;
    node.transform = loadTransform(gltfNode);

    node.meshIndex = gltfNode.mesh;
    node.skinId = gltfNode.skin;
    node.lightId = gltfNode.light;
    node.cameraId = gltfNode.camera;

    // load children
    node.children.resize(gltfNode.children.size());
    for (std::size_t childIdx = 0; childIdx < gltfNode.children.size(); ++childIdx) {
        const auto& childNode = model.nodes[gltfNode.children[childIdx]];
        auto& childPtr = node.children[childIdx];
        childPtr = std::make_unique<SceneNode>();
        auto& child = *childPtr;
        loadNode(child, childNode, model);
    }
}

void loadGltfFile(tinygltf::Model& gltfModel, const std::filesystem::path& path)
{
    tinygltf::TinyGLTF loader;
    setEmptyImageLoaders(loader);

    std::string err;
    std::string warn;

    bool res = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, path.string());
    if (!warn.empty()) {
        std::cout << "WARNING: " << warn << std::endl;
    }
    if (!res) {
        std::cout << "Failed to load glTF scene " << path << std::endl;
        if (!err.empty()) {
            std::cout << "ERROR: " << err << std::endl;
        }
        assert(false);
    }
}

} // end of anonymous namespace

namespace util
{
Scene loadGltfFile(BaseRenderer& renderer, const std::filesystem::path& path)
{
    const auto fileDir = path.parent_path();

    tinygltf::Model gltfModel;
    ::loadGltfFile(gltfModel, path);

    const auto& gltfScene = gltfModel.scenes[gltfModel.defaultScene];

    // gltf material id -> material cache id
    std::unordered_map<std::size_t, MaterialId> materialMapping;
    // load materials
    {
        for (std::size_t materialIdx = 0; materialIdx < gltfModel.materials.size(); ++materialIdx) {
            const auto& gltfMaterial = gltfModel.materials[materialIdx];
            const auto materialId =
                renderer.addMaterial(loadMaterial(renderer, gltfModel, fileDir, gltfMaterial));
            materialMapping.emplace(materialIdx, materialId);
        }
    }

    Scene scene{};
    // load meshes
    scene.meshes.reserve(gltfModel.meshes.size());
    std::string meshName;
    meshName.resize(255);
    for (const auto& gltfMesh : gltfModel.meshes) {
        SceneMesh mesh;
        mesh.primitives.resize(gltfMesh.primitives.size());
        for (std::size_t primitiveIdx = 0; primitiveIdx < gltfMesh.primitives.size();
             ++primitiveIdx) {
            // load on CPU
            const auto& gltfPrimitive = gltfMesh.primitives[primitiveIdx];
            meshName = path.string();
            meshName += ":";
            meshName += gltfMesh.name;
            const auto cpuMesh = loadPrimitive(gltfModel, meshName, gltfPrimitive);
            if (cpuMesh.indices.empty()) {
                continue;
            }

            // upload to GPU
            auto materialId = NULL_MATERIAL_ID;
            if (gltfPrimitive.material != -1) {
                materialId = materialMapping.at(gltfPrimitive.material);
            }
            const auto meshId = renderer.addMesh(cpuMesh, materialId);
            mesh.primitives[primitiveIdx] = meshId;
        }
        scene.meshes.push_back(std::move(mesh));
    }

    // gltf node id -> JointId
    // for now only one skeleton per scene is supported
    std::unordered_map<int, JointId> gltfNodeIdxToJointId;
    scene.skeletons.reserve(gltfModel.skins.size());
    for (const auto& skin : gltfModel.skins) {
        scene.skeletons.push_back(loadSkeleton(gltfNodeIdxToJointId, gltfModel, skin));
    }

    // load animations
    if (!gltfModel.skins.empty()) {
        assert(gltfModel.skins.size() == 1); // for now only one skeleton supported
        scene.animations = loadAnimations(scene.skeletons[0], gltfNodeIdxToJointId, gltfModel);
    }

    // load lights
    scene.lights.reserve(gltfModel.lights.size());
    for (const auto& light : gltfModel.lights) {
        scene.lights.push_back(loadLight(light));
    }

    // load nodes
    scene.nodes.resize(gltfScene.nodes.size());
    for (std::size_t nodeIdx = 0; nodeIdx < gltfScene.nodes.size(); ++nodeIdx) {
        const auto& gltfNode = gltfModel.nodes[gltfScene.nodes[nodeIdx]];

        // HACK: load mesh with skin (for now only one assumed)
        if (gltfNode.children.size() == 2) {
            const auto& c1 = gltfModel.nodes[gltfNode.children[0]];
            const auto& c2 = gltfModel.nodes[gltfNode.children[1]];
            if ((c1.mesh != -1 && c1.skin != -1) || (c2.mesh != -1 && c2.skin != -1)) {
                const auto& meshNode = (c1.mesh != -1) ? c1 : c2;

                auto& nodePtr = scene.nodes[nodeIdx];
                nodePtr = std::make_unique<SceneNode>();
                auto& node = *nodePtr;
                loadNode(node, meshNode, gltfModel);

                // sometimes the armature node can have scaling
                // this is BAD, but we can't avoid it for models
                const auto parentTransform = loadTransform(gltfNode);
                node.transform = parentTransform * node.transform;

                continue;
            }
        }

        auto& nodePtr = scene.nodes[nodeIdx];
        nodePtr = std::make_unique<SceneNode>();
        auto& node = *nodePtr;
        loadNode(node, gltfNode, gltfModel);
    }

    return scene;
}

} // end of namespace util
