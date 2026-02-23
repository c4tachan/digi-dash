#include <iostream>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include "svg_preprocessor.hpp"

namespace {

std::string to_lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

void load_sidecar_animation_config(const std::string& input_svg, digidash::GaugeDocument& doc) {
    std::filesystem::path json_path = std::filesystem::path(input_svg).replace_extension(".json");
    if (!std::filesystem::exists(json_path)) {
        return;
    }

    std::ifstream file(json_path);
    if (!file.is_open()) {
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    const std::regex element_regex(
        R"json(\{\s*"id"\s*:\s*"([^"]+)"[\s\S]*?"animation"\s*:\s*\{[\s\S]*?"type"\s*:\s*"([^"]+)"[\s\S]*?"mode"\s*:\s*"([^"]+)"[\s\S]*?"min_value"\s*:\s*([-+]?[0-9]*\.?[0-9]+)[\s\S]*?"max_value"\s*:\s*([-+]?[0-9]*\.?[0-9]+)[\s\S]*?"binding"\s*:\s*\{[\s\S]*?"pid"\s*:\s*"([^"]+)"[\s\S]*?\})json");

    for (std::sregex_iterator it(content.begin(), content.end(), element_regex), end;
         it != end; ++it) {
        const std::smatch& match = *it;
        const std::string path_id = match[1].str();
        const std::string type = to_lower(match[2].str());
        const std::string mode = to_lower(match[3].str());
        const float min_value = std::stof(match[4].str());
        const float max_value = std::stof(match[5].str());
        const std::string pid = match[6].str();

        if (type == "trim" && mode == "sweep") {
            digidash::PathAnimationBinding binding;
            binding.path_id = path_id;
            binding.type = digidash::AnimationType::TrimSweep;
            binding.min_value = min_value;
            binding.max_value = max_value;
            binding.pid = pid;
            doc.animations.push_back(std::move(binding));
        }
    }

    std::cout << "Loaded " << doc.animations.size() << " animation bindings from "
              << json_path.string() << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: svg_preprocessor input.svg output.gauge\n";
        return 1;
    }

    const std::string input_svg = argv[1];
    const std::string output_bin = argv[2];

    try {
        digidash::SvgLoader loader;
        digidash::SvgNormalizer normalizer;
        digidash::PathFlattener flattener;
        digidash::GaugeSerializer serializer;

        auto doc = loader.load_from_file(input_svg);
        normalizer.normalize(doc);
        flattener.flatten(doc);
        load_sidecar_animation_config(input_svg, doc);
        serializer.write_binary(doc, output_bin);

        std::cout << "Wrote gauge file: " << output_bin << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}