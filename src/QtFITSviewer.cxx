//
//  mainwindow.cpp
//  EuclidSQL
//
//  Created by GILLARD William on 08/10/2014.
//  Copyright (c) 2014 Centre de Physique des Particules de Marseille (CPPM) - Université Aix-Marseille (AMU). All rights reserved.
//

#include "QtFITSviewer.h"
#include "ui_QtFITSviewer.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QApplication>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

#include <iostream>

std::mutex QtFITS::io_mutex;
std::mutex QtFITS::msg_m;
std::condition_variable QtFITS::msg_cv;
bool QtFITS::finished = false;

/**
 *  @details Default constructor. Initiate application wich will open windows and display plots.
 *  @param parent
 */
QtFITSviewer::QtFITSviewer(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow),resizeFunctor(nullptr)
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
QtFITSviewer::QtFITSviewer(const int& x0, const int& y0, const int& xSize, const int& ySize, QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow),resizeFunctor(nullptr)
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

// ---------------------------------------------------------------------------
// enumerateHDUs
// ---------------------------------------------------------------------------

std::vector<HduInfo> enumerateHDUs(DSL::FITSmanager& fm)
{
    std::vector<HduInfo> result;
    const int n = fm.NumberOfHeader();
    for (int i = 1; i <= n; ++i) {
        try {
            HduInfo info;
            info.index  = i;
            info.naxis  = 0;
            info.naxis3 = 0;

            auto hdr = fm.GetHeaderAtIndex(i);

            // EXTNAME
            if (hdr->Exists("EXTNAME")) {
                std::string name = hdr->GetValueForKey("EXTNAME");
                auto first = name.find_first_not_of(" \t'\"");
                auto last  = name.find_last_not_of(" \t'\"");
                info.extname = (first != std::string::npos)
                               ? name.substr(first, last - first + 1)
                               : "";
            }
            if (info.extname.empty())
                info.extname = "HDU" + std::to_string(i);

            // Type
            if (i == 1 || !hdr->Exists("XTENSION")) {
                info.type = HduInfo::Type::IMG;
            } else {
                std::string xt = hdr->GetValueForKey("XTENSION");
                auto f = xt.find_first_not_of(" \t'\"");
                auto l = xt.find_last_not_of(" \t'\"");
                xt = (f != std::string::npos) ? xt.substr(f, l - f + 1) : xt;
                if      (xt == "IMAGE")    info.type = HduInfo::Type::IMG;
                else if (xt == "BINTABLE") info.type = HduInfo::Type::BTBL;
                else if (xt == "TABLE")    info.type = HduInfo::Type::TBL;
                else                       info.type = HduInfo::Type::UNKNOWN;
            }

            // Dimensions
            info.naxis  = hdr->Exists("NAXIS")  ? (int)hdr->GetUInt64ValueForKey("NAXIS")  : 0;
            info.naxis3 = hdr->Exists("NAXIS3") ? (int)hdr->GetUInt64ValueForKey("NAXIS3") : 0;

            result.push_back(info);
        }
        catch (const DSL::FITSexception& e) {
            std::cerr << "[WARNING] enumerateHDUs: HDU " << i << ": " << e.what() << std::endl;
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// HduListWindow
// ---------------------------------------------------------------------------

static QString typeString(const HduInfo& hdu)
{
    switch (hdu.type) {
        case HduInfo::Type::IMG: {
            QString s = QString("IMG (NAXIS=%1)").arg(hdu.naxis);
            if (hdu.naxis == 3)
                s += QString(", NAXIS3=%1").arg(hdu.naxis3);
            return s;
        }
        case HduInfo::Type::BTBL:    return "BINTABLE";
        case HduInfo::Type::TBL:     return "TABLE";
        case HduInfo::Type::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

HduListWindow::HduListWindow(const std::string& fitsPath,
                             const std::vector<HduInfo>& hdus,
                             std::function<QWidget*(const HduInfo&)> viewerFactory,
                             QWidget* parent)
    : QMainWindow(parent)
    , m_fitsPath(fitsPath)
    , m_hdus(hdus)
    , m_viewerFactory(std::move(viewerFactory))
{
    // Window title: base name only
    std::string base = fitsPath;
    auto slash = base.rfind('/');
    if (slash != std::string::npos) base = base.substr(slash + 1);
    setWindowTitle(QString("FitsViewer \xe2\x80\x94 %1").arg(QString::fromStdString(base)));

    // Table
    m_table = new QTableWidget(static_cast<int>(hdus.size()), 3, this);
    m_table->setHorizontalHeaderLabels({"Index", "Extension", "Type"});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->hide();

    for (int row = 0; row < static_cast<int>(hdus.size()); ++row) {
        const HduInfo& h = hdus[static_cast<size_t>(row)];
        m_table->setItem(row, 0, new QTableWidgetItem(QString::number(h.index)));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(h.extname)));
        m_table->setItem(row, 2, new QTableWidgetItem(typeString(h)));
    }
    m_table->resizeColumnsToContents();

    connect(m_table, &QTableWidget::cellDoubleClicked,
            [this](int row, int /*col*/) {
                if (row >= 0 && row < static_cast<int>(m_hdus.size()))
                    emit hduActivated(m_hdus[static_cast<size_t>(row)].index);
            });
    connect(this, &HduListWindow::hduActivated, this, &HduListWindow::openViewer);

    // Menu bar: a "Color" menu lets the user opt-in to a colour composite.
    // The window itself does no compositing — it only emits colorComposeRequested();
    // the host application wires that signal to its compose dialog.
    QMenu* colorMenu = menuBar()->addMenu(tr("Color"));
    QAction* composeAct = colorMenu->addAction(tr("Compose color image\xe2\x80\xa6"));
    connect(composeAct, &QAction::triggered,
            this, [this]() { emit colorComposeRequested(); });

    setCentralWidget(m_table);
    resize(520, 300);
}

void HduListWindow::openViewer(int index)
{
    const HduInfo* hdu = nullptr;
    for (const auto& h : m_hdus) {
        if (h.index == index) { hdu = &h; break; }
    }
    if (!hdu) return;

    QRect prevGeometry;
    if (m_currentViewer) {
        prevGeometry = m_currentViewer->geometry();
        m_currentViewer->close();
        m_currentViewer->deleteLater();
        m_currentViewer = nullptr;
    }

    if (m_viewerFactory) {
        m_currentViewer = m_viewerFactory(*hdu);
        if (m_currentViewer) {
            if (prevGeometry.isValid())
                m_currentViewer->setGeometry(prevGeometry);
            m_currentViewer->show();
        }
    }
}

void HduListWindow::closeEvent(QCloseEvent* event)
{
    if (m_currentViewer) {
        m_currentViewer->close();
        m_currentViewer = nullptr;
    }
    QMainWindow::closeEvent(event);
    QApplication::quit();
}

