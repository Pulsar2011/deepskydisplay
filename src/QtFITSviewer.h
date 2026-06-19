//
//  QfitsViewer
//
//  Created by GILLARD William on 08/10/2014.
//  Copyright (c) 2014 Centre de Physique des Particules de Marseille (CPPM) - Université Aix-Marseille (AMU). All rights reserved.
//

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDebug>

#include <QtGlobal>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QDesktopWidget>
#else
#include <QScreen>
#include <QGuiApplication>
#endif

#include <QScreen>
#include <QMessageBox>
#include <QMetaEnum>
#include <QMainWindow>
#include <QTimer>
#include <QApplication>

#include <DSTdisplay/QtDisplay.h>
#include "ui_QtFITSviewer.h"

#include <DSTfits/FITSimg.h>
#include <DSTfits/FITSmanager.h>

using namespace DSL;

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future> 
#include <exception>

namespace QtFITS
{

    extern std::mutex io_mutex;
    extern std::mutex msg_m;
    extern std::condition_variable msg_cv;
    extern bool finished;

    typedef std::map<std::string,int> qPlotList;

    /**
     *  @enum QtFITS::zScale
     *  @details Defines the scaling applied to the colour axis (z) during Qt
     *           display. This setting does not affect ROOT-based display.
     */
    enum zScale
    {
        fLIN,   ///< Linear scale
        fLOG,   ///< Logaritmic scale
        fSINH,  ///< Sinus hyperbolic scale
        fQUAD,  ///< Quadratic scale
        fSQRT,  ///< Squared root scale
    };
    
    /**
     *  @class QtFITS::rgb QtFITSviewer.h "QtFITSviewer.h"
     *  @brief RGBA colour value used by plotting views.
     *  @details Stores red, green, blue, and alpha components as unsigned
     *           integers in the range [0, 255]. Component values are clamped
     *           on construction.
     */
    class rgb
    {
    public:
        unsigned int red;    ///< The quantity of red, ranging from 0 to 255
        unsigned int blue;    ///< The quantity of blue, ranging from 0 to 255
        unsigned int green;    ///< The quantity of green, ranging from 0 to 255
        unsigned int alpha;    ///< The transparency : 0 transparant, 255 opac
        
        /**
         *  @brief Default constructor
         *  @details Default constructor
         *  @param r : Red color fraction from 0 to 255
         *  @param g : Green color fraction from 0 to 255
         *  @param b : Blue color fraction from 0 to 255
         *  @param a : Transparancy.
         *        * 0 \f$\rightarrow\f$ Transparent
         *        * 255 \f$\rightarrow\f$ Opac
         */
        rgb(unsigned int r, unsigned int g, unsigned int b, unsigned int a = 255)
        {
            red   = (r < 255) ? r : 255;
            green = (g < 255) ? g : 255;
            blue  = (b < 255) ? b : 255;
            alpha = (a < 255) ? a : 255;
        }
        
        /**
         *  @brief Copy constructor
         *  @details Copy constructor
         *  @param color : Color to copy.
         */
        rgb(const rgb &color)
        {
            red   = color.red;
            green = color.green;
            blue  = color.blue;
            alpha = color.alpha;
        }
        
        /**
         *  @brief Default destructor
         *  @details Default destructor
         */
        ~rgb()
        {
            red   = 0;
            green = 0;
            blue  = 0;
            alpha = 0;
        }
        
        static const rgb qRed;   ///< Red color
        static const rgb qBlue;  ///< Blue color
        static const rgb qGreen; ///< Green color
        static const rgb qBlack; ///< Black color
        static const rgb qWhite; ///< White color
        static const rgb qGray;  ///< Neutral gray color
    };
    
    const rgb qRed  = rgb(255,0  ,0  ,255);
    const rgb qGreen= rgb(0  ,255,0  ,255);
    const rgb qBlue = rgb(0  ,0  ,255,255);
    const rgb qBlack= rgb(255,255,255,255);
    const rgb qWhite= rgb(0  ,0  ,0  ,255);
    const rgb qGray = rgb(128,128,128,255);

    /**
     * @struct pixelDistribution
     * @brief Pixel intensity distribution computed from a FITS image.
     *
     * @details Holds the result of a histogram analysis of pixel values,
     * including the normalised probability density function (PDF) and the
     * cumulative distribution function (CDF). Produced by
     * QtFITSviewer::pixHistogram().
     */
    struct pixelDistribution
    {
        pixelDistribution(): bin_edges(), pdf(), cdf() {};
        pixelDistribution(const pixelDistribution& o):
            bin_edges(o.bin_edges), pdf(o.pdf), cdf(o.cdf) {};
        pixelDistribution& operator=(const pixelDistribution& o)
        {
            if(this != &o)
            {
                bin_edges = o.bin_edges;
                pdf       = o.pdf;
                cdf       = o.cdf;
            }
            return *this;
        }

        ~pixelDistribution() = default;
        /// @brief Dump the distribution table to @c stdout (bin edges, PDF, CDF columns).
        void print() const
        {
            for(int i = 0; i < bin_edges.size(); i++)
                std::cout << bin_edges[i] << "    " << pdf[i] << "    " << cdf[i] << std::endl;
        }

        QVector<double> bin_edges; ///< Left edge of each histogram bin (size = nBins + 2)
        QVector<double> pdf;       ///< Normalised PDF: fraction of pixels per bin
        QVector<double> cdf;       ///< Cumulative sum of @c pdf (running integral)
    };
}

/**
 * @class QCPColorMapRGB
 * @brief Multi-channel colour plottable for QCustomPlot, designed for true-colour
 *        FITS image visualisation.
 *
 * @details
 * @c QCPColorMapRGB extends @c QCPAbstractPlottable to render up to four independent
 * scalar channels (R, G, B, and optionally K) as a single composited colour image
 * inside a QCustomPlot canvas.  It is the rendering engine behind
 * @c QtFITSviewer::QtDisplayRGB() and @c QtFITSviewer::OverlayRGB().
 *
 * ### Compositing pipeline
 *
 * Each active channel is processed independently:
 * -# The raw double array is passed to @c QCPColorGradient::colorize() using the
 *    channel's own intensity range and gradient (default: single-hue black-to-colour
 *    ramp).  This produces a @c QImage in @c Format_ARGB32_Premultiplied.
 * -# The per-channel images are painted onto a black base image using
 *    @c QPainter::CompositionMode_Plus (additive blending), giving correct
 *    component-wise colour addition: e.g. red + green → yellow.
 *
 * In CMYK mode (enabled by @c setCMYKMode()) the four channel values are first
 * converted pixel-by-pixel to RGB using the standard formula
 * \f[
 *   R = (1-C)(1-K), \quad G = (1-M)(1-K), \quad B = (1-Y)(1-K)
 * \f]
 * (components normalised to [0, 1]) and the result is written directly to the
 * composed image without the gradient step.
 *
 * The composed @c QImage is cached in @c mComposedImage and regenerated lazily
 * (only when @c mInvalidated is @c true).
 *
 * ### Data layout
 *
 * Pixel data supplied to @c setChannelData() must be a flat @c std::vector<double>
 * with linear index @c idx = @c x + @c y * @c nx, where x ∈ [0, nx) and y ∈ [0, ny),
 * with **y = 0 at the image bottom** (FITS convention).  The compositor
 * automatically flips the y-axis when copying scan lines into the @c QImage.
 *
 * ### Default colour gradients
 *
 * | Channel | Role (RGB mode) | Default gradient |
 * |:-------:|:----------------|:-----------------|
 * | 0       | Red             | black → red       |
 * | 1       | Green           | black → green     |
 * | 2       | Blue            | black → blue      |
 * | 3       | Key / black (CMYK) | black → white  |
 *
 * Gradients can be replaced at any time with @c setChannelGradient(); this allows
 * arbitrary filter-to-colour associations (e.g. Hα → red, OIII → green, SII → blue).
 *
 * ### Relationship to QCP
 *
 * @c QCPColorMapRGB inherits @c QCPAbstractPlottable directly rather than
 * @c QCPColorMap, because @c QCPColorMap::updateMapImage() is private and
 * non-virtual.  The @c draw() implementation mirrors the logic of
 * @c QCPColorMap::draw(): it computes the screen-space @c QRectF from
 * @c coordsToPixels(), expands by half a cell, handles axis mirroring, and
 * calls @c QPainter::drawImage().
 *
 * @note The class emits no signals of its own; invalidation and recomposition are
 *       driven by the setter methods and triggered on the next @c QCustomPlot::replot().
 *
 * @since DSTdisplay v2.x (ColorDisplay branch)
 * @author GILLARD William — CPPM / Aix-Marseille Université
 *
 * @see QtFITSviewer::QtDisplayRGB()
 * @see QtFITSviewer::OverlayRGB()
 */
class QCPColorMapRGB : public QCPAbstractPlottable
{
    Q_OBJECT
public:
    /// Maximum number of channels supported (R/G/B + optional K).
    static constexpr int kMaxChannels = 4;

    /**
     * @brief Construct a @c QCPColorMapRGB and register it with the parent plot.
     * @details Initialises default single-hue gradients for all channels
     *          (black→red, black→green, black→blue, black→white) and sets
     *          every channel scale type to @c QCPAxis::stLinear.
     *          The plottable is automatically owned and managed by the
     *          @c QCustomPlot that owns @p keyAxis / @p valueAxis.
     * @param keyAxis   Horizontal (key) axis; typically @c plot->xAxis.
     * @param valueAxis Vertical (value) axis; typically @c plot->yAxis.
     */
    explicit QCPColorMapRGB(QCPAxis *keyAxis, QCPAxis *valueAxis);

    /** @brief Destructor. */
    ~QCPColorMapRGB() override;

    /**
     * @brief Supply raw pixel data for one channel.
     * @details The data vector is moved into the plottable's internal storage.
     *          Supplying data for any channel automatically sets the grid
     *          dimensions (@p nx, @p ny) and coordinate extents for all channels;
     *          the composed image is invalidated.
     * @param ch          Channel index in [0, kMaxChannels).  0 = R/C, 1 = G/M,
     *                    2 = B/Y, 3 = K.
     * @param data        Flat pixel array, @c data.size() must equal @c nx * ny.
     *                    Index layout: @c idx = x + y * nx, y = 0 at image bottom.
     * @param nx          Number of columns (NAXIS1).
     * @param ny          Number of rows    (NAXIS2).
     * @param keyRange    Plot-coordinate range in the horizontal direction
     *                    (usually @c QCPRange(0, nx-1)).
     * @param valueRange  Plot-coordinate range in the vertical direction
     *                    (usually @c QCPRange(0, ny-1)).
     * @note  Calling this method does not trigger a recompose; call
     *        @c QCustomPlot::replot() (or @c composedImage()) to update the
     *        display.
     */
    void setChannelData(int ch, std::vector<double> data, int nx, int ny,
                        const QCPRange &keyRange, const QCPRange &valueRange);

    /**
     * @brief Set the intensity range used for gradient mapping on one channel.
     * @details The range is passed directly to @c QCPColorGradient::colorize():
     *          pixel values at @c intensityRange.lower map to the start of the
     *          gradient and values at @c intensityRange.upper map to its end.
     *          Values outside the range are clamped.  Call this method after
     *          @c setChannelData() and before the next @c replot().
     * @param ch             Channel index in [0, kMaxChannels).
     * @param intensityRange [min, max] intensity window for this channel.
     */
    void setChannelRange(int ch, const QCPRange &intensityRange);

    /**
     * @brief Auto-compute the intensity range from the stored data min/max.
     * @details Scans @c mChannelRaw[ch] to find its minimum and maximum value
     *          and updates the channel's intensity range accordingly.  This is
     *          equivalent to calling @c setChannelRange() with the exact data
     *          bounds and is the most common post-load operation.
     * @param ch  Channel index in [0, kMaxChannels).
     *            If no data has been set for this channel, the method is a no-op.
     */
    void rescaleChannelRange(int ch);

