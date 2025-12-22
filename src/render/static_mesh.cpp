#include "static_mesh.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include "material.h"
#include "misc/logger.h"
#include "render/rhi/buffer.h"
#include "render/rhi/command_buffer.h"

namespace Nano
{
    StaticMesh::StaticMesh() {}

    StaticMesh::~StaticMesh() noexcept { cleanup(); }

    void StaticMesh::cleanup()
    {
        m_vertex_buffer.reset();
        m_index_buffer.reset();
        m_material     = nullptr;
        m_vertex_count = 0;
        m_index_count  = 0;
    }

    void StaticMesh::getVertexInputBindings(std::vector<VkVertexInputBindingDescription>& bindings)
    {
        bindings.resize(1);
        bindings[0].binding   = 0;
        bindings[0].stride    = sizeof(Vertex);
        bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }

    void StaticMesh::getVertexInputAttributes(std::vector<VkVertexInputAttributeDescription>& attributes)
    {
        attributes.resize(4);

        attributes[0].binding  = 0;
        attributes[0].location = 0;
        attributes[0].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[0].offset   = offsetof(Vertex, position);

        attributes[1].binding  = 0;
        attributes[1].location = 1;
        attributes[1].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[1].offset   = offsetof(Vertex, texcoord);

        attributes[2].binding  = 0;
        attributes[2].location = 2;
        attributes[2].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[2].offset   = offsetof(Vertex, normal);

        attributes[3].binding  = 0;
        attributes[3].location = 3;
        attributes[3].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[3].offset   = offsetof(Vertex, tangent);
    }

