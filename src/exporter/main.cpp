#include "args.h"
#include "assimp_loader.h"
#include "nanite_encoder.h"

#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
  try {
    const ExporterOptions opt = ParseArgs(argc, argv);

    const auto loaded = LoadMeshAssimp(opt.inputPath, opt.targetExtent);

    const auto pages = BuildPages(loaded.positions, loaded.triangles,
                                    opt.mipValues, opt.trianglesPerCluster,
                                    opt.indexCount, opt.maxClustersPerMip);

    EncodeNaniteMesh(pages, opt.indexCount, opt.outNaniteMeshPath);
    EncodeBVH(pages, opt.mipValues, opt.outBvhPath);

    std::cout << "Export done.\n"
              << "  pages: " << pages.size() << "\n"
              << "  triangles: " << loaded.triangles.size() << "\n"
              << "  out bvh: " << opt.outBvhPath << "\n"
              << "  out nanitemesh: " << opt.outNaniteMeshPath << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "nanite_exporter error: " << e.what() << "\n";
    return 1;
  }
}

