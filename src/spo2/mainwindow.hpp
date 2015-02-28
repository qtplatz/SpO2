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

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <fancymainwindow.h>
#include <QMap>
#include <QPointer>
#include <memory>
#include <chrono>

class Plot;
class Knob;
class WheelBox;
class LCDBox;

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QFontComboBox)
QT_FORWARD_DECLARE_CLASS(QTextEdit)
QT_FORWARD_DECLARE_CLASS(QTextCharFormat)
QT_FORWARD_DECLARE_CLASS(QMenu)
QT_FORWARD_DECLARE_CLASS(QPrinter)
QT_FORWARD_DECLARE_CLASS(QStackedWidget)
QT_FORWARD_DECLARE_CLASS(QDockWidget)

class QStackedWidget;

namespace malpix {
    class document;
    class SpectrogramPlot;
}

namespace Manhattan {
    class StyledBar;
}

class MainWindow : public Manhattan::FancyMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void onInitialUpdate();

    void start();
    double amplitude() const;
    double frequency() const;
    double signalInterval() const;

protected:
    void closeEvent( QCloseEvent * ) override;

private:
    void setupFileActions();
    void setupEditActions();
    void setupTextActions();
    bool load(const QString &f);
    bool maybeSave();
    void setCurrentFileName(const QString &fileName);
    void createDockWidgets();
    QDockWidget * createDockWidget( QWidget *, const QString&, const QString& );
    Manhattan::StyledBar * createMidStyledBar();
    Manhattan::StyledBar * createTopStyledBar();
    // QToolBar * createMidStyledBar();

private slots:
    void fileNew();
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();
    void filePrint();
    void filePrintPreview();
    void filePrintPdf();

    void clipboardDataChanged();
    void about();
    void printPreview(QPrinter *);

    void handleFeatureSelected( int );
    void handleFeatureActivated( int );
    void handleInstState( int );
    void handleData( double );

Q_SIGNALS:
    void amplitudeChanged( double );
    void frequencyChanged( double );
    void signalIntervalChanged( double );

 private:
    QAction *actionSave;
    QToolBar *tb_;
    Knob *d_frequencyKnob;
    Knob *d_amplitudeKnob;
    WheelBox *d_intervalWheel;
    Plot *d_plot1;
    Plot *d_plot2;    
    LCDBox * d_lcd1;
    LCDBox * d_lcd2;
    LCDBox * d_lcd3;
    std::chrono::steady_clock::time_point tp_;
};

#endif // MAINWINDOW_HPP
