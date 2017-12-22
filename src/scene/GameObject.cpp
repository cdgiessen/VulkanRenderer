#include "GameObject.h"

GameObject::GameObject() {}

GameObject::~GameObject() { std::cout << "game object deleted\n"; }

void GameObject::InitGameObject(std::shared_ptr<VulkanRenderer> renderer,
                                VulkanBuffer &global, VulkanBuffer &lighting)
{
    this->renderer = renderer;

    SetupUniformBuffer();
    SetupImage();
    SetupModel();
    SetupDescriptor(global, lighting);

    VulkanPipeline &pipeMan = renderer->pipelineManager;
    mvp = pipeMan.CreateManagedPipeline();
    mvp->ObjectCallBackFunction = std::make_unique<std::function<void(void)>>( std::bind(&GameObject::SetupPipeline, this));

    SetupPipeline();
}

void GameObject::CleanUp()
{
    gameObjectModel.destroy(renderer->device);
    gameObjectVulkanTexture.destroy(renderer->device);

    modelUniformBuffer.cleanBuffer();

    vkDestroyDescriptorSetLayout(renderer->device.device, descriptorSetLayout,
                                 nullptr);
    vkDestroyDescriptorPool(renderer->device.device, descriptorPool, nullptr);
}

void GameObject::LoadModel(std::string filename)
{
    gameObjectMesh = std::make_shared<Mesh>();
    // this->gameObjectMesh->importFromFile(filename);
}

void GameObject::LoadModel(std::shared_ptr<Mesh> mesh)
{
    this->gameObjectMesh = mesh;
}

