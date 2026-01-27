#!/usr/bin/env cpp
/**
 * SvgLoader Example - ThorVG-based SVG to GaugeDocument conversion
 * 
 * Demonstrates how to use the new SvgLoader::load_from_string() implementation
 * to convert SVG files into a structured GaugeDocument with paths and commands.
 */

#include "digidash/svg_preprocessor.hpp"
#include <iostream>
#include <iomanip>

using namespace digidash;

void print_command(const PathCommand& cmd, int index) {
    std::cout << "    [" << index << "] ";
    
    switch (cmd.type) {
        case PathCommand::Type::MoveTo:
            std::cout << "MoveTo(" << cmd.x1 << ", " << cmd.y1 << ")";
            break;
        case PathCommand::Type::LineTo:
            std::cout << "LineTo(" << cmd.x1 << ", " << cmd.y1 << ")";
            break;
        case PathCommand::Type::CubicTo:
            std::cout << "CubicTo("
                      << cmd.x1 << "," << cmd.y1 << " "
                      << cmd.x2 << "," << cmd.y2 << " "
                      << cmd.x3 << "," << cmd.y3 << ")";
            break;
        case PathCommand::Type::Close:
            std::cout << "Close()";
            break;
    }
    std::cout << std::endl;
}

void print_color(const std::string& label, const Color& color) {
    std::cout << label << "rgba("
              << (int)color.r << ","
              << (int)color.g << ","
              << (int)color.b << ","
              << (int)color.a << ")";
}

void print_path(const Path& path, int index) {
    std::cout << "  [Path " << index << "] " << path.id << std::endl;
    
    std::cout << "    Stroke: width=" << path.stroke.width << " ";
    print_color("", path.stroke.color);
    std::cout << std::endl;
    
    std::cout << "    Fill: " << (path.fill.enabled ? "enabled " : "disabled ");
    if (path.fill.enabled) {
        print_color("", path.fill.color);
    }
    std::cout << std::endl;
    
    std::cout << "    Commands (" << path.commands.size() << "):" << std::endl;
    for (size_t i = 0; i < path.commands.size(); ++i) {
        print_command(path.commands[i], i);
    }
}

// Example 1: Load from SVG string (simple circle)
void example_circle() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Example 1: Simple Circle" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::string svg_data = R"(
        <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
            <circle cx="50" cy="50" r="40" fill="blue" stroke="red" stroke-width="2"/>
        </svg>
    )";
    
    SvgLoader loader;
    try {
        auto doc = loader.load_from_string(svg_data);
        
        std::cout << "Successfully loaded SVG with " << doc.paths.size() << " path(s)" << std::endl;
        
        for (size_t i = 0; i < doc.paths.size(); ++i) {
            print_path(doc.paths[i], i);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// Example 2: Load from SVG string (path with curves)
void example_path() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Example 2: Complex Path" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::string svg_data = R"(
        <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
            <path d="M 10 10 L 50 10 C 50 50 100 50 100 100" 
                  stroke="green" stroke-width="1.5" fill="none"/>
        </svg>
    )";
    
    SvgLoader loader;
    try {
        auto doc = loader.load_from_string(svg_data);
        
        std::cout << "Successfully loaded SVG with " << doc.paths.size() << " path(s)" << std::endl;
        
        for (size_t i = 0; i < doc.paths.size(); ++i) {
            print_path(doc.paths[i], i);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// Example 3: Load from SVG string (grouped shapes)
void example_group() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Example 3: Grouped Shapes" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    std::string svg_data = R"(
        <svg viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
            <g fill="none" stroke="black">
                <circle cx="25" cy="25" r="20"/>
                <rect x="50" y="50" width="40" height="40"/>
            </g>
        </svg>
    )";
    
    SvgLoader loader;
    try {
        auto doc = loader.load_from_string(svg_data);
        
        std::cout << "Successfully loaded SVG with " << doc.paths.size() << " path(s)" << std::endl;
        
        for (size_t i = 0; i < doc.paths.size(); ++i) {
            print_path(doc.paths[i], i);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

// Example 4: Load from file
void example_from_file(const std::string& filename) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Example 4: Load from File" << std::endl;
    std::cout << "File: " << filename << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    SvgLoader loader;
    try {
        auto doc = loader.load_from_file(filename);
        
        std::cout << "Successfully loaded SVG with " << doc.paths.size() << " path(s)" << std::endl;
        
        for (size_t i = 0; i < std::min(doc.paths.size(), size_t(3)); ++i) {
            print_path(doc.paths[i], i);
        }
        
        if (doc.paths.size() > 3) {
            std::cout << "  ... and " << (doc.paths.size() - 3) << " more paths" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     SvgLoader ThorVG Integration - Examples            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    // Run examples
    example_circle();
    example_path();
    example_group();
    
    // Load from file if argument provided
    // example_from_file("dashboard_tiny.svg");
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "All examples completed!" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    return 0;
}

/*
 * USAGE:
 * 
 * 1. Build the project:
 *    cd /home/catachan/projects/digi-dash
 *    rm -rf build && mkdir build && cd build
 *    cmake -DCMAKE_BUILD_TYPE=Debug ..
 *    make -j4
 * 
 * 2. Run this example:
 *    ./build/svg_loader_example
 * 
 * 3. Expected Output:
 * 
 *    Example 1: Simple Circle
 *    Successfully loaded SVG with 1 path(s)
 *      [Path 0] path_0
 *        Stroke: width=2 rgba(255,0,0,255)
 *        Fill: enabled rgba(0,0,255,255)
 *        Commands (4):
 *          [0] MoveTo(90, 50)
 *          [1] CubicTo(90,-21.16 59.16,-50 30,-50)
 *          [2] CubicTo(0.84,-50 -29.16,-21.16 -29.16,10)
 *          [3] CubicTo(-29.16,41.16 0.84,70 30,70)
 * 
 * NOTES:
 * - Circles are automatically converted to 4 cubic Bézier curves
 * - Rectangles become line segments
 * - All shapes are flattened to paths by ThorVG
 * - Colors are extracted as RGBA values
 * - Strokes and fills are separately tracked
 */
