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
#include <DSTdisplay/QtFitsPloting.h>
#include <DSTdisplay/QCustomContours.h>
#include "QtFITSviewer.h"
#include <DSTdisplay/DSD_version.h>
#include <DSTfits/FITSexception.h>
#include <DSTfits/FITSimg.h>

using namespace DSL;

void QtFITSviewer::QtDisplay(QCustomPlot* qPlot,const  std::shared_ptr<const FITScube>& img  , std::string OPTION)
{
    return QtDisplay(qPlot, img, 0 , img->Nelements(), OPTION);
}

void QtFITSviewer::QtDisplay(QCPAxisRect* qPlot,const  std::shared_ptr<const FITScube>& img  , std::string OPTION)
{
    return QtDisplay(qPlot, img, 0 , img->Nelements(), OPTION);
}

void QtFITSviewer::QtDisplay(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, std::string OPTION)
{
    qPlot->yAxis->setScaleRatio(qPlot->xAxis,1.0);
    
    //- Fill the graph
    qPlot->xAxis->setLabel("pixel [px]");
    qPlot->yAxis->setLabel("pixel [px]");
    
    Overlay(qPlot, img, iMin, iMax, OPTION);
    return;
}

void QtFITSviewer::QtDisplay(QCPAxisRect* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, std::string OPTION)
{

    //- Fill the graph
    qPlot->axis(QCPAxis::AxisType::atLeft)->setLabel("pixel [px]");
    qPlot->axis(QCPAxis::AxisType::atBottom)->setLabel("pixel [px]");
    qPlot->axis(QCPAxis::AxisType::atLeft)->setScaleRatio(qPlot->axis(QCPAxis::AxisType::atBottom),1.0);

    Overlay(qPlot, img, iMin, iMax, OPTION);
    return;
}

void QtFITSviewer::QtDisplay(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>& img, double zMin, double zMax, std::string OPTION)
{
    QtDisplay(qPlot, img, OPTION);   // full image, auto range first

    // Override the colour-scale range with the caller-supplied intensity bounds.
    // Walk the parent plot's plottables to find the QCPColorMap that belongs to qPlot.
    QCustomPlot* plt = qPlot->parentPlot();
    for(int i = 0; i < plt->plottableCount(); ++i)
    {
        QCPColorMap* cm = qobject_cast<QCPColorMap*>(plt->plottable(i));
        if(cm && cm->keyAxis()->axisRect() == qPlot)
        {
            cm->setDataRange(QCPRange(zMin, zMax));
            break;
        }
    }
}

void QtFITSviewer::QtDisplayInWCS(QCustomPlot* qPlot,const  std::shared_ptr<const FITScube>& img  , std::string OPTION)
{
    return QtDisplayInWCS(qPlot, img, 0 , img->Nelements(), 0, OPTION);
}

void QtFITSviewer::QtDisplayInWCS(QCustomPlot* qPlot,const  std::shared_ptr<const FITScube>& img, const size_t& wcsIdx, std::string OPTION)
{
    return QtDisplayInWCS(qPlot, img, 0 , img->Nelements(), wcsIdx, OPTION);
}

void QtFITSviewer::QtDisplayInWCS(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, std::string OPTION)
{
    return QtDisplayInWCS(qPlot, img, iMin , iMax, 0, OPTION);
}

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

void QtFITSviewer::QtDisplayInWCS(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>& img, std::string OPTION)
{
    return QtDisplayInWCS(qPlot, img, 0, img->Nelements(), 0, OPTION);
}

void QtFITSviewer::QtDisplayInWCS(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>& img, const size_t& wcsIdx, std::string OPTION)
{
    return QtDisplayInWCS(qPlot, img, 0, img->Nelements(), wcsIdx, OPTION);
}

void QtFITSviewer::QtDisplayInWCS(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>& img, const size_t& iMin, const size_t& iMax, std::string OPTION)
{
    return QtDisplayInWCS(qPlot, img, iMin, iMax, 0, OPTION);
}

void QtFITSviewer::QtDisplayInWCS(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>& img, const size_t& iMin, const size_t& iMax, const size_t& wcsIdx, std::string OPTION)
{
    qPlot->axis(QCPAxis::atLeft)->setScaleRatio(qPlot->axis(QCPAxis::atBottom), 1.0);

    const FITSwcs thisWcs = img->getWCS();
    std::string xLabel = thisWcs.CTYPE(wcsIdx)[0]+" ["+thisWcs.CUNIT(wcsIdx)[0]+"]";
    std::string yLabel = thisWcs.CTYPE(wcsIdx)[1]+" ["+thisWcs.CUNIT(wcsIdx)[1]+"]";

    qPlot->axis(QCPAxis::atBottom)->setLabel(xLabel.c_str());
    qPlot->axis(QCPAxis::atLeft)->setLabel(yLabel.c_str());

    OverlayInWCS(qPlot, img, iMin, iMax, wcsIdx, OPTION);
    return;
}

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

