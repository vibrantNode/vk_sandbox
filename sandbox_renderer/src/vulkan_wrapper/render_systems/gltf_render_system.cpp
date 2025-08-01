#include"vulkan_wrapper/render_systems/gltf_render_system.h"
#include <stdexcept>



GltfRenderSystem::GltfRenderSystem(
    VkSandboxDevice& device,
    VkRenderPass renderPass,
    VkDescriptorSetLayout globalSetLayout
) : m_device(device), m_globalSetLayout(globalSetLayout) {

}

GltfRenderSystem::~GltfRenderSystem() {
    vkDestroyPipelineLayout(m_device.device(), m_pipelineLayout, nullptr);
}


void GltfRenderSystem::init(
    VkSandboxDevice& device,
    VkRenderPass renderPass,
    VkDescriptorSetLayout globalSetLayout,
    VkSandboxDescriptorPool& descriptorPool
) {
    m_globalSetLayout = globalSetLayout;


    createPipelineLayout(globalSetLayout);
    createPipeline(renderPass);

    m_iblSetLayout = VkSandboxDescriptorSetLayout::Builder(m_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // brdfLUT
        .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // irradianceMap
        .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // prefilteredMap
        .build()
        ->getDescriptorSetLayout();


}



void GltfRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout) {
    const std::vector<VkDescriptorSetLayout> layouts = {
        globalSetLayout,
        vkglTF::descriptorSetLayoutUbo,
        vkglTF::descriptorSetLayoutImage
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();

    if (vkCreatePipelineLayout(m_device.device(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create GLTF pipeline layout");
    }
}

void GltfRenderSystem::createPipeline(VkRenderPass renderPass) {
    assert(m_pipelineLayout != VK_NULL_HANDLE);

    auto vertSpv = std::string(PROJECT_ROOT_DIR) + "/res/shaders/spirV/gltf_vert.vert.spv";
    auto fragSpv = std::string(PROJECT_ROOT_DIR) + "/res/shaders/spirV/gltf_frag.frag.spv";

    std::vector<VkVertexInputBindingDescription> bindings = {
        vkinit::vertexInputBindingDescription(0, sizeof(vkglTF::Vertex), VK_VERTEX_INPUT_RATE_VERTEX)
    };

    std::vector<VkVertexInputAttributeDescription> attributes = {
        vkinit::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, pos)),
        vkinit::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(vkglTF::Vertex, normal)),
        vkinit::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(vkglTF::Vertex, uv)),
        vkinit::vertexInputAttributeDescription(0, 3, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vkglTF::Vertex, color)),
        vkinit::vertexInputAttributeDescription(0, 4, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(vkglTF::Vertex, tangent))
    };

    // OPAQUE
    PipelineConfigInfo opaqueConfig{};
    VkSandboxPipeline::defaultPipelineConfigInfo(opaqueConfig);
    opaqueConfig.pipelineLayout = m_pipelineLayout;
    opaqueConfig.renderPass = renderPass;
    opaqueConfig.bindingDescriptions = bindings;
    opaqueConfig.attributeDescriptions = attributes;

    m_opaquePipeline = std::make_unique<VkSandboxPipeline>(
        m_device, vertSpv, fragSpv, opaqueConfig);

    // MASK
    PipelineConfigInfo maskConfig{};
    VkSandboxPipeline::defaultPipelineConfigInfo(maskConfig);
    maskConfig.pipelineLayout = m_pipelineLayout;
    maskConfig.renderPass = renderPass;
    maskConfig.bindingDescriptions = bindings;
    maskConfig.attributeDescriptions = attributes;
    maskConfig.colorBlendAttachment.blendEnable = VK_FALSE;

    struct SpecData { VkBool32 alphaMask; float cutoff; };
    static SpecData specData{ VK_TRUE, 0.5f };
    static VkSpecializationMapEntry mapEntries[2] = {
        { 0, offsetof(SpecData, alphaMask), sizeof(VkBool32) },
        { 1, offsetof(SpecData, cutoff),    sizeof(float) }
    };
    static VkSpecializationInfo specInfo{};
    specInfo.mapEntryCount = 2;
    specInfo.pMapEntries = mapEntries;
    specInfo.dataSize = sizeof(specData);
    specInfo.pData = &specData;

    maskConfig.fragSpecInfo = &specInfo;

    m_maskPipeline = std::make_unique<VkSandboxPipeline>(
        m_device, vertSpv, fragSpv, maskConfig);

    // BLEND
    PipelineConfigInfo blendConfig{};
    VkSandboxPipeline::defaultPipelineConfigInfo(blendConfig);
    blendConfig.pipelineLayout = m_pipelineLayout;
    blendConfig.renderPass = renderPass;
    blendConfig.bindingDescriptions = bindings;
    blendConfig.attributeDescriptions = attributes;

    blendConfig.colorBlendAttachment.blendEnable = VK_TRUE;
    blendConfig.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendConfig.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendConfig.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    blendConfig.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendConfig.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendConfig.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    blendConfig.colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    m_blendPipeline = std::make_unique<VkSandboxPipeline>(
        m_device, vertSpv, fragSpv, blendConfig);
}



void GltfRenderSystem::render(FrameInfo& frame) {
    vkCmdBindDescriptorSets(
        frame.commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelineLayout,
        0, 1,
        &frame.globalDescriptorSet,
        0, nullptr);

    for (auto& [id, go] : frame.gameObjects) {
        auto baseModel = go->getModel();
        if (!baseModel) continue;

        auto model = std::dynamic_pointer_cast<vkglTF::Model>(baseModel);
        if (!model) continue;

        model->bind(frame.commandBuffer);

        for (auto* node : model->m_linearNodes) {
            if (!node->mesh) continue;

            glm::mat4 world = go->getTransform().mat4() * node->getMatrix();
            glm::mat4 normalMat = glm::transpose(glm::inverse(world));
            memcpy(node->mesh->uniformBuffer.mapped, &world, sizeof(world));
            memcpy((char*)node->mesh->uniformBuffer.mapped + sizeof(world), &normalMat, sizeof(normalMat));

            vkCmdBindDescriptorSets(
                frame.commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                m_pipelineLayout,
                1, 1,
                &node->mesh->uniformBuffer.descriptorSet,
                0, nullptr);

            const auto& mat = node->mesh->primitives[0]->material;
            switch (mat.alphaMode) {
            case vkglTF::Material::ALPHAMODE_OPAQUE:
                m_opaquePipeline->bind(frame.commandBuffer);
                break;
            case vkglTF::Material::ALPHAMODE_MASK:
                m_maskPipeline->bind(frame.commandBuffer);
                break;
            case vkglTF::Material::ALPHAMODE_BLEND:
            default:
                m_blendPipeline->bind(frame.commandBuffer);
                break;
            }

            model->drawNode(
                node,
                frame.commandBuffer,
                vkglTF::RenderFlags::BindImages,
                m_pipelineLayout,
                2 // bindImageSet
            );
        }
    }
}