    /**
     * @brief Override the colour gradient used for one channel.
     * @details By default each channel uses a single-hue gradient
     *          (black → channel colour).  This method replaces that gradient with
     *          any @c QCPColorGradient, enabling arbitrary filter-to-colour mappings
     *          (e.g. associating Hα with a warm orange ramp instead of pure red).
     * @code
     *   QCPColorGradient orange;
     *   orange.clearColorStops();
     *   orange.setColorStops({{0.0, Qt::black}, {1.0, QColor(255,140,0)}});
     *   rgbMap->setChannelGradient(0, orange);
     *   rgbMap->invalidate();
     *   plot->replot();
     * @endcode
     * @param ch        Channel index in [0, kMaxChannels).
     * @param gradient  New colour gradient.
     */
    void setChannelGradient(int ch, const QCPColorGradient &gradient);

    /**
     * @brief Select linear or logarithmic gradient mapping for one channel.
     * @details When set to @c QCPAxis::stLogarithmic the @c logarithmic flag of
     *          @c QCPColorGradient::colorize() is set to @c true, which applies
     *          a log10 mapping from intensity to gradient position.  Use this
     *          for images with high dynamic range (e.g. stellar images where
     *          the brightest source is many decades above the noise floor).
     * @param ch    Channel index in [0, kMaxChannels).
     * @param type  @c QCPAxis::stLinear (default) or @c QCPAxis::stLogarithmic.
     * @note  For a coarser but simpler alternative, the @c "LOGZ" / @c "SQRTZ"
     *        tokens in the @c OPTION string of @c OverlayRGB() / @c QtDisplayRGB()
     *        apply the stretch to the raw data before storage, which is equivalent
     *        to a linear mapping of the stretched values.
     */
    void setChannelScaleType(int ch, QCPAxis::ScaleType type);

    /**
     * @brief Set the number of active channels to composite.
     * @details The value is clamped to [1, kMaxChannels].  Channels with an index
     *          ≥ @p n are ignored during @c recompose().
     *          - @c n = 1 : single-channel pseudo-colour (monochrome input)
     *          - @c n = 2 : two-channel composite (e.g. RG)
     *          - @c n = 3 : standard RGB (default)
     *          - @c n = 4 : CMYK (requires @c setCMYKMode(true))
     * @param n  Number of channels, clamped to [1, @c kMaxChannels].
     */
    void setChannelCount(int n);

    /**
     * @brief Enable or disable CMYK compositing mode.
     * @details When @c enabled is @c true and @c channelCount() ≥ 4, channel values
     *          are interpreted as cyan (ch0), magenta (ch1), yellow (ch2), and
     *          black/key (ch3).  Each pixel is converted to RGB with
     *          \f$ R=(1-C)(1-K),\ G=(1-M)(1-K),\ B=(1-Y)(1-K) \f$
     *          (with all components normalised to [0, 1]).
     *          In CMYK mode the per-channel gradients are not used.
     * @param enabled  @c true to activate CMYK mode, @c false (default) for RGB.
     * @see setChannelCount()
     */
    void setCMYKMode(bool enabled);

    /**
     * @brief Enable or disable bilinear interpolation when the image is zoomed in.
     * @details Corresponds to the @c QPainter::SmoothPixmapTransform render hint.
     *          When @c false (default), pixels are displayed as hard-edged squares
     *          on zoom, which is often preferred for astronomical images so that
     *          individual detector pixels remain identifiable.
     * @param enabled  @c true to enable smooth interpolation.
     */
    void setInterpolate(bool enabled);

    /**
     * @brief Mark the cached composed image as stale.
     * @details Forces @c recompose() to run on the next call to @c draw() or
     *          @c composedImage().  This method must be called whenever channel
     *          data, ranges, gradients, or the channel count are changed outside
     *          the normal setter methods (e.g. after modifying data in-place).
     */
    void invalidate();

    /**
     * @brief Return the cached composed @c QImage, recomposing first if stale.
     * @details Primarily intended for unit testing and image export.  In normal
     *          rendering use, the composed image is accessed internally by @c draw().
     * @return  Const reference to the internal @c QImage in
     *          @c Format_ARGB32_Premultiplied.  The image is null if no channel
     *          data has been supplied.
     */
    const QImage& composedImage();

    /**
     * @brief Hit-test: return 0.0 if @p pos is inside the image rect, -1 otherwise.
     * @details Overrides @c QCPAbstractPlottable::selectTest().  The image rect is
     *          derived from the current key/value coordinate ranges via
     *          @c coordsToPixels().  Returns -1.0 if the plottable has no data.
     * @param pos            Screen-space position to test.
     * @param onlySelectable If @c true and the plottable is not selectable,
     *                       always returns -1.
     * @param details        Unused; set to @c nullptr.
     * @return 0.0 if the position is inside the image bounding box, -1.0 otherwise.
     */
    double selectTest(const QPointF &pos, bool onlySelectable,
                      QVariant *details = nullptr) const override;

    /**
     * @brief Return the key (horizontal) axis range of the pixel grid.
     * @param foundRange     Set to @c true if data has been loaded, @c false otherwise.
     * @param inSignDomain   Unused; present for interface compatibility.
     * @return The @c QCPRange set by the last call to @c setChannelData().
     */
    QCPRange getKeyRange(bool &foundRange,
                         QCP::SignDomain inSignDomain = QCP::sdBoth) const override;

    /**
     * @brief Return the value (vertical) axis range of the pixel grid.
     * @param foundRange     Set to @c true if data has been loaded, @c false otherwise.
     * @param inSignDomain   Unused; present for interface compatibility.
     * @param inKeyRange     Unused; present for interface compatibility.
     * @return The @c QCPRange set by the last call to @c setChannelData().
     */
    QCPRange getValueRange(bool &foundRange,
                           QCP::SignDomain inSignDomain = QCP::sdBoth,
                           const QCPRange &inKeyRange   = QCPRange()) const override;

protected:
    /**
     * @brief Render the composed colour image onto the plot.
     * @details Called by the QCP rendering engine during @c QCustomPlot::replot().
     *          Recomposes the image if @c mInvalidated is @c true, then maps
     *          the pixel grid to screen coordinates (including half-cell border
     *          expansion and axis-reversal mirroring) and paints it with
     *          @c QPainter::drawImage().
     * @param painter  QCP painter provided by the rendering engine.
     */
    void draw(QCPPainter *painter) override;

    /**
     * @brief Draw a thumbnail of the composed image in the plot legend.
     * @param painter  QCP painter provided by the rendering engine.
     * @param rect     Bounding rect allocated for this legend icon.
     */
    void drawLegendIcon(QCPPainter *painter, const QRectF &rect) const override;

private:
    std::vector<double>  mChannelRaw[kMaxChannels];       ///< Raw pixel data per channel (x + y*mNx)
    QCPColorGradient     mChannelGradient[kMaxChannels];  ///< Colour gradient per channel
    QCPRange             mChannelIntRange[kMaxChannels];  ///< [min, max] intensity window per channel
    QCPAxis::ScaleType   mChannelScaleType[kMaxChannels]; ///< Linear or logarithmic mapping per channel
    int                  mNx           = 0;               ///< Grid width  (NAXIS1)
    int                  mNy           = 0;               ///< Grid height (NAXIS2)
    QCPRange             mKeyRange;                       ///< Plot-coordinate extent in x
    QCPRange             mValueRange;                     ///< Plot-coordinate extent in y
    int                  mChannelCount = 3;               ///< Number of active channels (1–4)
    bool                 mCMYKMode     = false;           ///< If true, interpret ch0–3 as C/M/Y/K
    bool                 mInterpolate  = false;           ///< Enable smooth zoom interpolation
    bool                 mInvalidated  = true;            ///< True when mComposedImage needs rebuilding
    QImage               mComposedImage;                  ///< Cached result of recompose()
    QPixmap              mLegendIcon;                     ///< 16×16 thumbnail for the legend

    /**
     * @brief Rebuild @c mComposedImage from all active channel arrays.
     * @details In RGB mode: allocates a black base image, colourises each channel
     *          with @c QCPColorGradient::colorize() (inverting the y-axis to convert
     *          from FITS bottom-up convention to screen top-down), and additively
     *          blends the per-channel images onto the base using
     *          @c QPainter::CompositionMode_Plus.
     *          In CMYK mode: computes per-pixel CMYK→RGB conversion and writes
     *          directly to the base image.
     *          Sets @c mInvalidated to @c false on completion.
     */
    void recompose();

    /**
     * @brief Build the default single-hue gradient for channel @p ch.
     * @details Returns a gradient from black (position 0) to the channel's
     *          canonical colour (position 1):
     *          ch 0 → red, ch 1 → green, ch 2 → blue, ch ≥ 3 → white.
     * @param ch  Channel index.
     * @return    A @c QCPColorGradient with two colour stops.
     */
    static QCPColorGradient makeChannelGradient(int ch);
};

// ---------------------------------------------------------------------------
// HDU enumeration
// ---------------------------------------------------------------------------

/// Plain-old-data: describes one HDU as shown in the HDU list window.
struct HduInfo {
    int         index;   ///< 1-based HDU position in the file
    std::string extname; ///< EXTNAME keyword value, or "HDU<index>" if absent
    enum class Type { IMG, TBL, BTBL, UNKNOWN } type;
    int naxis;           ///< NAXIS value (0 = empty primary / no image)
    int naxis3;          ///< NAXIS3 when type==IMG && naxis==3; 0 otherwise
};

/// Enumerate all HDUs in an open FITSmanager instance.
/// Header-only traversal: no pixel data is loaded.
std::vector<HduInfo> enumerateHDUs(DSL::FITSmanager& fm);

// ---------------------------------------------------------------------------
// HduListWindow
// ---------------------------------------------------------------------------

class QTableWidget;

/// Main application window for the FitsViewer utility.
/// Shows all HDUs in a FITS file and dispatches viewers on selection.
/// Closing this window terminates the application.
class HduListWindow : public QMainWindow
{
    Q_OBJECT

public:
    HduListWindow(const std::string& fitsPath,
                  const std::vector<HduInfo>& hdus,
                  std::function<QWidget*(const HduInfo&)> viewerFactory,
                  QWidget* parent = nullptr);

signals:
    void hduActivated(int index);   ///< 1-based; emitted on double-click or Enter
    void colorComposeRequested();   ///< Emitted when the user picks "Color ▸ Compose color image…"

public slots:
    void openViewer(int index);     ///< Creates and shows the correct viewer for HDU N

protected:
    void closeEvent(QCloseEvent*) override;

private:
    std::string                              m_fitsPath;
    std::vector<HduInfo>                     m_hdus;
    std::function<QWidget*(const HduInfo&)>  m_viewerFactory;
    QWidget*                                 m_currentViewer = nullptr;
    QTableWidget*                            m_table = nullptr;
};

// ---------------------------------------------------------------------------

namespace Ui
{
    class MainWindow;
}

/**
 * @class QtFITSviewer QtFITSviewer.h "QtFITSviewer.h"
 * @brief Qt/QCustomPlot-based viewer for FITS images and tables.
 *
 * @details Provides a QMainWindow wrapper around QCustomPlot for displaying
 *          FITS data.  It requires Qt ≥ 4 with the QCustomPlot widget source
 *          (http://www.qcustomplot.com/).  All plotting methods are static so
 *          they can be called without an instance; the class itself is only
 *          needed when a windowed viewer with resize callbacks is required.
 */
class QtFITSviewer : public QMainWindow
{

