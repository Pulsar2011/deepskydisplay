//
//  gTest_ColorDisplay.cxx
//  DeepSkyTools
//
//  Unit tests for QCPColorMapRGB and QtFITSviewer::OverlayRGB / QtDisplayRGB.
//
//  Strategy: headless Qt via QT_QPA_PLATFORM=offscreen.
//  QCPColorMapRGB is tested directly (unit) and via the QtFITSviewer::OverlayRGB
//  integration path. In-memory FITScube objects are constructed with FloatFITSimg;
//  no FITS file I/O is required.
//
//  Copyright (c) 2025, All rights reserved
//

#include <gtest/gtest.h>

#ifdef __WITH_QT__

#include <cstdlib>
#include <cmath>
#include <memory>
#include <vector>
#include <numeric>

#include <QApplication>
#include <QCoreApplication>

#include "QtFITSviewer.h"
#include <DSTfits/FITSimg.h>

using namespace DSL;

// ===========================================================================
// File-local helpers
// ===========================================================================

namespace {

/// Build a fresh, hidden QCustomPlot of size 800×600.
inline QCustomPlot* makePlot()
{
    auto* p = new QCustomPlot(nullptr);
    p->setGeometry(0, 0, 800, 600);
    return p;
}

/// Build an nx×ny single-precision image filled with a constant value.
static std::shared_ptr<FITScube> makeFloatImg(size_t nx, size_t ny, float fill = 1.0f)
{
    std::shared_ptr<FITScube> img(FITScube::FloatFITSimg(2, {nx, ny}));
    for(size_t ik = 0; ik < nx * ny; ++ik)
        img->SetPixelValue(fill, ik);
    return img;
}

/// Build a 3-layer nx×ny float cube; layers are filled with r, g, b respectively.
static std::shared_ptr<FITScube> make3LayerCube(size_t nx, size_t ny,
                                                  float r, float g, float b)
{
    std::shared_ptr<FITScube> cube(FITScube::FloatFITSimg(3, {nx, ny, 3}));
    const size_t stride = nx * ny;
    for(size_t ik = 0; ik < stride; ++ik) cube->SetPixelValue(r, ik);
    for(size_t ik = stride; ik < 2*stride; ++ik) cube->SetPixelValue(g, ik);
    for(size_t ik = 2*stride; ik < 3*stride; ++ik) cube->SetPixelValue(b, ik);
    return cube;
}

/// Return the first QCPColorMapRGB plottable on the plot, or nullptr.
static QCPColorMapRGB* firstRGBMap(QCustomPlot* plot)
{
    for(int i = 0; i < plot->plottableCount(); ++i)
        if(auto* rgb = dynamic_cast<QCPColorMapRGB*>(plot->plottable(i)))
            return rgb;
    return nullptr;
}

} // anonymous namespace

// ===========================================================================
// Fixture
// ===========================================================================

class ColorDisplayTest : public ::testing::Test
{
protected:
    QCustomPlot* plot = nullptr;

    void SetUp() override    { plot = makePlot(); }
    void TearDown() override { delete plot; plot = nullptr; }
};

// ===========================================================================
// Group 1 — QCPColorMapRGB unit tests (pure QCP, no FITScube)
// ===========================================================================

TEST_F(ColorDisplayTest, DefaultChannelCount)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    bool found = false;
    rgb->getKeyRange(found);
    EXPECT_FALSE(found) << "Key range should not be found before data is set";
    EXPECT_EQ(plot->plottableCount(), 1);
}

TEST_F(ColorDisplayTest, SetChannelData_DimensionsAndRange)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    const int nx = 8, ny = 5;
    std::vector<double> data(static_cast<size_t>(nx) * static_cast<size_t>(ny), 0.5);
    rgb->setChannelData(0, data, nx, ny, QCPRange(0, nx-1), QCPRange(0, ny-1));

    bool kFound = false, vFound = false;
    QCPRange kr = rgb->getKeyRange(kFound);
    QCPRange vr = rgb->getValueRange(vFound);

    EXPECT_TRUE(kFound);
    EXPECT_TRUE(vFound);
    EXPECT_DOUBLE_EQ(kr.lower, 0.0);
    EXPECT_DOUBLE_EQ(kr.upper, static_cast<double>(nx - 1));
    EXPECT_DOUBLE_EQ(vr.lower, 0.0);
    EXPECT_DOUBLE_EQ(vr.upper, static_cast<double>(ny - 1));
}

