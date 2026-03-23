#include "mesh_loader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

constexpr float kEpsilon = 1e-9f;

uint32_t GetAssimpFlags(const std::string &path) {
  std::string lower = path;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  const bool isGlb = lower.size() >= 4 && lower.substr(lower.size() - 4) == ".glb";
  const bool isGltf = lower.size() >= 5 && lower.substr(lower.size() - 5) == ".gltf";
  const bool isFbx = lower.size() >= 4 && lower.substr(lower.size() - 4) == ".fbx";

  uint32_t flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                   aiProcess_GenNormals |  // Ensure normals if missing
                   aiProcess_ImproveCacheLocality | aiProcess_OptimizeMeshes;

  if (isGlb || isGltf) {
    // GLTF/GLB: pre-transform for single-mesh output
    flags |= aiProcess_PreTransformVertices;
  }
  if (isFbx) {
    flags |= aiProcess_PreTransformVertices;
  }

  return flags;
}

}  // namespace

LoadedMesh LoadMesh(const std::string &inputPath, float targetExtent) {
  Assimp::Importer importer;
  const uint32_t flags = GetAssimpFlags(inputPath);
  const aiScene *scene =
      importer.ReadFile(inputPath, flags);

  if (!scene || !scene->mRootNode) {
    throw std::runtime_error("Assimp failed: " +
                             std::string(importer.GetErrorString()));
  }
  if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
    throw std::runtime_error("Assimp scene incomplete");
  }

  LoadedMesh out;
  out.positions.reserve(256 * 1024);
  out.triangles.reserve(256 * 1024);

  for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
    const aiMesh *mesh = scene->mMeshes[mi];
    if (!mesh || !mesh->HasFaces()) continue;

    const uint32_t baseVertex = static_cast<uint32_t>(out.positions.size());
    const unsigned int vCount = mesh->mNumVertices;

    for (unsigned int vi = 0; vi < vCount; ++vi) {
      const aiVector3D &p = mesh->mVertices[vi];
      out.positions.push_back({p.x, p.y, p.z});
    }

    for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi) {
      const aiFace &face = mesh->mFaces[fi];
      if (face.mNumIndices != 3) continue;
      out.triangles.push_back(
          {baseVertex + static_cast<uint32_t>(face.mIndices[0]),
           baseVertex + static_cast<uint32_t>(face.mIndices[1]),
           baseVertex + static_cast<uint32_t>(face.mIndices[2])});
    }
  }

  if (out.triangles.empty() || out.positions.empty()) {
    throw std::runtime_error("No geometry: empty triangles or vertices");
  }

  // Bounds and normalize to target extent
  Vec3 bmin{std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::infinity()};
  Vec3 bmax{-std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity()};

  for (const auto &p : out.positions) {
    bmin.x = std::min(bmin.x, p.x);
    bmin.y = std::min(bmin.y, p.y);
    bmin.z = std::min(bmin.z, p.z);
    bmax.x = std::max(bmax.x, p.x);
    bmax.y = std::max(bmax.y, p.y);
    bmax.z = std::max(bmax.z, p.z);
  }

  Vec3 center{(bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f,
              (bmin.z + bmax.z) * 0.5f};
  float extent =
      std::max({bmax.x - bmin.x, bmax.y - bmin.y, bmax.z - bmin.z});
  if (extent < kEpsilon) extent = 1.0f;
  const float scale = targetExtent / extent;

  for (auto &p : out.positions) {
    p.x = (p.x - center.x) * scale;
    p.y = (p.y - center.y) * scale;
    p.z = (p.z - center.z) * scale;
  }

  return out;
}
