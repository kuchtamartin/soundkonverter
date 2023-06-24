#ifndef SOUNDKONVERTERAPP_H
#define SOUNDKONVERTERAPP_H


#include <QObject>
#include <QCommandLineParser>

class soundKonverter;

/**
 * @short The soundKonverter application. It controlles ensures that there can only run one instance of soundKonverter.
 * @author Daniel Faust <hessijames@gmail.com>
 * @version 0.3
 */
class soundKonverterApp : public QObject
{
    Q_OBJECT
public:
    /** Constructor */
    soundKonverterApp();

    /** Destructor */
    ~soundKonverterApp();

    /** This function is called, when a new instance of soundKonverter should be created */
    virtual int newInstance(QCommandLineParser *args);

// private:
    soundKonverter *mainWindow;
};

#endif // SOUNDKONVERTERAPP_H
