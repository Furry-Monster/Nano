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

static void SplitPath(const std::string &path, std::string &dir,
                      std::string &base) {
  size_t slash = path.find_last_of("/\\");
  size_t dot = path.find_last_of('.');
  if (slash != std::string::npos) {
    dir = path.substr(0, slash + 1);
    base = (dot != std::string::npos && dot > slash)
               ? path.substr(slash + 1, dot - slash - 1)
               : path.substr(slash + 1);
  } else {
    dir = "";
    base = (dot != std::string::npos) ? path.substr(0, dot) : path;
  }
}

void Usage(const char *exe) {
  std::cout
      << "Usage: " << exe << " -i <model> [options]\n"
      << "  -i, --input <path>       Input model (fbx/gltf/glb)\n"
      << "  -o, --output-dir <path>  Output directory (default: input dir)\n"
      << "  --bvh <path>             BVH output (overrides -o)\n"
      << "  --mesh <path>            NanoMesh output (overrides -o)\n"
      << "  --mip-values \"0,1,2,...\" LOD mip levels (default: "
         "0,1,2,3,4,5,6,7,8,10)\n"
      << "  --target-extent <float>  Bounding extent (default: 500)\n";
}

ExporterOptions ParseArgs(int argc, char **argv) { // NOLINT
  ExporterOptions opt;
  opt.mipValues = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10};
  std::string outputDir;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto next = [&]() -> std::string {
      if (i + 1 >= argc)
        throw std::runtime_error("Missing value for " + arg);
      return argv[++i];
    };

    if (arg == "-i" || arg == "--input") {
      opt.inputPath = next();
    } else if (arg == "-o" || arg == "--output-dir") {
      outputDir = next();
      if (!outputDir.empty() && outputDir.back() != '/' &&
          outputDir.back() != '\\')
        outputDir += '/';
    } else if (arg == "--bvh") {
      opt.outBvhPath = next();
    } else if (arg == "--mesh") {
      opt.outNaniteMeshPath = next();
    } else if (arg == "--mip-values") {
      opt.mipValues = ParseIntListComma(next());
    } else if (arg == "--triangles-per-cluster") {
      opt.trianglesPerCluster = static_cast<uint32_t>(std::stoul(next()));
    } else if (arg == "--index-count") {
      opt.indexCount = static_cast<uint32_t>(std::stoul(next()));
    } else if (arg == "--max-clusters-per-mip") {
      opt.maxClustersPerMip = static_cast<uint32_t>(std::stoul(next()));
    } else if (arg == "--target-extent") {
      opt.targetExtent = std::stof(next());
    } else if (arg == "-h" || arg == "--help") {
      Usage(argv[0]);
      throw std::runtime_error("");
    } else {
      Usage(argv[0]);
      throw std::runtime_error("Unknown argument: " + arg);
    }
  }

  if (opt.inputPath.empty()) {
    Usage(argv[0]);
    throw std::runtime_error("Missing -i/--input");
  }

  std::string inDir, inBase;
  SplitPath(opt.inputPath, inDir, inBase);
  if (outputDir.empty())
    outputDir = inDir;

  if (opt.outBvhPath.empty())
    opt.outBvhPath = outputDir + inBase + ".bvh";
  if (opt.outNaniteMeshPath.empty())
    opt.outNaniteMeshPath = outputDir + inBase + ".nanomesh";

  if (opt.mipValues.empty())
    throw std::runtime_error("--mip-values must not be empty");
  if (opt.indexCount % 3u != 0u)
    throw std::runtime_error("--index-count must be divisible by 3");

  return opt;
}