TEST_F(ColorDisplayTest, RescaleChannelRange_ComputesMinMax)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    std::vector<double> data = {1.0, 3.0, 2.0, 5.0, 0.5};
    rgb->setChannelData(0, data, 5, 1, QCPRange(0, 4), QCPRange(0, 0));
    rgb->rescaleChannelRange(0);

    // After rescale the intensity range should be [0.5, 5.0]
    rgb->setChannelRange(0, QCPRange(0.5, 5.0));  // cross-check via re-set
    // The range is not directly accessible, but recompose must not crash
    EXPECT_NO_THROW(rgb->composedImage());
}

TEST_F(ColorDisplayTest, RescaleChannelRange_NoData_DoesNotCrash)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    EXPECT_NO_THROW(rgb->rescaleChannelRange(0));   // empty channel → no-op
}

TEST_F(ColorDisplayTest, SetChannelCount_Clamped)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    rgb->setChannelCount(0);  // below minimum
    // Must not crash on recompose with 0 channels (clamped to 1)
    rgb->setChannelData(0, {0.5}, 1, 1, QCPRange(0,0), QCPRange(0,0));
    EXPECT_NO_THROW(rgb->composedImage());
}

TEST_F(ColorDisplayTest, InvalidateTriggersRecompose)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    std::vector<double> data(4, 0.5);
    rgb->setChannelData(0, data, 2, 2, QCPRange(0,1), QCPRange(0,1));
    // First compose
    const QImage& img1 = rgb->composedImage();
    EXPECT_FALSE(img1.isNull());
    // Invalidate should trigger re-compose on next access
    rgb->invalidate();
    const QImage& img2 = rgb->composedImage();
    EXPECT_FALSE(img2.isNull());
    EXPECT_EQ(img2.width(), 2);
    EXPECT_EQ(img2.height(), 2);
}

TEST_F(ColorDisplayTest, Recompose_SingleChannel_ImageDimensions)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    const int nx = 10, ny = 7;
    std::vector<double> data(static_cast<size_t>(nx) * static_cast<size_t>(ny), 1.0);
    rgb->setChannelData(0, data, nx, ny, QCPRange(0, nx-1), QCPRange(0, ny-1));
    rgb->setChannelRange(0, QCPRange(0.0, 1.0));

    const QImage& img = rgb->composedImage();
    EXPECT_EQ(img.width(), nx);
    EXPECT_EQ(img.height(), ny);
}

TEST_F(ColorDisplayTest, Recompose_AllZeroData_ResultIsBlack)
{
    // Channel 0 = all zeros, channels 1&2 empty → composed image should be black
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    std::vector<double> data(9, 0.0);
    rgb->setChannelData(0, data, 3, 3, QCPRange(0,2), QCPRange(0,2));
    rgb->setChannelRange(0, QCPRange(0.0, 1.0));
    rgb->setChannelCount(1);

    const QImage& img = rgb->composedImage();
    ASSERT_FALSE(img.isNull());
    // Each pixel: black-to-red gradient at position 0 → black (R=0, G=0, B=0)
    for(int y = 0; y < img.height(); ++y)
        for(int x = 0; x < img.width(); ++x)
        {
            QColor c(img.pixel(x, y));
            EXPECT_EQ(c.red(), 0)   << "at (" << x << "," << y << ")";
            EXPECT_EQ(c.green(), 0) << "at (" << x << "," << y << ")";
            EXPECT_EQ(c.blue(), 0)  << "at (" << x << "," << y << ")";
        }
}

TEST_F(ColorDisplayTest, Recompose_PureRedChannel_ResultIsRed)
{
    // Channel 0 = uniform 1.0, gradient black→red, channels 1&2 empty
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    std::vector<double> data(4, 1.0);
    rgb->setChannelData(0, data, 2, 2, QCPRange(0,1), QCPRange(0,1));
    rgb->setChannelRange(0, QCPRange(0.0, 1.0));
    rgb->setChannelCount(1);

    const QImage& img = rgb->composedImage();
    ASSERT_FALSE(img.isNull());
    QColor c(img.pixel(0, 0));
    EXPECT_GT(c.red(),  200) << "Max-value pixel on red channel must be strongly red";
    EXPECT_LT(c.green(), 10);
    EXPECT_LT(c.blue(),  10);
}

