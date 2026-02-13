#pragma once

#include <array>
#include <span>

#include "tiny_skia/Edge.h"

namespace tiny_skia::path_geometry {

std::size_t chopQuadAt(std::array<Point, 3> src, double t, std::array<Point, 5>& dst);

std::size_t chopQuadAtXExtrema(std::array<Point, 3> src, std::array<Point, 5>& dst);

std::size_t chopQuadAtYExtrema(std::array<Point, 3> src, std::array<Point, 5>& dst);

std::size_t chopCubicAt(std::span<const Point> src,
                        std::span<const double> tValues,
                        std::span<Point> dst);

std::size_t chopCubicAtXExtrema(std::array<Point, 4> src, std::array<Point, 10>& dst);

std::size_t chopCubicAtYExtrema(std::array<Point, 4> src, std::array<Point, 10>& dst);

bool chopMonoQuadAtX(std::array<Point, 3> src, float x, double& t);

bool chopMonoQuadAtY(std::array<Point, 3> src, float y, double& t);

bool chopMonoCubicAtX(std::array<Point, 4> src, float x, std::array<Point, 7>& dst);

bool chopMonoCubicAtY(std::array<Point, 4> src, float y, std::array<Point, 7>& dst);

}  // namespace tiny_skia::path_geometry
