message(STATUS "Building libraries...")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libs" FORCE)

if(NOT TARGET glfw)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "..." FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "..." FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "..." FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "..." FORCE)
    set(GLFW_VULKAN_STATIC ON CACHE BOOL "..." FORCE)
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

if(NOT TARGET spdlog)
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "..." FORCE)
    set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "..." FORCE)
    set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "..." FORCE)
    set(SPDLOG_INSTALL OFF CACHE BOOL "..." FORCE)
    add_subdirectory(spdlog)
endif()

if(UNIX AND NOT APPLE)
    find_package(Vulkan REQUIRED)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
elseif(APPLE)
    find_package(Vulkan REQUIRED)
endif()

target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)
target_include_directories(imgui PUBLIC ${Vulkan_INCLUDE_DIRS})

add_library(reflibs INTERFACE)
target_link_libraries(reflibs INTERFACE stb glfw glm imgui spdlog Vulkan::Vulkan)
target_include_directories(reflibs INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/glfw/include
    ${Vulkan_INCLUDE_DIRS}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(reflibs INTERFACE
        ${X11_LIBRARIES}
        Threads::Threads
    )
elseif(APPLE)
    target_link_libraries(reflibs INTERFACE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework QuartzCore"
    )
endif()