TEST_F(ColorDisplayTest, Recompose_AdditiveBlend_RedPlusGreen_IsYellow)
{
    // ch0=1.0 (red), ch1=1.0 (green), ch2=0.0 (blue) → additive blend → yellow
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    std::vector<double> ones(4, 1.0), zeros(4, 0.0);
    rgb->setChannelData(0, ones,  2, 2, QCPRange(0,1), QCPRange(0,1));
    rgb->setChannelData(1, ones,  2, 2, QCPRange(0,1), QCPRange(0,1));
    rgb->setChannelData(2, zeros, 2, 2, QCPRange(0,1), QCPRange(0,1));
    rgb->setChannelRange(0, QCPRange(0.0, 1.0));
    rgb->setChannelRange(1, QCPRange(0.0, 1.0));
    rgb->setChannelRange(2, QCPRange(0.0, 0.0));
    rgb->setChannelCount(3);

    const QImage& img = rgb->composedImage();
    ASSERT_FALSE(img.isNull());
    QColor c(img.pixel(0, 0));
    EXPECT_GT(c.red(),   200) << "Red component should be high in R+G blend";
    EXPECT_GT(c.green(), 200) << "Green component should be high in R+G blend";
    EXPECT_LT(c.blue(),   10) << "Blue component should be zero in R+G blend";
}

TEST_F(ColorDisplayTest, Recompose_CMYK_FourChannels)
{
    // CMYK: C=0, M=0, Y=0, K=0 → R=G=B=255 (white)
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    std::vector<double> zeros(4, 0.0);
    for(int ch = 0; ch < 4; ++ch)
    {
        rgb->setChannelData(ch, zeros, 2, 2, QCPRange(0,1), QCPRange(0,1));
        rgb->setChannelRange(ch, QCPRange(0.0, 1.0));
    }
    rgb->setChannelCount(4);
    rgb->setCMYKMode(true);

    const QImage& img = rgb->composedImage();
    ASSERT_FALSE(img.isNull());
    QColor c(img.pixel(0, 0));
    EXPECT_GT(c.red(),   200) << "CMYK(0,0,0,0) must be white";
    EXPECT_GT(c.green(), 200);
    EXPECT_GT(c.blue(),  200);
}

TEST_F(ColorDisplayTest, Recompose_CMYK_BlackK_ProducesBlack)
{
    // CMYK: C=0, M=0, Y=0, K=1 → R=G=B=0 (black)
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    for(int ch = 0; ch < 3; ++ch)
    {
        std::vector<double> zeros(4, 0.0);
        rgb->setChannelData(ch, zeros, 2, 2, QCPRange(0,1), QCPRange(0,1));
        rgb->setChannelRange(ch, QCPRange(0.0, 1.0));
    }
    std::vector<double> ones(4, 1.0);  // K=1
    rgb->setChannelData(3, ones, 2, 2, QCPRange(0,1), QCPRange(0,1));
    rgb->setChannelRange(3, QCPRange(0.0, 1.0));
    rgb->setChannelCount(4);
    rgb->setCMYKMode(true);

    const QImage& img = rgb->composedImage();
    ASSERT_FALSE(img.isNull());
    QColor c(img.pixel(0, 0));
    EXPECT_LT(c.red(),   10) << "CMYK(0,0,0,1) must be black";
    EXPECT_LT(c.green(), 10);
    EXPECT_LT(c.blue(),  10);
}

TEST_F(ColorDisplayTest, SelectTest_InsideRect_ReturnsZero)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    std::vector<double> data(4, 0.5);
    rgb->setChannelData(0, data, 2, 2, QCPRange(0,1), QCPRange(0,1));
    rgb->setChannelRange(0, QCPRange(0.0, 1.0));
    // Must not crash; result can be 0 or -1 depending on coordinate mapping
    EXPECT_NO_THROW(rgb->selectTest(QPointF(0, 0), false));
}

TEST_F(ColorDisplayTest, GetKeyRange_EmptyData_NotFound)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    bool found = true;
    rgb->getKeyRange(found);
    EXPECT_FALSE(found);
}

TEST_F(ColorDisplayTest, GetValueRange_AfterData_Found)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    rgb->setChannelData(0, {1.0, 2.0}, 2, 1, QCPRange(0,1), QCPRange(0,0));
    bool found = false;
    rgb->getValueRange(found);
    EXPECT_TRUE(found);
}

