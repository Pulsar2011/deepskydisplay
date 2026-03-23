//
//  QtFitsPlotting.cxx
//  DeepSkyTools
//
//  File created by GILLARD William on 28/09/25.
//  Handle the plotting of FITS data
//
//	Copyright (c) 2025, All rights reserved

#include <ranges>
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <deque>
#include <future>
#include <limits>
#include <unordered_map>
#include <vector>

#include <DSTdisplay/QtDisplay.h>
#include "QtFITSviewer.h"
#include <DSTdisplay/DSD_version.h>
#include <DSTfits/FITSexception.h>
#include <DSTfits/FITSimg.h>

using namespace DSL;

/**
 *  Display FITS 2D images onto the scrren.
 *  @param img: Pointer to the FITS image to be displayed
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 *  @return Pointer to the displayed plot structure for further customization and overplotting
 */
void QtFITSviewer::QtDisplay(QCustomPlot* qPlot,const  std::shared_ptr<const FITScube>& img  , std::string OPTION)
{
    return QtDisplay(qPlot, img, 0 , img->Nelements(), OPTION);
}

/**
 *  Display sub FITS 2D images onto the scrren.
 *  @param img: Pointer to the FITS image to be displayed
 *  @param iMin: index of the pixel used as the lower left corner
 *  @param iMax: index of the pixel used as the upper left corner
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 *  @return Pointer to the displayed plot structur for further customization and overplotting
 */
void QtFITSviewer::QtDisplay(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, std::string OPTION)
{
    qPlot->yAxis->setScaleRatio(qPlot->xAxis,1.0);
    
    //- Fill the graph
    qPlot->xAxis->setLabel("pixel [px]");
    qPlot->yAxis->setLabel("pixel [px]");
    
    Overlay(qPlot, img, iMin, iMax, OPTION);
    return;
}

/**
 *  Display FITS 2D images onto the screen using default WCS as pixel coordinates.
 *  @param img: Pointer to the FITS image to be displayed
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 *  @return Pointer to the displayed plot structure for further customization and overplotting
 */
void QtFITSviewer::QtDisplayInWCS(QCustomPlot* qPlot,const  std::shared_ptr<const FITScube>& img  , std::string OPTION)
{
    return QtDisplayInWCS(qPlot, img, 0 , img->Nelements(), 0, OPTION);
}

/**
 *  Display FITS 2D images onto the screen using WCS as pixel coordinates.
 *  @param img: Pointer to the FITS image to be displayed
 *  @param wcsIdx: index of the WCS to be used
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 *  @return Pointer to the displayed plot structure for further customization and overplotting
 */
void QtFITSviewer::QtDisplayInWCS(QCustomPlot* qPlot,const  std::shared_ptr<const FITScube>& img, const size_t& wcsIdx, std::string OPTION)
{
    return QtDisplayInWCS(qPlot, img, 0 , img->Nelements(), wcsIdx, OPTION);
}

/**
 *  Display sub FITS 2D images onto the screen using default WCS as pixel coordinates.
 *  @param img: Pointer to the FITS image to be displayed
 *  @param iMin: index of the pixel used as the lower left corner
 *  @param iMax: index of the pixel used as the upper left corner
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 *  @return Pointer to the displayed plot structur for further customization and overplotting
 */
void QtFITSviewer::QtDisplayInWCS(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, std::string OPTION)
{
    return QtDisplayInWCS(qPlot, img, iMin , iMax, 0, OPTION);
}
/**
 *  Display sub FITS 2D images onto the screen using WCS as pixel coordinates.
 *  @param img: Pointer to the FITS image to be displayed
 *  @param iMin: index of the pixel used as the lower left corner
 *  @param wcsIdx: index of the WCS to be used
 *  @param iMax: index of the pixel used as the upper left corner
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 *  @return Pointer to the displayed plot structur for further customization and overplotting
 */
void QtFITSviewer::QtDisplayInWCS(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, const size_t& wcsIdx, std::string OPTION)
{
    qPlot->yAxis->setScaleRatio(qPlot->xAxis,1.0);

    const FITSwcs thisWcs = img->getWCS();
    std::string xLabel = thisWcs.CTYPE(wcsIdx)[0]+" ["+thisWcs.CUNIT(wcsIdx)[0]+"]";
    std::string yLabel = thisWcs.CTYPE(wcsIdx)[1]+" ["+thisWcs.CUNIT(wcsIdx)[1]+"]";
    
    //- Fill the graph
    qPlot->xAxis->setLabel(xLabel.c_str());
    qPlot->yAxis->setLabel(yLabel.c_str());
    
    OverlayInWCS(qPlot, img, iMin, iMax, wcsIdx, OPTION);
    return;
}

