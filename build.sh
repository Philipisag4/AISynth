#!/usr/bin/env bash
# ============================================================================
#  AI Synth – one-shot build script for macOS / Linux
#  Usage: ./build.sh [Release|Debug]
# ============================================================================
set -e

CONFIG="${1:-Release}"

echo ""
echo "=== Configuring CMake (Ninja) ==="
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE="$CONFIG"

echo ""
echo "=== Building VST3 plugin ($CONFIG) ==="
cmake --build build --target AISynth_VST3

echo ""
echo "=== Building Standalone app ($CONFIG) ==="
cmake --build build --target AISynth_Standalone

echo ""
echo "=== Build succeeded ==="
echo ""
echo "VST3 plugin : build/AISynth_artefacts/$CONFIG/VST3/AI Synth.vst3"
echo "Standalone  : build/AISynth_artefacts/$CONFIG/Standalone/AI Synth"
echo ""
echo "Next steps:"
echo "  macOS  : cp -r 'build/AISynth_artefacts/$CONFIG/VST3/AI Synth.vst3' ~/Library/Audio/Plug-Ins/VST3/"
echo "  Linux  : cp -r 'build/AISynth_artefacts/$CONFIG/VST3/AI Synth.vst3' ~/.vst3/"
echo ""
