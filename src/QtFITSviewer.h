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

using namespace DSL;

#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future> 

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
        void print() const
        {
            for(int i = 0; i < bin_edges.size(); i++)
                std::cout << bin_edges[i] << "    " << pdf[i] << "    " << cdf[i] << std::endl;
        }

        QVector<double> bin_edges;
        QVector<double> pdf;
        QVector<double> cdf;
    };
}

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

#pragma region - Public member
public:

#pragma constructor/destructor
    QtFITSviewer(QWidget *parent = 0); ///< @brief Default constructor
    QtFITSviewer(const int&, const int&, const int&, const int&, QWidget *parent = 0);
    
    /**
     *  @brief Default destructor
     *  @details Default destructor
     */
    virtual ~QtFITSviewer();
#pragma endregion
#pragma region Accessors
    
    QCustomPlot* GetPlot();
    void SetPlot( QCustomPlot* );

#pragma endregion
#pragma region Plotting methods

    static void QtDisplay(QCustomPlot*      , const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());
    static void QtDisplay(QCustomPlot*      , const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());
    static void QtDisplayLayer(QCustomPlot* , const std::shared_ptr<const FITScube>&, const size_t&, std::string OPTION=std::string());

    static void Overlay(QCustomPlot*, const std::shared_ptr<const FITScube>&, std::string OPTION=std::string());
    static void Overlay(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, const size_t&, std::string OPTION=std::string());

    static void QtHistogram(QCustomPlot* , const std::shared_ptr<const FITScube>&, const size_t&, QCPGraph*, QCPGraph*);
    static void QtHistogram(QCustomPlot* , const std::shared_ptr<const FITScube>&, const size_t&, const double&, const double&, QCPGraph*, QCPGraph*);

    static QtFITS::pixelDistribution pixHistogram(const std::shared_ptr<const FITScube>&, const size_t& n_bins);
    static QtFITS::pixelDistribution pixHistogram(const std::shared_ptr<const FITScube>&, const size_t& n_bins, const double& min, const double& max);

    template<typename T>
    static QtFITS::pixelDistribution pixHistogram(const std::shared_ptr<const FITScube>&, const size_t& n_bins);

    template<typename T>
    static QtFITS::pixelDistribution pixHistogram(const std::shared_ptr<const FITScube>&, const size_t& n_bins, const double& min, const double& max);

    static void QtProfileX(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, QCPGraph*, bool horizontal=true);
    static void QtProfileY(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, QCPGraph*, bool horizontal=true);

    template<typename T>
    static void QtProfileX(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, QCPGraph*, bool horizontal=true);

    template<typename T>
    static void QtProfileY(QCustomPlot*, const std::shared_ptr<const FITScube>&, const size_t&, QCPGraph*, bool horizontal=true);
    
private:
    Ui::MainWindow *ui;    ///< Main window GUI
    
    QtFITS::qPlotList plot_list;
    
    void qInitPlot();
    bool eventFilter(QObject*, QEvent *);

};

template<typename T>
QtFITS::pixelDistribution QtFITSviewer::pixHistogram(const std::shared_ptr<const FITScube>& in, const size_t& n_bins)
{
    double max = in->GetMaximum();
    double min = in->GetMinimum();
    return pixHistogram(in,n_bins,min,max);
}

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

template<typename T>
void  QtFITSviewer::QtProfileY(QCustomPlot* customPlot,const std::shared_ptr<const FITScube>& in, const size_t& iX, QCPGraph* y, bool horizontal)
{
    if(iX >= in->Size(1))
        throw std::out_of_range("QtProfileY@["+std::to_string(iX)+"] iX is out of range");

    std::string UNITS = (in->HDU().Exists("UNITS"))?in->HDU().GetValueForKey("UNITS"):"ARB. UNIT";
    
    std::pair<QVector<double>, QVector<double> > profile;
    profile.first.clear();
    profile.second.clear();

    for(size_t k=0; k<in->Size(1); k++)
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