void QtFITSviewer::QtDisplayLayer(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>& img, const size_t& _iLayer, std::string OPTION)
{
    size_t iLayer = _iLayer;
    if(img->GetDimension() <= 2)
        QtDisplay(qPlot, img, OPTION);

    if(iLayer > img->Size(2))
    {
        iLayer = img->GetDimension();
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

void QtFITSviewer::Overlay(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , std::string OPTION)
{
    Overlay(qPlot, img, 0, img->Nelements(), OPTION);
}

void QtFITSviewer::Overlay(QCPAxisRect* qPlot, const  std::shared_ptr<const FITScube>& img  , std::string OPTION)
{
    Overlay(qPlot, img, 0, img->Nelements(), OPTION);
}

void QtFITSviewer::OverlayInWCS(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& wcsIdx, std::string OPTION)
{
    OverlayInWCS(qPlot, img, 0, img->Nelements(), wcsIdx, OPTION);
}

void QtFITSviewer::OverlayInWCS(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , std::string OPTION)
{
    OverlayInWCS(qPlot, img, 0, img->Nelements(), 0, OPTION);
}

void QtFITSviewer::Overlay(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, std::string OPTION)
{
    return Overlay(qPlot->axisRect(), img, iMin, iMax, OPTION);
}

void QtFITSviewer::Overlay(QCPAxisRect* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, std::string OPTION)
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

    QCPColorMap *colorMap = new QCPColorMap(qPlot->axis(QCPAxis::atBottom), qPlot->axis(QCPAxis::atLeft));
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
    // Place the color scale in the same QCPLayoutGrid that owns qPlot, immediately to
    // its right. This makes every axis rect's scale independent: the main panel's scale
    // lives in plt->plotLayout() at (0,1), while a sigRect nested in rightPanel gets
    // its own scale at (0,1) of rightPanel — they never share or overwrite each other.
    QCPColorScale *colorScale = nullptr;

    QCPLayoutGrid* parentGrid = qobject_cast<QCPLayoutGrid*>(qPlot->layout());
    int rectRow = 0, rectCol = 0;
    if(parentGrid)
    {
        for(int r = 0; r < parentGrid->rowCount(); ++r)
            for(int c = 0; c < parentGrid->columnCount(); ++c)
                if(parentGrid->element(r, c) == qPlot)
                    { rectRow = r; rectCol = c; }
    }
    const int scaleCol = rectCol + 1;

    QCPLayoutElement* existing = parentGrid ? parentGrid->element(rectRow, scaleCol) : nullptr;
    if(!existing)
    {
        colorScale = new QCPColorScale(qPlot->parentPlot());
        if(parentGrid)
            parentGrid->addElement(rectRow, scaleCol, colorScale);
        else
            qPlot->parentPlot()->plotLayout()->addElement(0, 1, colorScale);
        QCPMarginGroup *marginGroup = new QCPMarginGroup(qPlot->parentPlot());
        const QCP::MarginSides sides = QCP::MarginSides(QCP::msBottom) | QCP::msTop;
        qPlot->setMarginGroup(sides, marginGroup);
        colorScale->setMarginGroup(sides, marginGroup);
    }
    else
    {
        colorScale = static_cast<QCPColorScale*>(existing);
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
                                        qPlot->parentPlot()->rescaleAxes();
                                        qPlot->parentPlot()->yAxis->setScaleRatio(qPlot->parentPlot()->xAxis,1.0);
                                        qPlot->parentPlot()->replot();
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

void QtFITSviewer::OverlayInWCS(QCustomPlot* qPlot, const  std::shared_ptr<const FITScube>& img  , const size_t& iMin, const size_t& iMax, std::string OPTION)
{
    OverlayInWCS(qPlot, img, iMin, iMax, 0, OPTION);
}

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

void QtFITSviewer::OverlayInWCS(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>& img, std::string OPTION)
{
    OverlayInWCS(qPlot, img, 0, img->Nelements(), 0, OPTION);
}

void QtFITSviewer::OverlayInWCS(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>& img, const size_t& wcsIdx, std::string OPTION)
{
    OverlayInWCS(qPlot, img, 0, img->Nelements(), wcsIdx, OPTION);
}

void QtFITSviewer::OverlayInWCS(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>& img, const size_t& iMin, const size_t& iMax, std::string OPTION)
{
    OverlayInWCS(qPlot, img, iMin, iMax, 0, OPTION);
}

void QtFITSviewer::OverlayInWCS(QCPAxisRect* qPlot, const std::shared_ptr<const FITScube>& img, const size_t& iMin, const size_t& iMax, const size_t& wcsIdx, std::string OPTION)
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

    QCPColorMap *colorMap = new QCPColorMap(qPlot->axis(QCPAxis::atBottom), qPlot->axis(QCPAxis::atLeft));
    colorMap->data()->setSize(nx, ny);

    std::valarray<bool> mask = img->GetMask();

    double min_x = std::numeric_limits<double>::max();
    double max_x = -1*std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_y = -1*std::numeric_limits<double>::max();

    worldCoords wcs = img->WorldCoordinates(0, wcsIdx);
    min_x = std::min(min_x,wcs[0]); max_x = std::max(max_x,wcs[0]);
    min_y = std::min(min_y,wcs[1]); max_y = std::max(max_y,wcs[1]);

    wcs = img->WorldCoordinates(img->Size(1)-1, wcsIdx);
    min_x = std::min(min_x,wcs[0]); max_x = std::max(max_x,wcs[0]);
    min_y = std::min(min_y,wcs[1]); max_y = std::max(max_y,wcs[1]);

    wcs = img->WorldCoordinates((img->Size(2)-1)*img->Size(1), wcsIdx);
    min_x = std::min(min_x,wcs[0]); max_x = std::max(max_x,wcs[0]);
    min_y = std::min(min_y,wcs[1]); max_y = std::max(max_y,wcs[1]);

    wcs = img->WorldCoordinates((img->Size(1)-1) + (img->Size(2)-1)*img->Size(1), wcsIdx);
    min_x = std::min(min_x,wcs[0]); max_x = std::max(max_x,wcs[0]);
    min_y = std::min(min_y,wcs[1]); max_y = std::max(max_y,wcs[1]);

    colorMap->data()->setRange(QCPRange(min_x, max_x), QCPRange(min_y, max_y));
    colorMap->data()->fill(0);
    colorMap->data()->fillAlpha(0);

    for(size_t ik = iMin; ik < iMax; ik++)
    {
        if(mask[ik])
            continue;
        double Intensity = img->at(ik);

        xPixels.clear();
        xPixels = img->WorldCoordinates(ik);
        double xIndex = xPixels[0];
        double yIndex = xPixels[1];

        switch (useScale)
        {
            case QtFITS::fLOG:  Intensity = std::log10(Intensity); break;
            case QtFITS::fQUAD: Intensity = std::pow(Intensity, 2.); break;
            case QtFITS::fSQRT: Intensity = std::sqrt(Intensity); break;
            case QtFITS::fSINH: Intensity = std::sinh(Intensity); break;
            default: break;
        }

        colorMap->data()->setData(xIndex, yIndex, Intensity);
        int xIdx, yIdx;
        colorMap->data()->coordToCell(xIndex, yIndex, &xIdx, &yIdx);
        colorMap->data()->setAlpha(xIdx, yIdx, 255);
    }

    std::string UNITS = (img->HDU().Exists("UNITS"))?img->HDU().GetValueForKey("UNITS"):"ARB. UNIT";
    switch (useScale)
    {
        case QtFITS::fLOG:  UNITS = "log10("+UNITS+")"; break;
        case QtFITS::fQUAD: UNITS += "^2"; break;
        case QtFITS::fSQRT: UNITS += "^(1/2)"; break;
        case QtFITS::fSINH: UNITS = "sinh("+UNITS+")"; break;
        default: break;
    }

    QCPColorScale *colorScale = NULL;
    if(!(qPlot->parentPlot()->plotLayout()->hasElement(0, 1)))
    {
        colorScale = new QCPColorScale(qPlot->parentPlot());
        qPlot->parentPlot()->plotLayout()->addElement(0, 1, colorScale);
        QCPMarginGroup *marginGroup = new QCPMarginGroup(qPlot->parentPlot());
        const QCP::MarginSides sides = QCP::MarginSides(QCP::msBottom) | QCP::msTop;
        qPlot->setMarginGroup(sides, marginGroup);
        colorScale->setMarginGroup(sides, marginGroup);
    }
    else
    {
        colorScale = static_cast<QCPColorScale*>(qPlot->parentPlot()->plotLayout()->element(0, 1));
    }
    colorScale->setGradient(QCPColorGradient::gpNight);
    colorScale->axis()->setLabel(UNITS.c_str());
    colorScale->setType(QCPAxis::atRight);

    colorMap->setColorScale(colorScale);
    colorMap->rescaleDataRange();

    if(std::abs(img->GetMaximum()/img->GetMinimum()) > 100)
        colorMap->setDataRange(QCPRange(img->Get5thpercentil(), img->Get95thpercentil()));

    colorMap->setInterpolate(false);

    QCustomPlot* parentPlot = qPlot->parentPlot();
    QMetaObject::invokeMethod(parentPlot, [parentPlot]() mutable
                                {
                                    {
                                        std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
                                        parentPlot->rescaleAxes();
                                        parentPlot->yAxis->setScaleRatio(parentPlot->xAxis, 1.0);
                                        parentPlot->replot();
                                    } // io_mutex released here

                                    {
                                        std::lock_guard<std::mutex> dk(QtFITS::msg_m);
                                        if (!QtFITS::finished)
                                             QtFITS::finished |= true;
                                        QtFITS::msg_cv.notify_one();
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

static void applyPlotOptions(QCPGraph* curve, const std::string& OPTION)
{
    auto optVal = [&](const std::string& key) -> std::string {
        auto pos = OPTION.find(key + "=");
        if (pos == std::string::npos) return {};
        auto start = pos + key.size() + 1;
        auto end   = OPTION.find_first_of(" \t;", start);
        return OPTION.substr(start, end == std::string::npos ? end : end - start);
    };

    // --- line pen ---
    QPen pen = curve->pen();

    if (auto v = optVal("line.color"); !v.empty())
        pen.setColor(QColor(QString::fromStdString(v)));

    if (auto v = optVal("line.width"); !v.empty())
        pen.setWidthF(std::stof(v));

    if (auto v = optVal("line.style"); !v.empty()) {
        static const std::unordered_map<std::string, Qt::PenStyle> penStyles = {
            {"solid",      Qt::SolidLine},
            {"dash",       Qt::DashLine},
            {"dot",        Qt::DotLine},
            {"dashdot",    Qt::DashDotLine},
            {"dashdotdot", Qt::DashDotDotLine},
        };
        if (auto it = penStyles.find(v); it != penStyles.end())
            pen.setStyle(it->second);
    }

    curve->setPen(pen);

    // --- scatter / marker ---
    QCPScatterStyle scatter = curve->scatterStyle();

    if (auto v = optVal("marker.style"); !v.empty()) {
        static const std::unordered_map<std::string, QCPScatterStyle::ScatterShape> shapes = {
            {"none",     QCPScatterStyle::ssNone},
            {"dot",      QCPScatterStyle::ssDot},
            {"cross",    QCPScatterStyle::ssCross},
            {"plus",     QCPScatterStyle::ssPlus},
            {"circle",   QCPScatterStyle::ssCircle},
            {"disc",     QCPScatterStyle::ssDisc},
            {"square",   QCPScatterStyle::ssSquare},
            {"diamond",  QCPScatterStyle::ssDiamond},
            {"triangle", QCPScatterStyle::ssTriangle},
            {"star",     QCPScatterStyle::ssStar},
        };
        if (auto it = shapes.find(v); it != shapes.end())
            scatter.setShape(it->second);
    }

    if (auto v = optVal("marker.size"); !v.empty())
        scatter.setSize(std::stof(v));

    if (auto v = optVal("marker.color"); !v.empty()) {
        QColor col(QString::fromStdString(v));
        scatter.setPen(QPen(col));
        scatter.setBrush(QBrush(col));
    }

    curve->setScatterStyle(scatter);
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtCurve( QCustomPlot* qPlot, const std::shared_ptr<const FITStable>& tab, const std::string &xCol  , const std::string &yCol)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtCurve","Null pointer provided for the FITStable.");
    
    try
    {
        tab->getColumn(xCol);
        tab->getColumn(yCol);
    }
    catch(...)
    {
        throw;
    }

    DSL::dtype xType = tab->getColumn(xCol)->getType();
    DSL::dtype yType = tab->getColumn(yCol)->getType();

    size_t x_nElem = tab->getColumn(xCol)->getNelem();
    size_t y_nElem = tab->getColumn(yCol)->getNelem();

    if(x_nElem != y_nElem || x_nElem == 0 || y_nElem == 0)
        throw FITSexception(BAD_DIMEN,"QtFITSviewer","QtCurve","Columns "+xCol+" and "+yCol+" have different number of elements or one or both are empty ("+std::to_string(x_nElem)+" vs "+std::to_string(y_nElem)+").");

    if(xType == DSL::dtype::tsbyte         || yType == DSL::dtype::tsbyte   ||
       xType == DSL::dtype::tstring        || yType == DSL::dtype::tstring  ||
       xType == DSL::dtype::tlogical       || yType == DSL::dtype::tlogical ||
       xType == DSL::dtype::tbit           || yType == DSL::dtype::tbit     ||
       xType == DSL::dtype::tbyte          || yType == DSL::dtype::tbyte    ||
       xType == DSL::dtype::tcplx          || yType == DSL::dtype::tdbcplx   )
        throw FITSexception(BAD_TFORM_DTYPE,"QtFITSviewer","QtCurve",tab->getColumn(xCol)->getDataType(xType)+" combined to "+tab->getColumn(yCol)->getDataType(yType)+"  is not supported for plotting.");

    const uint32_t xydT = QtFITS::dtype_pair(xType, yType);

    std::vector<std::shared_ptr<QCPGraph>> curves;

    if(x_nElem == 1)
    {
        // QCustomPlot takes ownership; use a no-op deleter to avoid double-free.
        auto curve = std::shared_ptr<QCPGraph>(new QCPGraph(qPlot->xAxis, qPlot->yAxis), [](QCPGraph*){});
        switch (xydT)
        {
            // tshort = int16_t
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tshort):
                _QtCurve<int16_t, int16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tushort):
                _QtCurve<int16_t, uint16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tint):
                _QtCurve<int16_t, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tuint):
                _QtCurve<int16_t, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tlong):
                _QtCurve<int16_t, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tulong):
                _QtCurve<int16_t, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tlonglong):
                _QtCurve<int16_t, int64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tulonglong):
                _QtCurve<int16_t, uint64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tfloat):
                _QtCurve<int16_t, float>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tdouble):
                _QtCurve<int16_t, double>(*curve, tab, xCol, yCol);
                break;

            // tushort = uint16_t
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tshort):
                _QtCurve<uint16_t, int16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tushort):
                _QtCurve<uint16_t, uint16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tint):
                _QtCurve<uint16_t, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tuint):
                _QtCurve<uint16_t, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tlong):
                _QtCurve<uint16_t, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tulong):
                _QtCurve<uint16_t, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tlonglong):
                _QtCurve<uint16_t, int64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tulonglong):
                _QtCurve<uint16_t, uint64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tfloat):
                _QtCurve<uint16_t, float>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tdouble):
                _QtCurve<uint16_t, double>(*curve, tab, xCol, yCol);
                break;

            // tint = int32_t
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tshort):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tshort):
                _QtCurve<int32_t, int16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tushort):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tushort):
                _QtCurve<int32_t, uint16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tint):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tint):
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tlong):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tlong):
                _QtCurve<int32_t, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tuint):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tuint):
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tulong):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tulong):
                _QtCurve<int32_t, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tlonglong):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tlonglong):
                _QtCurve<int32_t, int64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tulonglong):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tulonglong):
                _QtCurve<int32_t, uint64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tfloat):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tfloat):
                _QtCurve<int32_t, float>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tdouble):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tdouble):
                _QtCurve<int32_t, double>(*curve, tab, xCol, yCol);
                break;

            // tuint = uint32_t
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tshort):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tshort):
                _QtCurve<uint32_t, int16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tushort):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tushort):
                _QtCurve<uint32_t, uint16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tint):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tint):
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tlong):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tlong):
                _QtCurve<uint32_t, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tuint):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tuint):
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tulong):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tulong):
                _QtCurve<uint32_t, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tlonglong):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tlonglong):
                _QtCurve<uint32_t, int64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tulonglong):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tulonglong):
                _QtCurve<uint32_t, uint64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tfloat):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tfloat):
                _QtCurve<uint32_t, float>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tdouble):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tdouble):
                _QtCurve<uint32_t, double>(*curve, tab, xCol, yCol);
                break;

            // tlonglong = int64_t
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tshort):
                _QtCurve<int64_t, int16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tushort):
                _QtCurve<int64_t, uint16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tint):
                _QtCurve<int64_t, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tuint):
                _QtCurve<int64_t, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tlong):
                _QtCurve<int64_t, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tulong):
                _QtCurve<int64_t, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tlonglong):
                _QtCurve<int64_t, int64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tulonglong):
                _QtCurve<int64_t, uint64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tfloat):
                _QtCurve<int64_t, float>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tdouble):
                _QtCurve<int64_t, double>(*curve, tab, xCol, yCol);
                break;

            // tulonglong = uint64_t
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tshort):
                _QtCurve<uint64_t, int16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tushort):
                _QtCurve<uint64_t, uint16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tint):
                _QtCurve<uint64_t, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tuint):
                _QtCurve<uint64_t, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tlong):
                _QtCurve<uint64_t, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tulong):
                _QtCurve<uint64_t, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tlonglong):
                _QtCurve<uint64_t, int64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tulonglong):
                _QtCurve<uint64_t, uint64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tfloat):
                _QtCurve<uint64_t, float>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tdouble):
                _QtCurve<uint64_t, double>(*curve, tab, xCol, yCol);
                break;

            // tfloat = float
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tshort):
                _QtCurve<float, int16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tushort):
                _QtCurve<float, uint16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tint):
                _QtCurve<float, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tuint):
                _QtCurve<float, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tlong):
                _QtCurve<float, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tulong):
                _QtCurve<float, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tlonglong):
                _QtCurve<float, int64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tulonglong):
                _QtCurve<float, uint64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tfloat):
                _QtCurve<float, float>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tdouble):
                _QtCurve<float, double>(*curve, tab, xCol, yCol);
                break;

            // tdouble = double
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tshort):
                _QtCurve<double, int16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tushort):
                _QtCurve<double, uint16_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tint):
                _QtCurve<double, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tuint):
                _QtCurve<double, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tlong):
                _QtCurve<double, int32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tulong):
                _QtCurve<double, uint32_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tlonglong):
                _QtCurve<double, int64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tulonglong):
                _QtCurve<double, uint64_t>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tfloat):
                _QtCurve<double, float>(*curve, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tdouble):
                _QtCurve<double, double>(*curve, tab, xCol, yCol);
                break;

            default:
                break;
        }

        curves.push_back(curve);
    }
    else
    {
        switch (xydT)
        {
            // tshort = int16_t
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tshort):
                return _QtVectorCurve<int16_t, int16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tushort):
                return _QtVectorCurve<int16_t, uint16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tint):
                return _QtVectorCurve<int16_t, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tuint):
                return _QtVectorCurve<int16_t, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tlong):
                return _QtVectorCurve<int16_t, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tulong):
                return _QtVectorCurve<int16_t, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tlonglong):
                return _QtVectorCurve<int16_t, int64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tulonglong):
                return _QtVectorCurve<int16_t, uint64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tfloat):
                return _QtVectorCurve<int16_t, float>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tshort, DSL::dtype::tdouble):
                return _QtVectorCurve<int16_t, double>(qPlot, tab, xCol, yCol);
                break;

            // tushort = uint16_t
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tshort):
                return _QtVectorCurve<uint16_t, int16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tushort):
                return _QtVectorCurve<uint16_t, uint16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tint):
                return _QtVectorCurve<uint16_t, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tuint):
                return _QtVectorCurve<uint16_t, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tlong):
                return _QtVectorCurve<uint16_t, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tulong):
                return _QtVectorCurve<uint16_t, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tlonglong):
                return _QtVectorCurve<uint16_t, int64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tulonglong):
                return _QtVectorCurve<uint16_t, uint64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tfloat):
                return _QtVectorCurve<uint16_t, float>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tushort, DSL::dtype::tdouble):
                return _QtVectorCurve<uint16_t, double>(qPlot, tab, xCol, yCol);
                break;

            // tint = int32_t
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tshort):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tshort):
                return _QtVectorCurve<int32_t, int16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tushort):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tushort):
                return _QtVectorCurve<int32_t, uint16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tint):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tint):
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tlong):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tlong):
                return _QtVectorCurve<int32_t, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tuint):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tuint):
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tulong):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tulong):
                return _QtVectorCurve<int32_t, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tlonglong):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tlonglong):
                return _QtVectorCurve<int32_t, int64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tulonglong):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tulonglong):
                return _QtVectorCurve<int32_t, uint64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tfloat):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tfloat):
                return _QtVectorCurve<int32_t, float>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tint, DSL::dtype::tdouble):
            case QtFITS::dtype_pair(DSL::dtype::tlong, DSL::dtype::tdouble):
                return _QtVectorCurve<int32_t, double>(qPlot, tab, xCol, yCol);
                break;

            // tuint = uint32_t
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tshort):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tshort):
                return _QtVectorCurve<uint32_t, int16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tushort):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tushort):
                return _QtVectorCurve<uint32_t, uint16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tint):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tint):
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tlong):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tlong):
                return _QtVectorCurve<uint32_t, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tuint):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tuint):
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tulong):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tulong):
                return _QtVectorCurve<uint32_t, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tlonglong):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tlonglong):
                return _QtVectorCurve<uint32_t, int64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tulonglong):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tulonglong):
                return _QtVectorCurve<uint32_t, uint64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tfloat):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tfloat):
                return _QtVectorCurve<uint32_t, float>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tuint, DSL::dtype::tdouble):
            case QtFITS::dtype_pair(DSL::dtype::tulong, DSL::dtype::tdouble):
                return _QtVectorCurve<uint32_t, double>(qPlot, tab, xCol, yCol);
                break;

            // tlonglong = int64_t
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tshort):
                return _QtVectorCurve<int64_t, int16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tushort):
                return _QtVectorCurve<int64_t, uint16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tint):
                return _QtVectorCurve<int64_t, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tuint):
                return _QtVectorCurve<int64_t, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tlong):
                _QtVectorCurve<int64_t, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tulong):
                return _QtVectorCurve<int64_t, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tlonglong):
                return _QtVectorCurve<int64_t, int64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tulonglong):
                return _QtVectorCurve<int64_t, uint64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tfloat):
                return _QtVectorCurve<int64_t, float>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tlonglong, DSL::dtype::tdouble):
                return _QtVectorCurve<int64_t, double>(qPlot, tab, xCol, yCol);
                break;

            // tulonglong = uint64_t
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tshort):
                return _QtVectorCurve<uint64_t, int16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tushort):
                return _QtVectorCurve<uint64_t, uint16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tint):
                return _QtVectorCurve<uint64_t, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tuint):
                return _QtVectorCurve<uint64_t, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tlong):
                return _QtVectorCurve<uint64_t, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tulong):
                return _QtVectorCurve<uint64_t, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tlonglong):
                return _QtVectorCurve<uint64_t, int64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tulonglong):
                return _QtVectorCurve<uint64_t, uint64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tfloat):
                return _QtVectorCurve<uint64_t, float>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tulonglong, DSL::dtype::tdouble):
                return _QtVectorCurve<uint64_t, double>(qPlot, tab, xCol, yCol);
                break;

            // tfloat = float
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tshort):
                return _QtVectorCurve<float, int16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tushort):
                return _QtVectorCurve<float, uint16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tint):
                return _QtVectorCurve<float, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tuint):
                return _QtVectorCurve<float, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tlong):
                return _QtVectorCurve<float, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tulong):
                return _QtVectorCurve<float, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tlonglong):
                return _QtVectorCurve<float, int64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tulonglong):
                return _QtVectorCurve<float, uint64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tfloat):
                return _QtVectorCurve<float, float>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tfloat, DSL::dtype::tdouble):
                return _QtVectorCurve<float, double>(qPlot, tab, xCol, yCol);
                break;

            // tdouble = double
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tshort):
                return _QtVectorCurve<double, int16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tushort):
                return _QtVectorCurve<double, uint16_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tint):
                return _QtVectorCurve<double, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tuint):
                return _QtVectorCurve<double, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tlong):
                return _QtVectorCurve<double, int32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tulong):
                return _QtVectorCurve<double, uint32_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tlonglong):
                return _QtVectorCurve<double, int64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tulonglong):
                return _QtVectorCurve<double, uint64_t>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tfloat):
                return _QtVectorCurve<double, float>(qPlot, tab, xCol, yCol);
                break;
            case QtFITS::dtype_pair(DSL::dtype::tdouble, DSL::dtype::tdouble):
                return _QtVectorCurve<double, double>(qPlot, tab, xCol, yCol);
                break;

            default:
                break;
        }
    }

    return curves;  // QCustomPlot owns lifetime; shared_ptr is non-owning
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtCurve( QCustomPlot* qPlot, const std::shared_ptr<const FITStable>& tab, const std::string &yCol)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtCurve","Null pointer provided for the FITStable.");
    
    try
    {
        tab->getColumn(yCol);
    }
    catch(...)
    {
        throw;
    }

    DSL::dtype yType = tab->getColumn(yCol)->getType();

    size_t y_nElem = tab->getColumn(yCol)->getNelem();

    if(y_nElem == 0)
        throw FITSexception(BAD_DIMEN,"QtFITSviewer","QtCurve","Columns "+yCol+" ise empty ("+std::to_string(y_nElem)+").");

    if(yType == DSL::dtype::tsbyte   ||
       yType == DSL::dtype::tstring  ||
       yType == DSL::dtype::tlogical ||
       yType == DSL::dtype::tbit     ||
       yType == DSL::dtype::tbyte    )
        throw FITSexception(BAD_TFORM_DTYPE,"QtFITSviewer","QtCurve",tab->getColumn(yCol)->getDataType(yType)+"  is not supported for plotting.");

    
    std::vector<std::shared_ptr<QCPGraph>> curves;

    if(y_nElem == 1)
    {
        // QCustomPlot takes ownership; use a no-op deleter to avoid double-free.
        auto curve = std::shared_ptr<QCPGraph>(new QCPGraph(qPlot->xAxis, qPlot->yAxis), [](QCPGraph*){});
        switch (yType)
        {
            // tshort = int16_t
            case DSL::dtype::tshort:
                _QtCurve<int16_t>(*curve, tab, yCol);
                break;

            // tushort = uint16_t
            case DSL::dtype::tushort:
                _QtCurve<uint16_t>(*curve, tab, yCol);
                break;

            // tint = int32_t
            case DSL::dtype::tint:
            case DSL::dtype::tlong:
                _QtCurve<int32_t>(*curve, tab, yCol);
                break;

            // tuint = uint32_t
            case DSL::dtype::tuint:
            case DSL::dtype::tulong:
                _QtCurve<uint32_t>(*curve, tab, yCol);
                break;

            // tlonglong = int64_t
            case DSL::dtype::tlonglong:
                _QtCurve<int64_t>(*curve, tab, yCol);
                break;

            // tulonglong = uint64_t
            case DSL::dtype::tulonglong:
                _QtCurve<uint64_t>(*curve, tab, yCol);
                break;

            // tfloat = float
            case DSL::dtype::tfloat:
                _QtCurve<float>(*curve, tab, yCol);
                break;

            // tdouble = double
            case DSL::dtype::tdouble:
                _QtCurve<double>(*curve, tab, yCol);
                break;

            // float complex
            case DSL::dtype::tcplx:
                _QtCplxCurve<float>(*curve, tab, yCol);
                break;

            // double complex
            case DSL::dtype::tdbcplx:
                _QtCplxCurve<double>(*curve, tab, yCol);
                break;

            default:
                break;
        }
        curves.push_back(curve);
    }
    else
    {
        switch (yType)
        {
            // tshort = int16_t
            case DSL::dtype::tshort:
                return _QtVectorCurve<int16_t>(qPlot, tab, yCol);
                break;

            // tushort = uint16_t
            case DSL::dtype::tushort:
                return _QtVectorCurve<uint16_t>(qPlot, tab, yCol);
                break;

            // tint = int32_t
            case DSL::dtype::tint:
            case DSL::dtype::tlong:
                return _QtVectorCurve<int32_t>(qPlot, tab, yCol);
                break;

            // tuint = uint32_t
            case DSL::dtype::tuint:
            case DSL::dtype::tulong:
                return _QtVectorCurve<uint32_t>(qPlot, tab, yCol);
                break;

            // tlonglong = int64_t
            case DSL::dtype::tlonglong:
                return _QtVectorCurve<int64_t>(qPlot, tab, yCol);
                break;

            // tulonglong = uint64_t
            case DSL::dtype::tulonglong:
                return _QtVectorCurve<uint64_t>(qPlot, tab, yCol);
                break;

            // tfloat = float
            case DSL::dtype::tfloat:
                return _QtVectorCurve<float>(qPlot, tab, yCol);
                break;

            // tdouble = double
            case DSL::dtype::tdouble:
                return _QtVectorCurve<double>(qPlot, tab, yCol);
                break;

            // float complex
            case DSL::dtype::tcplx:
                return _QtCplxVectorCurve<float>(qPlot, tab, yCol);
                break;

            // double complex
            case DSL::dtype::tdbcplx:
                return _QtCplxVectorCurve<double>(qPlot, tab, yCol);
                break;

            default:
                break;
        }

    }

    return curves;  // QCustomPlot owns lifetime; shared_ptr is non-owning
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtScatter(QCustomPlot* qPlot, const std::shared_ptr<const FITStable>& tab, const std::string &xCol, const std::string &yCol)
{
    std::vector<std::shared_ptr<QCPGraph>> scatter = QtFITSviewer::QtCurve(qPlot, tab, xCol, yCol);

    for(auto& g : scatter)
        g->setLineStyle(QCPGraph::lsNone);

    return scatter;
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtScatter(QCustomPlot* qPlot, const std::shared_ptr<const FITStable>& tab, const std::string &yCol)
{
    std::vector<std::shared_ptr<QCPGraph>> scatter = QtFITSviewer::QtCurve(qPlot, tab, yCol);

    for(auto& g : scatter)
        g->setLineStyle(QCPGraph::lsNone);

    return scatter;
}

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
// ===========================================================================
// File-local helpers (anonymous namespace — not part of QtFITSviewer API)
// ===========================================================================

namespace {

/// Collect a scalar numeric column as vector<double>, optionally filtered by a RowSet.
/// Throws BAD_TFORM_DTYPE for non-numeric types (string, logical, bit, byte).
static std::vector<double> _colToDoubles(
    const std::shared_ptr<const FITStable>& tab,
    const std::string& col,
    const char* caller,
    const RowSet* rows = nullptr)
{
    std::vector<double> out;
    const auto& form = *tab->getColumn(col);
    const DSL::dtype t = form.getType();

    if(t == DSL::dtype::tstring  || t == DSL::dtype::tlogical ||
       t == DSL::dtype::tbit     || t == DSL::dtype::tsbyte   || t == DSL::dtype::tbyte)
        throw FITSexception(BAD_TFORM_DTYPE, "QtFITSviewer", caller,
            "Column '"+col+"': type "+form.getDataType(t)+" is not a supported numeric type.");

    const size_t n = rows ? rows->indices().size() : tab->nrows();
    out.reserve(n);

    auto fill = [&](const auto& vec)
    {
        if(rows)
        {
            // values<T>() is empty when the column holds vectors: indexing it would read out of bounds.
            if(!rows->empty() && rows->indices().back() >= vec.size())
                throw FITSexception(BAD_TFORM_DTYPE, "QtFITSviewer", caller,
                    "Column '"+col+"' is not a scalar column.");
            for(size_t idx : rows->indices())
                out.push_back(static_cast<double>(vec[idx]));
        }
        else
            for(const auto& x : vec) out.push_back(static_cast<double>(x));
    };

    switch(t)
    {
        case DSL::dtype::tshort:     fill(form.values<int16_t>());   break;
        case DSL::dtype::tushort:    fill(form.values<uint16_t>());  break;
        case DSL::dtype::tint:
        case DSL::dtype::tlong:      fill(form.values<int32_t>());   break;
        case DSL::dtype::tuint:
        case DSL::dtype::tulong:     fill(form.values<uint32_t>());  break;
        case DSL::dtype::tlonglong:  fill(form.values<int64_t>());   break;
        case DSL::dtype::tulonglong: fill(form.values<uint64_t>());  break;
        case DSL::dtype::tfloat:     fill(form.values<float>());     break;
        case DSL::dtype::tdouble:    fill(form.values<double>());    break;
        default:
            throw FITSexception(BAD_TFORM_DTYPE, "QtFITSviewer", caller,
                "Column '"+col+"': type "+form.getDataType(t)+" is not a real scalar type.");
    }
    return out;
}

static bool _isComplex(const FITSform& form)
{
    return form.getType() == DSL::dtype::tcplx || form.getType() == DSL::dtype::tdbcplx;
}

/// Collect a vector and/or complex column row by row: each selected row yields its
/// elements, complex values contributing their real then imaginary part.
static std::vector<std::vector<double>> _rowsToElements(
    const std::shared_ptr<const FITStable>& tab,
    const std::string& col,
    const char* caller,
    const RowSet& rows)
{
    const auto& form = *tab->getColumn(col);
    const DSL::dtype t = form.getType();

    std::vector<std::vector<double>> out;
    out.reserve(rows.indices().size());

    // values<T>() is empty when the payload isn't T: indexing it would read out of bounds.
    auto checkRows = [&](const auto& vec)
    {
        if(!rows.empty() && rows.indices().back() >= vec.size())
            throw FITSexception(BAD_TFORM_DTYPE, "QtFITSviewer", caller,
                "Column '"+col+"' does not hold the data its type "+form.getDataType(t)+" announces.");
    };

    auto fillVector = [&](const auto& vec)
    {
        checkRows(vec);
        for(size_t idx : rows.indices())
            out.emplace_back(vec[idx].begin(), vec[idx].end());
    };

    auto fillCplxScalar = [&](const auto& vec)
    {
        checkRows(vec);
        for(size_t idx : rows.indices())
            out.push_back({static_cast<double>(vec[idx].first), static_cast<double>(vec[idx].second)});
    };

    auto fillCplxVector = [&](const auto& vec)
    {
        checkRows(vec);
        for(size_t idx : rows.indices())
        {
            std::vector<double> elems;
            elems.reserve(2 * vec[idx].size());
            for(const auto& c : vec[idx])
            {
                elems.push_back(static_cast<double>(c.first));
                elems.push_back(static_cast<double>(c.second));
            }
            out.push_back(std::move(elems));
        }
    };

    const bool isScalar = (form.getNelem() == 1);

    switch(t)
    {
        case DSL::dtype::tshort:     fillVector(form.values<FITSform::int16Vector>());   break;
        case DSL::dtype::tushort:    fillVector(form.values<FITSform::uint16Vector>());  break;
        case DSL::dtype::tint:
        case DSL::dtype::tlong:      fillVector(form.values<FITSform::int32Vector>());   break;
        case DSL::dtype::tuint:
        case DSL::dtype::tulong:     fillVector(form.values<FITSform::uint32Vector>());  break;
        case DSL::dtype::tlonglong:  fillVector(form.values<FITSform::int64Vector>());   break;
        case DSL::dtype::tulonglong: fillVector(form.values<FITSform::uint64Vector>());  break;
        case DSL::dtype::tfloat:     fillVector(form.values<FITSform::floatVector>());   break;
        case DSL::dtype::tdouble:    fillVector(form.values<FITSform::doubleVector>());  break;
        case DSL::dtype::tcplx:
            if(isScalar) fillCplxScalar(form.values<FITSform::complex>());
            else         fillCplxVector(form.values<FITSform::complexVector>());
            break;
        case DSL::dtype::tdbcplx:
            if(isScalar) fillCplxScalar(form.values<FITSform::dblcomplex>());
            else         fillCplxVector(form.values<FITSform::dblcomplexVector>());
            break;
        default:
            throw FITSexception(BAD_TFORM_DTYPE, "QtFITSviewer", caller,
                "Column '"+col+"': type "+form.getDataType(t)+" is not a supported numeric type.");
    }
    return out;
}

/// Values of the selected rows to histogram: vector and complex columns are
/// flattened, as in the unfiltered overload.
static std::vector<double> _rowsToHistValues(
    const std::shared_ptr<const FITStable>& tab,
    const std::string& col,
    const RowSet& rows)
{
    const auto& form = *tab->getColumn(col);
    if(!_isComplex(form) && form.getNelem() == 1)
        return _colToDoubles(tab, col, "QtHistogram", &rows);

    std::vector<double> data;
    for(const auto& elems : _rowsToElements(tab, col, "QtHistogram", rows))
        data.insert(data.end(), elems.begin(), elems.end());
    return data;
}

/// Collect a ColumnHandle's data as vector<double> (uses ColumnHandle::all<T>().data()).
static std::vector<double> _handleToDoubles(ColumnHandle handle)
{
    std::vector<double> out;
    const DSL::dtype t = handle.colType();

    auto fill = [&](auto view)
    {
        const auto& d = view.data();
        out.reserve(d.size());
        for(const auto& x : d) out.push_back(static_cast<double>(x));
    };

    switch(t)
    {
        case DSL::dtype::tshort:     fill(handle.all<int16_t>());   break;
        case DSL::dtype::tushort:    fill(handle.all<uint16_t>());  break;
        case DSL::dtype::tint:
        case DSL::dtype::tlong:      fill(handle.all<int32_t>());   break;
        case DSL::dtype::tuint:
        case DSL::dtype::tulong:     fill(handle.all<uint32_t>());  break;
        case DSL::dtype::tlonglong:  fill(handle.all<int64_t>());   break;
        case DSL::dtype::tulonglong: fill(handle.all<uint64_t>());  break;
        case DSL::dtype::tfloat:     fill(handle.all<float>());     break;
        case DSL::dtype::tdouble:    fill(handle.all<double>());    break;
        default:
            throw FITSexception(BAD_TFORM_DTYPE, "QtFITSviewer", "QtPlot(ColumnHandle)",
                "Column type is not a supported numeric type for plotting.");
    }
    return out;
}

} // anonymous namespace

// ===========================================================================
// _QtDensityCore — 2-D binned density kernel
// ===========================================================================

std::shared_ptr<QCPColorMap> QtFITSviewer::_QtDensityCore(
    QCustomPlot* qPlot,
    std::vector<double> xs, std::vector<double> ys, std::vector<double> zs,
    size_t nBinsX, size_t nBinsY, uint8_t logFlag)
{
    if(xs.size() != ys.size())
        throw FITSexception(BAD_DIMEN,"QtFITSviewer","_QtDensityCore",
            "xs.size()="+std::to_string(xs.size())+" != ys.size()="+std::to_string(ys.size()));
    if(!zs.empty() && zs.size() != xs.size())
        throw FITSexception(BAD_DIMEN,"QtFITSviewer","_QtDensityCore",
            "zs.size() must equal xs.size() when weights are provided.");
    if(xs.empty())
        throw FITSexception(BAD_ELEM_NUM,"QtFITSviewer","_QtDensityCore","No data points provided.");

    const bool xLog = (logFlag & 0x1) != 0;
    const bool yLog = (logFlag & 0x2) != 0;
    const bool zLog = (logFlag & 0x4) != 0;

    // --- transform and filter invalid points for log axes ---
    std::vector<double> xt, yt, zt;
    xt.reserve(xs.size());
    yt.reserve(xs.size());
    zt.reserve(xs.size());
    size_t skipped = 0;

    for(size_t i = 0; i < xs.size(); ++i)
    {
        double xv = xs[i], yv = ys[i];
        if(xLog) { if(xv <= 0.0) { ++skipped; continue; } xv = std::log10(xv); }
        if(yLog) { if(yv <= 0.0) { ++skipped; continue; } yv = std::log10(yv); }
        xt.push_back(xv);
        yt.push_back(yv);
        zt.push_back(zs.empty() ? 1.0 : zs[i]);
    }

    if(skipped > 0)
        std::cerr << "[FITSwarning] QtFITSviewer::QtDensityPlot: "
                  << skipped << " point(s) skipped (non-positive value on log axis)." << std::endl;
    if(xt.empty())
        throw FITSexception(BAD_ELEM_NUM,"QtFITSviewer","_QtDensityCore",
            "All data points were filtered out (non-positive values on log axis).");

    // --- compute axis ranges ---
    const double xMin = *std::min_element(xt.begin(), xt.end());
    const double xMax = *std::max_element(xt.begin(), xt.end());
    const double yMin = *std::min_element(yt.begin(), yt.end());
    const double yMax = *std::max_element(yt.begin(), yt.end());
    const double xRange = (xMax > xMin) ? (xMax - xMin) : 1.0;
    const double yRange = (yMax > yMin) ? (yMax - yMin) : 1.0;

    // --- accumulate into 2-D bin array ---
    std::vector<std::vector<double>> bins(nBinsX, std::vector<double>(nBinsY, 0.0));
    for(size_t i = 0; i < xt.size(); ++i)
    {
        int xi = static_cast<int>(((xt[i] - xMin) / xRange) * static_cast<double>(nBinsX-1)+0.5);
        int yi = static_cast<int>(((yt[i] - yMin) / yRange) * static_cast<double>(nBinsY-1)+0.5);
        xi = std::max(0, std::min(xi, static_cast<int>(nBinsX) - 1));
        yi = std::max(0, std::min(yi, static_cast<int>(nBinsY) - 1));
        bins[static_cast<size_t>(xi)][static_cast<size_t>(yi)] += zt[i];
    }

    // --- create QCPColorMap (QCustomPlot owns it; use no-op deleter) ---
    auto colorMap = std::shared_ptr<QCPColorMap>(
        new QCPColorMap(qPlot->xAxis, qPlot->yAxis), [](QCPColorMap*){});
    colorMap->data()->setSize(static_cast<int>(nBinsX), static_cast<int>(nBinsY));
    colorMap->data()->setRange(QCPRange(xMin, xMax), QCPRange(yMin, yMax));
    colorMap->data()->fill(0);

    for(size_t ix = 0; ix < nBinsX; ++ix)
        for(size_t iy = 0; iy < nBinsY; ++iy)
        {
            double val = bins[ix][iy];
            if(zLog) val = (val > 0.0) ? std::log10(val) : 0.0;
            colorMap->data()->setCell(static_cast<int>(ix), static_cast<int>(iy), val);
        }

    // --- color scale setup (reuse existing if already in layout) ---
    QCPColorScale* colorScale = nullptr;
    if(!qPlot->plotLayout()->hasElement(0, 1))
    {
        colorScale = new QCPColorScale(qPlot);
        qPlot->plotLayout()->addElement(0, 1, colorScale);
        QCPMarginGroup* marginGroup = new QCPMarginGroup(qPlot);
        const QCP::MarginSides sides = QCP::MarginSides(QCP::msBottom) | QCP::msTop;
        qPlot->axisRect()->setMarginGroup(sides, marginGroup);
        colorScale->setMarginGroup(sides, marginGroup);
    }
    else
    {
        colorScale = static_cast<QCPColorScale*>(qPlot->plotLayout()->element(0, 1));
    }
    colorScale->setGradient(QtFITS::makeViridisGradient());
    colorScale->axis()->setLabel(zLog ? "log10(count)" : "count");
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    colorMap->rescaleDataRange();
    colorMap->setInterpolate(false);

    qPlot->xAxis->setLabel(xLog ? "log10(x)" : "x");
    qPlot->yAxis->setLabel(yLog ? "log10(y)" : "y");

    QMetaObject::invokeMethod(qPlot, [qPlot]() mutable
    {
        {
            std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
            qPlot->rescaleAxes();
            qPlot->replot();
        }
        {
            std::lock_guard<std::mutex> dk(QtFITS::msg_m);
            if(!QtFITS::finished) QtFITS::finished |= true;
            QtFITS::msg_cv.notify_one();
            QtFITS::finished &= false;
        }
    });

    return colorMap;
}

// ===========================================================================
// _QtHistogramCore — 1-D histogram kernel
// ===========================================================================

std::shared_ptr<QCPGraph> QtFITSviewer::_QtHistogramCore(
    QCustomPlot* qPlot,
    std::vector<double> data, size_t nBins,
    uint8_t logFlag, const std::string& OPTION,
    const std::pair<double,double>* range)
{
    if(data.empty())
        throw FITSexception(BAD_ELEM_NUM,"QtFITSviewer","_QtHistogramCore","No data provided.");

    // Auto-select bin count via Sturges rule if nBins == 0
    if(nBins == 0)
        nBins = static_cast<size_t>(std::max(10.0,
            std::ceil(1.0 + std::log2(static_cast<double>(data.size())))));

    const bool xLog = (logFlag & 0x1) != 0;
    const bool yLog = (logFlag & 0x2) != 0;

    // Filter and optionally log-transform values
    std::vector<double> vals;
    vals.reserve(data.size());
    size_t skipped = 0;
    for(double v : data)
    {
        if(xLog)
        {
            if(v <= 0.0) { ++skipped; continue; }
            vals.push_back(std::log10(v));
        }
        else
            vals.push_back(v);
    }
    if(skipped > 0)
        std::cerr << "[FITSwarning] QtFITSviewer::QtHistogram: "
                  << skipped << " value(s) skipped (non-positive on log x-axis)." << std::endl;
    if(vals.empty())
        throw FITSexception(BAD_ELEM_NUM,"QtFITSviewer","_QtHistogramCore",
            "All values were filtered out (non-positive for log x-axis).");

    std::sort(vals.begin(), vals.end());
    const double vMin = range ? range->first  : vals.front();
    const double vMax = range ? range->second : vals.back();
    const double vRange   = (vMax > vMin) ? (vMax - vMin) : 1.0;
    const double binWidth = vRange / static_cast<double>(nBins-1);

    // Bin counts
    std::vector<double> counts(nBins, 0.0);
    for(double v : vals)
    {
        int idx = static_cast<int>((v - vMin) / binWidth+0.5);
        idx = std::max(0, std::min(idx, static_cast<int>(nBins) - 1));
        counts[static_cast<size_t>(idx)] += 1.0;
    }

    // Build a step-curve from bin edges (not centres).
    // For each bin i spanning [leftEdge, rightEdge], emit two points at the same
    // height c_i. Because rightEdge_i == leftEdge_{i+1}, consecutive bins share
    // their boundary x-coordinate at different y values, so lsLine draws the
    // vertical transition automatically — giving a visually continuous histogram.
    //
    // Log X: vals[] is already in log₁₀ space, so edge = vMin + i*binWidth is
    // correct in transformed space; pow(10,·) converts back for axis display.
    //
    // Log Y: y=0 is undefined in log space. Zero-count bins are skipped and a
    // NaN sentinel is inserted to break the polyline there rather than forcing a
    // zero (which would cause a log-axis rendering artefact or crash).
    const double nanVal = std::numeric_limits<double>::quiet_NaN();
    QVector<double> xVec, yVec;
    xVec.reserve(static_cast<int>(nBins) * 2 + 2);
    yVec.reserve(static_cast<int>(nBins) * 2 + 2);

    // Opening baseline at y=0 (omitted for log Y — zero undefined on log axis)
    if (!yLog) {
        xVec.push_back(xLog ? std::pow(10.0, vMin) : vMin);
        yVec.push_back(0.0);
    }

    bool prevNaN = true;
    for (size_t i = 0; i < nBins; ++i)
    {
        const double left  = xLog ? std::pow(10.0, vMin + static_cast<double>(i)     * binWidth)
                                   : vMin + static_cast<double>(i)     * binWidth;
        const double right = xLog ? std::pow(10.0, vMin + static_cast<double>(i + 1) * binWidth)
                                   : vMin + static_cast<double>(i + 1) * binWidth;
        const double c = counts[i];

        if (yLog && c <= 0.0) {
            if (!prevNaN) { xVec.push_back(nanVal); yVec.push_back(nanVal); prevNaN = true; }
            continue;
        }
        xVec.push_back(left);   yVec.push_back(c);
        xVec.push_back(right);  yVec.push_back(c);
        prevNaN = false;
    }

    // Closing baseline at y=0 (linear Y only)
    if (!yLog && !xVec.empty()) {
        xVec.push_back(xVec.back());
        yVec.push_back(0.0);
    }

    // QCustomPlot owns the QCPGraph; use a no-op deleter.
    auto graph = std::shared_ptr<QCPGraph>(
        new QCPGraph(qPlot->xAxis, qPlot->yAxis), [](QCPGraph*){});
    // alreadySorted=true: data is in ascending x order by construction.
    // Without this, setData() re-sorts and a non-stable sort can swap the two
    // equal-x points at each bin boundary, converting vertical steps into diagonals.
    graph->setData(xVec, yVec, true);
    graph->setLineStyle(QCPGraph::lsLine);

    if(xLog)
        qPlot->xAxis->setScaleType(QCPAxis::stLogarithmic);
    if(yLog)
        qPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);

    qPlot->xAxis->setLabel("value");
    qPlot->yAxis->setLabel("count");

    applyPlotOptions(graph.get(), OPTION);

    QMetaObject::invokeMethod(qPlot, [qPlot]() mutable
    {
        {
            std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
            qPlot->rescaleAxes();
            qPlot->replot();
        }
        {
            std::lock_guard<std::mutex> dk(QtFITS::msg_m);
            if(!QtFITS::finished) QtFITS::finished |= true;
            QtFITS::msg_cv.notify_one();
            QtFITS::finished &= false;
        }
    });

    return graph;
}

