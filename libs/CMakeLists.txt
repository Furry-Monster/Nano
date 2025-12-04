message(STATUS "Building libraries...")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)

if(NOT TARGET glfw)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "..." FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "..." FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "..." FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "..." FORCE)
    add_subdirectory(glfw)
endif()

if(NOT TARGET glm)
    add_library(glm INTERFACE)
    target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm)
endif()

if(NOT TARGET imgui)
    set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
    file(GLOB IMGUI_SOURCE
        "${IMGUI_DIR}/*.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
    )
    add_library(imgui STATIC ${IMGUI_SOURCE})
    target_include_directories(imgui PUBLIC
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC glfw)
endif()

if(NOT TARGET stb)
    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb)
endif()

add_library(reflibs INTERFACE)
target_link_libraries(reflibs INTERFACE stb glfw glm imgui)

if(UNIX AND NOT APPLE)
    find_package(Vulkan REQUIRED)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)

    target_link_libraries(reflibs INTERFACE
        Vulkan::Vulkan
        ${X11_LIBRARIES}
        Threads::Threads
    )

    target_include_directories(reflibs INTERFACE
        ${Vulkan_INCLUDE_DIRS}
    )
elseif(WIN32)
    find_package(Vulkan REQUIRED)
    target_link_libraries(reflibs INTERFACE
        Vulkan::Vulkan
    )
elseif(APPLE)
    find_package(Vulkan REQUIRED)
    target_link_libraries(reflibs INTERFACE
        Vulkan::Vulkan
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework QuartzCore"
    )
endif()
