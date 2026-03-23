#include "args.h"
#include "cluster_builder.h"
#include "mesh_loader.h"
#include "nanite_encode.h"

#include <iostream>

int main(int argc, char **argv) {
  try {
    const ExporterOptions opt = ParseArgs(argc, argv);

    std::cout << "=== NaniteExporter ===\n";
    std::cout << "Input: " << opt.inputPath << "\n";

    const auto mesh = LoadMesh(opt.inputPath, opt.targetExtent);

    const auto build = BuildClustersAndPages(
        mesh, opt.mipValues, opt.trianglesPerCluster, opt.maxClustersPerMip);

    std::cout << "\nBuild summary: " << build.pages.size() << " pages\n";
    uint32_t totalClusters = 0;
    for (const auto &page : build.pages) {
      uint32_t nc = static_cast<uint32_t>(page.clusters.size());
      std::cout << "  mip " << page.mipLevel << ": " << nc << " clusters";
      if (!page.clusters.empty()) {
        std::cout << "  bounds [" << page.bounds.lo.x << "," << page.bounds.lo.y
                  << "," << page.bounds.lo.z << "] - [" << page.bounds.hi.x
                  << "," << page.bounds.hi.y << "," << page.bounds.hi.z << "]";
      }
      std::cout << "\n";
      totalClusters += nc;
    }
    std::cout << "  Total clusters: " << totalClusters << "\n\n";

    std::cout << "Encoding BVH...\n";
    EncodeBVH(build, opt.outBvhPath);

    std::cout << "Encoding NaniteMesh...\n";
    EncodeNaniteMesh(build, opt.indexCount, opt.outNaniteMeshPath);

    std::cout << "\nExport complete.\n"
              << "  BVH:        " << opt.outBvhPath << "\n"
              << "  NaniteMesh: " << opt.outNaniteMeshPath << "\n";
    return 0;
  } catch (const std::exception &e) {
    if (e.what()[0] != '\0')
      std::cerr << "NaniteExporter error: " << e.what() << "\n";
    return (e.what()[0] == '\0') ? 0 : 1;
  }
}