// ===========================================================================
// QtDensityPlot — FITStable overloads
// ===========================================================================

std::shared_ptr<QCPColorMap> QtFITSviewer::QtDensityPlot(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& xCol, const std::string& yCol,
    const std::string& zCol,
    size_t nBinsX, size_t nBinsY,
    uint8_t logFlag)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtDensityPlot","Null FITStable pointer.");
    try { tab->getColumn(xCol); tab->getColumn(yCol); }
    catch(...) { throw; }
    if(!zCol.empty())
        try { tab->getColumn(zCol); }
        catch(...) { throw; }

    // Only scalar numeric columns are accepted for the (x,y,[z]) form
    for(const std::string& c : {xCol, yCol})
    {
        const auto& form = *tab->getColumn(c);
        const DSL::dtype t = form.getType();
        if(t == DSL::dtype::tstring  || t == DSL::dtype::tlogical ||
           t == DSL::dtype::tbit     || t == DSL::dtype::tsbyte   || t == DSL::dtype::tbyte ||
           t == DSL::dtype::tcplx    || t == DSL::dtype::tdbcplx)
            throw FITSexception(BAD_TFORM_DTYPE,"QtFITSviewer","QtDensityPlot",
                "Column '"+c+"': type "+form.getDataType(t)+" is not supported for density plot.");
        if(form.getNelem() != 1)
            throw FITSexception(BAD_TFORM_DTYPE,"QtFITSviewer","QtDensityPlot",
                "Column '"+c+"' has "+std::to_string(form.getNelem())+
                " elements per row; QtDensityPlot (x,y form) requires scalar columns."
                " Use the single-column overload for complex or 2-vector columns.");
    }

    auto xs = _colToDoubles(tab, xCol, "QtDensityPlot");
    auto ys = _colToDoubles(tab, yCol, "QtDensityPlot");
    std::vector<double> zs;
    if(!zCol.empty()) zs = _colToDoubles(tab, zCol, "QtDensityPlot");

    return _QtDensityCore(qPlot, std::move(xs), std::move(ys), std::move(zs), nBinsX, nBinsY, logFlag);
}

