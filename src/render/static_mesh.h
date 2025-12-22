#ifndef STATIC_MESH_H
#define STATIC_MESH_H

#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace Nano
{
    class Buffer;
    class Material;
    class CommandBuffer;

    struct Vertex
    {
        float position[4];
        float texcoord[4];
        float normal[4];
        float tangent[4];
    };

    struct SubMesh
    {
        std::unique_ptr<Buffer> index_buffer;
        uint32_t                index_count {0};
    };

    class StaticMesh
    {
    public:
        StaticMesh();
        ~StaticMesh() noexcept;

        StaticMesh(const StaticMesh&)                = delete;
        StaticMesh& operator=(const StaticMesh&)     = delete;
        StaticMesh(StaticMesh&&) noexcept            = delete;
        StaticMesh& operator=(StaticMesh&&) noexcept = delete;

        bool loadFromFile(const char* path);
        bool
        createBuffers(const Vertex* vertices, uint32_t vertex_count, const uint32_t* indices, uint32_t index_count);

        void setMaterial(Material* material) { m_material = material; }
        void draw(CommandBuffer* cmd_buffer);

        static void getVertexInputBindings(std::vector<VkVertexInputBindingDescription>& bindings);
        static void getVertexInputAttributes(std::vector<VkVertexInputAttributeDescription>& attributes);

        uint32_t  getVertexCount() const { return m_vertex_count; }
        uint32_t  getIndexCount() const { return m_index_count; }
        Buffer*   getVertexBuffer() { return m_vertex_buffer.get(); }
        Buffer*   getIndexBuffer() { return m_index_buffer.get(); }
        Material* getMaterial() { return m_material; }

    private:
        void cleanup();

        std::unique_ptr<Buffer> m_vertex_buffer;
        std::unique_ptr<Buffer> m_index_buffer;
        Material*               m_material {nullptr};
        uint32_t                m_vertex_count {0};
        uint32_t                m_index_count {0};
    };

} // namespace Nano

#endif // !STATIC_MESH_H
