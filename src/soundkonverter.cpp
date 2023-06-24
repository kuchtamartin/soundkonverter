/*
 * soundkonverter.cpp
 *
 * Copyright (C) 2007 Daniel Faust <hessijames@gmail.com>
 */
#include "soundkonverter.h"
#include "soundkonverterview.h"
#include "global.h"
#include "config.h"
#include "configdialog/configdialog.h"
#include "logger.h"
#include "logviewer.h"
#include "replaygainscanner/replaygainscanner.h"
#include "aboutplugins.h"

#include <taglib.h>

#include <KActionCollection>
#include <KActionMenu>
#include <KLocale>
#include <KToolBar>
#include <KIcon>
#include <KStandardDirs>
#include <KMenu>
#include <KMessageBox>
#include <QDir>
#include <QApplication>

#include <KStatusNotifierItem>

soundKonverter::soundKonverter()
    : KXmlGuiWindow(),
      logViewer( 0 ),
      systemTray( 0 ),
      autoclose( false )
{
    // accept dnd
    setAcceptDrops(true);

    const int fontHeight = QFontMetrics(QApplication::font()).boundingRect("M").size().height();

    logger = new Logger( this );
    logger->log( 1000, i18n("This is soundKonverter %1",*SOUNDKONVERTER_VERSION_STRING) );

    logger->log( 1000, "\n" + i18n("Compiled with TagLib %1.%2.%3",TAGLIB_MAJOR_VERSION,TAGLIB_MINOR_VERSION,TAGLIB_PATCH_VERSION) );

    config = new Config( logger, this );
    config->load();

    m_view = new soundKonverterView( logger, config, cdManager, this );
    connect( m_view, SIGNAL(signalConversionStarted()), this, SLOT(conversionStarted()) );
    connect( m_view, SIGNAL(signalConversionStopped(bool)), this, SLOT(conversionStopped(bool)) );
    connect( m_view, SIGNAL(progressChanged(const QString&)), this, SLOT(progressChanged(const QString&)) );
    connect( m_view, SIGNAL(showLog(int)), this, SLOT(showLogViewer(int)) );

    // tell the KXmlGuiWindow that this is indeed the main widget
    setCentralWidget( m_view );

    // then, setup our actions
    setupActions();

    // a call to KXmlGuiWindow::setupGUI() populates the GUI
    // with actions, using KXMLGUI.
    // It also applies the saved mainwindow settings, if any, and ask the
    // mainwindow to automatically save settings if changed: window size,
    // toolbar position, icon size, etc.
    setupGUI( QSize(70*fontHeight,45*fontHeight), ToolBar | Keys | Save | Create );
}

soundKonverter::~soundKonverter()
{
    if( logViewer )
        delete logViewer;

    if( replayGainScanner )
        delete replayGainScanner.data();

    if( systemTray )
        delete systemTray;
}

void soundKonverter::saveProperties( KConfigGroup& configGroup )
{
    Q_UNUSED(configGroup)

    m_view->killConversion();

    m_view->saveFileList( false );
}

void soundKonverter::showSystemTray()
{
    systemTray = new KStatusNotifierItem( this );
    systemTray->setCategory( KStatusNotifierItem::ApplicationStatus );
    systemTray->setStatus( KStatusNotifierItem::Active );
    systemTray->setIconByName( "soundkonverter" );
    systemTray->setToolTip( "soundkonverter", i18n("Waiting"), "" );
}

void soundKonverter::addConvertFiles( const KUrl::List& urls, const QString& profile, const QString& format, const QString& directory, const QString& notifyCommand )
{
    m_view->addConvertFiles( urls, profile, format, directory, notifyCommand );
}

void soundKonverter::addReplayGainFiles( const KUrl::List& urls )
{
    showReplayGainScanner();
    replayGainScanner.data()->addFiles( urls );
}

bool soundKonverter::ripCd( const QString& device, const QString& profile, const QString& format, const QString& directory, const QString& notifyCommand )
{
    return m_view->showCdDialog( device != "auto" ? device : "", profile, format, directory, notifyCommand );
}

