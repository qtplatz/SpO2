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

#include "malpix4.hpp"
#include "document.hpp"
#include "masterobserver.hpp"
#include "tdcobserver.hpp"
#include "sampleprocessor.hpp"
#include <mpxcontrols/dataframe.hpp>
#include <mpxcontrols/scanlaw.hpp>
#include <sitcp/bcp_header.hpp>
#include <sitcp/message_serializer.hpp>
#include <sitcp/session.hpp>
#include <adcontrols/mappedspectrum.hpp>
#include <adcontrols/mappedspectra.hpp>
#include <adcontrols/mappedimage.hpp>
#include <adcontrols/massspectrum.hpp>
#include <adcontrols/metric/prefix.hpp>
#include <adcontrols/scanlaw.hpp>
#include <adcontrols/trace.hpp>
#include <adinterface/signalobserver.hpp>
#include <adportable/debug.hpp>
#include <adportable/scoped_debug.hpp>
#include <adportable/profile.hpp>
#include <adportable/date_string.hpp>
#include <boost/exception/all.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <memory>
#include <deque>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <utility>

namespace malpix {

    class malpix4::impl {
        
        boost::asio::io_service& io_service_;

        std::shared_ptr< signalobserver::DataReadBuffer > dframe_;
        std::shared_ptr< adcontrols::MappedSpectra > mappedSpectra_;
        std::shared_ptr< adcontrols::MappedImage > mappedTic_;
        std::shared_ptr< adcontrols::Trace > trace_;
        std::shared_ptr< adcontrols::MassSpectrum > spCoadded_;
        std::shared_ptr< adcontrols::Trace > timedTrace_;
        std::shared_ptr< adcontrols::ScanLaw > law_;
        std::chrono::system_clock::time_point tp_;
        std::chrono::system_clock::time_point last_logout_;
		std::string txtfile_;
        uint32_t npos_; 
        std::shared_ptr< signalobserver::Observer > masterObserver_;
    public:

        std::deque< std::shared_ptr< SampleProcessor > > sampleProcessors_;
    public:
        
        impl( boost::asio::io_service& io )
            : io_service_( io )
            , mappedSpectra_( std::make_shared< adcontrols::MappedSpectra >( malpix_size1, malpix_size2 ) )
            , mappedTic_( std::make_shared< adcontrols::MappedImage >( malpix_size1, malpix_size2 ) )
            , npos_(0)
            , spCoadded_( std::make_shared< adcontrols::MassSpectrum >() )
            , timedTrace_( std::make_shared< adcontrols::Trace >() ) 
            , masterObserver_( std::make_shared< MasterObserver >() ) {             

            tp_ = std::chrono::system_clock::now();
			std::string date = adportable::date_string::logformat(tp_, true);
			std::transform(date.begin(), date.end(), date.begin(), [](char c){ return c == ':' ? '-' : c; });
			boost::filesystem::path path(adportable::profile::user_data_dir<char>() + "/data/malpix4_" + date);
            path.replace_extension( ".txt" );
            txtfile_ = path.string();
			std::ofstream of(txtfile_.c_str());
			if (!of.fail())
				of << std::endl;

            if ( auto so = std::make_shared< TdcObserver >() ) {
                so->assign_objId( 1 );
                masterObserver_->addSibling( so.get() );
            }

        }
        void response_handler( boost::asio::streambuf&, size_t );
        void handle_data( uint32_t );
        void average( std::shared_ptr< mpxcontrols::dataFrame >& );
    };
}

using namespace malpix;

std::atomic< malpix4 * > malpix4::instance_(0);
std::mutex malpix4::mutex_;

#if _MSC_VER <= 1800
const double malpix4::CLKPERIOD = 0.04;
const double malpix4::VFINEcomp = 1.216;
#endif


malpix4::~malpix4()
{
    close_session();
}

malpix4::malpix4() : impl_( new impl( io_service_ ) )
{
}

