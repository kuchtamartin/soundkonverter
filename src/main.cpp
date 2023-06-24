

#include "soundkonverterapp.h"
#include "soundkonverter.h"
#include "global.h"

#include <QCommandLineParser>
#include <QApplication>
#include <QDebug>

#include <KMainWindow>
#include <KAboutData>
#include <KDBusService>
#include <KLocale>
#include <KLocalizedString>

static const char description[] =
I18N_NOOP("soundKonverter is a frontend to various audio converters, Replay Gain tools and CD rippers.\n\nPlease file bug reports at https://github.com/nphantasm/soundkonverter/issues");

static const char version[] = SOUNDKONVERTER_VERSION_STRING;

int main(int argc, char **argv)
{
    QApplication app(argc,argv);

    KLocalizedString::setApplicationDomain("soundkonverter");
    
    KAboutData about("soundkonverter", i18n("soundKonverter"), version, i18n(description), KAboutLicense::GPL, i18n("(C) 2005-2017 Daniel Faust"), QString(), 0, "hessijames@gmail.com");
    about.addAuthor( i18n("Daniel Faust"), QString(), "hessijames@gmail.com" );
    about.addCredit( i18n("David Vignoni"), i18n("Nuvola icon theme"), 0, "http://www.icon-king.com" );
    about.addCredit( i18n("Scott Wheeler"), i18n("TagLib"), "wheeler@kde.org", "http://ktown.kde.org/~wheeler" );
    about.addCredit( i18n("Marco Nelles"), i18n("Audex"), 0, "http://opensource.maniatek.de/audex" );
    about.addCredit( i18n("Amarok developers"), i18n("Amarok"), 0, "http://amarok.kde.org" );
    about.addCredit( i18n("All programmers of audio converters"), i18n("Backends") );
    about.addCredit( i18n("Patrick Auernig"), i18n("Inital Port to KDE Frameworks 5"), "patrick.auernig@gmail.com" );

    /* Manually set .desktop filename. Otherwise it would become "org.kde.soundkonverter" and icons wouldn't work */
    about.setDesktopFileName("soundkonverter");

    KAboutData::setApplicationData(about);

    /* Create a command line parser and options to it */
    QCommandLineParser parser;
    parser.addPositionalArgument( "files", i18n("Audio file(s) to append to the file list"), i18n("files") );
    parser.addOption( { "replaygain", i18n("Open the Replay Gain tool and add all given files") } );
    parser.addOption( { "rip", i18n("List all tracks on the cd drive <device>, 'auto' will search for a cd"), i18n("device") } );
    parser.addOption( { "profile", i18n("Add all files using the given profile"), i18n("profile") } );
    parser.addOption( { "format", i18n("Add all files using the given format"), i18n("format") } );
    parser.addOption( { "output", i18n("Output all files to <directory>"), i18n("output") } );
    parser.addOption( { "invisible", i18n("Start soundKonverter invisible") } );
    parser.addOption( { "autostart", i18n("Start the conversion immediately (enabled when using '--invisible')") } );
    parser.addOption( { "autoclose", i18n("Close soundKonverter after all files are converted (enabled when using '--invisible')") } );
    parser.addOption( { "command", i18n("Execute <command> after each file has been converted (%i=input file, %o=output file)"), i18n("command") } );
    parser.addOption( { "file-list", i18n("Load the file list at <path> after starting soundKonverter"), i18n("path") } );

    about.setupCommandLine(&parser);
    parser.process(app);

    about.processCommandLine(&parser);

    /* Make sure only one instance of soundkonverter is running */
    KDBusService service(KDBusService::Unique);

    /* Manually start new instance of application. Previously was somehow started via KUniqueApplication::start() from kdelibs4 */
    soundKonverterApp *mainWindow = new soundKonverterApp();
    mainWindow->newInstance(&parser);

    // mainWin has WDestructiveClose flag by default, so it will delete itself.
    return app.exec();
}
