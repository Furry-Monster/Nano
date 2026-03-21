#include "assimp_loader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

LoadedMesh LoadMeshAssimp(const std::string &inputPath, float targetExtent) {
  Assimp::Importer importer;
  const aiScene *scene = importer.ReadFile(
      inputPath, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                     aiProcess_OptimizeMeshes | aiProcess_ImproveCacheLocality |
                     aiProcess_PreTransformVertices);
  if (!scene || !scene->mRootNode ||
      (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0) {
    throw std::runtime_error(std::string("Assimp failed: ") +
                             importer.GetErrorString());
  }

  LoadedMesh out;
  out.positions.reserve(200000);
  out.triangles.reserve(200000);

  for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
    const aiMesh *mesh = scene->mMeshes[mi];
    if (!mesh)
      continue;

    const uint32_t baseVertex = static_cast<uint32_t>(out.positions.size());
    const unsigned int vCount = mesh->mNumVertices;
    for (unsigned int vi = 0; vi < vCount; ++vi) {
      const aiVector3D &p = mesh->mVertices[vi];
      out.positions.push_back({p.x, p.y, p.z});
    }

    for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi) {
      const aiFace &face = mesh->mFaces[fi];
      if (face.mNumIndices != 3)
        continue;
      out.triangles.push_back(
          {baseVertex + static_cast<uint32_t>(face.mIndices[0]),
           baseVertex + static_cast<uint32_t>(face.mIndices[1]),
           baseVertex + static_cast<uint32_t>(face.mIndices[2])});
    }
  }

  if (out.triangles.empty() || out.positions.empty()) {
    throw std::runtime_error(
        "Empty geometry: no triangles/vertices extracted.");
  }

  // Normalize to a reasonable scale so the demo camera can see it.
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
  float extent = std::max({bmax.x - bmin.x, bmax.y - bmin.y, bmax.z - bmin.z});
  if (extent <= 1e-12f)
    extent = 1.0f;
  float s = targetExtent / extent;

  for (auto &p : out.positions) {
    p.x = (p.x - center.x) * s;
    p.y = (p.y - center.y) * s;
    p.z = (p.z - center.z) * s;
  }

  return out;
}
