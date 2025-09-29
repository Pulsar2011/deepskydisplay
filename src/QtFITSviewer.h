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

namespace QtFITS
{
	typedef std::map<std::string,int> qPlotList;
	
	class rgb
	{
	public:
		/**
		 *  @class qfits::rgb QfitsViewer.h "QfitsViewer.h"
		 *  @details Color managment class used by plotting view.
		 */
		unsigned int red;	///< The quantity of red, ranging from 0 to 255
		unsigned int blue;	///< The quantity of blue, ranging from 0 to 255
		unsigned int green;	///< The quantity of green, ranging from 0 to 255
		unsigned int alpha;	///< The transparency : 0 transparant, 255 opac
		
		/**
		 *  @brief Default constructor
		 *  @details Default constructor
		 *  @param r : Red color fraction from 0 to 255
		 *  @param g : Green color fraction from 0 to 255
		 *  @param b : Blue color fraction from 0 to 255
		 *  @param a : Transparancy.
		 *		* 0 \f$\rightarrow\f$ Transparent
		 *		* 255 \f$\rightarrow\f$ Opac
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
}

namespace Ui
{
	class MainWindow;
}

/**
 *  @class QfitsViewer QfitsViewer.h "QfitsViewer.h"
 *	@brief Warper to Qt::QCustomPlot display
 *
 *	@details Base class to display FITS contents onto standard output. It require Qt v > 4 with QCustomPlot (http://www.qcustomplot.com/) widget source. QfitsViewer as well as its child class provide tha capability to export data to TROOT fromework. Thus, it is  required to install TROOT v6.00  minimum
 */
class QtFITSviewer : public QMainWindow
{

	Q_OBJECT
	
public:
	QtFITSviewer(QWidget *parent = 0); ///< @brief Default constructor
	
	/**
	 *  @brief Default destructor
	 *  @details Default destructor
	 */
	virtual ~QtFITSviewer();
	
    QCustomPlot* GetPlot();
    void SetPlot( QCustomPlot* );
	
private:
	Ui::MainWindow *ui;	///< Main window GUI
    
    QtFITS::qPlotList plot_list;
    
    void qInitPlot();
    bool eventFilter(QObject*, QEvent *);

};

#endif /* defined(MAINWINDOW_H) */
