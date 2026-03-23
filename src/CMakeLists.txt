set(NANO_SOURCES
    main.cpp

    input/input.cpp

    math/float4.cpp
    math/matrix4.cpp
    math/quaternion.cpp

    render/vulkan_rhi.cpp
    render/render_pass.cpp
    render/material.cpp
    render/static_mesh.cpp

    scene/scene.cpp
    scene/scene_node.cpp
)

add_executable(Nano ${NANO_SOURCES})

target_include_directories(Nano PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Nano PRIVATE reflibs)

add_dependencies(Nano copy_resources)

# Command-line model exporter:
add_executable(NaniteExporter
    exporter/main.cpp
    exporter/args.cpp
    exporter/mesh_loader.cpp
    exporter/cluster_builder.cpp
    exporter/nanite_encode.cpp
)

target_include_directories(NaniteExporter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(NaniteExporter PRIVATE assimp::assimp meshoptimizer)
