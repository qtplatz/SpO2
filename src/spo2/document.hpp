/**************************************************************************
** Copyright (C) 2010-2014 Toshinobu Hondo, Ph.D.
** Copyright (C) 2013-2014 MS-Cheminformatics LLC, Toin, Mie Japan
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

#if !defined malpix4_malpix_mpx4_document
#define malpix4_malpix_mpx4_document

#include <QObject>
#include <boost/numeric/ublas/fwd.hpp>
#include <boost/msm/back/state_machine.hpp>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

class QSettings;

namespace malpix {

    namespace mpx4 {

        class document : public QObject {
            Q_OBJECT
            document( const document& ) = delete;
            document( QObject * parent = 0 );
            static std::atomic< document * > instance_;
            static std::mutex mutex_;

        public:
            static document * instance();

            enum dataHint { eVthComp = 1, eSpectrum, eSpectrogram };

            void initialSetup();
            void finalClose();
            QSettings * settings();

            // action for automaton
            void handleResetFpga();
            void handleOnOff();
            void handleRun();

            void fileNew();

        signals:
            void dataChanged( int );
            void instStateChanged( int );
        
        private:
            class impl;
            impl * impl_;
        };
    }
}

#endif

