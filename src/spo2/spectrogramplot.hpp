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

#ifndef SPECTROGRAMPLOT_HPP
#define SPECTROGRAMPLOT_HPP

#include <qwt_plot.h>
#include <memory>

class QwtPlotSpectrogram;

namespace adcontrols { class MappedImage; }

namespace malpix {

    namespace detail { class SpectrogramData; }

    class SpectrogramPlot : public QwtPlot {
        Q_OBJECT
    public:
        explicit SpectrogramPlot(QWidget *parent = 0);

        void setData( std::shared_ptr< adcontrols::MappedImage >& );
        
    signals:
        void dataChanged();
                          
    public slots:
        void handle_showContour( bool on );
        void handle_showSpectrogram( bool on );
        void handle_setAlpha( int );
        void handle_printPlot();
        void handle_dataChanged();
        
    private:
        std::unique_ptr< QwtPlotSpectrogram > spectrogram_;
        detail::SpectrogramData * drawable_;
        void handle_signal();
    };

}

#endif // SPECTROGRAMPLOT_HPP
