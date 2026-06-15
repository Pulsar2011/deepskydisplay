//
//  QtFitsPloting.h
//  DeepSkyTools — public plotting utilities for the DSTdisplay library
//
//  Created by GILLARD William
//  Centre de Physique des Particules de Marseille
//  Licensed under CC BY-NC 4.0
//

#pragma once

#ifdef __WITH_QT__
#include <DSTdisplay/QtDisplay.h>   // QCPColorGradient, QColor
#include <DSTfits/FITSdata.h>   // QCPColorGradient, QColor

namespace QtFITS
{

    /**
     * @brief Build a Viridis-equivalent QCPColorGradient.
     *
     * QCustomPlot v2.1.1 does not include a @c gpViridis preset. This function
     * constructs an equivalent gradient from explicit colour stops matching the
     * Matplotlib Viridis palette (dark purple → blue → teal → green → yellow).
     *
     * @return A @c QCPColorGradient configured with 9 Viridis colour stops.
     */
    inline QCPColorGradient makeViridisGradient()
    {
        QCPColorGradient g;
        g.clearColorStops();
        g.setColorStops({
            {0.000, QColor( 68,   1,  84)},
            {0.125, QColor( 72,  40, 120)},
            {0.250, QColor( 62,  83, 140)},
            {0.375, QColor( 49, 104, 142)},
            {0.500, QColor( 38, 130, 142)},
            {0.625, QColor( 31, 158, 137)},
            {0.750, QColor( 53, 183, 121)},
            {0.875, QColor(110, 206,  88)},
            {1.000, QColor(253, 231,  37)},
        });
        return g;
    }

    /**
     * @brief Pack two @c DSL::dtype values into a single 32-bit key.
     *
     * @details Used internally by the plotting dispatchers to select a
     * type-specific code path from a @c std::unordered_map keyed on the
     * combination of the X and Y column types without requiring a nested
     * switch or a @c std::pair key.
     *
     * @param a Type of the first (e.g. x-axis) column.
     * @param b Type of the second (e.g. y-axis) column.
     * @return A @c uint32_t where the upper 16 bits hold @p a and the lower
     *         16 bits hold @p b.
     */
    constexpr uint32_t dtype_pair(DSL::dtype a, DSL::dtype b)
    {
        return (static_cast<uint32_t>(a) << 16) | static_cast<uint32_t>(b);
    }

} // namespace QtFITS

// ---------------------------------------------------------------------------
// RGB / multi-channel colour display
// ---------------------------------------------------------------------------