    Q_OBJECT

protected:
    /**
     * @brief Fill a @c QCPGraph with typed (x, y) data from two FITStable columns.
     * @tparam T Scalar type of x values.
     * @tparam U Scalar type of y values.
     * @param curve Graph to populate.
     * @param tab   Source table.
     * @param xCol  x-value column name.
     * @param yCol  y-value column name.
     */
    template<typename T, typename U>
    static void _QtCurve(QCPGraph& curve, std::shared_ptr<const FITStable> tab, const std::string &xCol, const std::string &yCol)
    {
        QVector<double> xData, yData;
        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            const double x = static_cast<double>(tab->at<T>(xCol, it));
            const double y = static_cast<double>(tab->at<U>(yCol, it));
            xData.push_back(x);
            yData.push_back(y);
        }

        curve.setData(xData, yData);
    }

    /**
     * @brief Fill a @c QCPGraph with typed y data; x is the 1-based row index.
     * @tparam T Scalar type of y values.
     * @param curve Graph to populate.
     * @param tab   Source table.
     * @param yCol  y-value column name.
     */
    template<typename T>
    static void _QtCurve(QCPGraph& curve, std::shared_ptr<const FITStable> tab, const std::string &yCol)
    {
        QVector<double> xData, yData;
        size_t x = 0;
        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            x++;
            const double y = static_cast<double>(tab->at<T>(yCol, it));
            xData.push_back(static_cast<double>(x));
            yData.push_back(y);
        }

        curve.setData(xData, yData);
    }

    /**
     * @brief Fill a @c QCPGraph from a complex-valued FITStable column.
     * @details Maps the real part to the x coordinate and the imaginary part
     *          to the y coordinate (Argand-diagram style).
     * @tparam T Base real type of the complex value (e.g. @c float for TFORM 'C').
     * @param curve Graph to populate.
     * @param tab   Source table.
     * @param yCol  Name of the complex column.
     */
    template<typename T>
    static void _QtCplxCurve(QCPGraph& curve, std::shared_ptr<const FITStable> tab, const std::string &yCol)
    {
        QVector<double> xData, yData;
        
        size_t x = 0;
        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            x++;
            const std::pair<T,T> cplx = tab->at< std::pair<T,T> >(yCol, it);
            xData.push_back(static_cast<double>(cplx.first));
            yData.push_back(static_cast<double>(cplx.second));
        }
        curve.setData(xData, yData);
    }

    /**
     * @brief Plot typed vector columns from a FITStable, one graph per row.
     * @details Each table row stores a @c std::vector<T> for x and @c std::vector<U>
     *          for y; each row becomes a separate named @c QCPGraph.
     * @tparam T Element type of the x vector column.
     * @tparam U Element type of the y vector column.
     * @param plt       Target plot widget.
     * @param tab       Source table.
     * @param xCol      x-value vector column name.
     * @param yCol      y-value vector column name.
     * @param isScatter If @c true, use @c QtScatter; otherwise @c QtCurve.
     * @return One @c QCPGraph per table row.
     */
    template<typename T, typename U>
    static std::vector<std::shared_ptr<QCPGraph>> _QtVectorCurve(QCustomPlot* plt, std::shared_ptr<const FITStable> tab, const std::string &xCol, const std::string &yCol, bool isScatter=false)
    {
        size_t i=0;
        std::vector<std::shared_ptr<QCPGraph>> curves;

        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            const std::vector<T>& xVec = tab->at< std::vector<T> >(xCol, it);
            const std::vector<U>& yVec = tab->at< std::vector<U> >(yCol, it);
            auto newCurves = isScatter ? QtScatter(plt, xVec, yVec) : QtCurve(plt, xVec, yVec);
            newCurves[0]->setName(std::string("Row #"+std::to_string(i)).c_str());
            curves.push_back(newCurves[0]);
            i++;
        }

        return curves;
    }

    /**
     * @brief Plot a typed vector column (y only) from a FITStable, one graph per row.
     * @details x is the 0-based element index within each row's vector.
     * @tparam T Element type of the y vector column.
     * @param plt       Target plot widget.
     * @param tab       Source table.
     * @param yCol      y-value vector column name.
     * @param isScatter If @c true, use @c QtScatter; otherwise @c QtCurve.
     * @return One @c QCPGraph per table row.
     */
    template<typename T>
    static std::vector<std::shared_ptr<QCPGraph>> _QtVectorCurve(QCustomPlot* plt, std::shared_ptr<const FITStable> tab, const std::string &yCol, bool isScatter=false)
    {
        size_t i=0;
        std::vector<std::shared_ptr<QCPGraph>> curves;

        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            std::vector<double> xVec;
            const std::vector<T>& yVec = tab->at< std::vector< T > >(yCol, it);
            for(size_t j=0; j<yVec.size(); j++)
                xVec.push_back(static_cast<double>(j));
            auto newCurves = isScatter ? QtScatter(plt, xVec, yVec) : QtCurve(plt, xVec, yVec);
            newCurves[0]->setName(std::string("Row #"+std::to_string(i)).c_str());
            curves.push_back(newCurves[0]);
            i++;
        }

        return curves;
    }

    /**
     * @brief Plot a complex vector column from a FITStable, one graph per row.
     * @details Each element of the complex vector maps real → x, imag → y.
     * @tparam T Base real type (e.g. @c float for TFORM 'PC').
     * @param plt       Target plot widget.
     * @param tab       Source table.
     * @param yCol      Name of the complex vector column.
     * @param isScatter If @c true, use scatter style; otherwise line.
     * @return One @c QCPGraph per table row.
     */
    template<typename T>
    static std::vector<std::shared_ptr<QCPGraph>> _QtCplxVectorCurve(QCustomPlot* plt, std::shared_ptr<const FITStable> tab, const std::string &yCol, bool isScatter=false)
    {
        size_t i=0;
        std::vector<std::shared_ptr<QCPGraph>> curves;

        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            std::vector<double> xVec;
            std::vector<double> yVec;
            const std::vector< std::pair<T,T> >& data = tab->at< std::vector< std::pair<T,T> > >(yCol, it);
            for(size_t j=0; j<data.size(); j++)
            {
                xVec.push_back(static_cast<double>(data[j].first));
                yVec.push_back(static_cast<double>(data[j].second));
            }
            auto newCurves = isScatter ? QtScatter(plt, xVec, yVec) : QtCurve(plt, xVec, yVec);
            newCurves[0]->setName(std::string("Row #"+std::to_string(i)).c_str());
            curves.push_back(newCurves[0]);
            i++;
        }

        return curves;
    }

    // -----------------------------------------------------------------------
    // Core non-template plotting kernels (implemented in QtFitsPloting.cxx)
    // -----------------------------------------------------------------------

    /**
     * @brief Non-template kernel: bin (x, y[, z]) data into a 2-D density map.
     * @details All public @c QtDensityPlot overloads collect data into
     *          @c std::vector<double> and then call this function.
     *          logFlag bits: @c 0x1 = log₁₀(x), @c 0x2 = log₁₀(y),
     *          @c 0x4 = apply log₁₀ to bin counts.
     * @param qPlot  Target plot widget.
     * @param xs     x coordinates (same length as @p ys).
     * @param ys     y coordinates.
     * @param zs     Optional per-point weights; empty → each point counts as 1.
     * @param nBinsX Number of bins along the x axis.
     * @param nBinsY Number of bins along the y axis.
     * @param logFlag Bitmask controlling logarithmic scaling (see details).
     * @return Non-owning @c shared_ptr to the new @c QCPColorMap (owned by Qt).
     */
    static std::shared_ptr<QCPColorMap> _QtDensityCore(
        QCustomPlot* qPlot,
        std::vector<double> xs, std::vector<double> ys, std::vector<double> zs,
        size_t nBinsX, size_t nBinsY, uint8_t logFlag);

    /**
     * @brief Non-template kernel: bin 1-D data into a bar histogram.
     * @details All public @c QtHistogram overloads collect data into
     *          @c std::vector<double> and then call this function.
     *          logFlag bits: @c 0x1 = bin in log₁₀ space (skips ≤0 values),
     *          @c 0x2 = logarithmic y axis.
     * @param qPlot  Target plot widget.
     * @param data   1-D data to histogram.
     * @param nBins  Number of bins (0 = auto via Sturges rule).
     * @param logFlag Bitmask controlling logarithmic scaling.
     * @param OPTION Style option string forwarded to @c applyPlotOptions.
     * @return Non-owning @c shared_ptr to the new @c QCPGraph.
     */
    static std::shared_ptr<QCPGraph> _QtHistogramCore(
        QCustomPlot* qPlot,
        std::vector<double> data, size_t nBins,
        uint8_t logFlag, const std::string& OPTION);

    // -----------------------------------------------------------------------
    // Template data-collection helpers (iterate FITStable rows into double vectors)
    // -----------------------------------------------------------------------

    /**
     * @brief Collect scalar (x, y) pairs from two FITStable columns into double vectors.
     * @tparam T Scalar type of x values.
     * @tparam U Scalar type of y values.
     * @param tab  Source table.
     * @param xCol x-value column name.
     * @param yCol y-value column name.
     * @param xs   Output x vector (values appended).
     * @param ys   Output y vector (values appended).
     */
    template<typename T, typename U>
    static void _collectXY(std::shared_ptr<const FITStable> tab,
                            const std::string& xCol, const std::string& yCol,
                            std::vector<double>& xs, std::vector<double>& ys)
    {
        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            xs.push_back(static_cast<double>(tab->at<T>(xCol, it)));
            ys.push_back(static_cast<double>(tab->at<U>(yCol, it)));
        }
    }

    /**
     * @brief Collect a scalar weight column from a FITStable into a double vector.
     * @tparam W Scalar type of weight values.
     * @param tab  Source table.
     * @param zCol Weight column name.
     * @param zs   Output weight vector (values appended).
     */
    template<typename W>
    static void _collectZ(std::shared_ptr<const FITStable> tab,
                           const std::string& zCol, std::vector<double>& zs)
    {
        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
            zs.push_back(static_cast<double>(tab->at<W>(zCol, it)));
    }

    /**
     * @brief Collect a complex scalar column into separate real and imaginary vectors.
     * @details Real part → @p xs, imaginary part → @p ys.
     * @tparam T Base real type (e.g. @c float for TFORM 'C').
     * @param tab Source table.
     * @param col Complex column name.
     * @param xs  Output real-part vector.
     * @param ys  Output imaginary-part vector.
     */
    template<typename T>
    static void _collectCplxXY(std::shared_ptr<const FITStable> tab,
                                 const std::string& col,
                                 std::vector<double>& xs, std::vector<double>& ys)
    {
        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            const std::pair<T,T> cplx = tab->at< std::pair<T,T> >(col, it);
            xs.push_back(static_cast<double>(cplx.first));
            ys.push_back(static_cast<double>(cplx.second));
        }
    }

    /**
     * @brief Collect a 2-element vector column into (x, y) double vectors.
     * @details Element [0] → @p xs, element [1] → @p ys. Throws if the column
     *          has a repeat count other than 2.
     * @tparam T Element type of the vector column.
     * @param tab Source table.
     * @param col 2-element vector column name.
     * @param xs  Output x vector.
     * @param ys  Output y vector.
     */
    template<typename T>
    static void _collectVec2XY(std::shared_ptr<const FITStable> tab,
                                 const std::string& col,
                                 std::vector<double>& xs, std::vector<double>& ys)
    {
        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            const std::vector<T>& v = tab->at< std::vector<T> >(col, it);
            xs.push_back(static_cast<double>(v[0]));
            ys.push_back(static_cast<double>(v[1]));
        }
    }

    /**
     * @brief Flatten a scalar column into a 1-D double vector for histogramming.
     * @tparam T Scalar element type.
     * @param tab Source table.
     * @param col Scalar column name.
     * @param out Output vector (values appended, one per row).
     */
    template<typename T>
    static void _collectHistScalar(std::shared_ptr<const FITStable> tab,
                                    const std::string& col, std::vector<double>& out)
    {
        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
            out.push_back(static_cast<double>(tab->at<T>(col, it)));
    }

    /**
     * @brief Flatten a vector column into a 1-D double vector for histogramming.
     * @details All N elements of each row are appended, so the total output
     *          size is @c nrows × @c nElem.
     * @tparam T Element type of the vector column.
     * @param tab Source table.
     * @param col Vector column name.
     * @param out Output vector (values appended, all elements of all rows).
     */
    template<typename T>
    static void _collectHistVector(std::shared_ptr<const FITStable> tab,
                                    const std::string& col, std::vector<double>& out)
    {
        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            const std::vector<T>& v = tab->at< std::vector<T> >(col, it);
            for(const T& elem : v)
                out.push_back(static_cast<double>(elem));
        }
    }

    /**
     * @brief Flatten a complex column into a 1-D double vector for histogramming.
     * @details Both the real and imaginary parts of each row value are appended,
     *          so the total output size is @c 2 × @c nrows.
     * @tparam T Base real type.
     * @param tab Source table.
     * @param col Complex column name.
     * @param out Output vector (real then imaginary per row).
     */
    template<typename T>
    static void _collectHistCplx(std::shared_ptr<const FITStable> tab,
                                   const std::string& col, std::vector<double>& out)
    {
        for(RowIterator it = tab->begin(); it != tab->end(); ++it)
        {
            const std::pair<T,T> cplx = tab->at< std::pair<T,T> >(col, it);
            out.push_back(static_cast<double>(cplx.first));
            out.push_back(static_cast<double>(cplx.second));
        }
    }

