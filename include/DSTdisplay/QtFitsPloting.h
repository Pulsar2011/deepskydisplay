#ifndef QTFITSPLOTTING__H
#define QTFITSPLOTTING__H

#include <string>
#include "QtDisplay.h"
#include <DSTfits/FITSimg.h>


namespace DSL
{
    enum zScale
    {
        /**
         *  @enum DSL::FITScube::zScale
         *  @details Defines the type of the z axis during Qt display. It do not affect the ROOT display.
         */
        fLIN,   ///< Linear scale
        fLOG,   ///< Logaritmic scale
        fSINH,  ///< Sinus hyperbolic scale
        fQUAD,  ///< Quadratic scale
        fSQRT,  ///< Squared root scale
    };


    void QtDisplay(QCustomPlot*      , const FITScube*, std::string OPTION=std::string());
    void QtDisplay(QCustomPlot*      , const FITScube*, const size_t&, const size_t&, std::string OPTION=std::string());
    void QtDisplayLayer(QCustomPlot* , const FITScube*, const size_t&, std::string OPTION=std::string());

    void Overlay(QCustomPlot*, const FITScube*, std::string OPTION=std::string());
    void Overlay(QCustomPlot*, const FITScube*, const size_t&, const size_t&, std::string OPTION=std::string());
    
} // namespace DSDisplay

#endif // QTFITSPLOTTING__H
