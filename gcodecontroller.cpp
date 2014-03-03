#include "gcodecontroller.h"

#include <QObject>


GCodeController::GCodeController()
{

}

void GCodeController::closePort(bool reopen)
{
    port.CloseComport();
    emit portIsClosed(reopen);
}

bool GCodeController::isPortOpen()
{
    return port.isPortOpen();
}

// Abort means stop file send after the end of this line
void GCodeController::setAbort()
{
    // cross-thread operation, only set one atomic variable in this method (bool in this case) or add critsec
    abortState.set(true);
}

// Reset means immediately stop waiting for a response
void GCodeController::setReset()
{
    // cross-thread operation, only set one atomic variable in this method (bool in this case) or add critsec
    resetState.set(true);
}

// Shutdown means app is shutting down - we give thread about .3 sec to exit what it is doing
void GCodeController::setShutdown()
{
    // cross-thread operation, only set one atomic variable in this method (bool in this case) or add critsec
    shutdownState.set(true);
}

void GCodeController::trimToEnd(QString& strline, QChar ch)
{
    int pos = strline.indexOf(ch);
    if (pos != -1)
        strline = strline.left(pos);
}
