/**************************************************************************
** Copyright (C) 2010-2015 Toshinobu Hondo, Ph.D.
** Copyright (C) 2013-2015 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid MS-Cheminfomatics commercial licenses may use this file in
** accordance with the MS-Cheminformatics Commercial License Agreement provided with
** the Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and MS-Cheminformatics.
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
#include <QApplication>
#include <manhattanstyle.h>
#include "samplingthread.h"

int
main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString basename = qApp->style()->objectName();
    auto style = new ManhattanStyle( "windowsvista" );
    qApp->setStyle( style );

    MainWindow w;
    w.resize( 800, 400 );
    w.onInitialUpdate();

    SamplingThread samplingThread;
    samplingThread.setFrequency( w.frequency() );
    samplingThread.setAmplitude( w.amplitude() );
    samplingThread.setInterval( w.signalInterval() );

    w.connect( &w, SIGNAL( frequencyChanged( double ) ), &samplingThread, SLOT( setFrequency( double ) ) );
    w.connect( &w, SIGNAL( amplitudeChanged( double ) ), &samplingThread, SLOT( setAmplitude( double ) ) );
    w.connect( &w, SIGNAL( signalIntervalChanged( double ) ), &samplingThread, SLOT( setInterval( double ) ) );

    w.show();
    samplingThread.start();
    w.start();
    
    bool res = a.exec();

    samplingThread.stop();
    samplingThread.wait( 1000 );

    return res;
}