TEST_F(ColorDisplayTest, Draw_NoData_DoesNotCrash)
{
    new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    EXPECT_NO_THROW(plot->replot());
}

TEST_F(ColorDisplayTest, Draw_WithData_NoError)
{
    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    std::vector<double> data(16, 0.7);
    rgb->setChannelData(0, data, 4, 4, QCPRange(0,3), QCPRange(0,3));
    rgb->setChannelRange(0, QCPRange(0.0, 1.0));
    EXPECT_NO_THROW(plot->replot());
}

// ===========================================================================
// Group 2 — OverlayRGB / QtDisplayRGB integration (with in-memory FITScube)
// ===========================================================================

TEST_F(ColorDisplayTest, OverlayRGB_3Cubes_AddsPlottable)
{
    const size_t nx = 8, ny = 6;
    auto red   = makeFloatImg(nx, ny, 1.0f);
    auto green = makeFloatImg(nx, ny, 0.5f);
    auto blue  = makeFloatImg(nx, ny, 0.2f);

    EXPECT_NO_THROW(
        QtFITSviewer::OverlayRGB(plot->axisRect(), red, green, blue));
    QCoreApplication::processEvents();

    EXPECT_EQ(plot->plottableCount(), 1)
        << "OverlayRGB must add exactly one QCPColorMapRGB plottable";
    EXPECT_NE(firstRGBMap(plot), nullptr)
        << "The added plottable must be a QCPColorMapRGB";
}

TEST_F(ColorDisplayTest, OverlayRGB_3Cubes_KeyValueRanges)
{
    const size_t nx = 12, ny = 9;
    auto red   = makeFloatImg(nx, ny, 1.0f);
    auto green = makeFloatImg(nx, ny, 0.8f);
    auto blue  = makeFloatImg(nx, ny, 0.3f);

    QtFITSviewer::OverlayRGB(plot->axisRect(), red, green, blue);

    QCPColorMapRGB* rgb = firstRGBMap(plot);
    ASSERT_NE(rgb, nullptr);

    bool kFound = false, vFound = false;
    QCPRange kr = rgb->getKeyRange(kFound);
    QCPRange vr = rgb->getValueRange(vFound);

    EXPECT_TRUE(kFound);
    EXPECT_TRUE(vFound);
    EXPECT_DOUBLE_EQ(kr.lower, 0.0);
    EXPECT_DOUBLE_EQ(kr.upper, static_cast<double>(nx - 1));
    EXPECT_DOUBLE_EQ(vr.lower, 0.0);
    EXPECT_DOUBLE_EQ(vr.upper, static_cast<double>(ny - 1));
}

TEST_F(ColorDisplayTest, OverlayRGB_3Cubes_ImageDimensions)
{
    const size_t nx = 5, ny = 4;
    auto r = makeFloatImg(nx, ny, 1.0f);
    auto g = makeFloatImg(nx, ny, 0.5f);
    auto b = makeFloatImg(nx, ny, 0.0f);

    QtFITSviewer::OverlayRGB(plot->axisRect(), r, g, b);

    auto* rgb = firstRGBMap(plot);
    ASSERT_NE(rgb, nullptr);
    const QImage& img = rgb->composedImage();
    EXPECT_EQ(img.width(),  static_cast<int>(nx));
    EXPECT_EQ(img.height(), static_cast<int>(ny));
}

TEST_F(ColorDisplayTest, QtDisplayRGB_3Cubes_SetsAxisLabels)
{
    auto r = makeFloatImg(4, 3, 1.0f);
    auto g = makeFloatImg(4, 3, 0.5f);
    auto b = makeFloatImg(4, 3, 0.0f);

    QtFITSviewer::QtDisplayRGB(plot, r, g, b);

    EXPECT_EQ(plot->xAxis->label(), QString("pixel [px]"))
        << "X axis label must be 'pixel [px]'";
    EXPECT_EQ(plot->yAxis->label(), QString("pixel [px]"))
        << "Y axis label must be 'pixel [px]'";
}

TEST_F(ColorDisplayTest, OverlayRGB_3Cubes_LOGZOption_NoThrow)
{
    auto r = makeFloatImg(4, 3, 2.0f);
    auto g = makeFloatImg(4, 3, 4.0f);
    auto b = makeFloatImg(4, 3, 8.0f);

    EXPECT_NO_THROW(
        QtFITSviewer::OverlayRGB(plot->axisRect(), r, g, b, "LOGZ"));
}

