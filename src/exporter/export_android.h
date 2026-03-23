#pragma once

#include <string>

/// Run NanoExporter: load mesh from inputPath, output .bvh and .nanomesh to outputDir.
/// outputDir should end with / and will contain <basename>.bvh and <basename>.nanomesh.
/// Returns empty string on success, error message on failure.
std::string ExportModelAndroid(const std::string &inputPath,
                               const std::string &outputDir);
