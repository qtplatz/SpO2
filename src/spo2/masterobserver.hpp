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

#pragma once

#include <adinterface/signalobserver.hpp>

namespace malpix {
    
    namespace so = signalobserver;
    class MasterObserver : public so::Observer {
    public:
        MasterObserver() {}
        virtual ~MasterObserver() {}
        
        uint64_t uptime() const { return 0; }
        void uptime_range( uint64_t& oldest, uint64_t& newest ) const { oldest = newest = 0; }
        std::shared_ptr< so::DataReadBuffer > readData( uint32_t pos ){ return false; }
        const wchar_t * dataInterpreterClsid() const                  { return L""; }
        int32_t posFromTime( uint64_t usec ) const                    { return 0; }
        bool readCalibration( int idx, so::octet_array& serialized, std::wstring& dataClass ) const { return false; }
    };
}