/**
 * @defgroup RGBDisplay True-colour FITS image display
 * @ingroup  DSTdisplay
 * @brief    Multi-channel colour compositing for @c QCustomPlot, designed for
 *           astronomical narrow-band and broad-band imaging.
 *
 * @details
 * The RGB display subsystem extends the monochromatic
 * @c QtFITSviewer::Overlay() / @c QtFITSviewer::QtDisplay() pipeline to render
 * two, three, or four scalar FITS channels as a single composited colour image.
 * It is built on top of QCustomPlot (v2.1.1) without modifying any QCP sources.
 *
 * ### Architecture overview
 *
 * @code{.unparsed}
 *  FITScube   FITScube   FITScube
 *    (red)     (green)    (blue)
 *      │          │          │
 *      ▼          ▼          ▼
 *   stretch    stretch    stretch     ← OPTION token ("logz", "sqrtz", …)
 *      │          │          │
 *      ▼          ▼          ▼
 *  colorize   colorize   colorize    ← QCPColorGradient::colorize()
 *  (black→R)  (black→G)  (black→B)      (customisable per channel)
 *      │          │          │
 *      └──────────┴──────────┘
 *                 │
 *          CompositionMode_Plus       ← QPainter additive blend
 *                 │
 *          QCPColorMapRGB::draw()     ← renders to QCustomPlot canvas
 * @endcode
 *
 * ### Entry points
 *
 * | Method | When to use |
 * |:-------|:------------|
 * | @c QtFITSviewer::QtDisplayRGB(plot, cube)       | Single 3-layer FITS cube, sets up the full plot |
 * | @c QtFITSviewer::QtDisplayRGB(plot, r, g, b)   | Three separate FITS files, sets up the full plot |
 * | @c QtFITSviewer::OverlayRGB(rect, cube)         | Single cube, lower-level (no axis setup) |
 * | @c QtFITSviewer::OverlayRGB(rect, r, g, b)     | Three cubes, lower-level (no axis setup) |
 *
 * ### OPTION string tokens
 *
 * All display methods accept an @c OPTION string that selects the per-pixel
 * stretch applied before gradient mapping.  Tokens are case-insensitive and
 * the same stretch is applied to all channels.
 *
 * | Token    | Stretch formula        | Best for                                 |
 * |:---------|:-----------------------|:-----------------------------------------|
 * | *(empty)*| @f$ v @f$ (linear)     | Pre-processed data, uniform backgrounds  |
 * | @c logz  | @f$ \log_{10}(v) @f$   | High dynamic range (stars, bright cores) |
 * | @c sqrtz | @f$ \sqrt{v} @f$       | Moderate contrast enhancement            |
 * | @c quadz | @f$ v^2 @f$            | Suppressing faint noise                  |
 * | @c sinhz | @f$ \sinh(v) @f$       | Non-linear enhancement of bright features|
 *
 * ### Filter-association example (Hubble palette)
 *
 * @code
 *   // Load narrow-band exposures
 *   auto ha   = std::make_shared<FITScube>(...);   // Hα  → red
 *   auto oiii = std::make_shared<FITScube>(...);   // OIII → green
 *   auto sii  = std::make_shared<FITScube>(...);   // SII  → blue
 *
 *   // High-level display (sets axis labels, 1:1 aspect ratio)
 *   QtFITSviewer::QtDisplayRGB(plot, ha, oiii, sii, "sqrtz");
 *
 *   // Optional: replace the default black→red ramp with a warm orange
 *   auto* rgb = dynamic_cast<QCPColorMapRGB*>(plot->plottable(0));
 *   QCPColorGradient orange;
 *   orange.clearColorStops();
 *   orange.setColorStops({{0.0, Qt::black}, {1.0, QColor(255,140,0)}});
 *   rgb->setChannelGradient(0, orange);
 *   rgb->invalidate();
 *   plot->replot();
 * @endcode
 *
 * @{
 */

/**
 * @class QCPColorMapRGB
 * @brief Multi-channel colour plottable for QCustomPlot.
 *
 * @details
 * Stores up to four scalar channel arrays and composites them into a single
 * @c QImage using @c QCPColorGradient::colorize() and
 * @c QPainter::CompositionMode_Plus (additive blending).
 *
 * The class is a @c QCPAbstractPlottable subclass; instances are owned and
 * rendered by the @c QCustomPlot to whose axes they are attached.
 *
 * Typical usage is through the high-level helpers
 * @c QtFITSviewer::QtDisplayRGB() and @c QtFITSviewer::OverlayRGB() rather
 * than directly.  Direct use is needed only when you want to override the
 * default colour gradients, toggle CMYK mode, or read back the composed image.
 *
 * ### Data layout
 *
 * Pixel data must be supplied as a flat @c std::vector<double> with index
 * layout @c idx = @c x + @c y × @c nx, where @c y = 0 is at the **bottom**
 * of the image (FITS convention).  The class flips the y-axis internally when
 * building the @c QImage.
 *
 * ### Default channel gradients
 *
 * | Channel | RGB role | CMYK role | Default ramp       |
 * |:-------:|:---------|:----------|:-------------------|
 * | 0       | Red      | Cyan      | black → red        |
 * | 1       | Green    | Magenta   | black → green      |
 * | 2       | Blue     | Yellow    | black → blue       |
 * | 3       | —        | Key/Black | black → white      |
 *
 * @see QtFITSviewer::QtDisplayRGB()
 * @see QtFITSviewer::OverlayRGB()
 */
class QCPColorMapRGB;   // declared and implemented in QtFITSviewer.h / QtFitsPloting.cxx

/**
 * @class QtFITSviewer
 * @brief Static collection of QCustomPlot-based display methods for FITS data.
 *
 * @details
 * All methods are static; the class acts as a namespace for FITS→QCP rendering
 * utilities.  The RGB sub-API (QtDisplayRGB / OverlayRGB) is documented below.
 */
class QtFITSviewer;     // full declaration in QtFITSviewer.h

