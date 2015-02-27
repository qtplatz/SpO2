/**************************************************************************
** Copyright (C) 2014 MS-Cheminformatics LLC, Toin, Mie Japan
*
** Contact: toshi.hondo@qtplatz.com
**
** Commercial Usage
**
** Licensees holding valid MS-Cheminfomatics commercial licenses may use this 
** file in accordance with the MS-Cheminformatics Commercial License Agreement
** provided with the Software or, alternatively, in accordance with the terms 
** contained in a written agreement between you and MS-Cheminformatics.
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

#pragma once

#include <atomic>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <thread>
#include <workaround/boost/asio.hpp>
#include <adportable/asio/thread.hpp>
#include <boost/numeric/ublas/fwd.hpp> // matrix forward decl

namespace malpix {

    class SampleProcessor;
        
    namespace sitcp { class session; }

    enum { malpix_size1 = 16
           , malpix_size2 = 16
           , malpix_size = ( malpix_size1 * malpix_size2)
           , log_size = 21
    };
    
    // singleton
    class malpix4 { 
        ~malpix4();
        malpix4();
        malpix4( const malpix4& ) = delete;
        const malpix4& operator = ( const malpix4& ) = delete;
    public:
#if _MSC_VER <= 1800
        static const double CLKPERIOD;
        static const double VFINEcomp;
#else
        static constexpr double CLKPERIOD = 0.04;
        static constexpr double VFINEcomp = 1.216;
#endif
        static malpix4 * instance();
        inline const sitcp::session * session() const { return session_.get(); }

        bool create_session( const char * host, const char * tcp_port, const char * udp_port );
        bool close_session();
        bool echo( int );
        bool reset();                       // RESET
        bool reset_detector( bool enable ); // RST_DET_EN
        bool trigger( bool on );            // TRIG
        bool da_converter( uint32_t dacdata );
        bool callexit();
        bool delay_time( uint32_t );
        bool update_delay_time( uint8_t );
        bool cumulative_number( uint32_t );
        bool dac_ready( bool );

        bool RESET()                           { return reset(); }
        bool RST_DET_EN( bool enable )         { return reset_detector( enable ); }
        bool DAConverter( uint32_t dac )       { return da_converter( dac ); }

        bool MA( uint32_t );
        bool DelayTime( uint32_t value )       { return delay_time( value ); }
        bool CumNum( uint32_t value )          { return cumulative_number( value ); }
        bool UpdateDelayTime( uint8_t value )  { return update_delay_time( value ); }
        bool DAQ_Ready( bool value )           { return dac_ready( value ); }

        bool VTHCompensate( const boost::numeric::ublas::matrix<uint16_t>&, const boost::numeric::ublas::matrix<uint16_t>& );
        void push_sample( std::shared_ptr< SampleProcessor >& );

        inline boost::asio::io_service& io_service() { return io_service_; }

    private:
        static std::atomic< malpix4 * > instance_;
        static std::mutex mutex_;
        class impl;
        impl * impl_;

        std::shared_ptr< sitcp::session > session_;
        boost::asio::io_service io_service_;
        std::vector< adportable::asio::thread > threads_;

        void vth_compensate_print( std::ostream&, const uint8_t * data, size_t size );
    };

}
