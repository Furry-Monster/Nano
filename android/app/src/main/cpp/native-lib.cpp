#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/android_sink.h>

#include "render/vulkan_rhi.h"
#include "scene/scene.h"
#include "input/input.h"

#include <sstream>

static ANativeWindow *sNativeWindow = nullptr;
static std::atomic<bool> sRunning{false};
static std::atomic<bool> sInitialized{false};
static std::thread sRenderThread;
static std::mutex sWindowMutex;
static int sSurfaceWidth = 0;
static int sSurfaceHeight = 0;

static const char *kDefaultBvh = "res/Mitsuba/mitsuba.bvh";
static const char *kDefaultMesh = "res/Mitsuba/mitsuba.nanomesh";

static std::string sBvhPath = kDefaultBvh;
static std::string sMeshPath = kDefaultMesh;

static void InitSpdlogAndroid() {
    auto sink = std::make_shared<spdlog::sinks::android_sink_mt>("Nano");
    auto logger = std::make_shared<spdlog::logger>("nano", sink);
    logger->set_level(spdlog::level::info);
    spdlog::set_default_logger(logger);
}

static void RenderThreadFunc() {
    spdlog::info("Render thread started: {}x{}", sSurfaceWidth, sSurfaceHeight);

    {
        std::lock_guard<std::mutex> lock(sWindowMutex);
        if (!sNativeWindow) {
            spdlog::error("No native window available");
            return;
        }

        if (!InitVulkan(sNativeWindow, sSurfaceWidth, sSurfaceHeight)) {
            spdlog::error("Failed to initialize Vulkan");
            return;
        }
    }

    try {
        InitScene(sSurfaceWidth, sSurfaceHeight, sBvhPath, sMeshPath);
    } catch (const std::exception &e) {
        spdlog::error("InitScene failed: {}", e.what());
        return;
    }

    sInitialized.store(true);

    auto lastTime = std::chrono::high_resolution_clock::now();

    while (sRunning.load()) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime =
            std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        InputPollAndroid(deltaTime);
        RenderOneFrame(deltaTime);
    }

    vkDeviceWaitIdle(GetVulkanDevice());
    sInitialized.store(false);
    spdlog::info("Render thread stopped");
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_example_nano_1android_NanoRenderer_nativeInit(
    JNIEnv *env, jobject, jobject assetManager) {
    InitSpdlogAndroid();
    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    SetAndroidAssetManager(mgr);
    InputInitFromLookAt(-330.0f, 330.0f, -330.0f, 0.0f, 80.0f, 0.0f);
    spdlog::info("Native init complete");
}

JNIEXPORT void JNICALL
Java_com_example_nano_1android_NanoRenderer_nativeSetModelPaths(
    JNIEnv *env, jobject, jstring bvhPath, jstring meshPath) {
    if (bvhPath && meshPath) {
        const char *bvh = env->GetStringUTFChars(bvhPath, nullptr);
        const char *mesh = env->GetStringUTFChars(meshPath, nullptr);
        if (bvh && mesh) {
            sBvhPath = bvh;
            sMeshPath = mesh;
            env->ReleaseStringUTFChars(bvhPath, bvh);
            env->ReleaseStringUTFChars(meshPath, mesh);
        }
    }
}

JNIEXPORT void JNICALL
Java_com_example_nano_1android_NanoRenderer_nativeSurfaceCreated(
    JNIEnv *env, jobject, jobject surface, jint width, jint height) {
    std::lock_guard<std::mutex> lock(sWindowMutex);

    if (sNativeWindow) {
        ANativeWindow_release(sNativeWindow);
    }
    sNativeWindow = ANativeWindow_fromSurface(env, surface);
    sSurfaceWidth = width;
    sSurfaceHeight = height;

    if (!sRunning.load()) {
        sRunning.store(true);
        sRenderThread = std::thread(RenderThreadFunc);
    }
}

JNIEXPORT void JNICALL
Java_com_example_nano_1android_NanoRenderer_nativeSurfaceDestroyed(
    JNIEnv *, jobject) {
    sRunning.store(false);
    if (sRenderThread.joinable()) {
        sRenderThread.join();
    }

    std::lock_guard<std::mutex> lock(sWindowMutex);
    if (sNativeWindow) {
        ANativeWindow_release(sNativeWindow);
        sNativeWindow = nullptr;
    }
}

JNIEXPORT void JNICALL
Java_com_example_nano_1android_NanoRenderer_nativeOnTouch(
    JNIEnv *, jobject, jint action, jint pointerCount, jfloat x, jfloat y) {
    InputOnTouchEvent(static_cast<int>(action), static_cast<int>(pointerCount),
                      static_cast<float>(x), static_cast<float>(y));
}

JNIEXPORT void JNICALL
Java_com_example_nano_1android_NanoRenderer_nativeDestroy(JNIEnv *, jobject) {
    sRunning.store(false);
    if (sRenderThread.joinable()) {
        sRenderThread.join();
    }
    spdlog::info("Native destroy complete");
}

JNIEXPORT void JNICALL
Java_com_example_nano_1android_NanoRenderer_nativeToggleAutoLOD(JNIEnv *, jobject) {
    if (sInitialized.load()) SceneToggleAutoLOD();
}

JNIEXPORT void JNICALL
Java_com_example_nano_1android_NanoRenderer_nativeLODUp(JNIEnv *, jobject) {
    if (sInitialized.load()) SceneLODUp();
}

JNIEXPORT void JNICALL
Java_com_example_nano_1android_NanoRenderer_nativeLODDown(JNIEnv *, jobject) {
    if (sInitialized.load()) SceneLODDown();
}

JNIEXPORT jstring JNICALL
Java_com_example_nano_1android_NanoRenderer_nativeGetStats(JNIEnv *env, jobject) {
    if (!sInitialized.load()) {
        return env->NewStringUTF("Initializing...");
    }
    SceneStats stats = SceneGetStats();
    std::ostringstream oss;
    oss << "FPS: " << static_cast<int>(stats.fps + 0.5f)
        << "\nLOD: " << (stats.autoLod ? "auto" : "manual") << " (" << stats.lodMipValue << ")"
        << "\nCam: " << static_cast<int>(stats.camX) << "," << static_cast<int>(stats.camY) << "," << static_cast<int>(stats.camZ);
    return env->NewStringUTF(oss.str().c_str());
}

}