std::shared_ptr<QCPColorMap> QtFITSviewer::QtDensityPlot(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& col,
    size_t nBinsX, size_t nBinsY,
    uint8_t logFlag)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtDensityPlot","Null FITStable pointer.");
    try { tab->getColumn(col); }
    catch(...) { throw; }

    const auto& form = *tab->getColumn(col);
    const DSL::dtype t = form.getType();
    const int64_t nElem = form.getNelem();

    std::vector<double> xs, ys;
    xs.reserve(tab->nrows());
    ys.reserve(tab->nrows());

    if(t == DSL::dtype::tcplx)
    {
        // complex<float>: real → x, imag → y
        _collectCplxXY<float>(tab, col, xs, ys);
    }
    else if(t == DSL::dtype::tdbcplx)
    {
        // complex<double>: real → x, imag → y
        _collectCplxXY<double>(tab, col, xs, ys);
    }
    else if(nElem == 2)
    {
        // 2-element vector column: elem[0] → x, elem[1] → y
        switch(t)
        {
            case DSL::dtype::tshort:     _collectVec2XY<int16_t>(tab, col, xs, ys);   break;
            case DSL::dtype::tushort:    _collectVec2XY<uint16_t>(tab, col, xs, ys);  break;
            case DSL::dtype::tint:
            case DSL::dtype::tlong:      _collectVec2XY<int32_t>(tab, col, xs, ys);   break;
            case DSL::dtype::tuint:
            case DSL::dtype::tulong:     _collectVec2XY<uint32_t>(tab, col, xs, ys);  break;
            case DSL::dtype::tlonglong:  _collectVec2XY<int64_t>(tab, col, xs, ys);   break;
            case DSL::dtype::tulonglong: _collectVec2XY<uint64_t>(tab, col, xs, ys);  break;
            case DSL::dtype::tfloat:     _collectVec2XY<float>(tab, col, xs, ys);     break;
            case DSL::dtype::tdouble:    _collectVec2XY<double>(tab, col, xs, ys);    break;
            default:
                throw FITSexception(BAD_TFORM_DTYPE,"QtFITSviewer","QtDensityPlot",
                    "Column '"+col+"': type "+form.getDataType(t)+
                    " is not supported for the single-column density plot overload.");
        }
    }
    else
    {
        throw FITSexception(BAD_TFORM_DTYPE,"QtFITSviewer","QtDensityPlot",
            "Single-column QtDensityPlot requires a complex column (TFORM 'C' or 'M') or "
            "a 2-element vector column. Column '"+col+"' has TFORM '"+
            form.getDataType(t)+"' with "+std::to_string(nElem)+" element(s) per row.");
    }

    std::vector<double> zs;
    return _QtDensityCore(qPlot, std::move(xs), std::move(ys), zs, nBinsX, nBinsY, logFlag);
}

