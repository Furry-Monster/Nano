#include "args.h"

#include <iostream>
#include <sstream>
#include <stdexcept>

static std::vector<int> ParseIntListComma(const std::string &s) {
  std::vector<int> out;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, ',')) {
    if (item.empty())
      continue;
    out.push_back(std::stoi(item));
  }
  return out;
}

void Usage(const char *exe) {
  std::cout << "Usage:\n"
            << "  " << exe << " --input <model.(fbx|gltf|glb|...)>\n"
            << "  --out-bvh <path>\n"
            << "  --out-nanitemesh <path>\n"
            << "  [--mip-values \"0,1,2,3,4,5,6,7,8,10\"]\n"
            << "  [--triangles-per-cluster 128]\n"
            << "  [--index-count 384]\n"
            << "  [--max-clusters-per-mip 511]\n"
            << "  [--target-extent 500]\n";
}

ExporterOptions ParseArgs(int argc, char **argv) {
  ExporterOptions opt;
  opt.mipValues = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10};

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto requireValue = [&](const char *name) -> std::string {
      if (i + 1 >= argc)
        throw std::runtime_error(std::string("Missing value for ") + name);
      return argv[++i];
    };

    if (arg == "--input") {
      opt.inputPath = requireValue("--input");
    } else if (arg == "--out-bvh") {
      opt.outBvhPath = requireValue("--out-bvh");
    } else if (arg == "--out-nanitemesh") {
      opt.outNaniteMeshPath = requireValue("--out-nanitemesh");
    } else if (arg == "--mip-values") {
      opt.mipValues = ParseIntListComma(requireValue("--mip-values"));
    } else if (arg == "--triangles-per-cluster") {
      opt.trianglesPerCluster = static_cast<uint32_t>(
          std::stoul(requireValue("--triangles-per-cluster")));
    } else if (arg == "--index-count") {
      opt.indexCount =
          static_cast<uint32_t>(std::stoul(requireValue("--index-count")));
    } else if (arg == "--max-clusters-per-mip") {
      opt.maxClustersPerMip = static_cast<uint32_t>(
          std::stoul(requireValue("--max-clusters-per-mip")));
    } else if (arg == "--target-extent") {
      opt.targetExtent = std::stof(requireValue("--target-extent"));
    } else {
      Usage(argv[0]);
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  if (opt.inputPath.empty() || opt.outBvhPath.empty() ||
      opt.outNaniteMeshPath.empty()) {
    Usage(argv[0]);
    throw std::runtime_error("Missing required arguments.");
  }
  if (opt.mipValues.empty()) {
    throw std::runtime_error("--mip-values must not be empty");
  }
  if (opt.indexCount % 3u != 0u) {
    throw std::runtime_error("--index-count must be divisible by 3");
  }
  return opt;
}
