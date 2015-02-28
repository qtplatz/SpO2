/**************************************************************************
** Copyright (C) 2010-2013 Toshinobu Hondo, Ph.D.
** Copyright (C) 2013 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid ScienceLiaison commercial licenses may use this file in
** accordance with the MS-Cheminformatics Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and MS-Cheminformatics LLC.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.TXT included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
**************************************************************************/
#include "mainwindow.hpp"
#include "document.hpp"
#include "knob.hpp"
#include "plot.hpp"
#include "wheelbox.hpp"
#include "lcdbox.hpp"
#include <manhattanstyle.h> // qt-manhattan-style
#include <styledbar.h>      // qt-manhattan-style
#include <stylehelper.h>    // qt-manhattan-style
#include <qtwrapper/trackingenabled.hpp>
#include <qtwrapper/waitcursor.hpp>
#include <boost/any.hpp>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QCloseEvent>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLCDNumber>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolBar>
#include <QtDebug>
#include <functional>

#ifdef Q_WS_MAC
const QString rsrcPath = ":/spo2/images/mac";
#else
const QString rsrcPath = ":/spo2/images/win";
#endif

MainWindow::MainWindow(QWidget *parent) : Manhattan::FancyMainWindow(parent)
                                        , d_lcd1(0)
                                        , d_lcd2(0)
                                        , d_lcd3(0)
{
    Manhattan::Utils::StyleHelper::setBaseColor( QColor( Manhattan::Utils::StyleHelper::DEFAULT_BASE_COLOR ) );

    malpix::mpx4::document::instance()->initialSetup();

    setWindowTitle( "SpO2" );
    
    setDockNestingEnabled( true );
    setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
    setCorner( Qt::BottomRightCorner, Qt::BottomDockWidgetArea );

    statusBar()->setProperty( "p_styled", true );
    statusBar()->addWidget( new QLabel );

    if ( auto p = statusBar()->findChild<QLabel *>() ) {
        p->setText( "STATUS:" );
    }

    //setToolButtonStyle( Qt::ToolButtonFollowStyle );
    setupFileActions();
    setupEditActions();
    {
        QMenu *helpMenu = new QMenu(tr("Help"), this);
        menuBar()->addMenu(helpMenu);
        helpMenu->addAction(tr("About"), this, SLOT(about()));
        helpMenu->addAction(tr("About &Qt"), qApp, SLOT(aboutQt()));
    }

    auto widget = new QWidget;
    setCentralWidget( widget );
    //--------
    const double intervalLength = 10.0; // seconds

    d_plot1 = new Plot( "&lambda;<sub>1</sub>/&lambda;<sub>2</sub>", this );
    d_plot2 = new Plot( "Pressure", this );
    d_plot1->setMinimumHeight( 20 );
    d_plot2->setMinimumHeight( 20 );
    // d_plot1->enableAxis( QwtPlot::yRight );
    d_plot1->setAxisTitle( QwtPlot::yLeft, "Ratio" );
    d_plot2->setAxisTitle( QwtPlot::yLeft, "kPa" );

	//d_plot->setIntervalLength(intervalLength);

    d_amplitudeKnob = new Knob( "Amplitude", 0.0, 200.0, this );
    d_amplitudeKnob->setValue( 160.0 );

    d_frequencyKnob = new Knob( "Frequency [Hz]", 0.1, 20.0, this );
    d_frequencyKnob->setValue( 17.8 );

    d_intervalWheel = new WheelBox( "Displayed [s]", 1.0, 100.0, 1.0, this );
    d_intervalWheel->setValue( intervalLength );

    // d_timerWheel = new WheelBox( "Sample Interval [ms]", 0.0, 20.0, 0.1, this );
    // d_timerWheel->setValue( 10.0 );

    auto gridLayout = new QGridLayout();
    d_lcd1 = new LCDBox( "&lambda;<sub>1</sub>", gridLayout, 0 );
    d_lcd2 = new LCDBox( "&lambda;<sub>2</sub>", gridLayout, 1 );
    d_lcd3 = new LCDBox( "&lambda;<sub>1</sub>/&lambda;<sub>2</sub>", gridLayout, 2 );
    gridLayout->setColumnStretch( 0, 10 );

    QVBoxLayout* vLayout1 = new QVBoxLayout();
    vLayout1->addWidget( d_intervalWheel );

    vLayout1->addLayout( gridLayout );
    
    vLayout1->addStretch( 10 );
    vLayout1->addWidget( d_amplitudeKnob );
    vLayout1->addWidget( d_frequencyKnob );

    auto vLayout2 = new QVBoxLayout();
    vLayout2->addWidget( d_plot1, 5 );
    vLayout2->addWidget( d_plot2, 5 );

    QHBoxLayout *layout = new QHBoxLayout( widget );

    layout->addLayout( vLayout2 );
    layout->addLayout( vLayout1 );
    layout->setStretchFactor( vLayout2, 10 );

    connect( d_amplitudeKnob, SIGNAL( valueChanged( double ) ), SIGNAL( amplitudeChanged( double ) ) );
    connect( d_frequencyKnob, SIGNAL( valueChanged( double ) ), SIGNAL( frequencyChanged( double ) ) );
    connect( d_intervalWheel, SIGNAL( valueChanged( double ) ), d_plot1, SLOT( setIntervalLength( double ) ) );

    connect( d_plot1, &Plot::onData, this, &MainWindow::handleData );
    connect( d_plot2, &Plot::onData, this, &MainWindow::handleData );    
}

