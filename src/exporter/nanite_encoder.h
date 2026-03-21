#pragma once

#include "common.h"

#include <cstdint>
#include <string>
#include <vector>

std::vector<ExportPage> BuildPages(const std::vector<Vec3> &positions,
                                   const std::vector<Triangle> &triangles,
                                   const std::vector<int> &mipValues,
                                   uint32_t trianglesPerCluster,
                                   uint32_t indexCount,
                                   uint32_t maxClustersPerMip);

void EncodeNaniteMesh(const std::vector<ExportPage> &pages, uint32_t indexCount,
                      const std::string &outPath);

void EncodeBVH(const std::vector<ExportPage> &pages,
               const std::vector<int> &mipValues, const std::string &outPath);