void soundKonverter::setupActions()
{
    KStandardAction::quit( this, SLOT(close()), actionCollection() );
    KStandardAction::preferences( this, SLOT(showConfigDialog()), actionCollection() );

    QAction *logviewer = actionCollection()->addAction("logviewer");
    logviewer->setText(i18n("View logs..."));
    logviewer->setIcon(KIcon("view-list-text"));
    connect( logviewer, SIGNAL(triggered()), this, SLOT(showLogViewer()) );

    QAction *replaygainscanner = actionCollection()->addAction("replaygainscanner");
    replaygainscanner->setText(i18n("Replay Gain tool..."));
    replaygainscanner->setIcon(KIcon("soundkonverter-replaygain"));
    connect( replaygainscanner, SIGNAL(triggered()), this, SLOT(showReplayGainScanner()) );

    QAction *aboutplugins = actionCollection()->addAction("aboutplugins");
    aboutplugins->setText(i18n("About plugins..."));
    aboutplugins->setIcon(KIcon("preferences-plugin"));
    connect( aboutplugins, SIGNAL(triggered()), this, SLOT(showAboutPlugins()) );

    QAction *add_files = actionCollection()->addAction("add_files");
    add_files->setText(i18n("Add files..."));
    add_files->setIcon(KIcon("audio-x-generic"));
    connect( add_files, SIGNAL(triggered()), m_view, SLOT(showFileDialog()) );

    QAction *add_folder = actionCollection()->addAction("add_folder");
    add_folder->setText(i18n("Add folder..."));
    add_folder->setIcon(KIcon("folder"));
    connect( add_folder, SIGNAL(triggered()), m_view, SLOT(showDirDialog()) );

    QAction *add_audiocd = actionCollection()->addAction("add_audiocd");
    add_audiocd->setText(i18n("Add CD tracks..."));
    add_audiocd->setIcon(KIcon("media-optical-audio"));
    connect( add_audiocd, SIGNAL(triggered()), m_view, SLOT(showCdDialog()) );

    QAction *add_url = actionCollection()->addAction("add_url");
    add_url->setText(i18n("Add url..."));
    add_url->setIcon(KIcon("network-workgroup"));
    connect( add_url, SIGNAL(triggered()), m_view, SLOT(showUrlDialog()) );

    QAction *add_playlist = actionCollection()->addAction("add_playlist");
    add_playlist->setText(i18n("Add playlist..."));
    add_playlist->setIcon(KIcon("view-media-playlist"));
    connect( add_playlist, SIGNAL(triggered()), m_view, SLOT(showPlaylistDialog()) );

    QAction *load = actionCollection()->addAction("load");
    load->setText(i18n("Load file list"));
    load->setIcon(KIcon("document-open"));
    connect( load, SIGNAL(triggered()), m_view, SLOT(loadFileList()) );

    QAction *save = actionCollection()->addAction("save");
    save->setText(i18n("Save file list"));
    save->setIcon(KIcon("document-save"));
    connect( save, SIGNAL(triggered()), m_view, SLOT(saveFileList()) );

    actionCollection()->addAction("start", m_view->start());
    actionCollection()->addAction("stop_menu", m_view->stopMenu());
}

void soundKonverter::showConfigDialog()
{
    ConfigDialog *dialog = new ConfigDialog( config, this/*, ConfigDialog::Page(configStartPage)*/ );
    connect( dialog, SIGNAL(updateFileList()), m_view, SLOT(updateFileList()) );

    dialog->resize( size() );
    dialog->exec();

    delete dialog;
}

void soundKonverter::showLogViewer( const int logId )
{
    if( !logViewer )
        logViewer = new LogViewer( logger, 0 );

    if( logId )
        logViewer->showLog( logId );

    logViewer->show();
    logViewer->raise();
}

void soundKonverter::showReplayGainScanner()
{
    if( !replayGainScanner )
    {
        replayGainScanner = new ReplayGainScanner( config, logger, !isVisible(), 0 );
        connect( replayGainScanner.data(), SIGNAL(finished()), this, SLOT(replayGainScannerClosed()) );
        connect( replayGainScanner.data(), SIGNAL(showMainWindow()), this, SLOT(showMainWindow()) );
    }

    replayGainScanner.data()->setAttribute( Qt::WA_DeleteOnClose );

    replayGainScanner.data()->show();
    replayGainScanner.data()->raise();
    replayGainScanner.data()->activateWindow();
}

