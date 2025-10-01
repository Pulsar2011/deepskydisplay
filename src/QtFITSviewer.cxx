//
//  mainwindow.cpp
//  EuclidSQL
//
//  Created by GILLARD William on 08/10/2014.
//  Copyright (c) 2014 Centre de Physique des Particules de Marseille (CPPM) - Université Aix-Marseille (AMU). All rights reserved.
//

#include "QtFITSviewer.h"
#include "ui_QtFITSviewer.h"

std::mutex QtFITS::io_mutex;
std::mutex QtFITS::msg_m;
std::condition_variable QtFITS::msg_cv;
bool QtFITS::finished = false;

/**
 *  @details Default constructor. Initiate application wich will open windows and display plots.
 *  @param parent
 */
QtFITSviewer::QtFITSviewer(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	QLocale loc = QLocale::system();
	QLocale::setDefault(loc);
    setMouseTracking(true);

	setGeometry(400, 250, 542, 390);
}

/**
 *  @details Default constructor. Initiate application wich will open windows and display plots.
 *  @param parent
 */
QtFITSviewer::QtFITSviewer(const int& x0, const int& y0, const int& xSize, const int& ySize, QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	QLocale loc = QLocale::system();
	QLocale::setDefault(loc);
    setMouseTracking(true);

	setGeometry(x0, y0, xSize, ySize);
}

QtFITSviewer::~QtFITSviewer()
{
    
	if(ui != NULL)
	{
        if(ui->customPlot != NULL)
			delete ui->customPlot;
		
        //delete ui;
	}
    
}

QCustomPlot* QtFITSviewer::GetPlot()
{
    if(ui->customPlot == NULL)
    {
        qInitPlot();
    }
    
    return ui->customPlot;
}

void QtFITSviewer::SetPlot(QCustomPlot* qplot)
{
    ui->customPlot = qplot;
    qInitPlot();
}

void QtFITSviewer::qInitPlot()
{
    if(ui->customPlot == NULL)
        ui->customPlot= new QCustomPlot();
    
    // configure axis rect:
    ui->customPlot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    ui->customPlot->axisRect()->setupFullAxesBox(true);
}

bool QtFITSviewer::eventFilter(QObject*, QEvent *event)
{
    if (event->type() == QEvent::MouseMove && ui->customPlot != NULL)
    {
        statusBar()->showMessage(ui->customPlot->window()->statusTip());
    }
    else if (event->type() == QEvent::Leave)
    {
        statusBar()->showMessage("");
    }
    return false;
}

