#ifndef SCENE_H
#define SCENE_H

namespace Nano
{
    class Scene
    {
    public:
        Scene();
        ~Scene() noexcept;

        Scene(Scene&&) noexcept            = default;
        Scene& operator=(Scene&&) noexcept = default;
        Scene(const Scene&)                = delete;
        Scene& operator=(const Scene&)     = delete;

    private:
    };

    extern Scene g_scene;
} // namespace Nano

#endif // !SCENE_H