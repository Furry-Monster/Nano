#include "mesh_loader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

constexpr float kEpsilon = 1e-9f;

std::string ToLower(const std::string &s) {
  std::string out = s;
  std::transform(out.begin(), out.end(), out.begin(), ::tolower);
  return out;
}

bool EndsWith(const std::string &s, const std::string &suffix) {
  if (suffix.size() > s.size())
    return false;
  return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

uint32_t GetAssimpFlags(const std::string &path) {
  const std::string lower = ToLower(path);
  uint32_t flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                   aiProcess_ImproveCacheLocality | aiProcess_OptimizeMeshes |
                   aiProcess_PreTransformVertices;

  if (EndsWith(lower, ".fbx")) {
    flags |= aiProcess_GenNormals;
  } else if (EndsWith(lower, ".gltf") || EndsWith(lower, ".glb")) {
    flags |= aiProcess_GenNormals;
  } else {
    flags |= aiProcess_GenNormals;
  }

  return flags;
}

// Triangle area via cross product. Returns 0 for degenerate triangles.
float TriangleArea(const Vec3 &a, const Vec3 &b, const Vec3 &c) {
  Vec3 ab = b - a;
  Vec3 ac = c - a;
  Vec3 cross{ab.y * ac.z - ab.z * ac.y, ab.z * ac.x - ab.x * ac.z,
             ab.x * ac.y - ab.y * ac.x};
  return 0.5f * cross.length();
}

} // namespace

LoadedMesh LoadMesh(const std::string &inputPath, float targetExtent) {
  Assimp::Importer importer;
  const uint32_t flags = GetAssimpFlags(inputPath);
  const aiScene *scene = importer.ReadFile(inputPath, flags);

  if (!scene || !scene->mRootNode)
    throw std::runtime_error("Assimp failed: " +
                             std::string(importer.GetErrorString()));
  if (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    throw std::runtime_error("Assimp scene incomplete");

  LoadedMesh out;
  out.positions.reserve(256 * 1024);
  out.triangles.reserve(256 * 1024);

  for (unsigned int mi = 0; mi < scene->mNumMeshes; ++mi) {
    const aiMesh *mesh = scene->mMeshes[mi];
    if (!mesh || !mesh->HasFaces())
      continue;

    const uint32_t baseVertex = static_cast<uint32_t>(out.positions.size());

    for (unsigned int vi = 0; vi < mesh->mNumVertices; ++vi) {
      const aiVector3D &p = mesh->mVertices[vi];
      out.positions.push_back({p.x, p.y, p.z});
    }

    for (unsigned int fi = 0; fi < mesh->mNumFaces; ++fi) {
      const aiFace &face = mesh->mFaces[fi];
      if (face.mNumIndices != 3)
        continue;
      out.triangles.push_back({baseVertex + face.mIndices[0],
                               baseVertex + face.mIndices[1],
                               baseVertex + face.mIndices[2]});
    }
  }

  if (out.positions.empty() || out.triangles.empty())
    throw std::runtime_error("No geometry found in " + inputPath);

  // Remove degenerate triangles (zero-area or duplicate-vertex).
  {
    size_t before = out.triangles.size();
    out.triangles.erase(
        std::remove_if(out.triangles.begin(), out.triangles.end(),
                       [&](const Triangle &t) {
                         if (t.a == t.b || t.b == t.c || t.a == t.c)
                           return true;
                         float area = TriangleArea(out.positions[t.a],
                                                   out.positions[t.b],
                                                   out.positions[t.c]);
                         return area < 1e-12f;
                       }),
        out.triangles.end());
    size_t removed = before - out.triangles.size();
    if (removed > 0)
      std::cout << "  Removed " << removed << " degenerate triangles\n";
  }

  if (out.triangles.empty())
    throw std::runtime_error("All triangles degenerate in " + inputPath);

  // Center and scale to targetExtent.
  AABB box;
  for (const auto &p : out.positions)
    box.expand(p);

  Vec3 center = box.center();
  float extent = std::max({box.hi.x - box.lo.x, box.hi.y - box.lo.y,
                           box.hi.z - box.lo.z, kEpsilon});
  float scale = targetExtent / extent;

  for (auto &p : out.positions) {
    p.x = (p.x - center.x) * scale;
    p.y = (p.y - center.y) * scale;
    p.z = (p.z - center.z) * scale;
  }

  std::cout << "  Mesh loaded: " << out.positions.size() << " vertices, "
            << out.triangles.size() << " triangles\n";
  std::cout << "  Scaled to extent " << targetExtent << " (original " << extent
            << ")\n";

  return out;
}