malpix4 * 
malpix4::instance()
{
    // double checked lock pattern
    typedef malpix4 T;
    
    T * tmp = instance_.load( std::memory_order_relaxed );
    std::atomic_thread_fence( std::memory_order_acquire );
    if ( tmp == nullptr ) {
        std::lock_guard< std::mutex > lock( mutex_ );
        tmp = instance_.load( std::memory_order_relaxed );
        if ( tmp == nullptr ) {
            tmp = new T();
            std::atomic_thread_fence( std::memory_order_release );
            instance_.store( tmp, std::memory_order_relaxed );
        }
    }
    return tmp;
}

bool
malpix4::create_session( const char * host, const char * tcp_port, const char * udp_port )
{
    std::lock_guard< std::mutex > lock( mutex_ );
    if ( session_ )
        return false;

    session_ = std::make_shared< sitcp::session >( io_service_, host, tcp_port, udp_port );

    session_->register_response_handler( [this]( boost::asio::streambuf& sbuf, size_t size ){
            impl_->response_handler( sbuf, size );
        });

    threads_.push_back( adportable::asio::thread( [=] { io_service_.run(); } ) );
    threads_.push_back( adportable::asio::thread( [=] { io_service_.run(); } ) );

    return true;
}

bool
malpix4::close_session()
{
    adportable::scoped_debug<adportable::debug> scope(__FILE__, __LINE__); scope << "close session;";

    std::lock_guard< std::mutex > lock( mutex_ );
    if ( session_ ) {
        io_service_.stop();
        for ( auto& t: threads_ )
            t.join();
        session_.reset();
    }
    return true;
}

bool
malpix4::echo( int x )
{
    sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x01, 0x10 ) );
    
    serializer << int8_t(x);

    session_->send_to( serializer.data(), serializer.length() );
    if ( session_->block_ack_wait() )
        return true;
    return false;
}

bool
malpix4::reset()
{
    ADDEBUG() << "Reset USER_FPGA";
    if ( session_ ) {
        //RBCPskeleton(0xff, 0x80, 0x00, 0x01, 0x10);
        sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x00, 0x10 ) );

        serializer << int8_t( 1 );

        session_->send_to( serializer.data(), serializer.length() );
        if ( session_->block_ack_wait() )
            return true;
    }
    else {
        ADDEBUG() << "Has no session, connect malpix4 [24 [4660] first!!!";
    }

    return false;  // failed
}

bool
malpix4::reset_detector( bool enable ) // RST_DET_EN
{
    if ( session_ ) {
        sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x00, 0x04 ) );
        serializer << int8_t( enable ? 1 : 0 );

        session_->send_to( serializer.data(), serializer.length() );
        if ( session_->block_ack_wait() )
            return true;
    } 
    else {
        ADDEBUG() << "Has no session, connect malpix4 [24 [4660] first!!!";
    }
    return false;
}

bool
malpix4::trigger( bool on )            // TRIG
{
    if ( session_ ) {
        //RBCPskeleton(0xff, 0x80, 0x00, 0x01, 0x03);
        sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x00, 0x03 ) );
        serializer << int8_t( on ? 1 : 0 );

        session_->send_to( serializer.data(), serializer.length() );
        if ( session_->block_ack_wait() )
            return true;
    }
    else {
        ADDEBUG() << "Has no session, connect malpix4 [24 [4660] first!!!";
    }

    return false;  // failed
}