    bool StaticMesh::createBuffers(const Vertex*   vertices,
                                   uint32_t        vertex_count,
                                   const uint32_t* indices,
                                   uint32_t        index_count)
    {
        if (vertices == nullptr || vertex_count == 0)
        {
            ERROR("Invalid vertex data for StaticMesh.");
            return false;
        }

        m_vertex_count = vertex_count;
        m_index_count  = index_count;

        m_vertex_buffer           = std::make_unique<Buffer>();
        size_t vertex_buffer_size = sizeof(Vertex) * vertex_count;
        if (!m_vertex_buffer->create(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                     vertex_buffer_size,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            ERROR("Failed to create vertex buffer for StaticMesh.");
            return false;
        }

        if (!m_vertex_buffer->uploadData(vertices, vertex_buffer_size))
        {
            ERROR("Failed to upload vertex data to buffer.");
            return false;
        }

        if (indices != nullptr && index_count > 0)
        {
            m_index_buffer           = std::make_unique<Buffer>();
            size_t index_buffer_size = sizeof(uint32_t) * index_count;
            if (!m_index_buffer->create(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                        index_buffer_size,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            {
                ERROR("Failed to create index buffer for StaticMesh.");
                return false;
            }

            if (!m_index_buffer->uploadData(indices, index_buffer_size))
            {
                ERROR("Failed to upload index data to buffer.");
                return false;
            }
        }

        return true;
    }

    bool StaticMesh::loadFromFile(const char* path)
    {
        FILE* file = std::fopen(path, "rb");
        if (file == nullptr)
        {
            ERROR("Failed to open mesh file: %s", path);
            return false;
        }

        uint32_t vertex_count = 0;
        if (std::fread(&vertex_count, sizeof(uint32_t), 1, file) != 1)
        {
            ERROR("Failed to read vertex count from mesh file: %s", path);
            std::fclose(file);
            return false;
        }

        if (vertex_count == 0)
        {
            ERROR("Mesh file has zero vertices: %s", path);
            std::fclose(file);
            return false;
        }

        std::vector<Vertex> vertices(vertex_count);
        size_t              read_size = std::fread(vertices.data(), sizeof(Vertex), vertex_count, file);
        if (read_size != vertex_count)
        {
            ERROR("Failed to read vertex data from mesh file: %s (read %zu/%u)", path, read_size, vertex_count);
            std::fclose(file);
            return false;
        }

        m_vertex_buffer           = std::make_unique<Buffer>();
        size_t vertex_buffer_size = sizeof(Vertex) * vertex_count;
        if (!m_vertex_buffer->create(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                     vertex_buffer_size,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            ERROR("Failed to create vertex buffer for StaticMesh.");
            std::fclose(file);
            return false;
        }

        if (!m_vertex_buffer->uploadData(vertices.data(), vertex_buffer_size))
        {
            ERROR("Failed to upload vertex data to buffer.");
            std::fclose(file);
            return false;
        }

        m_vertex_count = vertex_count;

        uint32_t              index_count = 0;
        std::vector<uint32_t> indices;

        while (!std::feof(file))
        {
            uint32_t name_length = 0;
            if (std::fread(&name_length, sizeof(uint32_t), 1, file) != 1)
            {
                if (std::feof(file))
                {
                    break;
                }
                ERROR("Failed to read submesh name length from mesh file: %s", path);
                std::fclose(file);
                return false;
            }

            if (name_length == 0 || name_length > 256)
            {
                ERROR("Invalid submesh name length in mesh file: %s", path);
                std::fclose(file);
                return false;
            }

            char submesh_name[256] = {0};
            if (std::fread(submesh_name, 1, name_length, file) != name_length)
            {
                ERROR("Failed to read submesh name from mesh file: %s", path);
                std::fclose(file);
                return false;
            }

            if (std::fread(&index_count, sizeof(uint32_t), 1, file) != 1)
            {
                ERROR("Failed to read submesh index count from mesh file: %s", path);
                std::fclose(file);
                return false;
            }

            if (index_count == 0)
            {
                continue;
            }

            std::vector<uint32_t> submesh_indices(index_count);
            if (std::fread(submesh_indices.data(), sizeof(uint32_t), index_count, file) != index_count)
            {
                ERROR("Failed to read submesh index data from mesh file: %s", path);
                std::fclose(file);
                return false;
            }

            if (indices.empty())
            {
                indices       = std::move(submesh_indices);
                m_index_count = index_count;
            }
            else
            {
                WARN("StaticMesh::loadFromFile: Multiple submeshes detected, using first one only.");
                break;
            }
        }

        std::fclose(file);

        if (!indices.empty())
        {
            m_index_buffer           = std::make_unique<Buffer>();
            size_t index_buffer_size = sizeof(uint32_t) * m_index_count;
            if (!m_index_buffer->create(VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                        index_buffer_size,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
            {
                ERROR("Failed to create index buffer for StaticMesh.");
                return false;
            }

            if (!m_index_buffer->uploadData(indices.data(), index_buffer_size))
            {
                ERROR("Failed to upload index data to buffer.");
                return false;
            }
        }

        return true;
    }

    void StaticMesh::draw(CommandBuffer* cmd_buffer)
    {
        if (cmd_buffer == nullptr)
        {
            ERROR("Cannot draw StaticMesh with null command buffer.");
            return;
        }

        if (m_vertex_buffer == nullptr)
        {
            ERROR("StaticMesh has no vertex buffer.");
            return;
        }

        VkCommandBuffer vk_cmd = cmd_buffer->getCommandBuffer();

        VkDeviceSize offsets[]  = {0};
        VkBuffer     vertex_buf = m_vertex_buffer->getBuffer();
        vkCmdBindVertexBuffers(vk_cmd, 0, 1, &vertex_buf, offsets);

        if (m_index_buffer != nullptr && m_index_count > 0)
        {
            VkBuffer index_buf = m_index_buffer->getBuffer();
            vkCmdBindIndexBuffer(vk_cmd, index_buf, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(vk_cmd, m_index_count, 1, 0, 0, 0);
        }
        else
        {
            vkCmdDraw(vk_cmd, m_vertex_count, 1, 0, 0);
        }
    }

} // namespace Nano
