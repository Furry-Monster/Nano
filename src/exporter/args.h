#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct ExporterOptions {
  std::string inputPath;
  std::string outBvhPath;
  std::string outNaniteMeshPath;

  std::vector<int> mipValues;
  uint32_t trianglesPerCluster = 128; // indexCount = trianglesPerCluster*3
  uint32_t indexCount = 384;          // must match Init.glsl vertexCount
  uint32_t maxClustersPerMip = 511;   // fits into NumChildren (9 bits)
  float targetExtent = 500.0f;
};

ExporterOptions ParseArgs(int argc, char **argv);
void Usage(const char *exe);