bool
malpix4::da_converter( uint32_t dac )
{
    if ( !session_ ) {
        ADDEBUG() << "Has no session, connect malpix4 [24 [4660] first!!!";
        return false;
    }
        
    if( ( ( dac >> 12 ) &0xf ) == 1 ) {
        ADDEBUG() << boost::format( "VRST_DET = %f V ( dac = 0x%x )" ) % ( double(dac & 0xfff) / 4096 * 3.0 ) % dac;
    }
    else if ( ( ( dac >> 12 ) & 0xf ) == 5 ) {
        ADDEBUG() << boost::format( "VTH = %f ( dac = 0x%x )" ) % ( double(dac & 0xfff)/4096*3.0 ) % dac;
    }
    else {
        ADDEBUG() << boost::format( "VFINE = %f ( dac = 0x%x) " ) % ( double(dac & 0xfff)/4096*3.0 ) % dac;
    }
        
    // RBCPskeleton(0xff, 0x80, 0x00, 0x02, 0x0a);
    sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x00, 0x000a ) );
    
    serializer.append( uint16_t( dac ), sitcp::little_endian );
    session_->send_to( serializer.data(), serializer.length() );
    if ( session_->block_ack_wait() ) {
        return true;
    }

    return false;  // failed    
}

bool
malpix4::callexit()
{
    if ( session_ ) {
        da_converter( 0 + (9<<12) ); // Update VoutC
        da_converter( 0 + (5<<12) ); // Update VoutB
        da_converter( 0 + (1<<12) ); // Update VoutA
        reset();
    }
    return true;
}

bool
malpix4::MA( uint32_t ma )
{
    ADDEBUG() << "Selected REQ/ACK[" << (3+4*ma) << ":" << 4*ma << "]";

    if ( !session_ ) {
        ADDEBUG() << "Has no session, connect malpix4 [24 [4660] first!!!";
        return false;
    }

    sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x01, 0x0001 ) );
    serializer << int8_t(ma);

    session_->send_to( serializer.data(), serializer.length() );
    if ( ! session_->block_ack_wait() )
        return false;

    std::this_thread::sleep_for( std::chrono::microseconds( 1 ) );

    return true;
}

bool
malpix4::delay_time( uint32_t delaytime )
{
    std::cout << "Delay for " << (double)delaytime*CLKPERIOD << " us (" << delaytime << " CLK) after Trigger." << std::endl;

    if ( !session_ ) {
        ADDEBUG() << "Has no session, connect malpix4 [24 [4660] first!!!";
        return false;
    }
    
    sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x01, 0x0005 ) );
    serializer.append( int16_t( delaytime ), sitcp::little_endian );

    session_->send_to( serializer.data(), serializer.length() );
    if ( ! session_->block_ack_wait() )
        return false;

    std::this_thread::sleep_for( std::chrono::microseconds( 1 ) );

    return true;
}

bool
malpix4::cumulative_number( uint32_t num )
{
    if ( !session_ ) {
        ADDEBUG() << "Has no session, connect malpix4 [24 [4660] first!!!";
        return false;
    }

    if( num == 0 ) {
        ADDEBUG() << "Update delaytime Unenable.";

        sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x00, 0x0007 ) );
        serializer.append( int16_t( 0 ), sitcp::little_endian );

        session_->send_to( serializer.data(), serializer.length() );
        if ( ! session_->block_ack_wait() )
            return false;

        std::this_thread::sleep_for( std::chrono::microseconds( 1 ) );
        return true;
    }
    else {
        std::cout << "Cummulate Number is " << num << " until updating delaytime" << std::endl;
        
        sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x00, 0x0007 ) );
        serializer.append( int16_t( num ), sitcp::little_endian );

        session_->send_to( serializer.data(), serializer.length() );
        if ( ! session_->block_ack_wait() )
            return false;

        std::this_thread::sleep_for( std::chrono::microseconds( 1 ) );
        return true;
    }
}

bool
malpix4::update_delay_time( uint8_t num )
{
    if ( !session_ ) {
        ADDEBUG() << "Has no session, connect malpix4 [24 [4660] first!!!";
        return false;
    }

    std::cout << "Update delaytime is " << (int)(num+1) << " clock (" << (double)(num+1)*CLKPERIOD << " us)"<< std::endl;

    sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x00, 0x0009 ) );
    serializer.append( num );

    session_->send_to( serializer.data(), serializer.length() );
    if ( ! session_->block_ack_wait() )
        return false;
    
    std::this_thread::sleep_for( std::chrono::microseconds( 1 ) );

    return true;
}