/**
 * @fn void QtFITSviewer::QtDisplayRGB(QCustomPlot *qPlot,
 *         const std::shared_ptr<const FITScube> &cube,
 *         std::string OPTION)
 * @brief Display a 3-layer FITScube as a true-colour RGB image.
 *
 * @details Sets axis labels (@c "pixel [px]"), locks the aspect ratio to 1:1,
 *          then calls @c OverlayRGB(plot->axisRect(), cube, OPTION).
 *
 *          Layer 0 → red, layer 1 → green, layer 2 → blue.
 *
 * @param qPlot  Target @c QCustomPlot widget.
 * @param cube   3-D FITS cube with NAXIS ≥ 3 and at least 3 layers.
 * @param OPTION Stretch token — see the @ref RGBDisplay module for the full table.
 *
 * @throws DSL::FITSexception  If @p cube has fewer than 3 dimensions or layers.
 * @ingroup RGBDisplay
 */

/**
 * @fn void QtFITSviewer::QtDisplayRGB(QCustomPlot *qPlot,
 *         const std::shared_ptr<const FITScube> &red,
 *         const std::shared_ptr<const FITScube> &green,
 *         const std::shared_ptr<const FITScube> &blue,
 *         std::string OPTION)
 * @brief Display three monochromatic FITScubes as a true-colour RGB image.
 *
 * @details Sets axis labels and aspect ratio, then delegates to
 *          @c OverlayRGB(plot->axisRect(), red, green, blue, OPTION).
 *          All three cubes must share the same NAXIS1 and NAXIS2.
 *
 * @param qPlot  Target @c QCustomPlot widget.
 * @param red    Cube mapped to the red   channel (ch 0).
 * @param green  Cube mapped to the green channel (ch 1).
 * @param blue   Cube mapped to the blue  channel (ch 2).
 * @param OPTION Stretch token — see the @ref RGBDisplay module for the full table.
 * @ingroup RGBDisplay
 */

/**
 * @fn void QtFITSviewer::OverlayRGB(QCPAxisRect *qPlot,
 *         const std::shared_ptr<const FITScube> &red,
 *         const std::shared_ptr<const FITScube> &green,
 *         const std::shared_ptr<const FITScube> &blue,
 *         std::string OPTION)
 * @brief Overlay three monochromatic FITScubes as an RGB image on an existing
 *        axis rect.
 *
 * @details Lower-level entry point that populates a @c QCPColorMapRGB without
 *          modifying axis labels or aspect ratio.
 *
 *          The method iterates all unmasked pixels of each cube, applies the
 *          chosen stretch, and stores the results in per-channel
 *          @c std::vector<double> arrays inside a newly allocated
 *          @c QCPColorMapRGB.  The intensity range of each channel is
 *          auto-scaled from its data.  A thread-safe @c replot() is
 *          scheduled via @c QMetaObject::invokeMethod().
 *
 *          To customise gradients after this call:
 * @code
 *   QtFITSviewer::OverlayRGB(rect, ha, oiii, sii);
 *   auto* rgb = dynamic_cast<QCPColorMapRGB*>(plot->plottable(0));
 *   rgb->setChannelGradient(0, myOrangeGradient);
 *   rgb->invalidate();
 *   plot->replot();
 * @endcode
 *
 * @param qPlot  Target @c QCPAxisRect.
 * @param red    Cube for the red   channel.
 * @param green  Cube for the green channel.
 * @param blue   Cube for the blue  channel.
 * @param OPTION Stretch token — see the @ref RGBDisplay module for the full table.
 * @ingroup RGBDisplay
 */

/**
 * @fn void QtFITSviewer::OverlayRGB(QCPAxisRect *qPlot,
 *         const std::shared_ptr<const FITScube> &cube,
 *         std::string OPTION)
 * @brief Overlay a 3-layer FITScube as an RGB image on an existing axis rect.
 *
 * @details Convenience overload of @c OverlayRGB(rect, r, g, b, OPTION) that
 *          accepts a single 3-D cube.  Layer 0 → ch 0 (red), layer 1 → ch 1
 *          (green), layer 2 → ch 2 (blue).
 *
 * @param qPlot  Target @c QCPAxisRect.
 * @param cube   3-D FITS cube (NAXIS ≥ 3, Size(3) ≥ 3).
 * @param OPTION Stretch token — see the @ref RGBDisplay module for the full table.
 *
 * @throws DSL::FITSexception (@c BAD_DIMEN) if @p cube has fewer than 3
 *         dimensions or fewer than 3 layers.
 * @ingroup RGBDisplay
 */

/** @} */ // end of RGBDisplay group

#endif // __WITH_QT__