MainWindow::~MainWindow()
{
}

void
MainWindow::closeEvent(QCloseEvent *e)
{
}

void
MainWindow::setupFileActions()
{
    QToolBar *tb = new QToolBar(this);
    tb->setWindowTitle(tr("File Actions"));
    addToolBar(tb);

    QMenu *menu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(menu);

    QAction *a;

    QIcon newIcon = QIcon::fromTheme("document-new", QIcon(rsrcPath + "/filenew.png"));
    a = new QAction( newIcon, tr("&New"), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::New);
    connect(a, SIGNAL(triggered()), this, SLOT(fileNew()));
    tb->addAction(a);
    menu->addAction(a);

    a = new QAction(QIcon::fromTheme("document-open", QIcon(rsrcPath + "/fileopen.png")), tr("&Open..."), this);
    a->setShortcut(QKeySequence::Open);
    connect(a, SIGNAL(triggered()), this, SLOT(fileOpen()));
    tb->addAction(a);
    menu->addAction(a);

    menu->addSeparator();

    actionSave = a = new QAction(QIcon::fromTheme("document-save", QIcon(rsrcPath + "/filesave.png")), tr("&Save"), this);
    a->setShortcut(QKeySequence::Save);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSave()));
    a->setEnabled(false);
    tb->addAction(a);
    menu->addAction(a);

    a = new QAction(tr("Save &As..."), this);
    a->setPriority(QAction::LowPriority);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    menu->addAction(a);
    menu->addSeparator();

#ifndef QT_NO_PRINTER
    a = new QAction(QIcon::fromTheme("document-print", QIcon(rsrcPath + "/fileprint.png")), tr("&Print..."), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Print);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrint()));
    tb->addAction(a);
    menu->addAction(a);

    a = new QAction(QIcon::fromTheme("fileprint", QIcon(rsrcPath + "/fileprint.png")),
                    tr("Print Preview..."), this);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPreview()));
    menu->addAction(a);

    a = new QAction(QIcon::fromTheme("exportpdf", QIcon(rsrcPath + "/exportpdf.png")),
                    tr("&Export PDF..."), this);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(Qt::CTRL + Qt::Key_D);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPdf()));
    tb->addAction(a);
    menu->addAction(a);

    menu->addSeparator();
