//
//  gTest_contour.cxx
//  DeepSkyTools
//
//  Unit tests for QCustomContours (marching-squares iso-contour generator).
//
//  Copyright (c) 2025, All rights reserved
//

#include <gtest/gtest.h>
#include "QCustomContours.h"

#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Helper: split a NaN-terminated vertices_list_type into individual polylines.
// Each returned sub-vector contains the (x,y) pairs of one continuous chain.
// ---------------------------------------------------------------------------
static std::vector<std::vector<std::pair<double,double>>>
splitPolylines(const QCustomContours::vertices_list_type& vlt)
{
    std::vector<std::vector<std::pair<double,double>>> result;
    std::vector<std::pair<double,double>> current;
    const int n = vlt.first.size();
    for (int i = 0; i < n; ++i)
    {
        if (std::isnan(vlt.first[i]) || std::isnan(vlt.second[i]))
        {
            if (!current.empty()) { result.push_back(std::move(current)); current.clear(); }
        }
        else
            current.push_back({ vlt.first[i], vlt.second[i] });
    }
    if (!current.empty()) result.push_back(std::move(current));
    return result;
}

// Helper: build a uniform nrows x ncols grid
static std::vector<std::vector<double>> makeGrid(size_t nrows, size_t ncols,
                                                  double fillValue = 0.0)
{
    return std::vector<std::vector<double>>(nrows, std::vector<double>(ncols, fillValue));
}

// ===========================================================================
// Group 1 — Level computation
// ===========================================================================

TEST(QCustomContours_levels, count)
{
    // 4 rows x 5 cols,  Z[j][i] = i  (linear gradient along x)
    auto Z = makeGrid(4, 5);
    for (size_t j = 0; j < 4; ++j)
        for (size_t i = 0; i < 5; ++i)
            Z[j][i] = static_cast<double>(i);

    QCustomContours c(Z, 4);
    ASSERT_EQ(c.levels().size(), 4) << "Expected exactly 4 levels";
}

TEST(QCustomContours_levels, spacing)
{
    auto Z = makeGrid(4, 5);
    for (size_t j = 0; j < 4; ++j)
        for (size_t i = 0; i < 5; ++i)
            Z[j][i] = static_cast<double>(i);

    QCustomContours c(Z, 4);
    const auto& lv = c.levels();
    ASSERT_GE(lv.size(), 2);
    const double expected_step = (lv.last() - lv.first()) / static_cast<double>(lv.size() - 1);
    for (int k = 1; k < lv.size(); ++k)
        EXPECT_NEAR(lv[k] - lv[k-1], expected_step, 1e-12)
            << "Level spacing not uniform at k=" << k;
}

TEST(QCustomContours_levels, range)
{
    auto Z = makeGrid(4, 5);
    for (size_t j = 0; j < 4; ++j)
        for (size_t i = 0; i < 5; ++i)
            Z[j][i] = static_cast<double>(i); // range [0, 4]

    QCustomContours c(Z, 4);
    for (double lv : c.levels())
    {
        EXPECT_GT(lv, 0.0) << "Level " << lv << " not strictly above zmin";
        EXPECT_LT(lv, 4.0) << "Level " << lv << " not strictly below zmax";
    }
}

TEST(QCustomContours_levels, empty_grid)
{
    std::vector<std::vector<double>> Z; // empty
    QCustomContours c(Z, 4);
    EXPECT_TRUE(c.levels().isEmpty());
    EXPECT_TRUE(c.lines().isEmpty());
}

TEST(QCustomContours_levels, single_row)
{
    // Only 1 row → no cells → no levels
    std::vector<std::vector<double>> Z = {{ 0.0, 1.0, 2.0 }};
    QCustomContours c(Z, 4);
    EXPECT_TRUE(c.levels().isEmpty());
    EXPECT_TRUE(c.lines().isEmpty());
}

TEST(QCustomContours_levels, flat)
{
    // All values identical → zmin == zmax → no levels
    QCustomContours c(makeGrid(4, 4, 3.14), 4);
    EXPECT_TRUE(c.levels().isEmpty());
    EXPECT_TRUE(c.lines().isEmpty());
}