/**
 *  Display a single layer of a FITS cube onto the scrren.
 *  @param img: Pointer to the FITS image to be displayed
 *  @param iLayer: index of the layer to be displayed
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 *  @return Pointer to the displayed plot structure for further customization and overplotting
 */
void QtFITSviewer::QtDisplayLayer(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& _iLayer, std::string OPTION)
{
    size_t iLayer = _iLayer;
    if(img->GetDimension() <= 2)
        QtDisplay(qPlot, img, OPTION);
    
    if(iLayer > img->Size(2))
    {
        iLayer=img->GetDimension();
        throw FITSwarning("FITSimg","QtDisplayLayer","Image only contains "+std::to_string(img->GetDimension())+" layers.\nThe last layer will be displayed");
    }

    if(iLayer < 1)
        throw FITSwarning("FITSimg","QtDisplayLayer","Layer should be comprised between 1 to "+std::to_string(img->Size(3))+".\nThe first layer will be displayed");
    
    size_t nx = img->Size(1);
    size_t ny = img->Size(2);
    
    size_t iMin = nx*ny*(iLayer-1);
    size_t iMax = nx*ny + iMin;
    
    return QtDisplay(qPlot, img, iMin, iMax, OPTION);
}

/**
 *  Overlay FITS 2D images onto an existing plot.
 *  @param qPlot: Pointer to the existing plot
 *  @param img: Pointer to the FITS image to be displayed
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 */
void QtFITSviewer::Overlay(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , std::string OPTION)
{
    Overlay(qPlot, img, 0, img->Nelements(), OPTION);
}

/**
 *  Overlay FITS 2D images onto an existing plot using WCS as pixel coordinates.
 *  @param qPlot: Pointer to the existing plot
 *  @param img: Pointer to the FITS image to be displayed
 *  @param wcsIdx: index of the WCS to be used
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 */
void QtFITSviewer::OverlayInWCS(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& wcsIdx, std::string OPTION)
{
    OverlayInWCS(qPlot, img, 0, img->Nelements(), wcsIdx, OPTION);
}

/**
 *  Overlay FITS 2D images onto an existing plot using WCS as pixel coordinates using default WCS as reference.
 *  @param qPlot: Pointer to the existing plot
 *  @param img: Pointer to the FITS image to be displayed
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 */
void QtFITSviewer::OverlayInWCS(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , std::string OPTION)
{
    OverlayInWCS(qPlot, img, 0, img->Nelements(), 0, OPTION);
}