#endif

    a = new QAction(tr("&Quit"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(a, SIGNAL(triggered()), this, SLOT(close()));
    menu->addAction(a);
}

void
MainWindow::setupEditActions()
{
    QToolBar *tb = new QToolBar(this);
    tb->setWindowTitle(tr("Edit Actions"));
    addToolBar(tb);
    QMenu *menu = new QMenu(tr("&Edit"), this);
    menuBar()->addMenu(menu);
}

void
MainWindow::setupTextActions()
{
}

bool
MainWindow::load(const QString &f)
{
    setCurrentFileName( f );
    return true;
}

bool
MainWindow::maybeSave()
{
    return true;
}

void
MainWindow::setCurrentFileName(const QString &fileName)
{
}

void
MainWindow::fileNew()
{
    malpix::mpx4::document::instance()->fileNew();
}

void MainWindow::fileOpen()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("Open File..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if ( !fn.isEmpty() )
        load(fn);
}

bool
MainWindow::fileSave()
{
    return true;
}

bool
MainWindow::fileSaveAs()
{
    return true;
}

void
MainWindow::filePrint()
{
}

void
MainWindow::filePrintPreview()
{
}

void
MainWindow::printPreview(QPrinter *printer)
{
}

void
MainWindow::filePrintPdf()
{
}

void
MainWindow::clipboardDataChanged()
{
}

void
MainWindow::about()
{
    QMessageBox::about(this, tr("About"), tr("This is the GUI version of MALPIX4 examination platrom.") );
}

void
MainWindow::createDockWidgets()
{
}

QDockWidget *
MainWindow::createDockWidget( QWidget * widget, const QString& title, const QString& pageName )
{
    QDockWidget * dockWidget = addDockForWidget( widget );
    dockWidget->setObjectName( pageName.isEmpty() ? widget->objectName() : pageName );

    if ( title.isEmpty() )
        dockWidget->setWindowTitle( widget->objectName() );
    else
        dockWidget->setWindowTitle( title );

    addDockWidget( Qt::BottomDockWidgetArea, dockWidget );

    return dockWidget;
}

Manhattan::StyledBar *
MainWindow::createTopStyledBar()
{
    auto toolBar = new Manhattan::StyledBar;
    // toolBar->setProperty( "topBorder", true );
    return toolBar;
}

Manhattan::StyledBar *
MainWindow::createMidStyledBar()
{
    auto toolBar2 = new Manhattan::StyledBar;

    if ( toolBar2 ) {
        toolBar2->setProperty( "topBorder", true );
        QHBoxLayout * toolBarLayout = new QHBoxLayout( toolBar2 );
        toolBarLayout->setMargin(0);
        toolBarLayout->setSpacing(0);
#if 0
        Core::ActionManager * am = Core::ActionManager::instance();
        if ( am ) {
            // print, method file open & save buttons
            toolBarLayout->addWidget(toolButton(am->command(Constants::PRINT_CURRENT_VIEW)->action()));
            toolBarLayout->addWidget(toolButton(am->command(Constants::METHOD_OPEN)->action()));
            toolBarLayout->addWidget(toolButton(am->command(Constants::METHOD_SAVE)->action()));
			toolBarLayout->addWidget(toolButton(am->command(Constants::CALIBFILE_APPLY)->action()));
#endif
            //----------
            toolBarLayout->addWidget( new Manhattan::StyledSeparator );
            //----------
    }
    return toolBar2;
}

void
MainWindow::handleInstState( int stat )
{
}

void
MainWindow::handleFeatureSelected( int )
{
}

void
MainWindow::handleFeatureActivated( int )
{
}

void
MainWindow::onInitialUpdate()
{
}

void
MainWindow::start()
{
	//d_plot->start();
}

double
MainWindow::frequency() const
{
    return d_frequencyKnob->value();
}

double MainWindow::amplitude() const
{
    return d_amplitudeKnob->value();
}

double MainWindow::signalInterval() const
{
    return 1; // d_timerWheel->value();
}

void
MainWindow::handleData( double )
{
    auto tp = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast< std::chrono::milliseconds >( tp - tp_ );
    if ( duration.count() > 500 ) {
        tp_ = tp;
        auto p1 = d_plot1->sample();
        auto p2 = d_plot2->sample( p1.first );
        d_lcd1->setValue( p1.second );
        d_lcd2->setValue( p2.second );
        d_lcd3->setValue( p1.second / p2.second );                
    }
}
    
