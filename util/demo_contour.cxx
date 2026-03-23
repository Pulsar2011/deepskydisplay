//
//  demo_contour.cxx
//  DeepSkyTools
//
//  Standalone visual demo: two windows, eight panels total, all using the
//  same synthetic scalar field rendered as a grayscale QCPColorMap.
//
//  Window A — flat pen colour, four panels:
//    A1  Solid white 1px,      8pt labels,  8 levels  — baseline
//    A2  Dashed cyan 1.5px,   12pt labels,  6 levels  — dashed + large text
//    A3  Dotted red  1px,   no labels,     10 levels  — text suppressed
//    A4  Thin yellow 0.8px, no labels,     20 levels  — dense topographic
//
//  Window B — level-dependent colour gradient, four panels:
//    B1  gpSpectrum, solid 1.2px,   8pt labels, 10 levels
//    B2  gpHot,      dashed 1.0px, 10pt labels,  8 levels
//    B3  gpCold,     solid  1.0px, no labels,   12 levels
//    B4  gpJet,      dotted 1.2px,  7pt labels, 10 levels
//
//  Build:  make demo_contour
//  Run:    ./build/exe/demo_contour
//
//  Copyright (c) 2025, All rights reserved
//

#include <QApplication>
#include <DSTdisplay/QtDisplay.h>   // QCustomPlot, QCPColorMap, QCPColorGradient, …
#include "QCustomContours.h"

#include <cmath>
#include <vector>

// ---------------------------------------------------------------------------
// Shared scalar field — filled once in main(), read by both helpers.
// ---------------------------------------------------------------------------
static constexpr int NX = 200;
static constexpr int NY = 200;

// ---------------------------------------------------------------------------
// applyDarkTheme — configure axis colours for a dark background panel.
// ---------------------------------------------------------------------------
static void applyDarkTheme(QCustomPlot* plot)
{
    plot->setBackground(QBrush(QColor(30, 30, 30)));
    for (auto* ax : { plot->xAxis, plot->yAxis, plot->xAxis2, plot->yAxis2 })
    {
        ax->setTickLabelColor(Qt::white);
        ax->setLabelColor(Qt::white);
        ax->setBasePen(QPen(Qt::white));
        ax->setTickPen(QPen(Qt::white));
        ax->setSubTickPen(QPen(Qt::white));
    }
    plot->xAxis->setLabel("x  (pixels)");
    plot->yAxis->setLabel("y  (pixels)");
}

// ---------------------------------------------------------------------------
// addColorMap — attach a grayscale QCPColorMap + scale bar to *plot*.
// ---------------------------------------------------------------------------
static void addColorMap(QCustomPlot* plot,
                        const std::vector<std::vector<double>>& Z)
{
    QCPColorMap* cm = new QCPColorMap(plot->xAxis, plot->yAxis);
    cm->data()->setSize(NX, NY);
    cm->data()->setRange(QCPRange(0, NX - 1), QCPRange(0, NY - 1));
    for (int j = 0; j < NY; ++j)
        for (int i = 0; i < NX; ++i)
            cm->data()->setCell(i, j, Z[j][i]);
    cm->setInterpolate(true);

    QCPColorScale* cs = new QCPColorScale(plot);
    plot->plotLayout()->addElement(0, 1, cs);
    cs->setType(QCPAxis::atRight);
    cs->setGradient(QCPColorGradient::gpGrayscale);
    cs->axis()->setTickLabelColor(Qt::white);
    cs->axis()->setLabelColor(Qt::white);
    cs->axis()->setBasePen(QPen(Qt::white));
    cs->axis()->setTickPen(QPen(Qt::white));
    cs->axis()->setSubTickPen(QPen(Qt::white));

    cm->setColorScale(cs);
    cm->setGradient(QCPColorGradient::gpGrayscale);
    cm->rescaleDataRange();

    QCPMarginGroup* mg = new QCPMarginGroup(plot);
    plot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, mg);
    cs->setMarginGroup(QCP::msBottom | QCP::msTop, mg);
}

// ---------------------------------------------------------------------------
// makePanel — create a panel with a flat-colour QPen.
// ---------------------------------------------------------------------------
static QCustomPlot* makePanel(
    const std::vector<std::vector<double>>& Z,
    const QString& title,
    const QPen& pen, int labelFontSize, int nLevels,
    int xOff, int yOff)
{
    auto* plot = new QCustomPlot();
    plot->setWindowTitle(title);
    plot->resize(600, 520);
    plot->move(xOff, yOff);

    applyDarkTheme(plot);
    addColorMap(plot, Z);

    QCustomContours c(Z, static_cast<size_t>(nLevels));
    c.setPen(pen);
    c.setLabelFontSize(labelFontSize);
    c.render(plot);

    plot->rescaleAxes();
    plot->replot();
    plot->show();
    return plot;
}

// ---------------------------------------------------------------------------
// makeGradientPanel — create a panel with level-dependent colour.
//
// @param pen     Controls width and style only; colour is overridden
//                per level by the gradient.
// @param preset  Any QCPColorGradient::GradientPreset value
//                (gpSpectrum, gpHot, gpCold, gpJet, gpViridis, …).
// ---------------------------------------------------------------------------
static QCustomPlot* makeGradientPanel(
    const std::vector<std::vector<double>>& Z,
    const QString& title,
    QCPColorGradient::GradientPreset preset,
    const QPen& pen, int labelFontSize, int nLevels,
    int xOff, int yOff)
{
    auto* plot = new QCustomPlot();
    plot->setWindowTitle(title);
    plot->resize(600, 520);
    plot->move(xOff, yOff);

    applyDarkTheme(plot);
    addColorMap(plot, Z);

    QCustomContours c(Z, static_cast<size_t>(nLevels));
    c.setPen(pen);
    c.setColorGradient(QCPColorGradient(preset));
    c.setLabelFontSize(labelFontSize);
    c.render(plot);

    plot->rescaleAxes();
    plot->replot();
    plot->show();
    return plot;
}