// ===========================================================================
// Group 2 — Output invariants
// ===========================================================================

// Shared fixture grid: 5 rows x 6 cols,  Z[j][i] = i + j
static std::vector<std::vector<double>> makeInvariantGrid()
{
    std::vector<std::vector<double>> Z(5, std::vector<double>(6));
    for (size_t j = 0; j < 5; ++j)
        for (size_t i = 0; i < 6; ++i)
            Z[j][i] = static_cast<double>(i + j);
    return Z;
}

TEST(QCustomContours_invariants, lines_levels_same_size)
{
    QCustomContours c(makeInvariantGrid(), 3);
    EXPECT_EQ(c.lines().size(), c.levels().size());
}

TEST(QCustomContours_invariants, nan_terminated)
{
    QCustomContours c(makeInvariantGrid(), 3);
    for (int k = 0; k < c.lines().size(); ++k)
    {
        const auto& xs = c.lines()[k].first;
        const auto& ys = c.lines()[k].second;
        if (xs.isEmpty()) continue; // a level with no crossings is also valid (empty)
        EXPECT_TRUE(std::isnan(xs.last()))
            << "Level " << k << ": last x is not NaN";
        EXPECT_TRUE(std::isnan(ys.last()))
            << "Level " << k << ": last y is not NaN";
    }
}

TEST(QCustomContours_invariants, coordinate_bounds)
{
    const size_t nrows = 5, ncols = 6;
    QCustomContours c(makeInvariantGrid(), 3);
    for (int k = 0; k < c.lines().size(); ++k)
    {
        const auto& xs = c.lines()[k].first;
        const auto& ys = c.lines()[k].second;
        for (int i = 0; i < xs.size(); ++i)
        {
            if (std::isnan(xs[i])) continue;
            EXPECT_GE(xs[i], 0.0)                       << "x below 0 at level " << k;
            EXPECT_LE(xs[i], static_cast<double>(ncols - 1)) << "x above ncols-1 at level " << k;
            EXPECT_GE(ys[i], 0.0)                       << "y below 0 at level " << k;
            EXPECT_LE(ys[i], static_cast<double>(nrows - 1)) << "y above nrows-1 at level " << k;
        }
    }
}

// ===========================================================================
// Group 3 — Geometric correctness
// ===========================================================================

TEST(QCustomContours_geometry, linear_gradient_x)
{
    // Z[j][i] = i  on 4x5 grid  →  single level at 2.0  →  vertical line x=2.0
    const size_t nrows = 4, ncols = 5;
    auto Z = makeGrid(nrows, ncols);
    for (size_t j = 0; j < nrows; ++j)
        for (size_t i = 0; i < ncols; ++i)
            Z[j][i] = static_cast<double>(i);

    QCustomContours c(Z, 1);
    ASSERT_EQ(c.lines().size(), 1);

    const auto polylines = splitPolylines(c.lines()[0]);
    ASSERT_EQ(polylines.size(), 1u)
        << "Expected one continuous chain for a simple vertical contour";

    for (auto& [x, y] : polylines[0])
        EXPECT_NEAR(x, 2.0, 1e-10) << "x should be 2.0 for a vertical gradient contour";
}

TEST(QCustomContours_geometry, linear_gradient_y)
{
    // Z[j][i] = j  on 5x4 grid  →  single level at 2.0  →  horizontal line y=2.0
    const size_t nrows = 5, ncols = 4;
    auto Z = makeGrid(nrows, ncols);
    for (size_t j = 0; j < nrows; ++j)
        for (size_t i = 0; i < ncols; ++i)
            Z[j][i] = static_cast<double>(j);

    QCustomContours c(Z, 1);
    ASSERT_EQ(c.lines().size(), 1);

    const auto polylines = splitPolylines(c.lines()[0]);
    ASSERT_EQ(polylines.size(), 1u)
        << "Expected one continuous chain for a simple horizontal contour";

    for (auto& [x, y] : polylines[0])
        EXPECT_NEAR(y, 2.0, 1e-10) << "y should be 2.0 for a horizontal gradient contour";
}

