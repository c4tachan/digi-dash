#include <iostream>
#include "digidash/svg_preprocessor.hpp"

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
        serializer.write_binary(doc, output_bin);

        std::cout << "Wrote gauge file: " << output_bin << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}