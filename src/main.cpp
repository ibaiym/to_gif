#include "utils/logger.hpp"
#include "common/types.hpp"
#include "factories/default_pipeline_factory.hpp"
#include "ipipeline_factory.hpp"
#include "optimizer/parameter_optimizer.hpp"
#include "optimizer/predictive_search.hpp"
#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

void print_usage(const char* prog) {
    std::cout << R"(
Smart GIF Optimizer
Automatically finds the best quality GIF under the target file size limit.

Pipeline:
  FFmpeg        → Decode video, extract frames, scale
  libimagequant → Global palette optimization, dithering quantization
  giflib        → LZW compression, GIF encoding

Design Patterns:
  Strategy      → ISearchStrategy Parameter search strategy
  DI            → Depend on interfaces rather than implementations

Usage:
  )" << prog << R"( -i <input_video> -o <output.gif> --limit <MB> [options]

Options:
  -i             Input video path                        [Required]
  -o             Output GIF path                         [Required]
  --limit, -l    Target file size limit (MB)           [Required]
  --max-width    Maximum output width (default 720)
  --max-fps      Maximum output fps (default 20)
  --min-width    Minimum output width (default 320)
  --min-fps      Minimum output fps (default 5)
  --colors       Palette color count 64/128/192/256 (default 256)
  --quality      libimagequant quality 1-100 (default 100)
  --max-resolution-iters  Max resolution search iterations (default 6)
  --priority     Search priority: resolution | fps (default resolution)
  --ss           Crop start time (seconds)
  --to           Crop end time (seconds)
  --verbose, -v  Enable debug logging
  -h, --help     Show this help

Examples:
  )" << prog << R"( -i input.mp4 -o output.gif --limit 5
  )" << prog << R"( -i input.mp4 -o output.gif --limit 3 --max-fps 15 --colors 128
  )" << prog << R"( -i input.mp4 -o output.gif --limit 2 --ss 2 --to 8 --quality 90
)" << std::endl;
}

std::string next_arg(int argc, char* argv[], int& i) {
    if (i + 1 >= argc) {
        throw std::runtime_error("missing value");
    }
    return argv[++i];
}

bool parse_args(int argc, char* argv[], to_gif::Config& cfg) {
    std::vector<std::string> positional_args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return false;
        }
        try {
            if (arg == "-i") {
                cfg.input_path = next_arg(argc, argv, i);
            } else if (arg == "-o") {
                cfg.output_path = next_arg(argc, argv, i);
            } else if (arg == "--limit" || arg == "-l") {
                cfg.target_mb = std::stod(next_arg(argc, argv, i));
            } else if (arg == "--max-width") {
                cfg.max_width = std::stoi(next_arg(argc, argv, i));
            } else if (arg == "--max-fps") {
                cfg.max_fps = std::stoi(next_arg(argc, argv, i));
            } else if (arg == "--min-width") {
                cfg.min_width = std::stoi(next_arg(argc, argv, i));
            } else if (arg == "--min-fps") {
                cfg.min_fps = std::stoi(next_arg(argc, argv, i));
            } else if (arg == "--colors") {
                cfg.max_colors = std::stoi(next_arg(argc, argv, i));
            } else if (arg == "--quality") {
                cfg.quality = std::stoi(next_arg(argc, argv, i));
            } else if (arg == "--max-resolution-iters") {
                cfg.max_resolution_iterations = std::stoi(next_arg(argc, argv, i));
            } else if (arg == "--priority") {
                std::string val = next_arg(argc, argv, i);
                if (val == "fps") {
                    cfg.search_priority = to_gif::SearchPriority::kFpsFirst;
                } else {
                    cfg.search_priority = to_gif::SearchPriority::kResolutionFirst;
                }
            } else if (arg == "--ss") {
                cfg.start_sec = std::stod(next_arg(argc, argv, i));
            } else if (arg == "--to") {
                cfg.end_sec = std::stod(next_arg(argc, argv, i));
            } else if (arg == "--verbose" || arg == "-v") {
                to_gif::Logger::instance().set_level(
                    to_gif::LogLevel::kDebug);
            } else if (arg[0] != '-') {
                // Positional argument (not starting with -)
                positional_args.push_back(arg);
            } else {
                LOG(kError) << "Unknown option: " << arg;
                return false;
            }
        } catch (const std::invalid_argument&) {
            LOG(kError) << "Invalid numeric value for " << arg;
            return false;
        } catch (const std::out_of_range&) {
            LOG(kError) << "Numeric value out of range for " << arg;
            return false;
        } catch (const std::runtime_error&) {
            LOG(kError) << "Missing value for " << arg;
            return false;
        }
    }
    
    // Handle positional arguments: input_video output.gif
    if (positional_args.size() >= 1 && cfg.input_path.empty()) {
        cfg.input_path = positional_args[0];
    }
    if (positional_args.size() >= 2 && cfg.output_path.empty()) {
        cfg.output_path = positional_args[1];
    }
    
    return true;
}

bool validate_config(const to_gif::Config& cfg) {
    try {
        cfg.validate();
        return true;
    } catch (const std::invalid_argument& e) {
        LOG(kError) << e.what();
        return false;
    }
}

bool run_pipeline(const to_gif::Config& cfg) {
    try {
        auto strategy = std::make_unique<to_gif::PredictiveSearchStrategy>();
        auto factory = to_gif::create_default_pipeline_factory();
        to_gif::ParameterOptimizer optimizer(cfg, std::move(strategy), std::move(factory));
        optimizer.optimize();
    } catch (const std::exception& e) {
        LOG(kError) << "Exception: " << e.what();
        return false;
    }
    return true;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    to_gif::Config cfg;
    

    if (!parse_args(argc, argv, cfg)) {
        return 1;
    }

    if (!validate_config(cfg)) {
        return 1;
    }

    if (!run_pipeline(cfg)) {
        return 1;
    }

    return 0;
}
