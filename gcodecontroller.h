#ifndef GCODECONTROLLER_H
#define GCODECONTROLLER_H

#include <QString>
#include <QFile>
#include <QThread>
#include <QTextStream>
#include "definitions.h"
#include "rs232.h"
#include "coord3d.h"
#include "controlparams.h"

class CmdResponse
{
public:
    CmdResponse(const char *buf, int c, int l) : cmd(buf), count(c), line(l)
    {
        waitForMe = false;
        if (buf[0] == 'M')
        {
            int value = cmd.mid(1,-1).toInt();
            if (value == 9)
                waitForMe = true;
        }
    }
public:
    QString cmd;
    int count;
    int line;
    bool waitForMe;
};

class DecimalFilter
{
public:
    DecimalFilter(QString t) : token(t), decimals(0) {}
public:
    QString token;
    int decimals;
};

class GCodeController : public QObject
{
    Q_OBJECT

public:
    GCodeController();
    void setAbort();
    void setReset();
    void setShutdown();
    int getSettingsItemCount();
    int getNumaxis();

    static void trimToEnd(QString& strline, QChar);

signals:
    void addList(QString line);
    void addListFull(QStringList list);
    void addListOut(QString line);
    void sendMsg(QString msg);
    void stopSending();
    void portIsClosed(bool reopen);
    void portIsOpen(bool sendCode);
    void setCommandText(QString value);
    void adjustedAxis();
    void gcodeResult(int id, QString result);
    void setProgress(int);
    void setQueuedCommands(int, bool);
    void resetTimer(bool timeIt);
    void enableGrblDialogButton();
    void updateCoordinates(Coord3D machineCoord, Coord3D workCoord);
    void setLastState(QString state);
    void setUnitsWork(QString value);
    void setUnitsMachine(QString value);
    void setLivePoint(double x, double y, bool isMM);
    void setVisCurrLine(int currLine);
    void setLcdState(bool valid);
    void setVisualLivenessCurrPos(bool isLiveCP);

public slots:
    virtual void openPort(QString commPortStr, QString baudRate) = 0;
    virtual void closePort(bool reopen);
    virtual void sendGcode(QString line) = 0;
    virtual void sendGcodeAndGetResult(int id, QString line) = 0;
    virtual void sendFile(QString path) = 0;
    virtual void gotoXYZFourth(QString line) = 0;
    virtual void axisAdj(char axis, float coord, bool inv, bool absoluteAfterAxisAdj, int sliderZCount) = 0;
    virtual void setResponseWait(ControlParams controlParams) = 0;
    virtual void controllerSetHome() = 0;
    virtual void sendControllerReset() = 0;
    virtual void sendControllerUnlock() = 0;
    virtual void performZLeveling(QRect extent, int xSteps, int ySteps, double zSafe) = 0;
    virtual void goToHome() = 0;

protected:
    enum PosReqStatus
    {
        POS_REQ_RESULT_OK,
        POS_REQ_RESULT_ERROR,
        POS_REQ_RESULT_TIMER_SKIP,
        POS_REQ_RESULT_UNAVAILABLE
    };
    virtual QString removeUnsupportedCommands(QString line) = 0;
    bool isPortOpen();
    AtomicIntBool abortState;
    AtomicIntBool resetState;
    AtomicIntBool shutdownState;

    RS232 port;
};

#endif // GCODECONTROLLER_H
