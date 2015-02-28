#ifndef _PLOT_H_
#define _PLOT_H_ 1

#include <qwt_plot.h>
#include <qwt_system_clock.h>
#include "settings.h"

class QwtPlotGrid;
class QwtPlotCurve;

class Plot: public QwtPlot
{
    Q_OBJECT

public:
    Plot( const QString& title, QWidget* = NULL );

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
    QwtPlotCurve *d_curve;

    QwtSystemClock d_clock;
    double d_interval;

    int d_timerId;

    Settings d_settings;
};

#endif