std::shared_ptr<QCPColorMap> QtFITSviewer::QtDensityPlot(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& xCol, const std::string& yCol,
    const RowSet& rows,
    const std::string& zCol,
    size_t nBinsX, size_t nBinsY,
    uint8_t logFlag)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtDensityPlot","Null FITStable pointer.");

    auto xs = _colToDoubles(tab, xCol, "QtDensityPlot", &rows);
    auto ys = _colToDoubles(tab, yCol, "QtDensityPlot", &rows);
    std::vector<double> zs;
    if(!zCol.empty()) zs = _colToDoubles(tab, zCol, "QtDensityPlot", &rows);

    return _QtDensityCore(qPlot, std::move(xs), std::move(ys), std::move(zs), nBinsX, nBinsY, logFlag);
}

std::shared_ptr<QCPColorMap> QtFITSviewer::QtDensityPlot(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& col,
    const RowSet& rows,
    size_t nBinsX, size_t nBinsY,
    uint8_t logFlag)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtDensityPlot","Null FITStable pointer.");

    const auto& form = *tab->getColumn(col);
    const int64_t nElem = form.getNelem();
    if(_isComplex(form) ? nElem != 1 : nElem != 2)
        throw FITSexception(BAD_TFORM_DTYPE,"QtFITSviewer","QtDensityPlot",
            "Single-column QtDensityPlot requires a complex column (TFORM 'C' or 'M') or "
            "a 2-element vector column. Column '"+col+"' has TFORM '"+
            form.getDataType(form.getType())+"' with "+std::to_string(nElem)+" element(s) per row.");

    // Real/imaginary parts or elem[0]/elem[1]: two values per row either way.
    std::vector<double> xs, ys, zs;
    xs.reserve(rows.size());
    ys.reserve(rows.size());
    for(const auto& elems : _rowsToElements(tab, col, "QtDensityPlot", rows))
    {
        xs.push_back(elems.at(0));
        ys.push_back(elems.at(1));
    }

    return _QtDensityCore(qPlot, std::move(xs), std::move(ys), std::move(zs), nBinsX, nBinsY, logFlag);
}