// ===========================================================================
// main
// ===========================================================================
int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    // -----------------------------------------------------------------------
    // Synthetic scalar field: sum of five anisotropic Gaussians.
    // -----------------------------------------------------------------------
    std::vector<std::vector<double>> Z(NY, std::vector<double>(NX, 0.0));

    struct Bump { double cx, cy, ax, ay, angle, amp; };
    const std::vector<Bump> bumps = {
        //  cx     cy     σx    σy   rotation  amplitude
        {  60.0,  60.0, 30.0, 20.0,  0.4,      1.00 },
        { 140.0,  80.0, 20.0, 35.0, -0.6,      0.75 },
        { 100.0, 150.0, 40.0, 25.0,  0.0,     -0.60 },
        {  50.0, 150.0, 15.0, 15.0,  0.0,      0.50 },
        { 160.0, 155.0, 25.0, 18.0,  0.9,      0.80 },
    };

    for (int j = 0; j < NY; ++j)
        for (int i = 0; i < NX; ++i)
        {
            double v = 0.0;
            for (const auto& b : bumps)
            {
                const double dx = i - b.cx,  dy = j - b.cy;
                const double ca = std::cos(b.angle), sa = std::sin(b.angle);
                const double u =  ca * dx + sa * dy;
                const double w = -sa * dx + ca * dy;
                v += b.amp * std::exp(-(u*u) / (2.0*b.ax*b.ax)
                                      -(w*w) / (2.0*b.ay*b.ay));
            }
            Z[j][i] = v;
        }

    // =======================================================================
    // Window A — flat pen colour
    // =======================================================================

    // A1 — solid white 1px, 8pt labels, 8 levels  (baseline)
    const QPen penA1(QColor(255, 255, 255, 200), 1.0);

    // A2 — dashed cyan 1.5px, 12pt labels, 6 levels
    QPen penA2(QColor(0, 220, 220), 1.5);
    penA2.setStyle(Qt::DashLine);

    // A3 — dotted red 1px, no labels, 10 levels
    QPen penA3(QColor(220, 60, 60), 1.0);
    penA3.setStyle(Qt::DotLine);

    // A4 — thin yellow 0.8px, no labels, 20 dense levels
    const QPen penA4(QColor(255, 230, 80), 0.8);

    QCustomPlot* a1 = makePanel(Z,
        "A1 — Solid white 1px | 8pt labels | 8 levels",
        penA1,  8,  8,   20,  40);

    QCustomPlot* a2 = makePanel(Z,
        "A2 — Dashed cyan 1.5px | 12pt labels | 6 levels",
        penA2, 12,  6,  640,  40);

    QCustomPlot* a3 = makePanel(Z,
        "A3 — Dotted red 1px | no labels | 10 levels",
        penA3,  0, 10,   20, 600);

    QCustomPlot* a4 = makePanel(Z,
        "A4 — Thin yellow 0.8px | no labels | 20 dense levels",
        penA4,  0, 20,  640, 600);

    // =======================================================================
    // Window B — level-dependent colour gradient
    //
    // The QPen here defines width and style only; the colour of each contour
    // line is taken from the gradient at the level's normalised position.
    // =======================================================================

    // B1 — gpSpectrum, solid 1.2px, 8pt labels, 10 levels
    const QPen penB1(Qt::black, 1.2);                             // colour ignored

    // B2 — gpHot, dashed 1.0px, 10pt labels, 8 levels
    QPen penB2(Qt::black, 1.0);
    penB2.setStyle(Qt::DashLine);

    // B3 — gpCold, solid 1.0px, no labels, 12 levels
    const QPen penB3(Qt::black, 1.0);

    // B4 — gpJet, dash-dot 1.2px, 7pt labels, 10 levels
    QPen penB4(Qt::black, 1.2);
    penB4.setStyle(Qt::DashDotLine);

    QCustomPlot* b1 = makeGradientPanel(Z,
        "B1 — Spectrum gradient | solid 1.2px | 8pt labels | 10 levels",
        QCPColorGradient::gpSpectrum, penB1,  8, 10,   20,  40);

    QCustomPlot* b2 = makeGradientPanel(Z,
        "B2 — Hot gradient | dashed 1px | 10pt labels | 8 levels",
        QCPColorGradient::gpHot,      penB2, 10,  8,  640,  40);

    QCustomPlot* b3 = makeGradientPanel(Z,
        "B3 — Cold gradient | solid 1px | no labels | 12 levels",
        QCPColorGradient::gpCold,     penB3,  0, 12,   20, 600);

    QCustomPlot* b4 = makeGradientPanel(Z,
        "B4 — Jet gradient | dash-dot 1.2px | 7pt labels | 10 levels",
        QCPColorGradient::gpJet,      penB4,  7, 10,  640, 600);

    (void)a1; (void)a2; (void)a3; (void)a4;
    (void)b1; (void)b2; (void)b3; (void)b4;

    return app.exec();
}