#pragma region - Public member
public:

#pragma region constructor/destructor
    QtFITSviewer(QWidget *parent = 0); ///< @brief Default constructor
    /**
     * @brief Construct a viewer at an explicit screen position and size.
     * @param x      Left edge of the window (pixels).
     * @param y      Top edge of the window (pixels).
     * @param width  Window width (pixels).
     * @param height Window height (pixels).
     * @param parent Optional Qt parent widget.
     */
    QtFITSviewer(const int&, const int&, const int&, const int&, QWidget *parent = 0);
    
    /**
     *  @brief Default destructor
     *  @details Default destructor
     */
    virtual ~QtFITSviewer();
#pragma endregion
#pragma region Accessors
    
    /// @brief Return a pointer to the embedded @c QCustomPlot widget.
    QCustomPlot* GetPlot();

    /**
     * @brief Replace the internal @c QCustomPlot with an external instance.
     * @param plot Pointer to the @c QCustomPlot to use. Ownership is not transferred.
     */
    void SetPlot( QCustomPlot* );

#pragma endregion
#pragma region Plotting methods

    /**
     * @brief Display all pixels of a FITS image on a @c QCustomPlot.
     * @details Renders the full pixel range [0, Nelements) of @p img as a
     *          colour-mapped @c QCPColorMap. The plot is refreshed
     *          thread-safely via @c QMetaObject::invokeMethod.
     * @param qPlot  Target plot widget (must not be @c nullptr).
     * @param img    FITS image to display.
     * @param OPTION Optional style string (passed to @c applyPlotOptions).
     */
    static void QtDisplay(QCustomPlot* qPlot, const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());

    /**
     * @brief Display all pixels of a FITS image on a @c QCustomPlot.
     * @details Renders the full pixel range [0, Nelements) of @p img as a
     *          colour-mapped @c QCPColorMap. The plot is refreshed
     *          thread-safely via @c QMetaObject::invokeMethod.
     * @param qPlot  Target plottable widget (must not be @c nullptr).
     * @param img    FITS image to display.
     * @param OPTION Optional style string (passed to @c applyPlotOptions).
     */
    static void QtDisplay(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());

    /**
     * @brief Display a pixel sub-range of a FITS image on a @c QCustomPlot.
     * @param qPlot  Target plot widget.
     * @param img    FITS image to display.
     * @param iMin   First pixel index to display (0-based, inclusive).
     * @param iMax   One-past-last pixel index to display.
     * @param OPTION Optional style string.
     */
    static void QtDisplay(QCustomPlot*      , const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());

    /**
     * @brief Display a pixel sub-range of a FITS image on a @c QCustomPlot.
     * @param qPlot  Target plot widget.
     * @param img    FITS image to display.
     * @param iMin   First pixel index to display (0-based, inclusive).
     * @param iMax   One-past-last pixel index to display.
     * @param OPTION Optional style string.
     */
    static void QtDisplay(QCPAxisRect*      , const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());

    /**
     * @brief Display a FITS image on a @c QCPAxisRect with an explicit colour-scale range.
     * @details Renders the full image then clamps the colormap display range to
     *          [@p zMin, @p zMax]. Use this instead of the @c size_t overload when
     *          you want to pass intensity values (e.g. percentiles) rather than
     *          pixel index bounds.
     * @param qPlot  Target axis rect.
     * @param img    FITS image to display.
     * @param zMin   Lower intensity bound for the colour scale.
     * @param zMax   Upper intensity bound for the colour scale.
     * @param OPTION Optional style string.
     */
    static void QtDisplay(QCPAxisRect*      , const std::shared_ptr<const FITScube>&, double zMin, double zMax, std::string OPTION=std::string());

    /**
     * @brief Display a single image layer (2-D slice) of a data cube.
     * @param qPlot  Target plot widget.
     * @param img    FITS data cube.
     * @param iLayer 0-based layer index along the third axis.
     * @param OPTION Optional style string.
     */
    static void QtDisplayLayer(QCustomPlot* , const std::shared_ptr<const FITScube>&, const size_t&, std::string OPTION=std::string());

    /**
     * @brief Display a single image layer (2-D slice) of a data cube on a @c QCPAxisRect.
     * @param qPlot  Target axis rect.
     * @param img    FITS data cube.
     * @param iLayer 0-based layer index along the third axis.
     * @param OPTION Optional style string.
     */
    static void QtDisplayLayer(QCPAxisRect* , const std::shared_ptr<const FITScube>&, const size_t&, std::string OPTION=std::string());

    /**
     * @brief Overlay all pixels of a second FITS image onto an existing plot.
     * @details Adds a second @c QCPColorMap layer on top of the current axes.
     *          Useful for overlaying, e.g., a detection mask on a science image.
     * @param qPlot  Target plot widget (must already contain a base image).
     * @param img    FITS image to overlay.
     * @param OPTION Optional style string.
     */
    static void Overlay(QCustomPlot*, const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());
    /// @copydoc Overlay(QCustomPlot*, const std::shared_ptr<const FITScube>&, std::string)
    static void Overlay(QCPAxisRect*, const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());

    /**
     * @brief Overlay a pixel sub-range of a FITS image onto an existing plot.
     * @param qPlot  Target plot widget.
     * @param img    FITS image to overlay.
     * @param iMin   First pixel index (0-based, inclusive).
     * @param iMax   One-past-last pixel index.
     * @param OPTION Optional style string.
     */
    static void Overlay(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());
    /// @copydoc Overlay(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string)
    static void Overlay(QCPAxisRect*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());

    /**
     * @brief Display a FITS image with axes labelled in WCS sky coordinates.
     * @details Uses the default WCS (index 0) stored in the FITS header to
     *          convert pixel positions to sky coordinates (RA/Dec or similar).
     * @param qPlot  Target plot widget.
     * @param img    FITS image containing at least one WCS solution.
     * @param OPTION Optional style string.
     */
    static void QtDisplayInWCS(QCustomPlot*      , const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());

    /**
     * @brief Display a FITS image using a specific WCS solution.
     * @param qPlot  Target plot widget.
     * @param img    FITS image.
     * @param wcsIdx 0-based index of the WCS solution to use.
     * @param OPTION Optional style string.
     */
    static void QtDisplayInWCS(QCustomPlot*      , const std::shared_ptr<const FITScube>&, const size_t& wcsIdx, std::string OPTION=std::string());

    /**
     * @brief Display a FITS image pixel sub-range in WCS coordinates.
     * @param qPlot  Target plot widget.
     * @param img    FITS image.
     * @param iMin   First pixel index (0-based, inclusive).
     * @param iMax   One-past-last pixel index.
     * @param OPTION Optional style string.
     */
    static void QtDisplayInWCS(QCustomPlot*      , const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());

    /**
     * @brief Display a FITS image pixel sub-range using a specific WCS solution.
     * @param qPlot  Target plot widget.
     * @param img    FITS image.
     * @param iMin   First pixel index.
     * @param iMax   One-past-last pixel index.
     * @param wcsIdx 0-based index of the WCS solution.
     * @param OPTION Optional style string.
     */
    static void QtDisplayInWCS(QCustomPlot*      , const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, const size_t&, std::string OPTION=std::string());

    /// @copydoc QtDisplayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, std::string)
    static void QtDisplayInWCS(QCPAxisRect*      , const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());

    /// @copydoc QtDisplayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t& wcsIdx, std::string)
    static void QtDisplayInWCS(QCPAxisRect*      , const std::shared_ptr<const FITScube>&, const size_t& wcsIdx, std::string OPTION=std::string());

    /// @copydoc QtDisplayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string)
    static void QtDisplayInWCS(QCPAxisRect*      , const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());

    /// @copydoc QtDisplayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, const size_t&, std::string)
    static void QtDisplayInWCS(QCPAxisRect*      , const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, const size_t&, std::string OPTION=std::string());

    /**
     * @brief Overlay a FITS image onto an existing WCS-labelled plot.
     * @param qPlot  Target plot widget.
     * @param img    FITS image to overlay (must share the same WCS frame).
     * @param OPTION Optional style string.
     */
    static void OverlayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());

    /**
     * @brief Overlay a FITS image using a specific WCS solution.
     * @param qPlot  Target plot widget.
     * @param img    FITS image to overlay.
     * @param wcsIdx 0-based index of the WCS solution.
     * @param OPTION Optional style string.
     */
    static void OverlayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, std::string OPTION=std::string());

    /**
     * @brief Overlay a FITS image pixel sub-range in WCS coordinates.
     * @param qPlot  Target plot widget.
     * @param img    FITS image to overlay.
     * @param iMin   First pixel index.
     * @param iMax   One-past-last pixel index.
     * @param OPTION Optional style string.
     */
    static void OverlayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());

    /**
     * @brief Overlay a pixel sub-range of a FITS image using a specific WCS.
     * @param qPlot  Target plot widget.
     * @param img    FITS image to overlay.
     * @param iMin   First pixel index.
     * @param iMax   One-past-last pixel index.
     * @param wcsIdx 0-based index of the WCS solution.
     * @param OPTION Optional style string.
     */
    static void OverlayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, const size_t &, std::string OPTION=std::string());

    /// @copydoc OverlayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, std::string)
    static void OverlayInWCS(QCPAxisRect*, const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());

    /// @copydoc OverlayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, std::string)
    static void OverlayInWCS(QCPAxisRect*, const std::shared_ptr<const FITScube>&, const size_t& wcsIdx, std::string OPTION=std::string());

    /// @copydoc OverlayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string)
    static void OverlayInWCS(QCPAxisRect*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());

    /// @copydoc OverlayInWCS(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, const size_t&, std::string)
    static void OverlayInWCS(QCPAxisRect*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, const size_t&, std::string OPTION=std::string());

    // ─── RGB / multi-channel display (QCPColorMapRGB) ───────────────────────

    /**
     * @brief Display a 3-layer FITScube as a true-colour RGB image.
     *
     * @details Convenience wrapper that sets axis labels, enforces a 1:1
     *          pixel aspect ratio, and delegates to
     *          @c OverlayRGB(QCPAxisRect*, cube, OPTION).
     *
     *          Layer 0 → red channel, layer 1 → green channel,
     *          layer 2 → blue channel.  Each channel is independently
     *          stretch-corrected and then composited with additive blending
     *          (see @c QCPColorMapRGB).
     *
     * @param qPlot  Target @c QCustomPlot widget.  Its main axis rect is used.
     * @param cube   A FITS data cube with NAXIS = 3 and at least 3 layers
     *               (i.e. @c cube->GetDimension() >= 3 and @c cube->Size(3) >= 3).
     * @param OPTION Case-insensitive stretch token (may be combined):
     *               | Token   | Effect                                  |
     *               |:--------|:----------------------------------------|
     *               | @c logz  | log₁₀ stretch before gradient mapping  |
     *               | @c sqrtz | square-root stretch                    |
     *               | @c quadz | quadratic (power-2) stretch            |
     *               | @c sinhz | hyperbolic-sine stretch                |
     *               Default (empty string) applies a linear stretch.
     *
     * @throws DSL::FITSexception If @p cube has fewer than 3 dimensions or
     *                            fewer than 3 layers.
     * @see OverlayRGB(QCPAxisRect*, const std::shared_ptr<const FITScube>&, std::string)
     * @see QCPColorMapRGB
     */
    static void QtDisplayRGB(QCustomPlot*,
                              const std::shared_ptr<const FITScube>& cube,
                              std::string OPTION = std::string());

    /**
     * @brief Display three separate monochromatic FITScubes as a true-colour RGB image.
     *
     * @details Convenience wrapper that sets axis labels, enforces a 1:1
     *          pixel aspect ratio, and delegates to
     *          @c OverlayRGB(QCPAxisRect*, red, green, blue, OPTION).
     *
     *          This overload is the standard entry point for filter-association
     *          imaging (e.g. Hubble palette: Hα → @p red, OIII → @p green,
     *          SII → @p blue).  The three cubes may be loaded from entirely
     *          different FITS files; they only need to share the same pixel
     *          dimensions (NAXIS1, NAXIS2).
     *
     * @param qPlot  Target @c QCustomPlot widget.  Its main axis rect is used.
     * @param red    Monochromatic @c FITScube mapped to the red channel.
     * @param green  Monochromatic @c FITScube mapped to the green channel.
     * @param blue   Monochromatic @c FITScube mapped to the blue channel.
     * @param OPTION Case-insensitive stretch token — see @c QtDisplayRGB(QCustomPlot*, cube, OPTION)
     *               for the full token table.  The same stretch is applied to all channels.
     *
     * @note  All three cubes must have the same NAXIS1 and NAXIS2; the method
     *        derives dimensions from @p red and uses them for all channels.
     * @see OverlayRGB(QCPAxisRect*, red, green, blue, OPTION)
     * @see QCPColorMapRGB::setChannelGradient()
     */
    static void QtDisplayRGB(QCustomPlot*,
                              const std::shared_ptr<const FITScube>& red,
                              const std::shared_ptr<const FITScube>& green,
                              const std::shared_ptr<const FITScube>& blue,
                              std::string OPTION = std::string());

    /**
     * @brief Overlay three monochromatic FITScubes as an RGB image on an existing
     *        axis rect.
     *
     * @details Lower-level entry point used by
     *          @c QtDisplayRGB(QCustomPlot*, red, green, blue, OPTION).
     *          No axis labels, title, or aspect-ratio lock are applied; the caller
     *          is responsible for any additional axis setup.
     *
     *          The method:
     *          -# Parses @p OPTION to determine the per-pixel stretch function.
     *          -# Fills one @c std::vector<double> per channel by iterating
     *             all unmasked pixels of the respective @c FITScube and applying
     *             the chosen stretch.
     *          -# Constructs a @c QCPColorMapRGB on @p qPlot's axes, loads the
     *             three channel arrays, and auto-scales each channel's intensity
     *             range.
     *          -# Schedules a thread-safe @c QCustomPlot::replot() via
     *             @c QMetaObject::invokeMethod().
     *
     *          The returned @c QCPColorMapRGB is owned by the parent plot;
     *          its per-channel gradients can be customised after this call
     *          (call @c QCPColorMapRGB::invalidate() and @c replot() afterwards):
     * @code
     *   QtFITSviewer::OverlayRGB(rect, ha, oiii, sii);
     *   auto* rgb = dynamic_cast<QCPColorMapRGB*>(plot->plottable(0));
     *   QCPColorGradient warmOrange;
     *   warmOrange.setColorStops({{0.0, Qt::black}, {1.0, QColor(255,140,0)}});
     *   rgb->setChannelGradient(0, warmOrange);  // Hα in orange instead of red
     *   rgb->invalidate();
     *   plot->replot();
     * @endcode
     *
     * @param qPlot  Target @c QCPAxisRect (must already be part of a live @c QCustomPlot).
     * @param red    FITScube mapped to the red   channel (ch 0).
     * @param green  FITScube mapped to the green channel (ch 1).
     * @param blue   FITScube mapped to the blue  channel (ch 2).
     * @param OPTION Case-insensitive stretch token — see @c QtDisplayRGB() for the
     *               full token table.
     *
     * @note  Masked pixels (returned @c true by @c FITScube::GetMask()) are skipped
     *        and left at their default value of 0 in the output array.
     * @see QCPColorMapRGB
     * @see QtDisplayRGB(QCustomPlot*, red, green, blue, OPTION)
     */
    static void OverlayRGB(QCPAxisRect*,
                            const std::shared_ptr<const FITScube>& red,
                            const std::shared_ptr<const FITScube>& green,
                            const std::shared_ptr<const FITScube>& blue,
                            std::string OPTION = std::string());

    /**
     * @brief Overlay a 3-layer FITScube as an RGB image on an existing axis rect.
     *
     * @details Convenience overload of
     *          @c OverlayRGB(QCPAxisRect*, red, green, blue, OPTION) that accepts
     *          a single 3-D FITS data cube.  Layer 0 → red (ch 0), layer 1 → green
     *          (ch 1), layer 2 → blue (ch 2).  Pixel ranges are extracted as
     *          contiguous slices of the flat data array
     *          ([0, nx·ny), [nx·ny, 2·nx·ny), [2·nx·ny, 3·nx·ny)).
     *
     * @param qPlot  Target @c QCPAxisRect.
     * @param cube   A FITS data cube with NAXIS = 3 and at least 3 layers
     *               (@c cube->GetDimension() >= 3 and @c cube->Size(3) >= 3).
     * @param OPTION Case-insensitive stretch token — see @c QtDisplayRGB() for the
     *               full token table.
     *
     * @throws DSL::FITSexception (@c BAD_DIMEN) if @p cube has fewer than 3
     *         dimensions or fewer than 3 layers.
     * @see OverlayRGB(QCPAxisRect*, red, green, blue, OPTION)
     */
    static void OverlayRGB(QCPAxisRect*,
                            const std::shared_ptr<const FITScube>& cube,
                            std::string OPTION = std::string());

    /**
     * @brief Overlay an arbitrary set of FITScube channels as a colour image and
     *        return the resulting plottable.
     *
     * @details Generic, returning counterpart to @c OverlayRGB.  It builds a
     *          @c QCPColorMapRGB from a vector of monochromatic channel cubes and
     *          hands the pointer back to the caller, so per-channel intensity
     *          ranges, scale types, and gradients can be adjusted live afterwards
     *          (e.g. by an interactive viewer).
     *
     *          The number of channels selects the colour model:
     *          - 2 or 3 channels → additive RGB compositing (ch 0 = red,
     *            1 = green, 2 = blue);
     *          - 4 channels with @p cmyk = @c true → CMYK compositing
     *            (ch 0 = cyan, 1 = magenta, 2 = yellow, 3 = key/black).
     *
     *          All channels must share the same NAXIS1/NAXIS2.  Each channel is
     *          filled from its cube (applying the @p OPTION stretch) and its
     *          intensity range is auto-scaled from the data.  A thread-safe
     *          @c replot() is scheduled exactly as in @c OverlayRGB.
     *
     * @param qPlot    Target @c QCPAxisRect (must belong to a live @c QCustomPlot).
     * @param channels 2–4 monochromatic FITScubes, one per channel.
     * @param cmyk     If @c true (requires 4 channels), composite in CMYK; else RGB.
     * @param OPTION   Case-insensitive stretch token — see @c QtDisplayRGB() for the
     *                 full table.  The same stretch is applied to all channels.
     * @return The newly created @c QCPColorMapRGB (owned by the parent plot), or
     *         @c nullptr if @p channels is empty.
     *
     * @note  In CMYK mode the per-channel gradients are ignored (see
     *        @c QCPColorMapRGB::setCMYKMode()); custom gradients apply in RGB mode only.
     * @see QCPColorMapRGB
     * @see OverlayRGB(QCPAxisRect*, red, green, blue, OPTION)
     */
    static QCPColorMapRGB* OverlayChannels(QCPAxisRect*,
                            const std::vector<std::shared_ptr<const FITScube>>& channels,
                            bool cmyk = false,
                            std::string OPTION = std::string());

    /**
     * @brief Compute and plot a pixel histogram for a full FITS image.
     * @details Fills @p pdf with the normalised PDF and @p cdf with the CDF
     *          using the image's full dynamic range.
     * @param qPlot  Target plot widget.
     * @param img    FITS image to analyse.
     * @param n_bins Number of histogram bins.
     * @param pdf    Graph to receive the PDF data (must be pre-added to @p qPlot).
     * @param cdf    Graph to receive the CDF data (must be pre-added to @p qPlot).
     */
    static void QtHistogram(QCustomPlot* , const std::shared_ptr<const FITScube>&, const size_t&, QCPGraph*, QCPGraph*);

    /**
     * @brief Compute and plot a pixel histogram for a FITS image over a custom range.
     * @param qPlot  Target plot widget.
     * @param img    FITS image to analyse.
     * @param n_bins Number of histogram bins.
     * @param min    Lower clip value for the histogram range.
     * @param max    Upper clip value for the histogram range.
     * @param pdf    Graph to receive the PDF data.
     * @param cdf    Graph to receive the CDF data.
     */
    static void QtHistogram(QCustomPlot* , const std::shared_ptr<const FITScube>&, const size_t&, const double&, const double&, QCPGraph*, QCPGraph*);

    /**
     * @brief Compute the pixel intensity distribution of a FITS image.
     * @details Auto-selects linear or logarithmic bin spacing based on the
     *          dynamic range of the image (span > 1000 → log spacing).
     * @param img    FITS image to analyse.
     * @param n_bins Number of histogram bins.
     * @return A @c pixelDistribution containing bin edges, PDF, and CDF.
     */
    static QtFITS::pixelDistribution pixHistogram(const std::shared_ptr<const FITScube>&, const size_t& n_bins);

    /**
     * @brief Compute the pixel intensity distribution of a FITS image over a custom range.
     * @param img    FITS image to analyse.
     * @param n_bins Number of histogram bins.
     * @param min    Lower bound of the histogram range.
     * @param max    Upper bound of the histogram range.
     * @return A @c pixelDistribution containing bin edges, PDF, and CDF.
     */
    static QtFITS::pixelDistribution pixHistogram(const std::shared_ptr<const FITScube>&, const size_t& n_bins, const double& min, const double& max);

    /**
     * @brief Typed pixel distribution — uses the image's full dynamic range.
     * @tparam T Pixel storage type (e.g. @c float, @c uint16_t).
     * @param img    FITS image to analyse.
     * @param n_bins Number of histogram bins.
     * @return A @c pixelDistribution containing bin edges, PDF, and CDF.
     */
    template<typename T>
    static QtFITS::pixelDistribution pixHistogram(const std::shared_ptr<const FITScube>&, const size_t& n_bins);

    /**
     * @brief Typed pixel distribution over a custom range.
     * @tparam T Pixel storage type.
     * @param img    FITS image to analyse.
     * @param n_bins Number of histogram bins.
     * @param min    Lower clip value.
     * @param max    Upper clip value.
     * @return A @c pixelDistribution containing bin edges, PDF, and CDF.
     */
    template<typename T>
    static QtFITS::pixelDistribution pixHistogram(const std::shared_ptr<const FITScube>&, const size_t& n_bins, const double& min, const double& max);

    /**
     * @brief Plot the horizontal intensity profile at a given image row.
     * @details Extracts the pixel values along row @p iY and plots them
     *          against the column index (or transposes axes when @p horizontal
     *          is @c false). The y-axis label is set from the FITS @c UNITS keyword.
     * @param qPlot      Target plot widget.
     * @param img        FITS image.
     * @param iY         0-based row index.
     * @param x          Pre-existing @c QCPGraph to receive profile data.
     * @param horizontal If @c true (default), x-axis = column index, y-axis = value.
     *                   If @c false, axes are swapped.
     */
    static void QtProfileX(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, QCPGraph*, bool horizontal=true);

    /**
     * @brief Plot the vertical intensity profile at a given image column.
     * @details Extracts the pixel values along column @p iX and plots them
     *          against the row index.
     * @param qPlot      Target plot widget.
     * @param img        FITS image.
     * @param iX         0-based column index.
     * @param y          Pre-existing @c QCPGraph to receive profile data.
     * @param horizontal If @c true (default), x-axis = row index, y-axis = value.
     */
    static void QtProfileY(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, QCPGraph*, bool horizontal=true);

    /**
     * @brief Typed horizontal profile — pixel type must match the image storage type.
     * @tparam T Pixel storage type (e.g. @c float, @c uint16_t).
     * @param qPlot      Target plot widget.
     * @param img        FITS image.
     * @param iY         0-based row index.
     * @param x          Pre-existing @c QCPGraph to receive profile data.
     * @param horizontal Axis orientation (see non-template overload).
     */
    template<typename T>
    static void QtProfileX(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, QCPGraph*, bool horizontal=true);

    /**
     * @brief Typed vertical profile.
     * @tparam T Pixel storage type.
     * @param qPlot      Target plot widget.
     * @param img        FITS image.
     * @param iX         0-based column index.
     * @param y          Pre-existing @c QCPGraph to receive profile data.
     * @param horizontal Axis orientation (see non-template overload).
     */
    template<typename T>
    static void QtProfileY(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, QCPGraph*, bool horizontal=true);


    /**
     * @brief Draw iso-contour lines over a FITS image already displayed on a plot.
     * @details Uses @c QCustomContours (marching squares) to compute and render
     *          @p nLevels equally-spaced contour levels spanning the image's
     *          dynamic range.
     * @param qPlot   Target plot widget (must already contain the base image).
     * @param img     FITS image providing the scalar field for contouring.
     * @param nLevels Number of contour levels to draw.
     */
    static void QtContours(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& nLevels);

    /**
     * @brief Plot a FITStable column as a line curve (y only, x = row index).
     * @details Iterates all rows of @p tab, using the 1-based row index as the
     *          x coordinate and the value of column @p yCol as the y coordinate.
     *          Column type is dispatched at runtime; scalar, vector, and complex
     *          TFORM values are all supported.
     * @param qPlot Target plot widget.
     * @param tab   Source table.
     * @param yCol  Name of the y-value column.
     * @return Vector of new @c QCPGraph pointers (one per curve; vector columns
     *         produce one graph per row).
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(QCustomPlot* qPlot, const std::shared_ptr<const FITStable>& tab , const std::string &yCol);

    /**
     * @brief Plot two FITStable columns as a line curve (x, y).
     * @param qPlot Target plot widget.
     * @param tab   Source table.
     * @param xCol  Name of the x-value column.
     * @param yCol  Name of the y-value column.
     * @return Vector of new @c QCPGraph pointers.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(QCustomPlot* qPlot, const std::shared_ptr<const FITStable>& tab, const std::string &xCol  , const std::string &yCol);

    /**
     * @brief Plot two @c std::vector containers as a line curve.
     * @tparam T Numeric type of x values (must satisfy @c is_allowed_storage_type_v).
     * @tparam U Numeric type of y values.
     * @param qPlot  Target plot widget.
     * @param xData  x-coordinate values.
     * @param yData  y-coordinate values (must be the same size as @p xData).
     * @return Single-element vector containing the new @c QCPGraph.
     * @throws FITSexception (BAD_DECIM) if @c T or @c U is not a numeric type.
     * @throws FITSexception (BAD_DIMEN) if @p xData and @p yData differ in size.
     */
    template<typename T, typename U>
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(QCustomPlot* qPlot, std::vector<T> xData, std::vector<U> yData)
    {
        if(!is_allowed_storage_type_v<T> && !is_allowed_storage_type_v<U>)
            throw FITSexception(BAD_DECIM,"QtFITSviewer","QtCurve","Unsupported data type for x or y data. "+std::string(typeid(T).name())+","+std::string(typeid(U).name())+"["+std::to_string(__LINE__)+"]");

        if(xData.size() != yData.size())
            throw FITSexception(BAD_DIMEN,"QtFITSviewer","QtCurve","x and y data size mismatch. xData.size()="+std::to_string(xData.size())+" yData.size()="+std::to_string(yData.size())+"["+std::to_string(__LINE__)+"]");

        // QCustomPlot takes ownership of QCPGraph; use a no-op deleter to avoid double-free.
        auto curve = std::shared_ptr<QCPGraph>(new QCPGraph(qPlot->xAxis, qPlot->yAxis), [](QCPGraph*){});
        QVector<double> x, y;
        for(size_t i = 0; i < xData.size(); i++)
        {
            x.push_back(static_cast<double>(xData[i]));
            y.push_back(static_cast<double>(yData[i]));
        }
        curve->setData(x, y);
        return {curve};
    }

    /**
     * @brief Plot a @c std::vector as a line curve (y only, x = 0-based index).
     * @tparam U Numeric type of y values (must satisfy @c is_allowed_storage_type_v).
     * @param qPlot Target plot widget.
     * @param yData y-coordinate values.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    template<typename U>
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(QCustomPlot* qPlot, std::vector<U> yData)
    {
        static_assert(is_allowed_storage_type_v<U>,
            "QtCurve: unsupported data type for yData");
        // QCustomPlot takes ownership of QCPGraph; use a no-op deleter to avoid double-free.
        auto curve = std::shared_ptr<QCPGraph>(new QCPGraph(qPlot->xAxis, qPlot->yAxis), [](QCPGraph*){});
        QVector<double> x, y;
        for(size_t i = 0; i < yData.size(); ++i)
        {
            x.push_back(static_cast<double>(i));
            y.push_back(static_cast<double>(yData[i]));
        }
        curve->setData(x, y);
        return {curve};
    }

    /**
     * @brief Plot a FITStable column as a scatter diagram (y only, x = row index).
     * @details Identical to the @c QtCurve y-only overload but with the line
     *          style set to @c QCPGraph::lsNone so only markers are drawn.
     * @param qPlot Target plot widget.
     * @param tab   Source table.
     * @param yCol  Name of the y-value column.
     * @return Vector of new @c QCPGraph pointers.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(QCustomPlot* qPlot, const std::shared_ptr<const FITStable>& tab, const std::string &yCol);

    /**
     * @brief Plot two FITStable columns as a scatter diagram.
     * @param qPlot Target plot widget.
     * @param tab   Source table.
     * @param xCol  Name of the x-value column.
     * @param yCol  Name of the y-value column.
     * @return Vector of new @c QCPGraph pointers.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(QCustomPlot* qPlot, const std::shared_ptr<const FITStable>& tab, const std::string &xCol, const std::string &yCol);

    /**
     * @brief Plot two @c std::vector containers as a scatter diagram.
     * @details Delegates to the @c QtCurve overload then sets line style to
     *          @c QCPGraph::lsNone so only markers are drawn.
     * @tparam T Numeric type of x values.
     * @tparam U Numeric type of y values.
     * @param qPlot  Target plot widget.
     * @param xData  x-coordinate values.
     * @param yData  y-coordinate values (must be the same size as @p xData).
     * @return Single-element vector containing the new @c QCPGraph.
     */
    template<typename T, typename U>
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(QCustomPlot* qPlot, std::vector<T> xData, std::vector<U> yData)
    {
        auto scatter = QtCurve(qPlot, std::move(xData), std::move(yData));
        for(auto& g : scatter) g->setLineStyle(QCPGraph::lsNone);
        return scatter;
    }

    /**
     * @brief Plot a @c std::vector as a scatter diagram (y only, x = 0-based index).
     * @tparam U Numeric type of y values.
     * @param qPlot Target plot widget.
     * @param yData y-coordinate values.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    template<typename U>
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(QCustomPlot* qPlot, std::vector<U> yData)
    {
        auto scatter = QtCurve(qPlot, std::move(yData));
        for(auto& g : scatter) g->setLineStyle(QCPGraph::lsNone);
        return scatter;
    }

    // ===== QtDensityPlot =====

    /**
     * @brief Plot a 2-D density map from two scalar FITStable columns.
     * @details Bins all rows of @p tab into a @c nBinsX × @c nBinsY grid and
     *          renders the result as a @c QCPColorMap. An optional weight
     *          column @p zCol accumulates per-bin sums instead of counts.
     *          Only scalar numeric columns are accepted; complex or multi-element
     *          vector columns throw @c FITSexception.
     * @param qPlot  Target plot widget.
     * @param tab    Source table.
     * @param xCol   Scalar x-value column name.
     * @param yCol   Scalar y-value column name.
     * @param zCol   Optional weight column name; empty → each point counts as 1.
     * @param nBinsX Number of bins along the x axis (default 100).
     * @param nBinsY Number of bins along the y axis (default 100).
     * @param logFlag Bitmask: @c 0x1 = log₁₀(x), @c 0x2 = log₁₀(y),
     *               @c 0x4 = log₁₀ applied to bin counts.
     * @return Non-owning @c shared_ptr to the new @c QCPColorMap (owned by Qt).
     */
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& xCol, const std::string& yCol,
        const std::string& zCol = std::string(),
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0);

    /**
     * @brief Plot a 2-D density map from a single complex or 2-element vector column.
     * @details For a complex column (TFORM @c 'C' or @c 'M'), maps real → x and
     *          imag → y. For a 2-element vector column, maps elem[0] → x and
     *          elem[1] → y. All other column forms throw @c FITSexception.
     * @param qPlot  Target plot widget.
     * @param tab    Source table.
     * @param col    Complex or 2-element vector column name.
     * @param nBinsX Number of bins along the x axis (default 100).
     * @param nBinsY Number of bins along the y axis (default 100).
     * @param logFlag Bitmask: @c 0x1 = log₁₀(x), @c 0x2 = log₁₀(y),
     *               @c 0x4 = log₁₀ applied to bin counts.
     * @return Non-owning @c shared_ptr to the new @c QCPColorMap (owned by Qt).
     */
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& col,
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0);

    /**
     * @brief Plot a 2-D density map from a row-filtered FITStable.
     * @details Same as the two-column overload but only the rows identified by
     *          @p rows are included. A @c RowSetBuilder<T> converts implicitly
     *          to @c RowSet via @c operator RowSet().
     * @param qPlot  Target plot widget.
     * @param tab    Source table.
     * @param xCol   Scalar x-value column name.
     * @param yCol   Scalar y-value column name.
     * @param rows   Row selector.
     * @param zCol   Optional weight column name; empty → each point counts as 1.
     * @param nBinsX Number of bins along the x axis (default 100).
     * @param nBinsY Number of bins along the y axis (default 100).
     * @param logFlag Bitmask: @c 0x1 = log₁₀(x), @c 0x2 = log₁₀(y),
     *               @c 0x4 = log₁₀ applied to bin counts.
     * @return Non-owning @c shared_ptr to the new @c QCPColorMap (owned by Qt).
     */
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& xCol, const std::string& yCol,
        const RowSet& rows,
        const std::string& zCol = std::string(),
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0);

    /**
     * @brief Plot a 2-D density map from a pair of @c ColumnHandle objects.
     * @details Column types are dispatched at runtime; all numeric types are
     *          supported. Use this overload when the column type is not known
     *          at compile time.
     * @param qPlot   Target plot widget.
     * @param xHandle Handle to the x-value column.
     * @param yHandle Handle to the y-value column.
     * @param nBinsX  Number of bins along the x axis (default 100).
     * @param nBinsY  Number of bins along the y axis (default 100).
     * @param logFlag Bitmask: @c 0x1 = log₁₀(x), @c 0x2 = log₁₀(y),
     *               @c 0x4 = log₁₀ applied to bin counts.
     * @return Non-owning @c shared_ptr to the new @c QCPColorMap (owned by Qt).
     */
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        ColumnHandle xHandle, ColumnHandle yHandle,
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0);

    /**
     * @brief Plot a 2-D density map from two @c std::vector containers (no per-point weight).
     * @tparam T Numeric type of x values (must satisfy @c is_allowed_storage_type_v).
     * @tparam U Numeric type of y values.
     * @param qPlot  Target plot widget.
     * @param xData  x-coordinate values.
     * @param yData  y-coordinate values (must be the same size as @p xData).
     * @param nBinsX Number of bins along the x axis (default 100).
     * @param nBinsY Number of bins along the y axis (default 100).
     * @param logFlag Bitmask: @c 0x1 = log₁₀(x), @c 0x2 = log₁₀(y),
     *               @c 0x4 = log₁₀ applied to bin counts.
     * @return Non-owning @c shared_ptr to the new @c QCPColorMap (owned by Qt).
     */
    template<typename T, typename U>
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        std::vector<T> xData, std::vector<U> yData,
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0)
    {
        static_assert(is_allowed_storage_type_v<T>, "QtDensityPlot: unsupported type for xData");
        static_assert(is_allowed_storage_type_v<U>, "QtDensityPlot: unsupported type for yData");
        if(xData.size() != yData.size())
            throw FITSexception(BAD_DIMEN,"QtFITSviewer","QtDensityPlot",
                "x and y size mismatch. xData.size()="+std::to_string(xData.size())+
                " yData.size()="+std::to_string(yData.size()));
        std::vector<double> xs, ys, zs;
        xs.reserve(xData.size());
        ys.reserve(yData.size());
        for(size_t i = 0; i < xData.size(); ++i)
        {
            xs.push_back(static_cast<double>(xData[i]));
            ys.push_back(static_cast<double>(yData[i]));
        }
        return _QtDensityCore(qPlot, std::move(xs), std::move(ys), zs, nBinsX, nBinsY, logFlag);
    }

    /**
     * @brief Plot a 2-D density map from three @c std::vector containers (with per-point weight).
     * @tparam T Numeric type of x values.
     * @tparam U Numeric type of y values.
     * @tparam W Numeric type of weight values.
     * @param qPlot  Target plot widget.
     * @param xData  x-coordinate values.
     * @param yData  y-coordinate values (must match @p xData size).
     * @param zData  Per-point weight values (must match @p xData size).
     * @param nBinsX Number of bins along the x axis (default 100).
     * @param nBinsY Number of bins along the y axis (default 100).
     * @param logFlag Bitmask: @c 0x1 = log₁₀(x), @c 0x2 = log₁₀(y),
     *               @c 0x4 = log₁₀ applied to bin counts.
     * @return Non-owning @c shared_ptr to the new @c QCPColorMap (owned by Qt).
     */
    template<typename T, typename U, typename W>
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        std::vector<T> xData, std::vector<U> yData, std::vector<W> zData,
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0)
    {
        static_assert(is_allowed_storage_type_v<T>, "QtDensityPlot: unsupported type for xData");
        static_assert(is_allowed_storage_type_v<U>, "QtDensityPlot: unsupported type for yData");
        static_assert(is_allowed_storage_type_v<W>, "QtDensityPlot: unsupported type for zData");
        if(xData.size() != yData.size() || xData.size() != zData.size())
            throw FITSexception(BAD_DIMEN,"QtFITSviewer","QtDensityPlot",
                "x, y, and z sizes must all match.");
        std::vector<double> xs, ys, zs;
        xs.reserve(xData.size());
        ys.reserve(yData.size());
        zs.reserve(zData.size());
        for(size_t i = 0; i < xData.size(); ++i)
        {
            xs.push_back(static_cast<double>(xData[i]));
            ys.push_back(static_cast<double>(yData[i]));
            zs.push_back(static_cast<double>(zData[i]));
        }
        return _QtDensityCore(qPlot, std::move(xs), std::move(ys), std::move(zs), nBinsX, nBinsY, logFlag);
    }

    /**
     * @brief Plot a 2-D density map from a pair of typed @c ColumnView objects.
     * @tparam T Element type of the x view.
     * @tparam U Element type of the y view.
     * @param qPlot  Target plot widget.
     * @param xView  Typed view over the x-value column.
     * @param yView  Typed view over the y-value column.
     * @param nBinsX Number of bins along the x axis (default 100).
     * @param nBinsY Number of bins along the y axis (default 100).
     * @param logFlag Bitmask: @c 0x1 = log₁₀(x), @c 0x2 = log₁₀(y),
     *               @c 0x4 = log₁₀ applied to bin counts.
     * @return Non-owning @c shared_ptr to the new @c QCPColorMap (owned by Qt).
     */
    template<typename T, typename U>
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        ColumnView<T> xView, ColumnView<U> yView,
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0)
    {
        const auto& xd = xView.data();
        const auto& yd = yView.data();
        return QtDensityPlot(qPlot,
            std::vector<T>(xd.begin(), xd.end()),
            std::vector<U>(yd.begin(), yd.end()),
            nBinsX, nBinsY, logFlag);
    }

    // ===== QtHistogram (FITStable-aware overloads) =====

    /**
     * @brief Histogram a FITStable column (scalar, vector, or complex TFORM).
     * @details For vector or complex columns all elements are flattened before
     *          binning. @p nBins = 0 auto-selects the bin count via the Sturges
     *          rule. String, logical, and bit columns throw @c FITSexception.
     * @param qPlot  Target plot widget.
     * @param tab    Source table.
     * @param col    Column name to histogram.
     * @param nBins  Number of bins (0 = auto via Sturges rule).
     * @param OPTION Style option string forwarded to @c applyPlotOptions.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtHistogram(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& col,
        size_t nBins = 0,
        std::string OPTION = std::string());

    /**
     * @brief Histogram a scalar FITStable column, restricted to a row subset.
     * @details Only the rows identified by @p rows are included. A
     *          @c RowSetBuilder<T> converts implicitly to @c RowSet.
     * @param qPlot  Target plot widget.
     * @param tab    Source table.
     * @param col    Scalar column name to histogram.
     * @param rows   Row selector.
     * @param nBins  Number of bins (0 = auto via Sturges rule).
     * @param OPTION Style option string forwarded to @c applyPlotOptions.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtHistogram(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& col,
        const RowSet& rows,
        size_t nBins = 0,
        std::string OPTION = std::string());

    /**
     * @brief Histogram a column accessed through a @c ColumnHandle.
     * @details Column type is dispatched at runtime. Use this overload when the
     *          column type is not known at compile time.
     * @param qPlot  Target plot widget.
     * @param handle Handle to the column to histogram.
     * @param nBins  Number of bins (0 = auto via Sturges rule).
     * @param OPTION Style option string forwarded to @c applyPlotOptions.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtHistogram(
        QCustomPlot* qPlot,
        ColumnHandle handle,
        size_t nBins = 0,
        std::string OPTION = std::string());

    /**
     * @brief Histogram a @c std::vector of values.
     * @tparam T Numeric element type (must satisfy @c is_allowed_storage_type_v).
     * @param qPlot  Target plot widget.
     * @param data   Values to histogram.
     * @param nBins  Number of bins (0 = auto via Sturges rule).
     * @param OPTION Style option string forwarded to @c applyPlotOptions.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    template<typename T>
    static std::vector<std::shared_ptr<QCPGraph>> QtHistogram(
        QCustomPlot* qPlot,
        std::vector<T> data,
        size_t nBins = 0,
        std::string OPTION = std::string())
    {
        static_assert(is_allowed_storage_type_v<T>, "QtHistogram: unsupported data type");
        std::vector<double> d;
        d.reserve(data.size());
        for(const T& v : data)
            d.push_back(static_cast<double>(v));
        return {_QtHistogramCore(qPlot, std::move(d), nBins, 0, OPTION)};
    }

    /**
     * @brief Histogram a typed @c ColumnView.
     * @tparam T Element type of the view.
     * @param qPlot  Target plot widget.
     * @param view   Typed view over the column data.
     * @param nBins  Number of bins (0 = auto via Sturges rule).
     * @param OPTION Style option string forwarded to @c applyPlotOptions.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    template<typename T>
    static std::vector<std::shared_ptr<QCPGraph>> QtHistogram(
        QCustomPlot* qPlot,
        ColumnView<T> view,
        size_t nBins = 0,
        std::string OPTION = std::string())
    {
        const auto& d = view.data();
        return QtHistogram(qPlot, std::vector<T>(d.begin(), d.end()), nBins, OPTION);
    }

    // ===== Helper-type overloads for QtCurve =====

    /**
     * @brief Plot a typed @c ColumnView as a line curve (y only, x = 0-based index).
     * @tparam T Element type of the view.
     * @param qPlot Target plot widget.
     * @param yView Typed view over the y-value column.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    template<typename T>
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot, ColumnView<T> yView)
    {
        const auto& d = yView.data();
        return QtCurve(qPlot, std::vector<T>(d.begin(), d.end()));
    }

    /**
     * @brief Plot a pair of typed @c ColumnView objects as a line curve.
     * @tparam T Element type of the x view.
     * @tparam U Element type of the y view.
     * @param qPlot Target plot widget.
     * @param xView Typed view over the x-value column.
     * @param yView Typed view over the y-value column.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    template<typename T, typename U>
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot, ColumnView<T> xView, ColumnView<U> yView)
    {
        const auto& xd = xView.data();
        const auto& yd = yView.data();
        return QtCurve(qPlot,
            std::vector<T>(xd.begin(), xd.end()),
            std::vector<U>(yd.begin(), yd.end()));
    }

    /**
     * @brief Plot a row-filtered FITStable column as a line curve (y only, x = row index).
     * @param qPlot Target plot widget.
     * @param tab   Source table.
     * @param yCol  y-value column name.
     * @param rows  Row selector.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& yCol, const RowSet& rows);

    /**
     * @brief Plot two row-filtered FITStable columns as a line curve.
     * @param qPlot Target plot widget.
     * @param tab   Source table.
     * @param xCol  x-value column name.
     * @param yCol  y-value column name.
     * @param rows  Row selector.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& xCol, const std::string& yCol, const RowSet& rows);

    /**
     * @brief Plot a @c ColumnHandle as a line curve (y only, x = row index).
     * @param qPlot   Target plot widget.
     * @param yHandle Handle to the y-value column.
     * @return Vector of new @c QCPGraph pointers.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot, ColumnHandle yHandle);

    /**
     * @brief Plot a pair of @c ColumnHandle objects as a line curve.
     * @param qPlot   Target plot widget.
     * @param xHandle Handle to the x-value column.
     * @param yHandle Handle to the y-value column.
     * @return Vector of new @c QCPGraph pointers.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot, ColumnHandle xHandle, ColumnHandle yHandle);

    // ===== Helper-type overloads for QtScatter =====

    /**
     * @brief Scatter-plot a typed @c ColumnView (y only, x = 0-based index).
     * @tparam T Element type of the view.
     * @param qPlot Target plot widget.
     * @param yView Typed view over the y-value column.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    template<typename T>
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot, ColumnView<T> yView)
    {
        auto g = QtCurve(qPlot, yView);
        for(auto& p : g) p->setLineStyle(QCPGraph::lsNone);
        return g;
    }

    /**
     * @brief Scatter-plot a pair of typed @c ColumnView objects.
     * @tparam T Element type of the x view.
     * @tparam U Element type of the y view.
     * @param qPlot Target plot widget.
     * @param xView Typed view over the x-value column.
     * @param yView Typed view over the y-value column.
     * @return Single-element vector containing the new @c QCPGraph.
     */
    template<typename T, typename U>
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot, ColumnView<T> xView, ColumnView<U> yView)
    {
        auto g = QtCurve(qPlot, xView, yView);
        for(auto& p : g) p->setLineStyle(QCPGraph::lsNone);
        return g;
    }

    /**
     * @brief Scatter-plot a row-filtered FITStable column (y only, x = row index).
     * @param qPlot Target plot widget.
     * @param tab   Source table.
     * @param yCol  y-value column name.
     * @param rows  Row selector.
     * @return Vector of new @c QCPGraph pointers.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& yCol, const RowSet& rows);

    /**
     * @brief Scatter-plot two row-filtered FITStable columns.
     * @param qPlot Target plot widget.
     * @param tab   Source table.
     * @param xCol  x-value column name.
     * @param yCol  y-value column name.
     * @param rows  Row selector.
     * @return Vector of new @c QCPGraph pointers.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& xCol, const std::string& yCol, const RowSet& rows);

    /**
     * @brief Scatter-plot a @c ColumnHandle (y only, x = row index).
     * @param qPlot   Target plot widget.
     * @param yHandle Handle to the y-value column.
     * @return Vector of new @c QCPGraph pointers.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot, ColumnHandle yHandle);

    /**
     * @brief Scatter-plot a pair of @c ColumnHandle objects.
     * @param qPlot   Target plot widget.
     * @param xHandle Handle to the x-value column.
     * @param yHandle Handle to the y-value column.
     * @return Vector of new @c QCPGraph pointers.
     */
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot, ColumnHandle xHandle, ColumnHandle yHandle);

    /**
     * @brief Register a callback that is called whenever the viewer is resized.
     * @details The functor @p f receives a pointer to the embedded
     *          @c QCustomPlot widget and can reflow axis labels, legends, or
     *          other layout elements in response to size changes.
     * @param f Callable with signature @c void(QCustomPlot*).
     */
    void linkResizeEvent(std::function<void(QCustomPlot*)> f)
    {
        resizeFunctor = f;
    }

