
#include "soundkonverterapp.h"
#include "soundkonverter.h"

#include <KCmdLineArgs>
#include <KStandardDirs>
#include <KUrl>
#include <QFile>
#include <QObject>
#include <QApplication>
#include <QCommandLineParser>

soundKonverterApp::soundKonverterApp()
    : QObject()
{
    mainWindow = new soundKonverter();
    qApp->setActiveWindow( mainWindow );
}

soundKonverterApp::~soundKonverterApp()
{}

int soundKonverterApp::newInstance(QCommandLineParser *args)
{
    static bool first = true;
    bool visible = true;
    bool autoclose = false;
    bool autostart = false;
    bool activateMainWindow = true;
    
    if( ( first || !mainWindow->isVisible() ) && args->isSet("replaygain") && args->positionalArguments().count() > 0 )
        visible = false;

    autoclose = args->isSet( "autoclose" );
    autostart = args->isSet( "autostart" );

    const QString profile = args->value( "profile" );
    const QString format = args->value( "format" );
    const QString directory = args->value( "output" );
    const QString notifyCommand = args->value( "command" );
    const QString fileListPath = args->value( "file-list" );

    if( args->isSet( "invisible" ) )
    {
        autoclose = true;
        autostart = true;
        visible = false;
        mainWindow->showSystemTray();
    }

    if( first && fileListPath.isEmpty() && QFile::exists(KStandardDirs::locateLocal("data","soundkonverter/filelist_autosave.xml")) )
    {
        if( !visible )
        {
            visible = true;
            autoclose = false;
            autostart = false;
            mainWindow->show();
        }
        mainWindow->show();
	qApp->processEvents();
        mainWindow->loadAutosaveFileList();
    }
    else if( !fileListPath.isEmpty() && QFile::exists(fileListPath) )
    {
        mainWindow->loadFileList(fileListPath);
    }

    const QString device = args->value( "rip" );
    if( !device.isEmpty() )
    {
        const bool success = mainWindow->ripCd( device, profile, format, directory, notifyCommand );
        if( !success && first )
        {
	    qApp->quit();
            return 0;
        }
    }

    if( visible )
        mainWindow->show();

    mainWindow->setAutoClose( autoclose );

    if( args->isSet( "replaygain" ) )
    {
        if( !args->positionalArguments().isEmpty() )
        {
            mainWindow->addReplayGainFiles( args->positionalArguments() );
            activateMainWindow = false;
        }
    }
    else
    {
        if( !args->positionalArguments().isEmpty() ) {
            mainWindow->addConvertFiles( args->positionalArguments(), profile, format, directory, notifyCommand );
	}
    }

    if( activateMainWindow )
        mainWindow->activateWindow();

    if( autostart )
        mainWindow->startConversion();

    if( first )
        mainWindow->startupChecks();

    first = false;

    return 0;
}