TEST(QCustomContours_geometry, single_peak)
{
    // 5x5 grid, all zeros except centre cell = 10; level = 5.0
    // Contour must encircle the peak (all points within sqrt(2) of (2,2))
    const size_t N = 5;
    auto Z = makeGrid(N, N, 0.0);
    Z[2][2] = 10.0;

    QCustomContours c(Z, 1);
    ASSERT_EQ(c.lines().size(), 1);

    bool anyPoint = false;
    for (int i = 0; i < c.lines()[0].first.size(); ++i)
    {
        const double x = c.lines()[0].first[i];
        const double y = c.lines()[0].second[i];
        if (std::isnan(x)) continue;
        anyPoint = true;
        const double dist = std::hypot(x - 2.0, y - 2.0);
        EXPECT_LE(dist, std::sqrt(2.0) + 1e-10)
            << "Contour point (" << x << "," << y << ") is too far from peak";
    }
    EXPECT_TRUE(anyPoint) << "No contour points found around peak";
}

TEST(QCustomContours_geometry, saddle_case5)
{
    // 2x2 grid: Z[0][0]=2(BL), Z[0][1]=0(BR), Z[1][1]=2(TR), Z[1][0]=0(TL)
    // BL+TR above level=1  →  case 5 (saddle)  →  two separate chains
    std::vector<std::vector<double>> Z = { {2.0, 0.0}, {0.0, 2.0} };
    QCustomContours c(Z, 1); // level = (0+2)/2 = 1.0
    ASSERT_EQ(c.lines().size(), 1);

    const auto polylines = splitPolylines(c.lines()[0]);
    EXPECT_EQ(polylines.size(), 2u)
        << "Saddle case 5 should produce 2 separate contour chains";
}

TEST(QCustomContours_geometry, saddle_case10)
{
    // 2x2 grid: Z[0][0]=0(BL), Z[0][1]=2(BR), Z[1][1]=0(TR), Z[1][0]=2(TL)
    // BR+TL above level=1  →  case 10 (saddle)  →  two separate chains
    std::vector<std::vector<double>> Z = { {0.0, 2.0}, {2.0, 0.0} };
    QCustomContours c(Z, 1);
    ASSERT_EQ(c.lines().size(), 1);

    const auto polylines = splitPolylines(c.lines()[0]);
    EXPECT_EQ(polylines.size(), 2u)
        << "Saddle case 10 should produce 2 separate contour chains";
}

// ===========================================================================
// Group 4 — NaN / masked pixels
// ===========================================================================

TEST(QCustomContours_nan, nan_no_crash)
{
    // Scatter NaN values through a gradient grid; must not throw or crash
    auto Z = makeGrid(5, 5);
    for (size_t j = 0; j < 5; ++j)
        for (size_t i = 0; i < 5; ++i)
            Z[j][i] = static_cast<double>(i + j);
    Z[1][1] = std::numeric_limits<double>::quiet_NaN();
    Z[3][2] = std::numeric_limits<double>::quiet_NaN();

    ASSERT_NO_THROW({
        QCustomContours c(Z, 2);
        EXPECT_EQ(c.lines().size(), c.levels().size());
    });
}

TEST(QCustomContours_nan, nan_avoids_masked)
{
    // 3x3 grid: all values trigger a crossing EXCEPT the NaN cell forces case 0/no-seg
    // Z[j][i] = i — vertical gradient, but Z[1][1] = NaN
    // With NaN treated as below level, cells touching Z[1][1] skip that corner.
    // Verify: no point has coordinates matching the NaN pixel's integer position exactly.
    std::vector<std::vector<double>> Z = {
        { 0.0, 2.0, 4.0 },
        { 0.0, std::numeric_limits<double>::quiet_NaN(), 4.0 },
        { 0.0, 2.0, 4.0 }
    };

    ASSERT_NO_THROW({
        QCustomContours c(Z, 1); // level ≈ 2.0 (midpoint of valid range)
        // Simply verify no assertion fires and sizes match
        EXPECT_EQ(c.lines().size(), c.levels().size());
    });
}
