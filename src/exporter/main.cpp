#include "args.h"
#include "cluster_builder.h"
#include "mesh_loader.h"
#include "nanite_encode.h"

#include <iostream>

int main(int argc, char **argv) {
  try {
    const ExporterOptions opt = ParseArgs(argc, argv);

    const auto mesh = LoadMesh(opt.inputPath, opt.targetExtent);
    std::cout << "Loaded: " << mesh.positions.size() << " vertices, "
              << mesh.triangles.size() << " triangles\n";

    const auto build = BuildClustersAndPages(
        mesh, opt.mipValues, opt.trianglesPerCluster, opt.maxClustersPerMip);

    std::cout << "Built: " << build.pages.size() << " pages (mips)\n";
    for (size_t i = 0; i < build.pages.size(); ++i) {
      std::cout << "  mip " << build.pages[i].mipLevel << ": "
                << build.pages[i].clusters.size() << " clusters\n";
    }

    EncodeBVH(build, opt.outBvhPath);
    EncodeNaniteMesh(build, opt.indexCount, opt.outNaniteMeshPath);

    std::cout << "Export done.\n"
              << "  BVH: " << opt.outBvhPath << "\n"
              << "  NaniteMesh: " << opt.outNaniteMeshPath << "\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "NaniteExporter error: " << e.what() << "\n";
    return 1;
  }
}
