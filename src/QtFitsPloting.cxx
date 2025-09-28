//
//  QtFitsPlotting.cxx
//  DeepSkyTools
//
//  File created by GILLARD William on 28/09/25.
//  Handle the plotting of FITS data
//
//	Copyright (c) 2025, All rights reserved

#include<DSTdisplay/QtDisplay.h>
#include<DSTdisplay/QtFitsPloting.h>
#include<DSTdisplay/DSD_version.h>
#include<DSTfits/FITSexception.h>

namespace DSL
{
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
    void QtDisplay(QCustomPlot* qPlot,const FITScube* img, std::string OPTION)
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
    void QtDisplay(QCustomPlot* qPlot, const FITScube* img, const size_t& iMin, const size_t& iMax, std::string OPTION)
    {
        //- SEARCH FOR EXISTING QtMainWindows        
        const size_t nx =  img->Size(1);
        const size_t ny =  img->Size(2);
        
        size_t xsize = (nx*2 < 900)? nx*2 : 900;
        xsize = (xsize < 542 )? 542 : xsize;
        
        size_t ysize = xsize*ny/nx-100;
        ysize = (ysize < 390)? 390 : ysize;
        ysize = (ysize > 900-100)? 900 : ysize;
        
        //mw->setGeometry(400, 250, xsize, ysize);
        
        //- Obtain the plot
       //QCustomPlot *qPlot = mw->GetPlot();
        qPlot->yAxis->setScaleRatio(qPlot->xAxis,1.0);
        
        //- Fill the graph
        qPlot->xAxis->setLabel("pixel [px]");
        qPlot->yAxis->setLabel("pixel [px]");
        
        Overlay(qPlot, img, iMin, iMax, OPTION);
        
        //mw->SetPlot(qPlot);
        //mw->show();

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
    void QtDisplayLayer(QCustomPlot* qPlot, const FITScube* img, const size_t& _iLayer, std::string OPTION)
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
    void Overlay(QCustomPlot* qPlot, const FITScube* img, std::string OPTION)
    {
        Overlay(qPlot, img, 0, img->Nelements(), OPTION);
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
    void Overlay(QCustomPlot* qPlot, const FITScube* img, const size_t& iMin, const size_t& iMax, std::string OPTION)
    {
    
        std::vector<size_t> xPixels;
        
        std::transform(OPTION.begin(), OPTION.end(), OPTION.begin(), ::tolower);

        const size_t nx = img->Size(1);
        const size_t ny = img->Size(2);
        
        zScale useScale = zScale::fLIN;
        if(OPTION.find("logz",0) != std::string::npos )
            useScale  = zScale::fLOG;
        else if(OPTION.find("sinhz",0) != std::string::npos )
            useScale  = zScale::fSINH;
        else if(OPTION.find("quadz",0) != std::string::npos )
            useScale  = zScale::fQUAD;
        else if(OPTION.find("sqrtz",0) != std::string::npos )
            useScale  = zScale::fSQRT;
        else
            useScale = zScale::fLIN;
        
        QCPColorMap *colorMap = new QCPColorMap(qPlot->xAxis, qPlot->yAxis);
        colorMap->data()->setSize(nx, ny);								// we want the color map to have nx * ny data points
                                                                        // set the color gradient of the color map to one of the presets:
        
        colorMap->data()->setRange(QCPRange(0, 0), QCPRange(nx, ny));	// and span the coordinate range in both key (x) and value (y) dimensions
        colorMap->data()->fill(0);
        
        // now we assign some data, by accessing the QCPColorMapData instance of the color map:
        
        size_t k = iMin;
        double* Intensity = new double[nx*ny];
        for(size_t ik = 0; ik< nx*ny; ik++) Intensity[ik]=0;

        std::valarray<bool> mask = img->GetMask();
        
        for(k = iMin; k < iMax; k++)
        {
            if(mask[k])
                continue;
                     
            Intensity[k%(nx*ny)] += img->at(k);
        }
        
        for(size_t ik = 0; ik< nx*ny; ik++)
        {
            xPixels.clear();
            
            xPixels = img->PixelCoordinates(k);

            double xIndex = xPixels[0];//-PixelCoordinates(0)[0];
            double yIndex = xPixels[1];//-PixelCoordinates(0)[1];
            
            if(Intensity[ik] == 0)
                continue;
                
            switch (useScale)
            {
                case fLIN:
                    Intensity[ik] *= 1.;
                    break;
                    
                case fLOG:
                    Intensity[ik] = log10(Intensity[ik]);
                    break;
                    
                case fQUAD:
                    Intensity[ik] = pow(Intensity[ik], 2.);
                    break;
                    
                case fSQRT:
                    Intensity[ik] = sqrt(Intensity[ik]);
                    break;
                    
                case fSINH:
                    Intensity[ik] = sinh(Intensity[ik]);
                    break;
                    
                default:
                    Intensity[ik] *= 1;
                    break;
            }
            
            colorMap->data()->setCell(xIndex, yIndex, static_cast<double>( Intensity[ik]) );
        }
        
        
        delete [] Intensity;
        
        std::string UNITS = img->HDU().GetValueForKey("UNITS");
        if(UNITS.size() < 1)
            UNITS += std::string("Intensity");
        
        switch (useScale)
        {
            case fLOG:
                UNITS = "log10("+UNITS+")";
                break;
                
            case fQUAD:
                UNITS += "^2";
                break;
                
            case fSQRT:
                UNITS += "^(1/2)";
                break;
                
            case fSINH:
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
            colorScale->setGradient(QCPColorGradient::gpThermal);
        
            colorScale->axis()->setLabel(UNITS.c_str());
            qPlot->plotLayout()->addElement(0, 1, colorScale);          // add it to the right of the main axis rect
            colorScale->setType(QCPAxis::atRight);						// scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
            
            // make sure the axis rect and color scale synchronize their bottom and top margins (so they line up):
            QCPMarginGroup *marginGroup = new QCPMarginGroup(qPlot);
            const QCP::MarginSides sides = QCP::MarginSides(QCP::msBottom) | QCP::msTop;
            qPlot->axisRect()->setMarginGroup(sides, marginGroup);
            colorScale->setMarginGroup(sides, marginGroup);
        }
        else
            colorScale = static_cast<QCPColorScale*>(qPlot->plotLayout()->element(0,1));
            
        
        colorMap->setColorScale(colorScale);						// associate the color map with the color scale
        // we could have also created a QCPColorGradient instance and added own	colors to
        // the gradient, see the documentation of QCPColorGradient for what's possible.
        
        // rescale the data dimension (color) such that all data points lie in the span visualized by the color gradient:
        colorMap->rescaleDataRange();
        
        if(std::abs(img->GetMaximum()/img->GetMinimum()) > 100)
            colorMap->setDataRange(QCPRange(img->Get5thpercentil(),img->Get95thpercentil()));
        
        colorMap->setInterpolate(false);
        
        //qPlot->addPlottable(colorMap);
        
        // rescale the key (x) and value (y) axes so the whole color map is visible:
        //qPlot->rescaleAxes();
        //qPlot->replot();
    }
}
// namespace DSL
