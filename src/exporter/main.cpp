#include "args.h"
#include "cluster_builder.h"
#include "mesh_loader.h"
#include "nanite_encode.h"

#include <iostream>

int main(int argc, char **argv) {
  try {
    const ExporterOptions opt = ParseArgs(argc, argv);

    std::cout << "Input: " << opt.inputPath << "\n";
    const auto mesh = LoadMesh(opt.inputPath, opt.targetExtent);

    const auto build = BuildClustersAndPages(
        mesh, opt.mipValues, opt.trianglesPerCluster, opt.maxClustersPerMip);

    uint32_t totalClusters = 0;
    for (const auto &page : build.pages)
      totalClusters += static_cast<uint32_t>(page.clusters.size());
    std::cout << "  " << build.pages.size() << " pages, " << totalClusters
              << " clusters\n";

    EncodeBVH(build, opt.outBvhPath);
    EncodeNaniteMesh(build, opt.indexCount, opt.outNaniteMeshPath);

    std::cout << "Done: " << opt.outBvhPath << ", " << opt.outNaniteMeshPath
              << "\n";
    return 0;
  } catch (const std::exception &e) {
    if (e.what()[0] != '\0')
      std::cerr << "NaniteExporter error: " << e.what() << "\n";
    return (e.what()[0] == '\0') ? 0 : 1;
  }
}
