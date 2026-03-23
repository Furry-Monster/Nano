#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// Exporter CLI options. indexCount = trianglesPerCluster*3, must match
/// Init.glsl vertexCount. maxClustersPerMip fits into BVH NumChildren (9 bits).
struct ExporterOptions {
  std::string inputPath;
  std::string outBvhPath;
  std::string outNaniteMeshPath;

  std::vector<int> mipValues;
  uint32_t trianglesPerCluster = 128;
  uint32_t indexCount = 384;
  uint32_t maxClustersPerMip = 511;
  float targetExtent = 500.0f;
};

ExporterOptions ParseArgs(int argc, char **argv);
void Usage(const char *exe);