#pragma endregion
private:
    Ui::MainWindow *ui;    ///< Main window GUI
    
    QtFITS::qPlotList plot_list; ///< Registry mapping plot names to numeric IDs.

    /// @brief Initialise the embedded @c QCustomPlot axes and legend.
    void qInitPlot();

    /// @brief Qt event filter — intercepts wheel/key events on the plot widget.
    bool eventFilter(QObject*, QEvent *) override;

    void resizeEvent(QResizeEvent *event) override
    {
        if(resizeFunctor)
            resizeFunctor(this->ui->customPlot);
    }

#pragma region Event functor helper
    std::function<void(QCustomPlot*)> resizeFunctor; ///< Optional resize callback; see linkResizeEvent().

#pragma endregion    

};

/**
 * @brief Typed pixel distribution — full range specialisation.
 * @tparam T Pixel storage type matching the image's internal storage.
 * @param in     FITS image to analyse.
 * @param n_bins Number of histogram bins.
 * @return Populated @c pixelDistribution (bin edges, PDF, CDF).
 */
template<typename T>
QtFITS::pixelDistribution QtFITSviewer::pixHistogram(const std::shared_ptr<const FITScube>& in, const size_t& n_bins)
{
    double max = in->GetMaximum();
    double min = in->GetMinimum();
    return pixHistogram(in,n_bins,min,max);
}