bool
malpix4::dac_ready( bool start )
{
    if ( !session_ ) {
        ADDEBUG() << "Has no session, connect malpix4 [24 [4660] first!!!";
        return false;
    }

    if (start)
        ADDEBUG() << "DAC Start!";
    else
        ADDEBUG() << "DAC Stop!";
    
    sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x00, 0x0000 ) );
    serializer.append( uint8_t( start ? 1 : 0 ) );

    session_->send_to( serializer.data(), serializer.length() );
    if ( ! session_->block_ack_wait() )
        return false;
    
    std::this_thread::sleep_for( std::chrono::microseconds( 1 ) );

    return true;
}

bool
malpix4::VTHCompensate( const boost::numeric::ublas::matrix<uint16_t>& enables
                        , const boost::numeric::ublas::matrix<uint16_t>& vthcomp )
{
    if ( !session_ ) {
        ADDEBUG() << "Has no session, connect malpix4 [24 [4660] first!!!";
        return false;
    }

    std::vector< uint8_t > data( vthcomp.size1() * vthcomp.size2() );

    for ( size_t i = 0; i < vthcomp.size1(); ++i ) {

        for ( size_t j = 0; j < vthcomp.size2(); j += 2 ) {

            if ( ( i % 2 ) == 0 ) {

                data[8 * i + j / 2] = 0;
                data[8 * i + j / 2]  += ( vthcomp(15-i, j     ) & 1 ) << 7;
                data[8 * i + j / 2]  += ( vthcomp(15-i, j     ) & 2 ) << 5;
                data[8 * i + j / 2]  += ( vthcomp(15-i, j     ) & 4 ) << 3;
                data[8 * i + j / 2]  += ( enables(15-i, j     ) & 1 ) << 4;

                data[8 * i + j / 2]  += ( vthcomp(15-i, j + 1 ) & 1 ) << 3;
                data[8 * i + j / 2]  += ( vthcomp(15-i, j + 1 ) & 2 ) << 1;
                data[8 * i + j / 2]  += ( vthcomp(15-i, j + 1 ) & 4 ) >> 1;
                data[8 * i + j / 2]  += ( enables(15-i, j + 1 ) & 1 );
                
            } else {

                data[8 * i + j / 2] = 0;
                data[8 * i + j / 2]  += ( enables(15-i, 15 - j     ) & 1 ) << 7;
                data[8 * i + j / 2]  += ( vthcomp(15-i, 15 - j     ) & 7 ) << 4;
                data[8 * i + j / 2]  += ( enables(15-i, 15 - j - 1 ) & 1 ) << 3;
                data[8 * i + j / 2]  += ( vthcomp(15-i, 15 - j - 1 ) & 7 );
                
            }
        }
    }

    vth_compensate_print( std::cout, data.data(), data.size() );

    do {
        // RBCPskeleton(0xff, 0x80, 0x00, 0x80, 0x100);
        sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x00, 0x100 ) );    

        serializer.append( data.data(), 128 );
    
        session_->send_to( serializer.data(), serializer.length() );
        if ( ! session_->block_ack_wait() )
            return false;
    } while (0);

    std::this_thread::sleep_for( std::chrono::microseconds( 1 ) );
    
    std::cout << "Shift Register on!\n";
    do {
        // RBCPskeleton(0xff, 0x80, 0x00, 0x01, 0x180);
        sitcp::message_serializer serializer( sitcp::bcp_header( 0xff, 0x80, 0x00, 0x00, 0x180 ) );    
        serializer << uint8_t( 1 );

        session_->send_to( serializer.data(), serializer.length() );
        if ( ! session_->block_ack_wait() )
            return false;
    } while ( 0 );

    std::this_thread::sleep_for( std::chrono::microseconds( 1 ) );

    return true;
}

