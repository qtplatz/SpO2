/**************************************************************************
** Copyright (C) 2010-2015 Toshinobu Hondo, Ph.D.
** Copyright (C) 2013-2015 MS-Cheminformatics LLC
*
** Contact: toshi.hondo@scienceliaison.com
**
** Commercial Usage
**
** Licensees holding valid MS-Cheminformatics commercial licenses may use this file in
** accordance with the MS-Cheminformatics Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
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

#include "tdcobserver.hpp"

using namespace malpix;
    
TdcObserver::TdcObserver()
{
    desc_.trace_method = so::eTRACE_IMAGE_TDC;
    desc_.spectrometer = so::eMassSpectrometer;
    desc_.trace_id = L"MALPIX4.TDC";  // unique name for the trace, can be used as 'data storage name'
    desc_.trace_display_name = L"MALPIX4 Spectra";
    desc_.axis_x_label = L"Laser shot#";
    desc_.axis_y_label = L"?";
    desc_.axis_x_decimals = 0;
    desc_.axis_y_decimals = 0;
}

TdcObserver::~TdcObserver()
{
}

uint64_t 
TdcObserver::uptime() const 
{
    return 0;
}

void 
TdcObserver::uptime_range( uint64_t& oldest, uint64_t& newest ) const 
{
    oldest = newest = 0;
    
    std::lock_guard< std::mutex > lock( const_cast< TdcObserver *>(this)->mutex_ );

    if ( ! que_.empty() ) {
        oldest = que_.front()->pos;
        newest = que_.back()->pos;
    }
    
}

std::shared_ptr< so::DataReadBuffer >
TdcObserver::readData( uint32_t pos )
{
    std::lock_guard< std::mutex > lock( mutex_ );
    
    if ( que_.empty() )
        return 0;
    
    if ( pos = std::numeric_limits<uint32_t>::max() ) {
        return que_.back();
    }

    auto it = std::find_if( que_.begin(), que_.end(), [pos]( const std::shared_ptr< so::DataReadBuffer >& p ){ return pos == p->pos; } );
    if ( it != que_.end() )
        return *it;
    
    return 0;
}

int32_t
TdcObserver::posFromTime( uint64_t usec ) const 
{
    std::lock_guard< std::mutex > lock( const_cast< TdcObserver *>(this)->mutex_ );
    
    if ( que_.empty() )
        return false;

    auto it = std::lower_bound( que_.begin(), que_.end(), usec
                                , []( const std::shared_ptr< so::DataReadBuffer >& p, uint64_t usec ){ return p->uptime < usec; } );
    if ( it != que_.end() )
        return (*it)->pos;

    return 0;
}

bool 
TdcObserver::readCalibration( int idx, so::octet_array& serialized, std::wstring& dataClass ) const 
{
    return false;
}

//
void
TdcObserver::operator << ( std::shared_ptr< so::DataReadBuffer >& t )
{
    std::lock_guard< std::mutex > lock( mutex_ );

    que_.push_back( t );

    if ( que_.size() > 4096 )
        que_.erase( que_.begin(), que_.begin() + 1024 );
}