/**
 * @brief Typed pixel distribution — custom range specialisation.
 * @tparam T Pixel storage type.
 * @param in     FITS image to analyse.
 * @param n_bins Number of histogram bins.
 * @param min    Lower clip value (pixels below are mapped to the first bin edge).
 * @param max    Upper clip value.
 * @return Populated @c pixelDistribution (bin edges, PDF, CDF).
 */
template<typename T>
QtFITS::pixelDistribution QtFITSviewer::pixHistogram(const std::shared_ptr<const FITScube>& in, const size_t& n_bins, const double& min, const double& max)
{
    double SPAN = min/max;
    QtFITS::pixelDistribution pd;

    QVector<double>& val = pd.bin_edges;
    QVector<double>& entry = pd.pdf;
    QVector<double>& sum = pd.cdf;
    
    if(SPAN > 1000)
    {
        for(size_t i = 0; i <= n_bins; i++)
            val.push_back( pow(10.,static_cast<double>(i)/(static_cast<double>(n_bins)-1)*log10(max/min)+log10(min)));
    }
    else
    {
        for(size_t i = 0; i <= n_bins; i++)
            val.push_back( static_cast<double>(i)/(static_cast<double>(n_bins)-1)*(max-min)+min);
    }



    val.push_back(val[val.size()-1]+(val[val.size()-1]-val[val.size()-2]));
    val.insert(0,val[0]-(val[2]-val[1]));

    const std::valarray<T>* _arr_ = in->GetData<T>();
    typename std::vector<T> array;
    array.assign(std::begin(*_arr_),std::end(*_arr_));
    
    std::sort(array.begin(),array.end());
    
    double total = static_cast<double>(array.size());
    double iMax  = 0;
    double this_sum   = 0;
    
    typename std::vector<T>::const_iterator i = array.cbegin();
    
    for(size_t iBins = 1; iBins < val.size(); iBins++)
    {
        size_t count = 0;
        while (i != array.cend() && (*i) < val[iBins])
        {
            double this_val = static_cast<double>(*i);
            if(this_val < min)
                this_val = val[0]; 
            else if(this_val > max)
                this_val = val[val.size()-1];
            if(this_val >= val[iBins-1])
                count++;
            i++;
        }
        
        entry.push_back(static_cast<double>(count)/total);
        this_sum += static_cast<double>(count);
        sum.push_back(this_sum/total);
       
        if(count > iMax)
            iMax = count;
    }

    while (entry.size() < val.size())
        entry.push_back(0);

    while (sum.size() < val.size())
        sum.push_back((*sum.rbegin())+0.);

    if((DSL::verbose&DSL::verboseLevel::VERBOSE_DEBUG) == DSL::verboseLevel::VERBOSE_DEBUG)
    {
        std::lock_guard<std::mutex> lk(QtFITS::msg_m);
        std::cout << "[DEBUG] QtFITSviewer::pixHistogram<"<< typeid(T).name() <<"> n_bins="<< n_bins <<" min="<< min <<" max="<< max << std::endl;
        pd.print();
    }

    return pd;
}

