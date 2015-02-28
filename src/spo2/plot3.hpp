
#pragma once

#include <qwt_plot.h>
#include <qwt_system_clock.h>
#include "settings.h"

class QwtPlotGrid;
class QwtPlotCurve;

class Plot3: public QwtPlot
{
    Q_OBJECT

public:
    Plot3( const QString& title, QWidget* = NULL );

    std::pair< int, double > sample( int pos = -1 );

public Q_SLOTS:
    void setSettings( const Settings & );

Q_SIGNALS:
    void onData( double );

protected:
    virtual void timerEvent( QTimerEvent *e );

private:
    void alignScales();

    QwtPlotGrid *d_grid;
    QwtPlotCurve *d_curve1;
    QwtPlotCurve *d_curve2;
    QwtPlotCurve *d_curve3;    

    QwtSystemClock d_clock;
    double d_interval;
    int d_timerId;
    Settings d_settings;

    
};

