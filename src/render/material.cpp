#include "material.h"
#include <cstring>
#include "misc/logger.h"
#include "render/rhi/buffer.h"
#include "render/rhi/command_buffer.h"
#include "render/rhi/descriptor_set.h"
#include "render/rhi/pipeline.h"
#include "render/rhi/shader.h"
#include "render/rhi/texture.h"

namespace Nano
{
    Material::Material() :
        m_primitive_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST), m_is_initialized(false), m_pipeline_created(false)
    {}

    Material::~Material() noexcept { cleanup(); }

    void Material::cleanup()
    {
        m_pipeline.reset();
        m_descriptor_set.reset();
        m_descriptor_set_layout.reset();
        m_vertex_shader.reset();
        m_fragment_shader.reset();
        m_geometry_shader.reset();
        m_tessellation_control_shader.reset();
        m_tessellation_evaluation_shader.reset();

        m_is_initialized   = false;
        m_pipeline_created = false;
        m_vertex_bindings.clear();
        m_vertex_attributes.clear();
    }

    bool Material::createDescriptorSetLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings(4);

        // Binding 0: Uniform Buffer (Vertex/Geometry)
        bindings[0].binding            = 0;
        bindings[0].descriptorCount    = 1;
        bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        // Binding 1: Uniform Buffer (Vertex)
        bindings[1].binding            = 1;
        bindings[1].descriptorCount    = 1;
        bindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[1].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        // Binding 2: Combined Image Sampler (Fragment)
        bindings[2].binding            = 2;
        bindings[2].descriptorCount    = 1;
        bindings[2].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[2].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[2].pImmutableSamplers = nullptr;

        // Binding 3: Combined Image Sampler - Cube (Fragment)
        bindings[3].binding            = 3;
        bindings[3].descriptorCount    = 1;
        bindings[3].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[3].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[3].pImmutableSamplers = nullptr;

        m_descriptor_set_layout = std::make_unique<DescriptorSetLayout>();
        if (!m_descriptor_set_layout->create(bindings))
        {
            ERROR("Failed to create descriptor set layout for material.");
            return false;
        }

        std::vector<VkDescriptorPoolSize> pool_sizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32},
        };

        m_descriptor_set = std::make_unique<DescriptorSet>();
        if (!m_descriptor_set->allocate(m_descriptor_set_layout->getLayout(), pool_sizes))
        {
            ERROR("Failed to allocate descriptor set for material.");
            return false;
        }

        return true;
    }

    bool Material::init(const char* vertex_shader_path, const char* fragment_shader_path)
    {
        if (m_is_initialized)
        {
            WARN("Material already initialized.");
            return false;
        }

        m_vertex_shader = std::make_unique<Shader>();
        if (!m_vertex_shader->loadFromFile(vertex_shader_path))
        {
            ERROR("Failed to load vertex shader: %s", vertex_shader_path);
            return false;
        }

        m_fragment_shader = std::make_unique<Shader>();
        if (!m_fragment_shader->loadFromFile(fragment_shader_path))
        {
            ERROR("Failed to load fragment shader: %s", fragment_shader_path);
            return false;
        }

        if (!createDescriptorSetLayout())
        {
            return false;
        }

        m_is_initialized = true;
        return true;
    }

    bool Material::initVGF(const char* vertex_shader_path,
                           const char* geometry_shader_path,
                           const char* fragment_shader_path)
    {
        if (m_is_initialized)
        {
            WARN("Material already initialized.");
            return false;
        }

        m_vertex_shader = std::make_unique<Shader>();
        if (!m_vertex_shader->loadFromFile(vertex_shader_path))
        {
            ERROR("Failed to load vertex shader: %s", vertex_shader_path);
            return false;
        }

        m_geometry_shader = std::make_unique<Shader>();
        if (!m_geometry_shader->loadFromFile(geometry_shader_path))
        {
            ERROR("Failed to load geometry shader: %s", geometry_shader_path);
            return false;
        }

        m_fragment_shader = std::make_unique<Shader>();
        if (!m_fragment_shader->loadFromFile(fragment_shader_path))
        {
            ERROR("Failed to load fragment shader: %s", fragment_shader_path);
            return false;
        }

        if (!createDescriptorSetLayout())
        {
            return false;
        }

        m_is_initialized = true;
        return true;
    }

    bool Material::initVTF(const char* vertex_shader_path,
                           const char* tessellation_control_shader_path,
                           const char* tessellation_evaluation_shader_path,
                           const char* fragment_shader_path)
    {
        if (m_is_initialized)
        {
            WARN("Material already initialized.");
            return false;
        }

        m_vertex_shader = std::make_unique<Shader>();
        if (!m_vertex_shader->loadFromFile(vertex_shader_path))
        {
            ERROR("Failed to load vertex shader: %s", vertex_shader_path);
            return false;
        }

        m_tessellation_control_shader = std::make_unique<Shader>();
        if (!m_tessellation_control_shader->loadFromFile(tessellation_control_shader_path))
        {
            ERROR("Failed to load tessellation control shader: %s", tessellation_control_shader_path);
            return false;
        }

        m_tessellation_evaluation_shader = std::make_unique<Shader>();
        if (!m_tessellation_evaluation_shader->loadFromFile(tessellation_evaluation_shader_path))
        {
            ERROR("Failed to load tessellation evaluation shader: %s", tessellation_evaluation_shader_path);
            return false;
        }

        m_fragment_shader = std::make_unique<Shader>();
        if (!m_fragment_shader->loadFromFile(fragment_shader_path))
        {
            ERROR("Failed to load fragment shader: %s", fragment_shader_path);
            return false;
        }

        if (!createDescriptorSetLayout())
        {
            return false;
        }

        m_is_initialized = true;
        return true;
    }

    void Material::setVertexInput(const std::vector<VkVertexInputBindingDescription>&   bindings,
                                  const std::vector<VkVertexInputAttributeDescription>& attributes)
    {
        m_vertex_bindings   = bindings;
        m_vertex_attributes = attributes;
    }

    void Material::setPrimitiveTopology(VkPrimitiveTopology topology) { m_primitive_topology = topology; }

    void Material::setViewport(const VkViewport& viewport) { m_viewport = viewport; }

    void Material::setScissor(const VkRect2D& scissor) { m_scissor = scissor; }

    bool Material::setUniformBuffer(uint32_t binding, Buffer* buffer)
    {
        if (!m_is_initialized || !m_descriptor_set)
        {
            ERROR("Material not initialized or descriptor set not created.");
            return false;
        }

        if (buffer == nullptr)
        {
            ERROR("Cannot set null buffer to material.");
            return false;
        }

        return m_descriptor_set->updateBuffer(binding, buffer, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    }

    bool Material::setTexture(uint32_t binding, Texture* texture, VkSampler sampler)
    {
        if (!m_is_initialized || !m_descriptor_set)
        {
            ERROR("Material not initialized or descriptor set not created.");
            return false;
        }

        if (texture == nullptr)
        {
            ERROR("Cannot set null texture to material.");
            return false;
        }

        return m_descriptor_set->updateTexture(binding, texture, sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    bool Material::createPipeline(VkRenderPass render_pass)
    {
        if (m_pipeline_created)
        {
            return true;
        }

        if (!m_is_initialized)
        {
            ERROR("Material not initialized. Call init() first.");
            return false;
        }

        if (m_vertex_bindings.empty() || m_vertex_attributes.empty())
        {
            ERROR("Vertex input not set. Call setVertexInput() first.");
            return false;
        }

        m_pipeline = std::make_unique<Pipeline>();

        GraphicsPipelineCreateInfo pipeline_info = {};
        pipeline_info.render_pass                = render_pass;
        pipeline_info.vertex_shader              = m_vertex_shader->getModule();
        pipeline_info.fragment_shader            = m_fragment_shader->getModule();

        if (m_geometry_shader)
        {
            pipeline_info.geometry_shader = m_geometry_shader->getModule();
        }

        if (m_tessellation_control_shader && m_tessellation_evaluation_shader)
        {
            pipeline_info.tessellation_control_shader    = m_tessellation_control_shader->getModule();
            pipeline_info.tessellation_evaluation_shader = m_tessellation_evaluation_shader->getModule();
            pipeline_info.patch_control_points           = 4;
        }

        pipeline_info.vertex_bindings       = m_vertex_bindings;
        pipeline_info.vertex_attributes     = m_vertex_attributes;
        pipeline_info.primitive_topology    = m_primitive_topology;
        pipeline_info.viewport              = m_viewport;
        pipeline_info.scissor               = m_scissor;
        pipeline_info.descriptor_set_layout = m_descriptor_set_layout->getLayout();

        if (!m_pipeline->createGraphicsPipeline(pipeline_info))
        {
            ERROR("Failed to create graphics pipeline for material.");
            return false;
        }

        m_pipeline_created = true;
        return true;
    }

    bool Material::bind(CommandBuffer* cmd_buffer, VkRenderPass render_pass)
    {
        if (!m_is_initialized)
        {
            ERROR("Material not initialized. Call init() first.");
            return false;
        }

        if (cmd_buffer == nullptr)
        {
            ERROR("Cannot bind material to null command buffer.");
            return false;
        }

        if (!m_pipeline_created)
        {
            if (!createPipeline(render_pass))
            {
                return false;
            }
        }

        VkCommandBuffer vk_cmd = cmd_buffer->getCommandBuffer();
        vkCmdBindPipeline(vk_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->getPipeline());

        VkPipelineLayout pipeline_layout = m_pipeline->getLayout();
        VkDescriptorSet  descriptor_set  = m_descriptor_set->getDescriptorSet();

        vkCmdBindDescriptorSets(
            vk_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);

        return true;
    }

    VkPipelineLayout Material::getPipelineLayout() const
    {
        if (!m_pipeline)
        {
            return VK_NULL_HANDLE;
        }
        return m_pipeline->getLayout();
    }

} // namespace Nano
