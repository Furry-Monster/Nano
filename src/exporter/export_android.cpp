#include "export_android.h"
#include "cluster_builder.h"
#include "mesh_loader.h"
#include "nanite_encode.h"

#include <algorithm>
#include <string>

static std::string BaseNameFromPath(const std::string &path) {
  size_t slash = path.find_last_of("/\\");
  size_t dot = path.find_last_of('.');
  std::string base;
  if (slash != std::string::npos) {
    base = (dot != std::string::npos && dot > slash)
               ? path.substr(slash + 1, dot - slash - 1)
               : path.substr(slash + 1);
  } else {
    base = (dot != std::string::npos) ? path.substr(0, dot) : path;
  }
  return base;
}

std::string ExportModelAndroid(const std::string &inputPath,
                               const std::string &outputDir) {
  try {
    std::string outDir = outputDir;
    if (!outDir.empty() && outDir.back() != '/' && outDir.back() != '\\')
      outDir += '/';

    const std::string base = BaseNameFromPath(inputPath);
    const std::string outBvh = outDir + base + ".bvh";
    const std::string outMesh = outDir + base + ".nanomesh";

    const auto mesh = LoadMesh(inputPath, 500.0f);

    std::vector<int> mipValues = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10};
    const auto build = BuildClustersAndPages(mesh, mipValues, 128, 511);

    EncodeBVH(build, outBvh);
    EncodeNaniteMesh(build, 384, outMesh);

    return "";
  } catch (const std::exception &e) {
    return std::string(e.what());
  }
}