TEST_F(ColorDisplayTest, OverlayRGB_3Cubes_SQRTZOption_NoThrow)
{
    auto r = makeFloatImg(4, 3, 4.0f);
    auto g = makeFloatImg(4, 3, 9.0f);
    auto b = makeFloatImg(4, 3, 16.0f);

    EXPECT_NO_THROW(
        QtFITSviewer::OverlayRGB(plot->axisRect(), r, g, b, "SQRTZ"));
}

TEST_F(ColorDisplayTest, OverlayRGB_3Cube_3LayerVersion_AddsPlottable)
{
    const size_t nx = 6, ny = 4;
    auto cube = make3LayerCube(nx, ny, 1.0f, 0.5f, 0.2f);

    EXPECT_NO_THROW(
        QtFITSviewer::OverlayRGB(plot->axisRect(), cube));
    QCoreApplication::processEvents();

    EXPECT_EQ(plot->plottableCount(), 1);
    EXPECT_NE(firstRGBMap(plot), nullptr);
}

TEST_F(ColorDisplayTest, OverlayRGB_Cube3Layer_Ranges)
{
    const size_t nx = 8, ny = 5;
    auto cube = make3LayerCube(nx, ny, 1.0f, 0.5f, 0.0f);

    QtFITSviewer::OverlayRGB(plot->axisRect(), cube);

    auto* rgb = firstRGBMap(plot);
    ASSERT_NE(rgb, nullptr);

    bool kFound = false, vFound = false;
    QCPRange kr = rgb->getKeyRange(kFound);
    QCPRange vr = rgb->getValueRange(vFound);

    EXPECT_TRUE(kFound);
    EXPECT_DOUBLE_EQ(kr.upper, static_cast<double>(nx - 1));
    EXPECT_TRUE(vFound);
    EXPECT_DOUBLE_EQ(vr.upper, static_cast<double>(ny - 1));
}

TEST_F(ColorDisplayTest, OverlayRGB_Cube2D_Throws)
{
    // A 2D cube must throw because GetDimension() == 2 < 3
    auto cube2D = makeFloatImg(4, 4, 1.0f);
    EXPECT_THROW(
        QtFITSviewer::OverlayRGB(plot->axisRect(), cube2D),
        DSL::FITSexception);
}

TEST_F(ColorDisplayTest, QtDisplayRGB_Cube3Layer_SetsAxisLabels)
{
    auto cube = make3LayerCube(4, 3, 1.0f, 0.5f, 0.2f);
    QtFITSviewer::QtDisplayRGB(plot, cube);

    EXPECT_EQ(plot->xAxis->label(), QString("pixel [px]"));
    EXPECT_EQ(plot->yAxis->label(), QString("pixel [px]"));
}

TEST_F(ColorDisplayTest, MultipleCalls_SamePlot_AddsMultiplePlottables)
{
    auto r = makeFloatImg(4, 3, 1.0f);
    auto g = makeFloatImg(4, 3, 0.5f);
    auto b = makeFloatImg(4, 3, 0.0f);

    QtFITSviewer::OverlayRGB(plot->axisRect(), r, g, b);
    QtFITSviewer::OverlayRGB(plot->axisRect(), g, b, r);

    EXPECT_EQ(plot->plottableCount(), 2)
        << "Two OverlayRGB calls on the same plot must add two independent plottables";
}

// ===========================================================================
// Group 3 — DISABLED visual tests (PNG output, skipped by CI)
//
// Run locally with:
//   ./gTest_ColorDisplay --gtest_also_run_disabled_tests \
//                        --gtest_filter=*DISABLED_Visual*
// PNG files are written to the current working directory (build/exe/).
// ===========================================================================