/**
 * @brief Typed horizontal profile implementation.
 * @tparam T Pixel storage type matching the image's internal storage.
 * @param customPlot Target plot widget.
 * @param in         FITS image.
 * @param iY         0-based row index (throws @c std::out_of_range if invalid).
 * @param x          Pre-existing @c QCPGraph to receive the profile data.
 * @param horizontal Axis orientation: @c true → column index on x, value on y.
 */
template<typename T>
void QtFITSviewer::QtFITSviewer::QtProfileX(QCustomPlot* customPlot,const std::shared_ptr<const FITScube>& in, const size_t& iY, QCPGraph* x, bool horizontal)
{
    if(iY >= in->Size(2))
        throw std::out_of_range("QtProfileX@["+std::to_string(iY)+"] iY is out of range");

    std::string UNITS = (in->HDU().Exists("UNITS"))?in->HDU().GetValueForKey("UNITS"):"ARB. UNIT";
    
    std::pair<QVector<double>, QVector<double> > profile;
    profile.first.clear();
    profile.second.clear();

    for(size_t k=0; k<in->Size(1); k++)
        profile.first.push_back(static_cast<double>(k));
    
    std::slice s = std::slice(iY*in->Size(1),in->Size(1),1);
    const typename std::valarray<T>* _val_ = in->GetData<T>();
    
    std::valarray<T> val = (*_val_)[s];
    
    profile.second.resize(val.size());
    std::transform(std::begin(val), std::end(val), profile.second.begin(),
                [](const T &v){ return static_cast<double>(v); });

    if(!horizontal)
    {
        std::swap(profile.second,profile.first);
        x->keyAxis()->setLabel(UNITS.c_str());
    }
    else
        x->valueAxis()->setLabel(UNITS.c_str());

    QMetaObject::invokeMethod(customPlot, [x, customPlot,
                                           profile ]() mutable
                                            {
                                                {
                                                    std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
                                                    x->setData(profile.first,profile.second,false);
                                                    customPlot->replot();
                                                } // io_mutex released here
                                            
                                                {
                                                    std::lock_guard<std::mutex> dk(QtFITS::msg_m);
                                                    if (!QtFITS::finished)
                                                         QtFITS::finished |= true;
                                                    QtFITS::msg_cv.notify_one(); // notify while holding msg_m (or move notify outside if preferred)
                                                    QtFITS::finished &= false;
                                                } // msg_m released here
                                            }
                                        );

    return;
}