void soundKonverter::replayGainScannerClosed()
{
    if( !isVisible() )
        qApp->quit();
}

void soundKonverter::showMainWindow()
{
    show();
}

void soundKonverter::showAboutPlugins()
{
    AboutPlugins *dialog = new AboutPlugins( config, this );
    dialog->exec();
    dialog->deleteLater();
}

void soundKonverter::startConversion()
{
    m_view->startConversion();
}

void soundKonverter::loadAutosaveFileList()
{
    m_view->loadAutosaveFileList();
}

void soundKonverter::loadFileList(const QString& fileListPath)
{
    m_view->loadFileList(fileListPath);
}

void soundKonverter::startupChecks()
{
    // check if codec plugins could be loaded
    if( config->pluginLoader()->getAllCodecPlugins().count() == 0 )
    {
        KMessageBox::error(this, i18n("No codec plugins could be loaded. Without codec plugins soundKonverter can't work.\nThis problem can have two causes:\n1. You just installed soundKonverter and the KDE System Configuration Cache is not up-to-date, yet.\nIn this case, run kbuildsycoca4 and restart soundKonverter to fix the problem.\n2. Your installation is broken.\nIn this case try reinstalling soundKonverter."));
    }

    // remove old KDE4 action menus created by soundKonverter 0.3 - don't change the paths, it's what soundKonverter 0.3 used
    if( config->data.app.configVersion < 1001 )
    {
        if( QFile::exists(QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/convert_with_soundkonverter.desktop") )
        {
            QFile::remove(QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/convert_with_soundkonverter.desktop");
            logger->log( 1000, i18n("Removing old file: %1",QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/convert_with_soundkonverter.desktop") );
        }
        if( QFile::exists(QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/add_replaygain_with_soundkonverter.desktop") )
        {
            QFile::remove(QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/add_replaygain_with_soundkonverter.desktop");
            logger->log( 1000, i18n("Removing old file: %1",QDir::homePath()+"/.kde4/share/kde4/services/ServiceMenus/add_replaygain_with_soundkonverter.desktop") );
        }
    }

    // clean up log directory
    QDir dir( KStandardDirs::locateLocal("data","soundkonverter/log/") );
    dir.setFilter( QDir::Files | QDir::Writable );

    QStringList list = dir.entryList();

    for( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
    {
        if( *it != "1000.log" && (*it).endsWith(".log") )
        {
            QFile::remove( dir.absolutePath() + "/" + (*it) );
            logger->log( 1000, i18n("Removing old file: %1",dir.absolutePath()+"/"+(*it)) );
        }
    }

    // check if new backends got installed and the backend settings can be optimized
    QList<CodecOptimizations::Optimization> optimizationList = config->getOptimizations();
    if( !optimizationList.isEmpty() )
    {
        CodecOptimizations *optimizationsDialog = new CodecOptimizations( optimizationList, this );
        connect( optimizationsDialog, SIGNAL(solutions(const QList<CodecOptimizations::Optimization>&)), config, SLOT(doOptimizations(const QList<CodecOptimizations::Optimization>&)) );
        optimizationsDialog->open();
    }
}

void soundKonverter::conversionStarted()
{
    if( systemTray )
    {
        systemTray->setToolTip( "soundkonverter", i18n("Converting") + ": 0%", "" );
    }
}

void soundKonverter::conversionStopped( bool failed )
{
    if( autoclose && !failed /*&& !m_view->isVisible()*/ )
        qApp->quit(); // close app on conversion stop unless the conversion was stopped by the user or the window is shown

    if( systemTray )
    {
        systemTray->setToolTip( "soundkonverter", i18n("Finished"), "" );
    }
}

void soundKonverter::progressChanged( const QString& progress )
{
    setWindowTitle( progress + " - soundKonverter" );

    if( systemTray )
    {
        #if KDE_IS_VERSION(4,4,0)
            systemTray->setToolTip( "soundkonverter", i18n("Converting") + ": " + progress, "" );
        #else
            systemTray->setToolTip( i18n("Converting") + ": " + progress );
        #endif
    }
}


#include "soundkonverter.moc"
