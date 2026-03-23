#pragma once

#include "cluster_builder.h"

#include <cstdint>
#include <string>

void EncodeBVH(const BuildResult &build, const std::string &outPath);
void EncodeNaniteMesh(const BuildResult &build, uint32_t indexCountPerCluster,
                      const std::string &outPath);