void
malpix4::vth_compensate_print( std::ostream& os, const uint8_t * data, size_t size )
{
    // unsigned char compMSB, compLSB;
    os << "VTH Compensation data:\n";
    os << std::hex;
    os << "     ";
    for(int i = 0; i < 16; ++i)
        os << "[" << i << "]";
    os << std::endl;

    for(int i = 0; i < 16;  ++i) {

        os << "[" << i << "]: ";

        if( ( i%2 ) ==0 ) {
            for ( int j = 0; j < 16; j += 2 ) {
                uint8_t compLSB = data[8 * (15-i) + (14-j) / 2] & 0x0f;
                uint8_t compMSB = ( data[8 * (15-i) + (14-j) / 2] & 0xf0 ) >> 4;
                
                if ( compLSB >> 3 )
                    os << " D ";
                else
                    os << ' ' << (compLSB&7) << ' ';
                
                if( compMSB >> 3 )
                    os << " D ";
                else
                    os << ' ' << (compMSB&7) << ' ';
            }
        } else {
            for ( int j = 0; j < 16; j += 2 ) {
                uint8_t compLSB = data[8 * (15-i) + j / 2] & 0x0f;
                uint8_t compMSB = (data[8 * (15-i) + j / 2] & 0xf0) >> 4;
                
                if(compMSB&1)
                    os << " D ";
                else
                    os << ' ' << ((compMSB&2)<<1) + ((compMSB&4)>>1) + ((compMSB&8)>>3) << ' ';
                
                if(compLSB&1)
                    os << " D ";
                else
                    os << ' ' << ((compLSB&2)<<1) + ((compLSB&4)>>1) + ((compLSB&8)>>3) << ' ';
            }
        }
        os << std::endl;
    }
    os << std::endl;
    os << std::dec;
}

void
malpix4::impl::response_handler( boost::asio::streambuf& response, size_t size )
{
    std::cout << response.size() << ", " << size << std::endl;

    std::lock_guard< std::mutex > lock( mutex_ );

    while ( response.size() >= log_size ) {

        if ( ! dframe_ ) {

            if ( response.size() >= malpix_size * sizeof(uint16_t) ) {

                const uint8_t * bp = boost::asio::buffer_cast<const uint8_t *>(response.data());                
                dframe_ = std::make_shared< signalobserver::DataReadBuffer >();
                dframe_->pos = npos_++;
                dframe_->uptime =
                    std::chrono::duration_cast< std::chrono::microseconds >( std::chrono::system_clock::now() - tp_ ).count();
                dframe_->xdata.resize( malpix_size * sizeof(uint16_t) );
                std::copy( bp, bp + malpix_size * sizeof(uint16_t), dframe_->xdata.begin() );
                response.consume( malpix_size * sizeof( uint16_t ) );

            } else {

                // invalid data frame align found -- attempt to recover ( maybe fpga reset required )
                response.consume( response.size() ); // discard all data in buffer
                return;
                
            }

        } else {

            if ( response.size() >= log_size ) {

                const uint8_t * bp = boost::asio::buffer_cast<const uint8_t *>( response.data() );
                dframe_->xmeta.resize( log_size );
                std::copy( bp, bp + log_size, dframe_->xmeta.begin() );

                response.consume( log_size );

                if ( auto so = masterObserver_->findObserver( 1, true ) ) {
                    try {
                        auto& obj = dynamic_cast<TdcObserver&>( *so );
                        obj << dframe_;
                    } catch ( std::exception& ex ) {
                        std::cerr << boost::diagnostic_information( ex );
                    }
                }
                
                auto tdc = reinterpret_cast< const uint16_t * >( dframe_->xdata.data() );
                size_t tic = std::accumulate( tdc, tdc + malpix_size, 0, []( uint16_t a, uint16_t b ){ return b ? a + 1 : a; } );
                std::cout << "TIC: " << tic << std::endl;
                auto pos = dframe_->pos;
                io_service_.post( [this, pos] () { handle_data( pos ); } );
                dframe_ = 0;
            }
        }
    }
}

