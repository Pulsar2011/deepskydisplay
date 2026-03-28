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

    enum zScale
    {
        /**
         *  @enum QtFITS::zScale
         *  @details Defines the type of the z axis during Qt display. It do not affect the ROOT display.
         */
        fLIN,   ///< Linear scale
        fLOG,   ///< Logaritmic scale
        fSINH,  ///< Sinus hyperbolic scale
        fQUAD,  ///< Quadratic scale
        fSQRT,  ///< Squared root scale
    };
    
    class rgb
    {
    public:
        /**
         *  @class qfits::rgb QfitsViewer.h "QfitsViewer.h"
         *  @details Color managment class used by plotting view.
         */
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
 *@class QfitsViewer QfitsViewer.h "QfitsViewer.h"
 *@brief Warper to Qt::QCustomPlot display
 *
 *@details Base class to display FITS contents onto standard output. It require Qt v > 4 with QCustomPlot (http://www.qcustomplot.com/) widget source. QfitsViewer as well as its child class provide tha capability to export data to TROOT fromework. Thus, it is  required to install TROOT v6.00  minimum
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
    static void QtDisplay(QCustomPlot*      , const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());

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
     * @brief Display a single image layer (2-D slice) of a data cube.
     * @param qPlot  Target plot widget.
     * @param img    FITS data cube.
     * @param iLayer 0-based layer index along the third axis.
     * @param OPTION Optional style string.
     */
    static void QtDisplayLayer(QCustomPlot* , const std::shared_ptr<const FITScube>&, const size_t&, std::string OPTION=std::string());

    /**
     * @brief Overlay all pixels of a second FITS image onto an existing plot.
     * @details Adds a second @c QCPColorMap layer on top of the current axes.
     *          Useful for overlaying, e.g., a detection mask on a science image.
     * @param qPlot  Target plot widget (must already contain a base image).
     * @param img    FITS image to overlay.
     * @param OPTION Optional style string.
     */
    static void Overlay(QCustomPlot*, const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());

    /**
     * @brief Overlay a pixel sub-range of a FITS image onto an existing plot.
     * @param qPlot  Target plot widget.
     * @param img    FITS image to overlay.
     * @param iMin   First pixel index (0-based, inclusive).
     * @param iMax   One-past-last pixel index.
     * @param OPTION Optional style string.
     */
    static void Overlay(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());

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

    /// Single-vector QtCurve: x is implicit 0-based index, y comes from yData.
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

    /// Two-vector QtScatter: delegates to QtCurve then removes line.
    template<typename T, typename U>
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(QCustomPlot* qPlot, std::vector<T> xData, std::vector<U> yData)
    {
        auto scatter = QtCurve(qPlot, std::move(xData), std::move(yData));
        for(auto& g : scatter) g->setLineStyle(QCPGraph::lsNone);
        return scatter;
    }

    /// Single-vector QtScatter: x is implicit 0-based index.
    template<typename U>
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(QCustomPlot* qPlot, std::vector<U> yData)
    {
        auto scatter = QtCurve(qPlot, std::move(yData));
        for(auto& g : scatter) g->setLineStyle(QCPGraph::lsNone);
        return scatter;
    }

    // ===== QtDensityPlot =====

    /// FITStable — two columns (x, y) with optional z weight.
    /// logFlag bits: 0x1=xLog, 0x2=yLog, 0x4=zLog (apply log₁₀ to bin counts).
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& xCol, const std::string& yCol,
        const std::string& zCol = std::string(),
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0);

    /// FITStable — single column whose TFORM is complex (C/M) or a 2-element vector.
    /// Complex: real→x, imag→y. 2-vector: elem[0]→x, elem[1]→y. All other forms throw.
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& col,
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0);

    /// FITStable + RowSet (RowSetBuilder<T> converts implicitly via operator RowSet()).
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& xCol, const std::string& yCol,
        const RowSet& rows,
        const std::string& zCol = std::string(),
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0);

    /// ColumnHandle pair — runtime type dispatch.
    static std::shared_ptr<QCPColorMap> QtDensityPlot(
        QCustomPlot* qPlot,
        ColumnHandle xHandle, ColumnHandle yHandle,
        size_t nBinsX = 100, size_t nBinsY = 100,
        uint8_t logFlag = 0);

    /// vector<T,U> overload — no z weight.
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

    /// vector<T,U,W> overload — with z weight per point.
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

    /// ColumnView<T,U> overload — extracts filtered data and forwards.
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

    /// FITStable — scalar, vector, or complex column.
    /// nBins=0: auto (Sturges rule). logFlag: 0x1=xLog (bin in log₁₀), 0x2=yLog (log₁₀ y-axis).
    static std::vector<std::shared_ptr<QCPGraph>> QtHistogram(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& col,
        size_t nBins = 0,
        std::string OPTION = std::string());

    /// FITStable + RowSet.
    static std::vector<std::shared_ptr<QCPGraph>> QtHistogram(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& col,
        const RowSet& rows,
        size_t nBins = 0,
        std::string OPTION = std::string());

    /// ColumnHandle — runtime type dispatch.
    static std::vector<std::shared_ptr<QCPGraph>> QtHistogram(
        QCustomPlot* qPlot,
        ColumnHandle handle,
        size_t nBins = 0,
        std::string OPTION = std::string());

    /// vector<T> overload.
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

    /// ColumnView<T> overload.
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

    /// QtCurve: typed ColumnView (y only, x = index).
    template<typename T>
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot, ColumnView<T> yView)
    {
        const auto& d = yView.data();
        return QtCurve(qPlot, std::vector<T>(d.begin(), d.end()));
    }

    /// QtCurve: typed ColumnView pair (x, y).
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

    /// QtCurve: FITStable + RowSet (y only).
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& yCol, const RowSet& rows);

    /// QtCurve: FITStable + RowSet (x, y).
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& xCol, const std::string& yCol, const RowSet& rows);

    /// QtCurve: ColumnHandle (y only).
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot, ColumnHandle yHandle);

    /// QtCurve: ColumnHandle pair (x, y).
    static std::vector<std::shared_ptr<QCPGraph>> QtCurve(
        QCustomPlot* qPlot, ColumnHandle xHandle, ColumnHandle yHandle);

    // ===== Helper-type overloads for QtScatter =====

    /// QtScatter: typed ColumnView (y only).
    template<typename T>
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot, ColumnView<T> yView)
    {
        auto g = QtCurve(qPlot, yView);
        for(auto& p : g) p->setLineStyle(QCPGraph::lsNone);
        return g;
    }

    /// QtScatter: typed ColumnView pair (x, y).
    template<typename T, typename U>
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot, ColumnView<T> xView, ColumnView<U> yView)
    {
        auto g = QtCurve(qPlot, xView, yView);
        for(auto& p : g) p->setLineStyle(QCPGraph::lsNone);
        return g;
    }

    /// QtScatter: FITStable + RowSet (y only).
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& yCol, const RowSet& rows);

    /// QtScatter: FITStable + RowSet (x, y).
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot,
        const std::shared_ptr<const FITStable>& tab,
        const std::string& xCol, const std::string& yCol, const RowSet& rows);

    /// QtScatter: ColumnHandle (y only).
    static std::vector<std::shared_ptr<QCPGraph>> QtScatter(
        QCustomPlot* qPlot, ColumnHandle yHandle);

    /// QtScatter: ColumnHandle pair (x, y).
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
