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

#include "spectrogramplot.hpp"
#include "document.hpp"
#include <adcontrols/mappedimage.hpp>
#include <qwt_plot_spectrogram.h>
#include <qwt_color_map.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_draw.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_renderer.h>
#include <QPrinter>
#include <QPrintDialog>
#include <QBrush>
#include <iostream>
#include <boost/numeric/ublas/matrix.hpp>

namespace malpix { namespace detail {

    class zoomer : public QwtPlotZoomer {
    public:
        zoomer( QWidget * canvas ) : QwtPlotZoomer( canvas ) {
            setTrackerMode( AlwaysOn );
        }
        virtual QwtText trackerTextF( const QPointF &pos ) const {
            QColor bg( Qt::white );
            bg.setAlpha( 200 );
            
            QwtText text = QwtPlotZoomer::trackerTextF( pos );
			text.setBackgroundBrush( QBrush( bg ) );
            return text;
        }
    };

    class SpectrogramData: public QwtRasterData  {
    public:
        SpectrogramData() {
            setInterval( Qt::XAxis, QwtInterval( 0, 16 ) );
            setInterval( Qt::YAxis, QwtInterval( 0, 16 ) );
            setInterval( Qt::ZAxis, QwtInterval( -1.0, 10000.0 ) );
        }

        SpectrogramData( std::shared_ptr< adcontrols::MappedImage >& data ) : data_( data ){
        }

        virtual double value( double x, double y ) const  {
            if ( data_ ) {
                size_t i = size_t(x);
                size_t j = size_t(y);
                if ( i < data_->size1() && j < data_->size2() ) {
                    double value = ( *data_ )( i, j );
                    return value * 100;
                }
            }
            return 0;
        }
        std::shared_ptr< adcontrols::MappedImage > data_;
    };

    class ColorMap: public QwtLinearColorMap {
    public:
        ColorMap(): QwtLinearColorMap( Qt::darkBlue, Qt::red ) {
            addColorStop( 0.1, Qt::cyan );
            addColorStop( 0.6, Qt::green );
            addColorStop( 0.95, Qt::yellow );
        }
    };
} // namespace detail
}

using namespace malpix;

SpectrogramPlot::SpectrogramPlot( QWidget *parent ) : QwtPlot(parent)
                                                    , spectrogram_( new QwtPlotSpectrogram() )
                                                    , drawable_( new detail::SpectrogramData() )
{
    spectrogram_->setRenderThreadCount( 0 ); // use system specific thread count

    spectrogram_->setColorMap( new detail::ColorMap() );
    spectrogram_->setCachePolicy( QwtPlotRasterItem::PaintCache );

    spectrogram_->setData( drawable_ );
    spectrogram_->attach( this );

    QList<double> contourLevels;
    for ( double level = 0.5; level < 10.0; level += 1.0 )
        contourLevels += level;
    spectrogram_->setContourLevels( contourLevels );

    const QwtInterval zInterval = spectrogram_->data()->interval( Qt::ZAxis );
    // A color bar on the right axis
    QwtScaleWidget *rightAxis = axisWidget( QwtPlot::yRight );
    rightAxis->setTitle( "Intensity" );
    rightAxis->setColorBarEnabled( true );
    rightAxis->setColorMap( zInterval, new detail::ColorMap() );

    setAxisScale( QwtPlot::yRight, zInterval.minValue(), zInterval.maxValue() );
    enableAxis( QwtPlot::yRight );

    plotLayout()->setAlignCanvasToScales( true );
    replot();

    // LeftButton for the zooming
    // MidButton for the panning
    // RightButton: zoom out by 1
    // Ctrl+RighButton: zoom out to full size

    QwtPlotZoomer* zoomer = new detail::zoomer( canvas() );
    zoomer->setMousePattern( QwtEventPattern::MouseSelect2,
        Qt::RightButton, Qt::ControlModifier );
    zoomer->setMousePattern( QwtEventPattern::MouseSelect3,
        Qt::RightButton );

    QwtPlotPanner *panner = new QwtPlotPanner( canvas() );
    panner->setAxisEnabled( QwtPlot::yRight, false );
    panner->setMouseButton( Qt::MidButton );

    // Avoid jumping when labels with more/less digits
    // appear/disappear when scrolling vertically

    const QFontMetrics fm( axisWidget( QwtPlot::yLeft )->font() );
    QwtScaleDraw *sd = axisScaleDraw( QwtPlot::yLeft );
    sd->setMinimumExtent( fm.width( "100.00" ) );

    const QColor c( Qt::darkBlue );
    zoomer->setRubberBandPen( c );
    zoomer->setTrackerPen( c );

    connect( this, SIGNAL( dataChanged() ), this, SLOT( handle_dataChanged() ) );
    //model::instance()->signal( std::bind( &SpectrogramPlot::handle_signal, this ) );
}

void
SpectrogramPlot::handle_dataChanged()
{
	spectrogram_->invalidateCache();
	replot();
}

void
SpectrogramPlot::handle_showContour( bool on )
{
    spectrogram_->setDisplayMode( QwtPlotSpectrogram::ContourMode, on );
    replot();
}

void
SpectrogramPlot::handle_showSpectrogram( bool on )
{
    spectrogram_->setDisplayMode( QwtPlotSpectrogram::ImageMode, on );
    spectrogram_->setDefaultContourPen( 
        on ? QPen( Qt::black, 0 ) : QPen( Qt::NoPen ) );

    replot();
}

void
SpectrogramPlot::handle_setAlpha( int alpha )
{
    spectrogram_->setAlpha( alpha );
    replot();
}

void
SpectrogramPlot::handle_printPlot()
{
    QPrinter printer( QPrinter::HighResolution );
    printer.setOrientation( QPrinter::Landscape );
    printer.setOutputFileName( "spectrogram.pdf" );

    QPrintDialog dialog( &printer );
    if ( dialog.exec() ) {
        QwtPlotRenderer renderer;

        if ( printer.colorMode() == QPrinter::GrayScale ) {
            renderer.setDiscardFlag( QwtPlotRenderer::DiscardBackground );
            renderer.setDiscardFlag( QwtPlotRenderer::DiscardCanvasBackground );
            renderer.setDiscardFlag( QwtPlotRenderer::DiscardCanvasFrame );
            renderer.setLayoutFlag( QwtPlotRenderer::FrameWithScales );
        }

        renderer.renderTo( this, printer );
    }
}

void
SpectrogramPlot::handle_signal()
{
    emit dataChanged();
}

void
SpectrogramPlot::setData( std::shared_ptr< adcontrols::MappedImage >& matrix )
{
    drawable_->data_ = matrix;
	spectrogram_->invalidateCache();
	replot();
}
