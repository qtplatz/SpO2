/**************************************************************************
** Copyright (C) 2014-2015 MS-Cheminformatics LLC, Toin, Mie Japan
** Author: Toshinobu Hondo, Ph.D.
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

#include "mpx4wnd.hpp"
#include "document.hpp"
#include "spectrogramplot.hpp"
#include <minisplitter.h>
#include <adplot/plotcurve.hpp>
#include <adplot/chromatogramwidget.hpp>
#include <adplot/spectrumwidget.hpp>
#include <QBoxLayout>

using namespace malpix;

namespace malpix {
    
    class Mpx4Wnd::impl {
    public:
        impl() : spectrogram_plot_(0)
               , timed_plot_(0)
               , spectrum_plot_(0)
            {}
        SpectrogramPlot * spectrogram_plot_;
        adplot::ChromatogramWidget * timed_plot_;
        adplot::SpectrumWidget * spectrum_plot_;
    };
}

Mpx4Wnd::~Mpx4Wnd()
{
    delete impl_;
}

Mpx4Wnd::Mpx4Wnd( QWidget * parent ) : QWidget( parent )
                                           , impl_( new impl )
{
    if ( auto splitter = new Manhattan::MiniSplitter ) {

        if ( auto splitter2 = new Manhattan::MiniSplitter ) {

            if ( (impl_->timed_plot_ = new adplot::ChromatogramWidget( this )) ) {
                impl_->timed_plot_->setMinimumHeight( 80 );
            }

            if ( (impl_->spectrum_plot_ = new adplot::SpectrumWidget( this )) ) {
                impl_->spectrum_plot_->setMinimumHeight( 80 );
            }

            splitter2->addWidget( impl_->timed_plot_ );
            splitter2->addWidget( impl_->spectrum_plot_ );
            splitter2->setOrientation( Qt::Vertical );

            splitter->addWidget( splitter2 );
        }

		impl_->spectrogram_plot_ = 0;
        splitter->setOrientation( Qt::Horizontal );

        auto layout = new QVBoxLayout( this );
        layout->setMargin( 0 );
        layout->setSpacing( 0 );
        layout->addWidget( splitter );
    }
    bool res = connect( mpx4::document::instance(), &mpx4::document::dataChanged, this, &Mpx4Wnd::dataChanged );
}

void
Mpx4Wnd::dataChanged( int hint )
{
}