void GameObject::SetupUniformBuffer()
{
    renderer->device.createBuffer(
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        (VkMemoryPropertyFlags)(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        &modelUniformBuffer, sizeof(ModelBufferObject));
}

void GameObject::SetupImage()
{
    gameObjectVulkanTexture.loadFromTexture(
        renderer->device, gameObjectTexture, VK_FORMAT_R8G8B8A8_UNORM,
        renderer->device.graphics_queue,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, false, 0);
}

void GameObject::SetupModel()
{
    gameObjectModel.loadFromMesh(gameObjectMesh, renderer->device,
                                 renderer->device.graphics_queue);

	//renderer->device.CreateUniformBuffer(gameObjectMesh->vertexCount);
}

void GameObject::SetupDescriptor(VulkanBuffer &global, VulkanBuffer &lighting)
{
    // setup layout
    VkDescriptorSetLayoutBinding cboLayoutBinding =
        initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1);
    VkDescriptorSetLayoutBinding uboLayoutBinding =
        initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1,
            1);
    VkDescriptorSetLayoutBinding lboLayoutBinding =
        initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 2,
            1);
    VkDescriptorSetLayoutBinding samplerLayoutBinding =
        initializers::descriptorSetLayoutBinding(
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            VK_SHADER_STAGE_FRAGMENT_BIT, 3, 1);

    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        cboLayoutBinding, uboLayoutBinding, lboLayoutBinding,
        samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo =
        initializers::descriptorSetLayoutCreateInfo(bindings);

    if (vkCreateDescriptorSetLayout(renderer->device.device, &layoutInfo,
                                    nullptr,
                                    &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    // setup pool
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.push_back(
        initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
    poolSizes.push_back(
        initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
    poolSizes.push_back(
        initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));
    poolSizes.push_back(initializers::descriptorPoolSize(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1));

    VkDescriptorPoolCreateInfo poolInfo =
        initializers::descriptorPoolCreateInfo(
            static_cast<uint32_t>(poolSizes.size()), poolSizes.data(), 1);

    if (vkCreateDescriptorPool(renderer->device.device, &poolInfo, nullptr,
                               &descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }

    // setup descriptor set
    VkDescriptorSetLayout layouts[] = {descriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo =
        initializers::descriptorSetAllocateInfo(descriptorPool, layouts, 1);

    if (vkAllocateDescriptorSets(renderer->device.device, &allocInfo,
                                 &descriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor set!");
    }

    modelUniformBuffer.setupDescriptor();

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.push_back(initializers::writeDescriptorSet(
        descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &global.descriptor,
        1));
    descriptorWrites.push_back(initializers::writeDescriptorSet(
        descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
        &modelUniformBuffer.descriptor, 1));
    descriptorWrites.push_back(initializers::writeDescriptorSet(
        descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2,
        &lighting.descriptor, 1));
    descriptorWrites.push_back(initializers::writeDescriptorSet(
        descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3,
        &gameObjectVulkanTexture.descriptor, 1));

    vkUpdateDescriptorSets(renderer->device.device,
                           static_cast<uint32_t>(descriptorWrites.size()),
                           descriptorWrites.data(), 0, nullptr);
}

void GameObject::SetupPipeline()
{

    VulkanPipeline &pipeMan = renderer->pipelineManager;

    pipeMan.SetVertexShader(
        mvp, loadShaderModule(renderer->device.device,
                              "assets/shaders/gameObject_shader.vert.spv"));
    pipeMan.SetFragmentShader(
        mvp, loadShaderModule(renderer->device.device,
                              "assets/shaders/gameObject_shader.frag.spv"));
    pipeMan.SetVertexInput(mvp, Vertex::getBindingDescription(),
                           Vertex::getAttributeDescriptions());
    pipeMan.SetInputAssembly(mvp, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0,
                             VK_FALSE);
    pipeMan.SetViewport(mvp, (float)renderer->vulkanSwapChain.swapChainExtent.width,
		(float)renderer->vulkanSwapChain.swapChainExtent.height, 0.0f,
                        1.0f, 0.0f, 0.0f);
    pipeMan.SetScissor(mvp, renderer->vulkanSwapChain.swapChainExtent.width,
                       renderer->vulkanSwapChain.swapChainExtent.height, 0, 0);
    pipeMan.SetViewportState(mvp, 1, 1, 0);
    pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT,
                          VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE,
                          1.0f, VK_TRUE);
    pipeMan.SetMultisampling(mvp, VK_SAMPLE_COUNT_1_BIT);
    pipeMan.SetDepthStencil(mvp, VK_TRUE, VK_TRUE, VK_COMPARE_OP_GREATER,
                            VK_FALSE, VK_FALSE);
    pipeMan.SetColorBlendingAttachment(
        mvp, VK_FALSE,
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_COLOR,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, VK_BLEND_OP_ADD,
        VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO);
    pipeMan.SetColorBlending(mvp, 1, &mvp->pco.colorBlendAttachment);
    pipeMan.SetDescriptorSetLayout(mvp, {&descriptorSetLayout}, 1);

    pipeMan.BuildPipelineLayout(mvp);
    pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);

    pipeMan.SetRasterizer(mvp, VK_POLYGON_MODE_LINE, VK_CULL_MODE_NONE,
                          VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, VK_FALSE,
                          1.0f, VK_TRUE);
    pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);

    pipeMan.CleanShaderResources(mvp);
    pipeMan.SetVertexShader(
        mvp, loadShaderModule(renderer->device.device,
                              "assets/shaders/normalVecDebug.vert.spv"));
    pipeMan.SetFragmentShader(
        mvp, loadShaderModule(renderer->device.device,
                              "assets/shaders/normalVecDebug.frag.spv"));
    pipeMan.SetGeometryShader(
        mvp, loadShaderModule(renderer->device.device,
                              "assets/shaders/normalVecDebug.geom.spv"));

    pipeMan.BuildPipeline(mvp, renderer->renderPass, 0);
    pipeMan.CleanShaderResources(mvp);
}

void GameObject::UpdateUniformBuffer(float time)
{
    ModelBufferObject ubo = {};
    ubo.model = glm::mat4();
    // ubo.model = glm::translate(ubo.model, glm::vec3(50, 0, 0));
    ubo.model = glm::rotate(ubo.model, time / 2.0f, glm::vec3(0.5, 1, 0));
    ubo.normal = glm::transpose(glm::inverse(glm::mat3(ubo.model)));

    modelUniformBuffer.map(renderer->device.device);
    modelUniformBuffer.copyTo(&ubo, sizeof(ubo));
    modelUniformBuffer.unmap();
}