/**
 *  Overlay sub FITS 2D images onto an existing plot.
 *  @param qPlot: Pointer to the existing plot
 *  @param img: Pointer to the FITS image to be displayed
 *  @param iMin: index of the pixel used as the lower left corner
 *  @param iMax: index of the pixel used as the upper left corner
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 */
void QtFITSviewer::Overlay(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, std::string OPTION)
{
    std::vector<size_t> xPixels;
    
    std::transform(OPTION.begin(), OPTION.end(), OPTION.begin(), ::tolower);
    const size_t nx = img->Size(1);
    const size_t ny = img->Size(2);
    if(iMin >= img->Nelements() || iMax > img->Nelements() || iMin >= iMax)
        throw FITSexception(BAD_ELEM_NUM,"FITSimg","Overlay","iMin and iMax should be comprised between 0 to "+std::to_string(img->Nelements())+" and iMin < iMax");
    
    QtFITS::zScale useScale = QtFITS::zScale::fLIN;
    if(OPTION.find("logz",0) != std::string::npos )
        useScale  = QtFITS::zScale::fLOG;
    else if(OPTION.find("sinhz",0) != std::string::npos )
        useScale  = QtFITS::zScale::fSINH;
    else if(OPTION.find("quadz",0) != std::string::npos )
        useScale  = QtFITS::zScale::fQUAD;
    else if(OPTION.find("sqrtz",0) != std::string::npos )
        useScale  = QtFITS::zScale::fSQRT;
    else
        useScale = QtFITS::zScale::fLIN;
    
    QCPColorMap *colorMap = new QCPColorMap(qPlot->xAxis, qPlot->yAxis);
    colorMap->data()->setSize(nx, ny);  // we want the color map to have nx * ny data points
                                
    // Set the data coordinate range to cover the full image extent in x (key) and y (value).
    // Using [0, 0] for the key range collapses the map horizontally and leaves axes at defaults (-2..2).
    // QCPColorMap expects key/value ranges matching the data index domain [0..nx-1], [0..ny-1].
    colorMap->data()->setRange(QCPRange(0, static_cast<double>(nx-1)),QCPRange(0, static_cast<double>(ny-1)));	// span the coordinate range in both key (x) and value (y)
    colorMap->data()->fill(0);
    
    std::valarray<bool> mask = img->GetMask();
    // now we assign some data, by accessing the QCPColorMapData instance of the color map:
    
    for(size_t ik = iMin; ik < iMax; ik++)
    {
        if(mask[ik])
            continue;
        double Intensity = 0;
        Intensity += img->at(ik);
    
        xPixels.clear();
        
        xPixels = img->PixelCoordinates(ik);
        double xIndex = xPixels[0];
        double yIndex = xPixels[1];
            
        switch (useScale)
        {
            case QtFITS::fLIN:
                Intensity *= 1.;
                break;
                
            case QtFITS::fLOG:
                Intensity = std::log10(Intensity);
                break;
                
            case QtFITS::fQUAD:
                Intensity = std::pow(Intensity, 2.);
                break;
                
            case QtFITS::fSQRT:
                Intensity = std::sqrt(Intensity);
                break;
                
            case QtFITS::fSINH:
                Intensity = std::sinh(Intensity);
                break;
                
            default:
                Intensity *= 1.;
                break;
        }
        
        colorMap->data()->setCell(static_cast<int>(xIndex), static_cast<int>(yIndex), Intensity );
    }
    
    std::string UNITS = (img->HDU().Exists("UNITS"))?img->HDU().GetValueForKey("UNITS"):"ARB. UNIT";
    
    switch (useScale)
    {
        case QtFITS::fLOG:
            UNITS = "log10("+UNITS+")";
            break;
            
        case QtFITS::fQUAD:
            UNITS += "^2";
            break;
            
        case QtFITS::fSQRT:
            UNITS += "^(1/2)";
            break;
            
        case QtFITS::fSINH:
            UNITS += "sinh("+UNITS+")";
            break;
            
        default:
            break;
    }
    QCPColorScale *colorScale = NULL;
    
    if(	!(qPlot->plotLayout()->hasElement (0, 1)) )
    {
        // add a color scale:
        colorScale = new QCPColorScale(qPlot);
        qPlot->plotLayout()->addElement(0, 1, colorScale);          // add it to the right of the main axis rect
        // make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
        QCPMarginGroup *marginGroup = new QCPMarginGroup(qPlot);
        const QCP::MarginSides sides = QCP::MarginSides(QCP::msBottom) | QCP::msTop;
        qPlot->axisRect()->setMarginGroup(sides, marginGroup);
        colorScale->setMarginGroup(sides, marginGroup);
    }
    else
    {
        colorScale = static_cast<QCPColorScale*>(qPlot->plotLayout()->element(0, 1));
    }
    colorScale->setGradient(QCPColorGradient::gpNight);
    
    colorScale->axis()->setLabel(UNITS.c_str());
    colorScale->setType(QCPAxis::atRight);						// scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
        
    colorMap->setColorScale(colorScale);						// associate the color map with the color scale
    // we could have also created a QCPColorGradient instance and added own	colors to
    // the gradient, see the documentation of QCPColorGradient for what's possible.
    
    // rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
    colorMap->rescaleDataRange();
    
    if(std::abs(img->GetMaximum()/img->GetMinimum()) > 100)
        colorMap->setDataRange(QCPRange(img->Get5thpercentil(),img->Get95thpercentil()));
    
    colorMap->setInterpolate(false);
    
    // rescale the key (x) and value (y) axes so the whole color map is visible:
    
    QMetaObject::invokeMethod(qPlot, [qPlot]() mutable
                                {
                                    {
                                        std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
                                        qPlot->rescaleAxes();
                                        qPlot->yAxis->setScaleRatio(qPlot->xAxis,1.0);
                                        qPlot->replot();
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
}

/**
 *  Overlay sub FITS 2D images onto an existing plot using WCS as pixel coordinates, using default WCS at idex 0 as reference.
 *  @param qPlot: Pointer to the existing plot
 *  @param img: Pointer to the FITS image to be displayed
 *  @param iMin: index of the pixel used as the lower left corner
 *  @param iMax: index of the pixel used as the upper left corner
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 */
void QtFITSviewer::OverlayInWCS(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, std::string OPTION)
{
    OverlayInWCS(qPlot, img, iMin, iMax, 0, OPTION);
}

/**
 *  Overlay sub FITS 2D images onto an existing plot using WCS as pixel coordinates.
 *  @param qPlot: Pointer to the existing plot
 *  @param img: Pointer to the FITS image to be displayed
 *  @param iMin: index of the pixel used as the lower left corner
 *  @param iMax: index of the pixel used as the upper left corner
 *  @param wcsIdx: index of the worldcoordinate system to be used as reference
 *  @param OPTION: Optional parameters used to customize the plot:
 *      - LOGZ  : Display color scale in LOG10 scale
 *      - QUADZ : Display color scale in QUADRATIC scale
 *      - SQRTZ : Display color scale in SQUARE_ROOT scale
 *      - SINH  : Display color scale in SINH scale
 */
void QtFITSviewer::OverlayInWCS(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, const size_t& wcsIdx, std::string OPTION)
{
    worldCoords xPixels;
    
    std::transform(OPTION.begin(), OPTION.end(), OPTION.begin(), ::tolower);
    const size_t nx = img->Size(1);
    const size_t ny = img->Size(2);
    if(iMin >= img->Nelements() || iMax > img->Nelements() || iMin >= iMax)
        throw FITSexception(BAD_ELEM_NUM,"FITSimg","Overlay","iMin and iMax should be comprised between 0 to "+std::to_string(img->Nelements())+" and iMin < iMax");
    
    QtFITS::zScale useScale = QtFITS::zScale::fLIN;
    if(OPTION.find("logz",0) != std::string::npos )
        useScale  = QtFITS::zScale::fLOG;
    else if(OPTION.find("sinhz",0) != std::string::npos )
        useScale  = QtFITS::zScale::fSINH;
    else if(OPTION.find("quadz",0) != std::string::npos )
        useScale  = QtFITS::zScale::fQUAD;
    else if(OPTION.find("sqrtz",0) != std::string::npos )
        useScale  = QtFITS::zScale::fSQRT;
    else
        useScale = QtFITS::zScale::fLIN;
    
    QCPColorMap *colorMap = new QCPColorMap(qPlot->xAxis, qPlot->yAxis);
    colorMap->data()->setSize(nx, ny);  // we want the color map to have nx * ny data points

    std::valarray<bool> mask = img->GetMask();
    // now we assign some data, by accessing the QCPColorMapData instance of the color map:

    double min_x = std::numeric_limits<double>::max();
    double max_x = -1*std::numeric_limits<double>::max();

    double min_y = std::numeric_limits<double>::max();
    double max_y = -1*std::numeric_limits<double>::max();

    worldCoords wcs = img->WorldCoordinates(0,wcsIdx);
    min_x = std::min(min_x,wcs[0]);
    max_x = std::max(max_x,wcs[0]);
    min_y = std::min(min_y,wcs[1]);
    max_y = std::max(max_y,wcs[1]);
    
    wcs = img->WorldCoordinates(img->Size(1)-1,wcsIdx);
    min_x = std::min(min_x,wcs[0]);
    max_x = std::max(max_x,wcs[0]);
    min_y = std::min(min_y,wcs[1]);
    max_y = std::max(max_y,wcs[1]);
    
    wcs = img->WorldCoordinates((img->Size(2)-1)*img->Size(1),wcsIdx);
    min_x = std::min(min_x,wcs[0]);
    max_x = std::max(max_x,wcs[0]);
    min_y = std::min(min_y,wcs[1]);
    max_y = std::max(max_y,wcs[1]);
    
    wcs = img->WorldCoordinates( (img->Size(1)-1) + (img->Size(2)-1)*img->Size(1),wcsIdx);
    min_x = std::min(min_x,wcs[0]);
    max_x = std::max(max_x,wcs[0]);
    min_y = std::min(min_y,wcs[1]);
    max_y = std::max(max_y,wcs[1]);

    colorMap->data()->setRange(QCPRange(min_x, max_x), QCPRange(min_y, max_y));	// and span the coordinate range in both key (x) and value (y) dimensions
    
    colorMap->data()->fill(0);
    colorMap->data()->fillAlpha(0);
    
    for(size_t ik = iMin; ik < iMax; ik++)
    {
        if(mask[ik])
            continue;
        double Intensity = 0;
        Intensity += img->at(ik);
    
        xPixels.clear();
        
        xPixels = img->WorldCoordinates(ik);
        double xIndex = xPixels[0];
        double yIndex = xPixels[1];
            
        switch (useScale)
        {
            case QtFITS::fLIN:
                Intensity *= 1.;
                break;
                
            case QtFITS::fLOG:
                Intensity = std::log10(Intensity);
                break;
                
            case QtFITS::fQUAD:
                Intensity = std::pow(Intensity, 2.);
                break;
                
            case QtFITS::fSQRT:
                Intensity = std::sqrt(Intensity);
                break;
                
            case QtFITS::fSINH:
                Intensity = std::sinh(Intensity);
                break;
                
            default:
                Intensity *= 1.;
                break;
        }
        
        colorMap->data()->setData(xIndex, yIndex, Intensity );
        
        int xIdx, yIdx;
        colorMap->data()->coordToCell(xIndex,yIndex,&xIdx,&yIdx);
        colorMap->data()->setAlpha(xIdx, yIdx, 255);
    }

    std::string UNITS = (img->HDU().Exists("UNITS"))?img->HDU().GetValueForKey("UNITS"):"ARB. UNIT";
    
    switch (useScale)
    {
        case QtFITS::fLOG:
            UNITS = "log10("+UNITS+")";
            break;
            
        case QtFITS::fQUAD:
            UNITS += "^2";
            break;
            
        case QtFITS::fSQRT:
            UNITS += "^(1/2)";
            break;
            
        case QtFITS::fSINH:
            UNITS += "sinh("+UNITS+")";
            break;
            
        default:
            break;
    }
    QCPColorScale *colorScale = NULL;
    
    if(	!(qPlot->plotLayout()->hasElement (0, 1)) )
    {
        // add a color scale:
        colorScale = new QCPColorScale(qPlot);
        qPlot->plotLayout()->addElement(0, 1, colorScale);          // add it to the right of the main axis rect
        // make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
        QCPMarginGroup *marginGroup = new QCPMarginGroup(qPlot);
        const QCP::MarginSides sides = QCP::MarginSides(QCP::msBottom) | QCP::msTop;
        qPlot->axisRect()->setMarginGroup(sides, marginGroup);
        colorScale->setMarginGroup(sides, marginGroup);
    }
    else
    {
        colorScale = static_cast<QCPColorScale*>(qPlot->plotLayout()->element(0, 1));
    }
    colorScale->setGradient(QCPColorGradient::gpNight);
    
    colorScale->axis()->setLabel(UNITS.c_str());
    colorScale->setType(QCPAxis::atRight);						// scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
        
    colorMap->setColorScale(colorScale);						// associate the color map with the color scale
    // we could have also created a QCPColorGradient instance and added own	colors to
    // the gradient, see the documentation of QCPColorGradient for what's possible.
    
    // rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
    colorMap->rescaleDataRange();
    
    if(std::abs(img->GetMaximum()/img->GetMinimum()) > 100)
        colorMap->setDataRange(QCPRange(img->Get5thpercentil(),img->Get95thpercentil()));
    
    colorMap->setInterpolate(false);
    
    // rescale the key (x) and value (y) axes so the whole color map is visible:
    
    QMetaObject::invokeMethod(qPlot, [qPlot]() mutable
                                {
                                    {
                                        std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
                                        qPlot->rescaleAxes();
                                        qPlot->yAxis->setScaleRatio(qPlot->xAxis,1.0);
                                        qPlot->replot();
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
}

void QtFITSviewer::QtHistogram(QCustomPlot* customPlot, const std::shared_ptr<const FITScube>& in, const size_t& n_bins, QCPGraph *px_pdf, QCPGraph *px_cdf)
{
    double min = in->GetMinimum();
    double max = in->GetMaximum();
    return QtHistogram(customPlot, in, n_bins, min, max, px_pdf, px_cdf);
}

void QtFITSviewer::QtHistogram(QCustomPlot* customPlot, const std::shared_ptr<const FITScube>& in, const size_t& n_bins, const double& min, const double& max, QCPGraph *px_pdf, QCPGraph *px_cdf)
{
    QtFITS::pixelDistribution pd = QtFITSviewer::pixHistogram(in, n_bins, min, max);
    QVector<double>& xVals   = pd.bin_edges;
    QVector<double>& pdfVals = pd.pdf;
    QVector<double>& cdfVals = pd.cdf;

    std::string UNITS = "Pixel value ["+((in->HDU().Exists("UNITS"))?in->HDU().GetValueForKey("UNITS"):"ARB. UNIT")+"]";

    px_pdf->keyAxis()->setLabel(UNITS.c_str());
    px_pdf->valueAxis()->setLabel("Normalized PDF");
    px_cdf->valueAxis()->setLabel("Normalized CDF");

    QMetaObject::invokeMethod(customPlot, [px_pdf, px_cdf, customPlot,
                                           xVals, pdfVals ,cdfVals ]() mutable
                                {
                                    {
                                        std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
                                        px_pdf->setData(xVals, pdfVals);
                                        px_cdf->setData(xVals, cdfVals);
                                        px_pdf->valueAxis()->rescale();
                                        px_pdf->keyAxis()->rescale();
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
}

QtFITS::pixelDistribution QtFITSviewer::pixHistogram(const std::shared_ptr<const FITScube>& in, const size_t& n_bins)
{
    double min = in->GetMinimum();
    double max = in->GetMaximum();
    return pixHistogram(in, n_bins, min, max);
}

QtFITS::pixelDistribution QtFITSviewer::pixHistogram(const std::shared_ptr<const FITScube>& in, const size_t& n_bins, const double& min, const double& max)
{
    switch(in->GetEqBitPerPixel())
    {
        case BYTE_IMG:
            return pixHistogram<uint8_t>(in, n_bins, min, max);
        case SBYTE_IMG:
            return pixHistogram<int8_t>(in, n_bins, min, max);
        case SHORT_IMG:
            return pixHistogram<int16_t>(in, n_bins, min, max);
        case USHORT_IMG:
            return pixHistogram<uint16_t>(in, n_bins, min, max);
        case LONG_IMG:
            return pixHistogram<int32_t>(in, n_bins, min, max);
        case ULONG_IMG:
            return pixHistogram<uint32_t>(in, n_bins, min, max);
        case LONGLONG_IMG:
            return pixHistogram<int64_t>(in, n_bins, min, max);
        case ULONGLONG_IMG:
            return pixHistogram<int64_t>(in, n_bins, min, max);
        case FLOAT_IMG:
            return pixHistogram<float>(in, n_bins, min, max);
        case DOUBLE_IMG:
            return pixHistogram<double>(in, n_bins, min, max);
        default:
            throw FITSexception(BAD_BITPIX,"FITSimg","DrawHistogram","Unsupported BITPIX="+std::to_string(in->GetEqBitPerPixel()));
    }
}

void QtFITSviewer::QtProfileX(QCustomPlot* customPlot,const std::shared_ptr<const FITScube>& in, const size_t& iY, QCPGraph* x, bool horizontal)
{
    switch(in->GetEqBitPerPixel())
    {
        case BYTE_IMG:
            return  QtFITSviewer::QtProfileX<uint8_t>(customPlot,in, iY, x, horizontal);
            break;
        case SBYTE_IMG:
            return  QtFITSviewer::QtProfileX<int8_t>(customPlot,in, iY, x, horizontal);
            break;
        case SHORT_IMG:
            return  QtFITSviewer::QtProfileX<int16_t>(customPlot,in, iY, x, horizontal);
            break;
        case USHORT_IMG:
            return  QtFITSviewer::QtProfileX<uint16_t>(customPlot,in, iY, x, horizontal);
            break;
        case LONG_IMG:
            return  QtFITSviewer::QtProfileX<int32_t>(customPlot,in, iY, x, horizontal);
            break;
        case ULONG_IMG:
            return  QtFITSviewer::QtProfileX<uint32_t>(customPlot,in, iY, x, horizontal);
            break;
        case LONGLONG_IMG:
            return  QtFITSviewer::QtProfileX<int64_t>(customPlot,in, iY, x, horizontal);
            break;
        case ULONGLONG_IMG:
            return  QtFITSviewer::QtProfileX<uint64_t>(customPlot,in, iY, x, horizontal);
            break;
        case FLOAT_IMG:
            return  QtFITSviewer::QtProfileX<float>(customPlot,in, iY, x, horizontal);
            break;
        case DOUBLE_IMG:
            return  QtFITSviewer::QtProfileX<double>(customPlot,in, iY, x, horizontal);
            break;
        default:
            throw FITSexception(BAD_BITPIX,"FITSimg","DrawHistogram","Unsupported BITPIX="+std::to_string(in->GetEqBitPerPixel()));
    }
}

void QtFITSviewer::QtProfileY(QCustomPlot* customPlot,const std::shared_ptr<const FITScube>& in, const size_t& iX, QCPGraph* y, bool horizontal)
{
    switch(in->GetEqBitPerPixel())
    {
        case BYTE_IMG:
            return  QtFITSviewer::QtProfileY<uint8_t>(customPlot,in, iX, y, horizontal);
            break;
        case SBYTE_IMG:
            return  QtFITSviewer::QtProfileY<int8_t>(customPlot,in, iX, y, horizontal);
            break;
        case SHORT_IMG:
            return  QtFITSviewer::QtProfileY<int16_t>(customPlot,in, iX, y, horizontal);
            break;
        case USHORT_IMG:
            return  QtFITSviewer::QtProfileY<uint16_t>(customPlot,in, iX, y, horizontal);
            break;
        case LONG_IMG:
            return  QtFITSviewer::QtProfileY<int32_t>(customPlot,in, iX, y, horizontal);
            break;
        case ULONG_IMG:
            return  QtFITSviewer::QtProfileY<uint32_t>(customPlot,in, iX, y, horizontal);
            break;
        case LONGLONG_IMG:
            return  QtFITSviewer::QtProfileY<int64_t>(customPlot,in, iX, y, horizontal);
            break;
        case ULONGLONG_IMG:
            return  QtFITSviewer::QtProfileY<uint64_t>(customPlot,in, iX, y, horizontal);
            break;
        case FLOAT_IMG:
            return  QtFITSviewer::QtProfileY<float>(customPlot,in, iX, y, horizontal);
            break;
        case DOUBLE_IMG:
            return  QtFITSviewer::QtProfileY<double>(customPlot,in, iX, y, horizontal);
            break;
        default:
            throw FITSexception(BAD_BITPIX,"FITSimg","DrawHistogram","Unsupported BITPIX="+std::to_string(in->GetEqBitPerPixel()));
    }
} 

#include "QCustomContours.h"


/**
 * @brief Draw contour levels from a 2D FITS image onto an existing QCustomPlot.
 * @param qPlot Existing QCustomPlot to draw contours on.
 * @param img Shared pointer to the 2D FITS image.
 * @param nLevels Number of contour levels to draw.
 * @throws FITSexception if the image is not 2D array.
 */
void QtFITSviewer::QtContours(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& nLevels)
{
    if(img->GetDimension() != 2)
        throw FITSexception(BAD_DIMEN,"FITSimg","QtContours","Image dimension should be 2D to compute contours.");

    const size_t nx = img->Size(1);
    const size_t ny = img->Size(2);
    std::vector<std::vector<double>> Z(ny, std::vector<double>(nx));
    for (size_t j = 0; j < ny; ++j)
        for (size_t i = 0; i < nx; ++i)
            Z[j][i] = img->at(img->PixelIndex({i, j}));

    QCustomContours c(Z, nLevels);
    c.setPen(QPen(Qt::green, 1.0));

    QMetaObject::invokeMethod(qPlot, [c, qPlot]()
    {
        {
            std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
            c.render(qPlot);
            qPlot->replot();

        } // io_mutex released here

        {
            std::lock_guard<std::mutex> dk(QtFITS::msg_m);
            if (!QtFITS::finished)
                 QtFITS::finished |= true;
            QtFITS::msg_cv.notify_one(); // notify while holding msg_m (or move notify outside if preferred)
            QtFITS::finished &= false;
        } // msg_m released here
    });
}

// namespace DSL