std::shared_ptr<QCPColorMap> QtFITSviewer::QtDensityPlot(
    QCustomPlot* qPlot,
    ColumnHandle xHandle, ColumnHandle yHandle,
    size_t nBinsX, size_t nBinsY,
    uint8_t logFlag)
{
    auto xs = _handleToDoubles(xHandle);
    auto ys = _handleToDoubles(yHandle);
    std::vector<double> zs;
    return _QtDensityCore(qPlot, std::move(xs), std::move(ys), zs, nBinsX, nBinsY, logFlag);
}

// ===========================================================================
// QtHistogram — FITStable overloads
// ===========================================================================

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtHistogram(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& col,
    size_t nBins,
    std::string OPTION)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtHistogram","Null FITStable pointer.");
    try { tab->getColumn(col); }
    catch(...) { throw; }

    const auto& form = *tab->getColumn(col);
    const DSL::dtype t  = form.getType();
    const int64_t nElem = form.getNelem();

    if(t == DSL::dtype::tstring  || t == DSL::dtype::tlogical ||
       t == DSL::dtype::tbit     || t == DSL::dtype::tsbyte   || t == DSL::dtype::tbyte)
        throw FITSexception(BAD_TFORM_DTYPE,"QtFITSviewer","QtHistogram",
            "Column '"+col+"': type "+form.getDataType(t)+" is not supported for histogram.");

    std::vector<double> data;
    data.reserve(tab->nrows() * (nElem > 0 ? static_cast<size_t>(nElem) : 2));

    if(t == DSL::dtype::tcplx)
        _collectHistCplx<float>(tab, col, data);
    else if(t == DSL::dtype::tdbcplx)
        _collectHistCplx<double>(tab, col, data);
    else if(nElem > 1)
    {
        switch(t)
        {
            case DSL::dtype::tshort:     _collectHistVector<int16_t>(tab, col, data);   break;
            case DSL::dtype::tushort:    _collectHistVector<uint16_t>(tab, col, data);  break;
            case DSL::dtype::tint:
            case DSL::dtype::tlong:      _collectHistVector<int32_t>(tab, col, data);   break;
            case DSL::dtype::tuint:
            case DSL::dtype::tulong:     _collectHistVector<uint32_t>(tab, col, data);  break;
            case DSL::dtype::tlonglong:  _collectHistVector<int64_t>(tab, col, data);   break;
            case DSL::dtype::tulonglong: _collectHistVector<uint64_t>(tab, col, data);  break;
            case DSL::dtype::tfloat:     _collectHistVector<float>(tab, col, data);     break;
            case DSL::dtype::tdouble:    _collectHistVector<double>(tab, col, data);    break;
            default: break;
        }
    }
    else
    {
        switch(t)
        {
            case DSL::dtype::tshort:     _collectHistScalar<int16_t>(tab, col, data);   break;
            case DSL::dtype::tushort:    _collectHistScalar<uint16_t>(tab, col, data);  break;
            case DSL::dtype::tint:
            case DSL::dtype::tlong:      _collectHistScalar<int32_t>(tab, col, data);   break;
            case DSL::dtype::tuint:
            case DSL::dtype::tulong:     _collectHistScalar<uint32_t>(tab, col, data);  break;
            case DSL::dtype::tlonglong:  _collectHistScalar<int64_t>(tab, col, data);   break;
            case DSL::dtype::tulonglong: _collectHistScalar<uint64_t>(tab, col, data);  break;
            case DSL::dtype::tfloat:     _collectHistScalar<float>(tab, col, data);     break;
            case DSL::dtype::tdouble:    _collectHistScalar<double>(tab, col, data);    break;
            default: break;
        }
    }

    return {_QtHistogramCore(qPlot, std::move(data), nBins, 0, OPTION)};
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtHistogram(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& col,
    const RowSet& rows,
    size_t nBins,
    std::string OPTION)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtHistogram","Null FITStable pointer.");

    return {_QtHistogramCore(qPlot, _rowsToHistValues(tab, col, rows), nBins, 0, OPTION)};
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtHistogram(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& col,
    const std::vector<RowSet>& groups,
    size_t nBins,
    std::string OPTION)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtHistogram","Null FITStable pointer.");

    std::vector<std::vector<double>> data;
    data.reserve(groups.size());
    double vMin = std::numeric_limits<double>::infinity();
    double vMax = -vMin;
    size_t nValues = 0;

    for(const RowSet& rows : groups)
    {
        data.push_back(_rowsToHistValues(tab, col, rows));
        if(data.back().empty())
            throw FITSexception(BAD_ELEM_NUM,"QtFITSviewer","QtHistogram","A group of rows is empty.");

        nValues += data.back().size();
        for(double v : data.back())
        {
            if(std::isfinite(v))
            {
                vMin = std::min(vMin, v);
                vMax = std::max(vMax, v);
            }
        }
    }
    if(vMin > vMax)
        throw FITSexception(BAD_ELEM_NUM,"QtFITSviewer","QtHistogram","Column '"+col+"' has no finite value in these rows.");

    // Resolved here: _QtHistogramCore would otherwise pick a bin count per group.
    if(nBins == 0)
        nBins = static_cast<size_t>(std::max(10.0,
            std::ceil(1.0 + std::log2(static_cast<double>(nValues)))));

    const std::pair<double,double> range(vMin, vMax);

    std::vector<std::shared_ptr<QCPGraph>> graphs;
    graphs.reserve(data.size());
    for(auto& values : data)
        graphs.push_back(_QtHistogramCore(qPlot, std::move(values), nBins, 0, OPTION, &range));
    return graphs;
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtHistogram(
    QCustomPlot* qPlot,
    ColumnHandle handle,
    size_t nBins,
    std::string OPTION)
{
    auto data = _handleToDoubles(handle);
    return {_QtHistogramCore(qPlot, std::move(data), nBins, 0, OPTION)};
}