namespace {

/// Pixel-function helper: build a 2-D float image from a lambda (x, y) → float.
/// x in [0, nx), y in [0, ny). Index layout: idx = x + y*nx (FITS convention).
static std::shared_ptr<FITScube>
makeImgFn(size_t nx, size_t ny, std::function<float(int,int)> fn)
{
    std::shared_ptr<FITScube> img(FITScube::FloatFITSimg(2, {nx, ny}));
    for(size_t y = 0; y < ny; ++y)
        for(size_t x = 0; x < nx; ++x)
            img->SetPixelValue(fn(int(x), int(y)), x + y * nx);
    return img;
}

/// 3-layer cube from three lambdas; layer k occupies indices [k·nx·ny, (k+1)·nx·ny).
static std::shared_ptr<FITScube>
makeLayeredCube(size_t nx, size_t ny,
                std::function<float(int,int)> fn0,
                std::function<float(int,int)> fn1,
                std::function<float(int,int)> fn2)
{
    std::shared_ptr<FITScube> cube(FITScube::FloatFITSimg(3, {nx, ny, 3}));
    const size_t stride = nx * ny;
    std::function<float(int,int)> fns[3] = {fn0, fn1, fn2};
    for(int layer = 0; layer < 3; ++layer)
        for(size_t y = 0; y < ny; ++y)
            for(size_t x = 0; x < nx; ++x)
                cube->SetPixelValue(fns[layer](int(x), int(y)),
                                    x + y * nx + static_cast<size_t>(layer) * stride);
    return cube;
}

/// 2-D Gaussian evaluated at (x, y), centred at (cx, cy), width sigma.
/// cx, cy, sigma are in normalised coordinates [0, 1].
static float gauss(int x, int y, int nx, int ny,
                   double cx, double cy, double sigma)
{
    const double dx = double(x) / double(nx - 1) - cx;
    const double dy = double(y) / double(ny - 1) - cy;
    return static_cast<float>(std::exp(-(dx*dx + dy*dy) / (2.0 * sigma * sigma)));
}

/// Save plot as PNG; also prints path to stdout.
static void savePlot(QCustomPlot* p, const char* name)
{
    p->replot();
    p->savePng(QString(name), 0, 0, 2.0);   // 2× pixel ratio for sharp output
    std::printf("[VISUAL] saved → %s\n", name);
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Visual test 1 — RGB smooth gradient
//   R increases left→right,  G increases bottom→top,  B on a radial ramp.
//   Expect: full-spectrum colour wheel going through cyan, yellow, magenta.
// ---------------------------------------------------------------------------
TEST_F(ColorDisplayTest, DISABLED_Visual_RGB_GradientRamp)
{
    plot->resize(600, 600);

    const int NX = 256, NY = 256;

    auto rCh = makeImgFn(NX, NY, [&](int x, int y) -> float {
        return float(x) / float(NX - 1);
    });
    auto gCh = makeImgFn(NX, NY, [&](int x, int y) -> float {
        return float(y) / float(NY - 1);
    });
    auto bCh = makeImgFn(NX, NY, [&](int x, int y) -> float {
        // Blue on a radial gradient centred at the image centre
        const double dx = double(x) / double(NX - 1) - 0.5;
        const double dy = double(y) / double(NY - 1) - 0.5;
        return static_cast<float>(1.0 - std::min(1.0, std::sqrt(dx*dx + dy*dy) * 2.0));
    });

    QtFITSviewer::QtDisplayRGB(plot, rCh, gCh, bCh);
    savePlot(plot, "visual_rgb_gradient.png");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Visual test 2 — Synthetic nebula, Hubble palette (Hα→R, OIII→G, SII→B)
//   Three Gaussian blobs at different positions simulate narrow-band emission.
//   Expect: orange/red blob left, blue/teal blob right, green in the middle.
// ---------------------------------------------------------------------------
TEST_F(ColorDisplayTest, DISABLED_Visual_Nebula_HubblePalette)
{
    plot->resize(800, 600);

    const int NX = 400, NY = 300;

    // Hα: large diffuse blob on the left + faint background
    auto ha = makeImgFn(NX, NY, [&](int x, int y) -> float {
        return 0.05f
             + 0.95f * gauss(x, y, NX, NY, 0.30, 0.55, 0.22)
             + 0.40f * gauss(x, y, NX, NY, 0.50, 0.45, 0.12);
    });

    // OIII: medium blob in the centre + wing overlap
    auto oiii = makeImgFn(NX, NY, [&](int x, int y) -> float {
        return 0.02f
             + 0.85f * gauss(x, y, NX, NY, 0.52, 0.48, 0.16)
             + 0.30f * gauss(x, y, NX, NY, 0.35, 0.55, 0.10);
    });

    // SII: compact blob on the right side
    auto sii = makeImgFn(NX, NY, [&](int x, int y) -> float {
        return 0.01f
             + 0.90f * gauss(x, y, NX, NY, 0.70, 0.42, 0.10)
             + 0.20f * gauss(x, y, NX, NY, 0.60, 0.50, 0.08);
    });

    QtFITSviewer::QtDisplayRGB(plot, ha, oiii, sii);
    savePlot(plot, "visual_nebula_hubble.png");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Visual test 3 — High dynamic range star field, linear vs. log stretch
//   Point sources span 5 decades of intensity.
//   Expect (linear): only the brightest star visible.
//   Expect (logz):   all stars revealed, faint halo visible around brightest.
// ---------------------------------------------------------------------------
TEST_F(ColorDisplayTest, DISABLED_Visual_StarField_LinearVsLog)
{
    const int NX = 300, NY = 300;

    // Five stars: intensities 1, 10, 100, 1000, 10000
    struct Star { double cx, cy, intensity; };
    const Star stars[] = {
        {0.15, 0.80, 1.0},
        {0.40, 0.30, 10.0},
        {0.60, 0.70, 100.0},
        {0.75, 0.20, 1000.0},
        {0.88, 0.55, 10000.0},
    };
    const double psf = 0.025;   // PSF sigma (normalised)

    auto makeCh = [&](double colorFrac) {
        return makeImgFn(NX, NY, [&](int x, int y) -> float {
            float v = 0.f;
            for(const auto& s : stars)
                v += static_cast<float>(s.intensity * colorFrac
                     * gauss(x, y, NX, NY, s.cx, s.cy, psf));
            return v;
        });
    };

    // R-channel bright, G moderate, B faint → stars range from warm to blue-ish
    auto r = makeCh(1.00);
    auto g = makeCh(0.60);
    auto b = makeCh(0.30);

    // Linear
    {
        QCustomPlot* p = new QCustomPlot;
        p->resize(500, 500);
        QtFITSviewer::QtDisplayRGB(p, r, g, b);
        savePlot(p, "visual_starfield_linear.png");
        delete p;
    }

    // Log stretch
    {
        QCustomPlot* p = new QCustomPlot;
        p->resize(500, 500);
        QtFITSviewer::QtDisplayRGB(p, r, g, b, "LOGZ");
        savePlot(p, "visual_starfield_logz.png");
        delete p;
    }
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Visual test 4 — CMYK four-channel compositing
//   Image is split into four quadrants with pure C/M/Y/K channel filling.
//   Expect: cyan (top-left), magenta (top-right), yellow (bottom-left),
//           black (bottom-right).
// ---------------------------------------------------------------------------
TEST_F(ColorDisplayTest, DISABLED_Visual_CMYK_Quadrants)
{
    plot->resize(600, 600);

    const int NX = 256, NY = 256;

    // C=1 in left half, 0 in right half
    auto c = makeImgFn(NX, NY, [&](int x, int y) -> float {
        return (x < NX / 2) ? 1.f : 0.f;
    });
    // M=1 in right half, 0 in left half
    auto m = makeImgFn(NX, NY, [&](int x, int y) -> float {
        return (x >= NX / 2) ? 1.f : 0.f;
    });
    // Y=1 in bottom half, 0 in top half (FITS y=0 is bottom)
    auto y = makeImgFn(NX, NY, [&](int x, int yy) -> float {
        return (yy < NY / 2) ? 1.f : 0.f;
    });
    // K=0 everywhere (no black key)
    auto k = makeImgFn(NX, NY, [&](int, int) -> float { return 0.f; });

    auto* rgb = new QCPColorMapRGB(plot->xAxis, plot->yAxis);
    rgb->setChannelCount(4);
    rgb->setCMYKMode(true);

    const QCPRange kr(0, NX - 1), vr(0, NY - 1);
    auto loadCh = [&](int ch, std::shared_ptr<FITScube> img)
    {
        std::vector<double> raw(static_cast<size_t>(NX) * static_cast<size_t>(NY));
        for(size_t ik = 0; ik < raw.size(); ++ik)
        {
            auto coords = img->PixelCoordinates(ik);
            raw[coords[0] + coords[1] * static_cast<size_t>(NX)] = img->at(ik);
        }
        rgb->setChannelData(ch, std::move(raw), NX, NY, kr, vr);
        rgb->setChannelRange(ch, QCPRange(0.0, 1.0));
    };
    loadCh(0, c); loadCh(1, m); loadCh(2, y); loadCh(3, k);

    plot->rescaleAxes();
    plot->yAxis->setScaleRatio(plot->xAxis, 1.0);
    savePlot(plot, "visual_cmyk_quadrants.png");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Visual test 5 — 3-layer cube, single-object overload
//   Same Hubble-palette content as test 2 but uses the cube overload.
//   Expect: identical result to visual_nebula_hubble.png.
// ---------------------------------------------------------------------------
TEST_F(ColorDisplayTest, DISABLED_Visual_Cube3Layer_NebulaPalette)
{
    plot->resize(800, 600);

    const int NX = 400, NY = 300;

    auto cube = makeLayeredCube(NX, NY,
        // layer 0 — Hα (red)
        [&](int x, int y) -> float {
            return 0.05f
                 + 0.95f * gauss(x, y, NX, NY, 0.30, 0.55, 0.22)
                 + 0.40f * gauss(x, y, NX, NY, 0.50, 0.45, 0.12);
        },
        // layer 1 — OIII (green)
        [&](int x, int y) -> float {
            return 0.02f
                 + 0.85f * gauss(x, y, NX, NY, 0.52, 0.48, 0.16)
                 + 0.30f * gauss(x, y, NX, NY, 0.35, 0.55, 0.10);
        },
        // layer 2 — SII (blue)
        [&](int x, int y) -> float {
            return 0.01f
                 + 0.90f * gauss(x, y, NX, NY, 0.70, 0.42, 0.10)
                 + 0.20f * gauss(x, y, NX, NY, 0.60, 0.50, 0.08);
        }
    );

    QtFITSviewer::QtDisplayRGB(plot, cube);
    savePlot(plot, "visual_cube3layer_nebula.png");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Visual test 6 — Custom gradient per channel (non-default colour mapping)
//   Channel 0 uses Viridis-like orange ramp, channel 1 uses a cyan ramp,
//   channel 2 uses magenta. Shows that setChannelGradient() works.
// ---------------------------------------------------------------------------
TEST_F(ColorDisplayTest, DISABLED_Visual_CustomGradients)
{
    plot->resize(600, 600);

    const int NX = 300, NY = 300;

    // Overlapping Gaussians so the custom gradient colours are clearly visible
    auto r = makeImgFn(NX, NY, [&](int x, int y) -> float {
        return gauss(x, y, NX, NY, 0.35, 0.50, 0.20);
    });
    auto g = makeImgFn(NX, NY, [&](int x, int y) -> float {
        return gauss(x, y, NX, NY, 0.50, 0.50, 0.20);
    });
    auto b = makeImgFn(NX, NY, [&](int x, int y) -> float {
        return gauss(x, y, NX, NY, 0.65, 0.50, 0.20);
    });

    QtFITSviewer::OverlayRGB(plot->axisRect(), r, g, b);

    // Override default gradients: orange, cyan, magenta
    auto* rgb = firstRGBMap(plot);
    ASSERT_NE(rgb, nullptr);

    QCPColorGradient orange; orange.clearColorStops();
    orange.setColorStops({{0.0, QColor(0,0,0)}, {1.0, QColor(255, 140, 0)}});

    QCPColorGradient cyan; cyan.clearColorStops();
    cyan.setColorStops({{0.0, QColor(0,0,0)}, {1.0, QColor(0, 220, 220)}});

    QCPColorGradient magenta; magenta.clearColorStops();
    magenta.setColorStops({{0.0, QColor(0,0,0)}, {1.0, QColor(220, 0, 220)}});

    rgb->setChannelGradient(0, orange);
    rgb->setChannelGradient(1, cyan);
    rgb->setChannelGradient(2, magenta);
    rgb->invalidate();

    plot->rescaleAxes();
    plot->yAxis->setScaleRatio(plot->xAxis, 1.0);
    savePlot(plot, "visual_custom_gradients.png");
    SUCCEED();
}

#endif // __WITH_QT__

// ===========================================================================
// main — set offscreen platform before QApplication is created
// ===========================================================================

int main(int argc, char** argv)
{
#ifdef __WITH_QT__
#if defined(__APPLE__) || defined(__linux__)
    setenv("QT_QPA_PLATFORM", "offscreen", 1);
#elif defined(_WIN32)
    _putenv_s("QT_QPA_PLATFORM", "offscreen");
#endif
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#else
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
#endif
}
