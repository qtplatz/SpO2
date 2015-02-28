/**************************************************************************
** Copyright (C) 2010-2015 Toshinobu Hondo, Ph.D.
** Copyright (C) 2013-2015 MS-Cheminformatics LLC, Toin, Mie Japan
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

#include "lcdbox.hpp"

#include <QLabel>
#include <QLCDNumber>
#include <QGridLayout>

LCDBox::LCDBox( const QString& title
                , QGridLayout * layout
                , int row ) : d_number( new QLCDNumber( this ) )
                            , d_label( new QLabel( title, this ) )
{
    d_number->setSegmentStyle( QLCDNumber::Filled );
    d_number->setAutoFillBackground( true );
    d_number->setFixedHeight( d_number->sizeHint().height() * 2 );
    QPalette pal( Qt::black );
    pal.setColor( QPalette::WindowText, Qt::green );
    d_number->setPalette( pal );

    QFont font( "Helvetica", 10 );
    font.setBold( true );
    d_label->setFont( font );

    layout->addWidget( d_number, row, 0 );
    layout->addWidget( d_label, row, 1, Qt::AlignTop | Qt::AlignHCenter );
}

void
LCDBox::setValue( double value )
{
    d_number->display( value );
}