// ===========================================================================
// QtCurve / QtScatter — RowSet overloads
// ===========================================================================

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtCurve(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& yCol, const RowSet& rows)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtCurve","Null FITStable pointer.");

    const auto& form = *tab->getColumn(yCol);
    const bool isComplex = _isComplex(form);
    const bool isScalar  = (form.getNelem() == 1);

    if(!isComplex && isScalar)
    {
        // x keeps the table's 1-based row number, as in the unfiltered overload,
        // so a selected row stays where it is on the axis.
        std::vector<double> xs;
        xs.reserve(rows.size());
        for(size_t idx : rows.indices())
            xs.push_back(static_cast<double>(idx + 1));
        auto ys = _colToDoubles(tab, yCol, "QtCurve", &rows);
        return QtCurve(qPlot, std::move(xs), std::move(ys));
    }

    const auto elems = _rowsToElements(tab, yCol, "QtCurve", rows);

    if(isComplex && isScalar)
    {
        // Argand diagram: real part on x, imaginary part on y.
        std::vector<double> xs, ys;
        xs.reserve(elems.size());
        ys.reserve(elems.size());
        for(const auto& e : elems)
        {
            xs.push_back(e.at(0));
            ys.push_back(e.at(1));
        }
        return QtCurve(qPlot, std::move(xs), std::move(ys));
    }

    // Vector column: one graph per row.
    std::vector<std::shared_ptr<QCPGraph>> curves;
    curves.reserve(elems.size());
    for(size_t i = 0; i < elems.size(); ++i)
    {
        std::vector<double> xs, ys;
        if(isComplex)
        {
            for(size_t j = 0; j + 1 < elems[i].size(); j += 2)
            {
                xs.push_back(elems[i][j]);
                ys.push_back(elems[i][j + 1]);
            }
        }
        else
        {
            ys = elems[i];
            xs.resize(ys.size());
            std::iota(xs.begin(), xs.end(), 0.0);
        }
        auto newCurves = QtCurve(qPlot, std::move(xs), std::move(ys));
        newCurves[0]->setName(QString::fromStdString("Row #"+std::to_string(rows.indices()[i])));
        curves.push_back(newCurves[0]);
    }
    return curves;
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtCurve(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& xCol, const std::string& yCol, const RowSet& rows)
{
    if(!tab)
        throw FITSexception(SEEK_ERROR,"QtFITSviewer","QtCurve","Null FITStable pointer.");

    const auto& xForm = *tab->getColumn(xCol);
    const auto& yForm = *tab->getColumn(yCol);

    if(xForm.getNelem() != yForm.getNelem())
        throw FITSexception(BAD_DIMEN,"QtFITSviewer","QtCurve","Columns "+xCol+" and "+yCol+" have different number of elements ("+std::to_string(xForm.getNelem())+" vs "+std::to_string(yForm.getNelem())+").");

    if(_isComplex(xForm) || _isComplex(yForm))
        throw FITSexception(BAD_TFORM_DTYPE,"QtFITSviewer","QtCurve",xForm.getDataType(xForm.getType())+" combined to "+yForm.getDataType(yForm.getType())+"  is not supported for plotting.");

    if(xForm.getNelem() == 1)
    {
        auto xs = _colToDoubles(tab, xCol, "QtCurve", &rows);
        auto ys = _colToDoubles(tab, yCol, "QtCurve", &rows);
        return QtCurve(qPlot, std::move(xs), std::move(ys));
    }

    // Vector columns: one graph per row.
    auto xElems = _rowsToElements(tab, xCol, "QtCurve", rows);
    auto yElems = _rowsToElements(tab, yCol, "QtCurve", rows);

    std::vector<std::shared_ptr<QCPGraph>> curves;
    curves.reserve(xElems.size());
    for(size_t i = 0; i < xElems.size(); ++i)
    {
        auto newCurves = QtCurve(qPlot, std::move(xElems[i]), std::move(yElems[i]));
        newCurves[0]->setName(QString::fromStdString("Row #"+std::to_string(rows.indices()[i])));
        curves.push_back(newCurves[0]);
    }
    return curves;
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtScatter(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& yCol, const RowSet& rows)
{
    auto g = QtCurve(qPlot, tab, yCol, rows);
    for(auto& p : g) p->setLineStyle(QCPGraph::lsNone);
    return g;
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtScatter(
    QCustomPlot* qPlot,
    const std::shared_ptr<const FITStable>& tab,
    const std::string& xCol, const std::string& yCol, const RowSet& rows)
{
    auto g = QtCurve(qPlot, tab, xCol, yCol, rows);
    for(auto& p : g) p->setLineStyle(QCPGraph::lsNone);
    return g;
}

// ===========================================================================
// QtCurve / QtScatter — ColumnHandle overloads
// ===========================================================================

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtCurve(
    QCustomPlot* qPlot, ColumnHandle yHandle)
{
    auto ys = _handleToDoubles(yHandle);
    return QtCurve(qPlot, std::move(ys));
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtCurve(
    QCustomPlot* qPlot, ColumnHandle xHandle, ColumnHandle yHandle)
{
    auto xs = _handleToDoubles(xHandle);
    auto ys = _handleToDoubles(yHandle);
    return QtCurve(qPlot, std::move(xs), std::move(ys));
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtScatter(
    QCustomPlot* qPlot, ColumnHandle yHandle)
{
    auto g = QtCurve(qPlot, yHandle);
    for(auto& p : g) p->setLineStyle(QCPGraph::lsNone);
    return g;
}

std::vector<std::shared_ptr<QCPGraph>> QtFITSviewer::QtScatter(
    QCustomPlot* qPlot, ColumnHandle xHandle, ColumnHandle yHandle)
{
    auto g = QtCurve(qPlot, xHandle, yHandle);
    for(auto& p : g) p->setLineStyle(QCPGraph::lsNone);
    return g;
}

// ===========================================================================
// QCPColorMapRGB — implementation
// ===========================================================================

QCPColorGradient QCPColorMapRGB::makeChannelGradient(int ch)
{
    QCPColorGradient g;
    g.clearColorStops();
    switch(ch)
    {
        case 0:  g.setColorStops({{0.0, QColor(0,0,0)},   {1.0, QColor(255,0,0)}});   break; // red
        case 1:  g.setColorStops({{0.0, QColor(0,0,0)},   {1.0, QColor(0,255,0)}});   break; // green
        case 2:  g.setColorStops({{0.0, QColor(0,0,0)},   {1.0, QColor(0,0,255)}});   break; // blue
        default: g.setColorStops({{0.0, QColor(0,0,0)},   {1.0, QColor(255,255,255)}});break; // white (K)
    }
    return g;
}

QCPColorMapRGB::QCPColorMapRGB(QCPAxis *keyAxis, QCPAxis *valueAxis)
    : QCPAbstractPlottable(keyAxis, valueAxis)
{
    for(int ch = 0; ch < kMaxChannels; ++ch)
    {
        mChannelGradient[ch]  = makeChannelGradient(ch);
        mChannelIntRange[ch]  = QCPRange(0.0, 1.0);
        mChannelScaleType[ch] = QCPAxis::stLinear;
    }
}

QCPColorMapRGB::~QCPColorMapRGB() = default;

void QCPColorMapRGB::setChannelData(int ch, std::vector<double> data,
                                    int nx, int ny,
                                    const QCPRange &keyRange,
                                    const QCPRange &valueRange)
{
    if(ch < 0 || ch >= kMaxChannels) return;
    mChannelRaw[ch] = std::move(data);
    mNx = nx;
    mNy = ny;
    mKeyRange   = keyRange;
    mValueRange = valueRange;
    mInvalidated = true;
}

void QCPColorMapRGB::setChannelRange(int ch, const QCPRange &intensityRange)
{
    if(ch < 0 || ch >= kMaxChannels) return;
    mChannelIntRange[ch] = intensityRange;
    mInvalidated = true;
}

void QCPColorMapRGB::rescaleChannelRange(int ch)
{
    if(ch < 0 || ch >= kMaxChannels) return;
    if(mChannelRaw[ch].empty()) return;
    const auto [mn, mx] = std::minmax_element(mChannelRaw[ch].begin(), mChannelRaw[ch].end());
    mChannelIntRange[ch] = QCPRange(*mn, *mx);
    mInvalidated = true;
}

void QCPColorMapRGB::setChannelGradient(int ch, const QCPColorGradient &gradient)
{
    if(ch < 0 || ch >= kMaxChannels) return;
    mChannelGradient[ch] = gradient;
    mInvalidated = true;
}

void QCPColorMapRGB::setChannelScaleType(int ch, QCPAxis::ScaleType type)
{
    if(ch < 0 || ch >= kMaxChannels) return;
    mChannelScaleType[ch] = type;
    mInvalidated = true;
}

void QCPColorMapRGB::setChannelCount(int n)
{
    mChannelCount = std::max(1, std::min(n, kMaxChannels));
    mInvalidated = true;
}

void QCPColorMapRGB::setCMYKMode(bool enabled)
{
    mCMYKMode    = enabled;
    mInvalidated = true;
}

void QCPColorMapRGB::setInterpolate(bool enabled)
{
    mInterpolate = enabled;
    mInvalidated = true;
}

void QCPColorMapRGB::invalidate()
{
    mInvalidated = true;
}

const QImage& QCPColorMapRGB::composedImage()
{
    if(mInvalidated) recompose();
    return mComposedImage;
}

double QCPColorMapRGB::selectTest(const QPointF &pos, bool onlySelectable,
                                   QVariant *details) const
{
    Q_UNUSED(details)
    if(onlySelectable && !mSelectable) return -1;
    if(mNx <= 0 || mNy <= 0) return -1;
    const QRectF rect = QRectF(coordsToPixels(mKeyRange.lower, mValueRange.lower),
                               coordsToPixels(mKeyRange.upper, mValueRange.upper)).normalized();
    return rect.contains(pos) ? 0.0 : -1.0;
}

QCPRange QCPColorMapRGB::getKeyRange(bool &foundRange, QCP::SignDomain inSignDomain) const
{
    Q_UNUSED(inSignDomain)
    foundRange = (mNx > 0);
    return mKeyRange;
}

QCPRange QCPColorMapRGB::getValueRange(bool &foundRange, QCP::SignDomain inSignDomain,
                                        const QCPRange &inKeyRange) const
{
    Q_UNUSED(inSignDomain)
    Q_UNUSED(inKeyRange)
    foundRange = (mNy > 0);
    return mValueRange;
}

void QCPColorMapRGB::recompose()
{
    if(mNx <= 0 || mNy <= 0) { mInvalidated = false; return; }

    QImage base(mNx, mNy, QImage::Format_ARGB32_Premultiplied);
    base.fill(qRgba(0, 0, 0, 255));

    if(mCMYKMode && mChannelCount >= 4)
    {
        // CMYK → RGB per pixel; no gradient used
        for(int y = 0; y < mNy; ++y)
        {
            QRgb *row = reinterpret_cast<QRgb*>(base.scanLine(mNy - 1 - y));
            for(int x = 0; x < mNx; ++x)
            {
                const size_t idx = static_cast<size_t>(x + y * mNx);
                auto norm = [&](int ch) -> double {
                    if(static_cast<int>(mChannelRaw[ch].size()) <= static_cast<int>(idx)) return 0.0;
                    const double rng = mChannelIntRange[ch].size();
                    if(rng <= 0.0) return 0.0;
                    return std::max(0.0, std::min(1.0,
                        (mChannelRaw[ch][idx] - mChannelIntRange[ch].lower) / rng));
                };
                const double C = norm(0), M = norm(1), Y = norm(2), K = norm(3);
                row[x] = qRgb(
                    std::max(0, std::min(255, static_cast<int>((1.0-C)*(1.0-K)*255.0 + 0.5))),
                    std::max(0, std::min(255, static_cast<int>((1.0-M)*(1.0-K)*255.0 + 0.5))),
                    std::max(0, std::min(255, static_cast<int>((1.0-Y)*(1.0-K)*255.0 + 0.5)))
                );
            }
        }
    }
    else
    {
        // RGB: colorize each channel separately, then additively blend
        QPainter painter(&base);
        painter.setCompositionMode(QPainter::CompositionMode_Plus);

        const int nCh = std::min(mChannelCount, 3);
        for(int ch = 0; ch < nCh; ++ch)
        {
            if(mChannelRaw[ch].empty()) continue;

            QImage chImg(mNx, mNy, QImage::Format_ARGB32_Premultiplied);
            chImg.fill(qRgba(0, 0, 0, 255));

            const bool isLog = (mChannelScaleType[ch] == QCPAxis::stLogarithmic);
            for(int line = 0; line < mNy; ++line)
            {
                QRgb *pixels = reinterpret_cast<QRgb*>(chImg.scanLine(mNy - 1 - line));
                mChannelGradient[ch].colorize(
                    mChannelRaw[ch].data() + static_cast<size_t>(line) * static_cast<size_t>(mNx),
                    mChannelIntRange[ch],
                    pixels,
                    mNx,
                    1,
                    isLog
                );
            }
            painter.drawImage(0, 0, chImg);
        }
    }

    mComposedImage = base;
    mLegendIcon    = QPixmap::fromImage(base.scaled(16, 16, Qt::IgnoreAspectRatio,
                                                    Qt::FastTransformation));
    mInvalidated   = false;
}

void QCPColorMapRGB::draw(QCPPainter *painter)
{
    if(mNx <= 0 || mNy <= 0) return;
    if(!mKeyAxis || !mValueAxis) return;
    applyDefaultAntialiasingHint(painter);

    if(mInvalidated) recompose();
    if(mComposedImage.isNull()) return;

    QRectF imageRect = QRectF(
        coordsToPixels(mKeyRange.lower,  mValueRange.lower),
        coordsToPixels(mKeyRange.upper,  mValueRange.upper)
    ).normalized();

    // Expand by half a cell so pixels are centred on their coordinate (matches QCPColorMap)
    double halfCellW = 0.0, halfCellH = 0.0;
    if(keyAxis()->orientation() == Qt::Horizontal)
    {
        if(mNx > 1) halfCellW = 0.5 * imageRect.width()  / double(mNx - 1);
        if(mNy > 1) halfCellH = 0.5 * imageRect.height() / double(mNy - 1);
    }
    else
    {
        if(mNx > 1) halfCellH = 0.5 * imageRect.height() / double(mNx - 1);
        if(mNy > 1) halfCellW = 0.5 * imageRect.width()  / double(mNy - 1);
    }
    imageRect.adjust(-halfCellW, -halfCellH, halfCellW, halfCellH);

    const bool mirrorX = (keyAxis()->orientation() == Qt::Horizontal
                          ? keyAxis() : valueAxis())->rangeReversed();
    const bool mirrorY = (valueAxis()->orientation() == Qt::Vertical
                          ? valueAxis() : keyAxis())->rangeReversed();

    const bool smoothBackup = painter->renderHints().testFlag(QPainter::SmoothPixmapTransform);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, mInterpolate);
    painter->drawImage(imageRect, mComposedImage.mirrored(mirrorX, mirrorY));
    painter->setRenderHint(QPainter::SmoothPixmapTransform, smoothBackup);
}

void QCPColorMapRGB::drawLegendIcon(QCPPainter *painter, const QRectF &rect) const
{
    applyDefaultAntialiasingHint(painter);
    if(!mLegendIcon.isNull())
    {
        QPixmap scaled = mLegendIcon.scaled(rect.size().toSize(),
                                            Qt::IgnoreAspectRatio,
                                            Qt::FastTransformation);
        QRectF iconRect(0, 0, scaled.width(), scaled.height());
        iconRect.moveCenter(rect.center());
        painter->drawPixmap(iconRect.topLeft(), scaled);
    }
}

// ===========================================================================
// OverlayRGB / QtDisplayRGB — FITS→QCPColorMapRGB pipeline
// ===========================================================================

// Internal helper: fill one channel of a QCPColorMapRGB from a 2-D FITScube.
// iMin / iMax select the pixel range within img (for 3-D cubes, pass the layer slice).
static void _fillRGBChannel(QCPColorMapRGB *rgbMap, int ch,
                             const std::shared_ptr<const FITScube> &img,
                             size_t iMin, size_t iMax,
                             QtFITS::zScale scale, int nx, int ny,
                             const QCPRange &keyRange, const QCPRange &valueRange)
{
    std::vector<double> raw(static_cast<size_t>(nx) * static_cast<size_t>(ny), 0.0);
    const std::valarray<bool> mask = img->GetMask();

    double dataMin =  std::numeric_limits<double>::max();
    double dataMax = -std::numeric_limits<double>::max();

    for(size_t ik = iMin; ik < iMax; ++ik)
    {
        if(mask[ik]) continue;
        double v = img->at(ik);
        switch(scale)
        {
            case QtFITS::fLOG:  v = (v > 0.0) ? std::log10(v) : 0.0; break;
            case QtFITS::fQUAD: v = std::pow(v, 2.0);                  break;
            case QtFITS::fSQRT: v = (v >= 0.0) ? std::sqrt(v) : 0.0;  break;
            case QtFITS::fSINH: v = std::sinh(v);                      break;
            default: break;
        }
        const auto coords = img->PixelCoordinates(ik);
        const size_t idx  = static_cast<size_t>(coords[0])
                          + static_cast<size_t>(coords[1]) * static_cast<size_t>(nx);
        raw[idx] = v;
        dataMin  = std::min(dataMin, v);
        dataMax  = std::max(dataMax, v);
    }

    rgbMap->setChannelData(ch, std::move(raw), nx, ny, keyRange, valueRange);
    if(dataMin < dataMax)
        rgbMap->setChannelRange(ch, QCPRange(dataMin, dataMax));
    else
        rgbMap->setChannelRange(ch, QCPRange(dataMin, dataMin + 1.0));
}

void QtFITSviewer::OverlayRGB(QCPAxisRect *qPlot,
                               const std::shared_ptr<const FITScube> &red,
                               const std::shared_ptr<const FITScube> &green,
                               const std::shared_ptr<const FITScube> &blue,
                               std::string OPTION)
{
    std::transform(OPTION.begin(), OPTION.end(), OPTION.begin(), ::tolower);

    QtFITS::zScale scale = QtFITS::fLIN;
    if     (OPTION.find("logz",0)  != std::string::npos) scale = QtFITS::fLOG;
    else if(OPTION.find("sinhz",0) != std::string::npos) scale = QtFITS::fSINH;
    else if(OPTION.find("quadz",0) != std::string::npos) scale = QtFITS::fQUAD;
    else if(OPTION.find("sqrtz",0) != std::string::npos) scale = QtFITS::fSQRT;

    // All channels must agree on spatial dimensions
    const int nx = static_cast<int>(red->Size(1));
    const int ny = static_cast<int>(red->Size(2));
    const QCPRange keyRange  (0, nx - 1);
    const QCPRange valueRange(0, ny - 1);

    QCPColorMapRGB *rgbMap = new QCPColorMapRGB(
        qPlot->axis(QCPAxis::atBottom),
        qPlot->axis(QCPAxis::atLeft));

    const std::shared_ptr<const FITScube>* cubes[3] = { &red, &green, &blue };
    for(int ch = 0; ch < 3; ++ch)
        _fillRGBChannel(rgbMap, ch, *cubes[ch],
                        0, (*cubes[ch])->Nelements(),
                        scale, nx, ny, keyRange, valueRange);

    QMetaObject::invokeMethod(qPlot->parentPlot(), [qPlot]() mutable
    {
        {
            std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
            qPlot->parentPlot()->rescaleAxes();
            qPlot->parentPlot()->yAxis->setScaleRatio(qPlot->parentPlot()->xAxis, 1.0);
            qPlot->parentPlot()->replot();
        }
        {
            std::lock_guard<std::mutex> dk(QtFITS::msg_m);
            if(!QtFITS::finished) QtFITS::finished |= true;
            QtFITS::msg_cv.notify_one();
            QtFITS::finished &= false;
        }
    });
}

void QtFITSviewer::OverlayRGB(QCPAxisRect *qPlot,
                               const std::shared_ptr<const FITScube> &cube,
                               std::string OPTION)
{
    if(cube->GetDimension() < 3 || cube->Size(3) < 3)
        throw FITSexception(BAD_DIMEN, "QtFITSviewer", "OverlayRGB",
            "Input cube must have NAXIS=3 and at least 3 layers (Naxis[2]>=3).");

    std::transform(OPTION.begin(), OPTION.end(), OPTION.begin(), ::tolower);

    QtFITS::zScale scale = QtFITS::fLIN;
    if     (OPTION.find("logz",0)  != std::string::npos) scale = QtFITS::fLOG;
    else if(OPTION.find("sinhz",0) != std::string::npos) scale = QtFITS::fSINH;
    else if(OPTION.find("quadz",0) != std::string::npos) scale = QtFITS::fQUAD;
    else if(OPTION.find("sqrtz",0) != std::string::npos) scale = QtFITS::fSQRT;

    const int nx = static_cast<int>(cube->Size(1));
    const int ny = static_cast<int>(cube->Size(2));
    const QCPRange keyRange  (0, nx - 1);
    const QCPRange valueRange(0, ny - 1);
    const size_t layerStride = static_cast<size_t>(nx) * static_cast<size_t>(ny);

    QCPColorMapRGB *rgbMap = new QCPColorMapRGB(
        qPlot->axis(QCPAxis::atBottom),
        qPlot->axis(QCPAxis::atLeft));

    for(int ch = 0; ch < 3; ++ch)
    {
        const size_t iMin = static_cast<size_t>(ch) * layerStride;
        const size_t iMax = iMin + layerStride;
        _fillRGBChannel(rgbMap, ch, cube, iMin, iMax,
                        scale, nx, ny, keyRange, valueRange);
    }

    QMetaObject::invokeMethod(qPlot->parentPlot(), [qPlot]() mutable
    {
        {
            std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
            qPlot->parentPlot()->rescaleAxes();
            qPlot->parentPlot()->yAxis->setScaleRatio(qPlot->parentPlot()->xAxis, 1.0);
            qPlot->parentPlot()->replot();
        }
        {
            std::lock_guard<std::mutex> dk(QtFITS::msg_m);
            if(!QtFITS::finished) QtFITS::finished |= true;
            QtFITS::msg_cv.notify_one();
            QtFITS::finished &= false;
        }
    });
}

QCPColorMapRGB* QtFITSviewer::OverlayChannels(QCPAxisRect *qPlot,
                               const std::vector<std::shared_ptr<const FITScube>> &channels,
                               bool cmyk,
                               std::string OPTION)
{
    if(channels.empty()) return nullptr;

    const int nChannels = std::min(static_cast<int>(channels.size()),
                                   QCPColorMapRGB::kMaxChannels);

    std::transform(OPTION.begin(), OPTION.end(), OPTION.begin(), ::tolower);

    QtFITS::zScale scale = QtFITS::fLIN;
    if     (OPTION.find("logz",0)  != std::string::npos) scale = QtFITS::fLOG;
    else if(OPTION.find("sinhz",0) != std::string::npos) scale = QtFITS::fSINH;
    else if(OPTION.find("quadz",0) != std::string::npos) scale = QtFITS::fQUAD;
    else if(OPTION.find("sqrtz",0) != std::string::npos) scale = QtFITS::fSQRT;

    // All channels must agree on spatial dimensions; derive them from channel 0.
    const int nx = static_cast<int>(channels[0]->Size(1));
    const int ny = static_cast<int>(channels[0]->Size(2));
    const QCPRange keyRange  (0, nx - 1);
    const QCPRange valueRange(0, ny - 1);

    QCPColorMapRGB *rgbMap = new QCPColorMapRGB(
        qPlot->axis(QCPAxis::atBottom),
        qPlot->axis(QCPAxis::atLeft));

    rgbMap->setChannelCount(nChannels);
    rgbMap->setCMYKMode(cmyk && nChannels >= 4);

    for(int ch = 0; ch < nChannels; ++ch)
        _fillRGBChannel(rgbMap, ch, channels[ch],
                        0, channels[ch]->Nelements(),
                        scale, nx, ny, keyRange, valueRange);

    QMetaObject::invokeMethod(qPlot->parentPlot(), [qPlot]() mutable
    {
        {
            std::lock_guard<std::mutex> lk(QtFITS::io_mutex);
            qPlot->parentPlot()->rescaleAxes();
            qPlot->parentPlot()->yAxis->setScaleRatio(qPlot->parentPlot()->xAxis, 1.0);
            qPlot->parentPlot()->replot();
        }
        {
            std::lock_guard<std::mutex> dk(QtFITS::msg_m);
            if(!QtFITS::finished) QtFITS::finished |= true;
            QtFITS::msg_cv.notify_one();
            QtFITS::finished &= false;
        }
    });

    return rgbMap;
}

void QtFITSviewer::QtDisplayRGB(QCustomPlot *qPlot,
                                  const std::shared_ptr<const FITScube> &red,
                                  const std::shared_ptr<const FITScube> &green,
                                  const std::shared_ptr<const FITScube> &blue,
                                  std::string OPTION)
{
    qPlot->yAxis->setScaleRatio(qPlot->xAxis, 1.0);
    qPlot->xAxis->setLabel("pixel [px]");
    qPlot->yAxis->setLabel("pixel [px]");
    OverlayRGB(qPlot->axisRect(), red, green, blue, OPTION);
}

void QtFITSviewer::QtDisplayRGB(QCustomPlot *qPlot,
                                  const std::shared_ptr<const FITScube> &cube,
                                  std::string OPTION)
{
    qPlot->yAxis->setScaleRatio(qPlot->xAxis, 1.0);
    qPlot->xAxis->setLabel("pixel [px]");
    qPlot->yAxis->setLabel("pixel [px]");
    OverlayRGB(qPlot->axisRect(), cube, OPTION);
}

// namespace DSL