/**
 * @brief Typed vertical profile implementation.
 * @tparam T Pixel storage type matching the image's internal storage.
 * @param customPlot Target plot widget.
 * @param in         FITS image.
 * @param iX         0-based column index (throws @c std::out_of_range if invalid).
 * @param y          Pre-existing @c QCPGraph to receive the profile data.
 * @param horizontal Axis orientation: @c true → row index on x, value on y.
 */
template<typename T>
void  QtFITSviewer::QtProfileY(QCustomPlot* customPlot,const std::shared_ptr<const FITScube>& in, const size_t& iX, QCPGraph* y, bool horizontal)
{
    if(iX >= in->Size(1))
        throw std::out_of_range("QtProfileY@["+std::to_string(iX)+"] iX is out of range");

    std::string UNITS = (in->HDU().Exists("UNITS"))?in->HDU().GetValueForKey("UNITS"):"ARB. UNIT";
    
    std::pair<QVector<double>, QVector<double> > profile;
    profile.first.clear();
    profile.second.clear();

    for(size_t k=0; k<in->Size(2); k++)
         profile.first.push_back(static_cast<double>(k));
    
    std::slice s = std::slice(iX,in->Size(2),in->Size(1));
    const typename std::valarray<T>* _val_ = in->GetData<T>();
    
    std::valarray<T> val = (*_val_)[s];
    
    profile.second.resize(val.size());
    std::transform(std::begin(val), std::end(val), profile.second.begin(),
                [](const T &v){ return static_cast<double>(v); });
    
    if(!horizontal)
    {
        std::swap(profile.second,profile.first);
        y->keyAxis()->setLabel(UNITS.c_str());
    }
    else
        y->valueAxis()->setLabel(UNITS.c_str());

    QMetaObject::invokeMethod(customPlot, [y, customPlot,
                                           profile ]() mutable
                                            {
                                                {
                                                    std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
                                                    y->setData(profile.first,profile.second,false);
                                                    customPlot->replot();
                                                } // io_mutex released here
                                            
                                                {
                                                    std::lock_guard<std::mutex> dk(QtFITS::msg_m);
                                                    if (!QtFITS::finished)
                                                         QtFITS::finished |= true;
                                                    QtFITS::msg_cv.notify_one(); // notify while holding msg_m (or move notify outside if preferred)
                                                    QtFITS::finished &= false;
                                                } // msg_m released here
                                            }
                                    );
    return;
}

#endif /* defined(MAINWINDOW_H) */