void
malpix4::impl::handle_data( uint32_t pos )
{
    try {
        if ( auto so = masterObserver_->findObserver( 1, true ) ) {
            
            if ( auto data = so->readData( pos ) ) {
                
                auto tdc = std::make_shared< mpxcontrols::dataFrame >( malpix_size1, malpix_size2
                                                                       , reinterpret_cast< const uint16_t *>(data->xdata.data())
                                                                       , data->xdata.size() / sizeof( uint16_t )
                                                                       , data->xmeta.data()
                                                                       , data->xmeta.size() );
                
                auto tp = std::chrono::system_clock::now();
                
                std::ofstream of ( txtfile_.c_str(), std::ios_base::out | std::ios_base::app );
                of << static_cast< const boost::numeric::ublas::matrix<uint16_t>& >(*tdc) << std::endl;
                std::for_each( data->xmeta.begin(), data->xmeta.end(), [&of](uint8_t d){ of << std::hex << int(d) << ","; });
                of << std::endl;

                if ( std::chrono::duration_cast< std::chrono::milliseconds >( tp - last_logout_ ).count() > 3000 ) {
                    std::cout << static_cast< const boost::numeric::ublas::matrix<uint16_t>& >(*tdc) << std::endl;
                    std::cout << "log: ";
                    std::for_each( data->xmeta.begin(), data->xmeta.end(), [](uint8_t d){
                            std::cout << boost::format( "%x," ) % int(d);
                        });
                    std::cout << "\t" << adportable::date_string::logformat( tdc->time_point(), true ) << std::endl;
                    last_logout_ = tp;
                }

                average( tdc );

                std::lock_guard< std::mutex > lock( mutex_ );
                if ( !sampleProcessors_.empty() ) {
                    sampleProcessors_.front()->handle_data( 1, pos, *data );
                }
            }
        }
    } catch ( ... ) {
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
    }
}

void
malpix4::impl::average( std::shared_ptr< mpxcontrols::dataFrame >& ptr )
{
    mappedTic_->merge( *ptr );
    (*mappedSpectra_) += *ptr;

    std::cout << static_cast< const boost::numeric::ublas::matrix<double>& >(*mappedTic_) << std::endl;

    malpix::ScanLaw scanLaw;
    if ( auto ms = std::make_shared< adcontrols::MassSpectrum >() ) {

        adcontrols::MappedSpectrum sp;
        
        for ( int i = 0; i < ptr->size1(); ++i ) {
            for ( int j = 0; j < ptr->size2(); ++j ) {
                if ( auto tof = ( *ptr )( i, j ) ) {
                    sp << std::make_pair( tof, 1 );
                }
            }
        }

        if ( sp.size() ) {
            ms->resize( sp.size() );
            ms->setCentroid( adcontrols::CentroidNative );
            ms->setAcquisitionMassRange( 1, 10000 );
            for ( size_t idx = 0; idx < sp.size(); ++idx ) {
                double tof = adcontrols::metric::scale_to_base( double( sp[ idx ].first ), adcontrols::metric::nano );
                ms->setTime( idx, tof );
                ms->setMass( idx, sp[ idx ].first ); //.getMass( tof, 0 ) );
                ms->setIntensity( idx, sp[ idx ].second );
            }
            mpx4::document::instance()->setData( ms );
        }
    }

    std::lock_guard< std::mutex > lock( mutex_ );
    mpx4::document::instance()->setData( mappedTic_ );        
    mpx4::document::instance()->setData( mappedSpectra_ );
}

void
malpix4::push_sample( std::shared_ptr< SampleProcessor >& p )
{
    std::lock_guard< std::mutex > lock( mutex_ );
    if ( !impl_->sampleProcessors_.empty() ) {
        auto cur = impl_->sampleProcessors_.front();
        impl_->sampleProcessors_.pop_front();
    }
    impl_->sampleProcessors_.push_back( p );
    p->prepare_storage();
}
