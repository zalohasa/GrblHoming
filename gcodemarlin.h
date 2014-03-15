#ifndef GCODEMARLIN_H
#define GCODEMARLIN_H

#include "gcodecontroller.h"

#include <QString>
#include <QFile>
#include <QThread>
#include <QTextStream>
#include "definitions.h"
#include "rs232.h"
#include "coord3d.h"
#include "controlparams.h"

#define BUF_SIZE 300

#define RESPONSE_OK "ok"
#define RESPONSE_ERROR "error"

// as defined in the grbl project on github...
#define GRBL_RX_BUFFER_SIZE     128

#define CTRL_X '\x18'
#define REQUEST_CURRENT_POS             "M114"

#define DEFAULT_AXIS_COUNT      3
#define MAX_AXIS_COUNT          4



class GCodeMarlin : public GCodeController
{
    Q_OBJECT

public:
    GCodeMarlin();

    int getSettingsItemCount();
    int getNumaxis();

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

public slots:
    void openPort(QString commPortStr, QString baudRate);
    void sendGcode(QString line);
    void sendGcodeAndGetResult(int id, QString line);
    void sendFile(QString path);
    void gotoXYZC(QString line);
    void axisAdj(char axis, float coord, bool inv, bool absoluteAfterAxisAdj, int sliderZCount);
    void setResponseWait(ControlParams controlParams);
    void controllerSetHome();
    void sendControllerReset();
    void sendControllerUnlock();
    void goToHome();

protected:
    void timerEvent(QTimerEvent *event);
    QString removeUnsupportedCommands(QString line);

private:
    bool sendGcodeLocal(QString line, bool recordResponseOnFail = false, int waitSec = -1, int currLine = 0);
    bool waitForOk(QString& result, int waitCount, bool sentReqForLocation, bool finalize);
    bool waitForStartupBanner(QString& result, int waitSec, bool failOnNoFound);
    bool sendGcodeInternal(QString line, QString& result, bool recordResponseOnFail, int waitSec, int currLine = 0);
    QString reducePrecision(QString line);
    bool isGCommandValid(float value, bool& toEndOfLine);
    bool isMCommandValid(float value);
    QString makeLineMarlinFriendly(const QString& line);

    QString getMoveAmountFromString(QString prefix, QString item);
    bool SendJog(QString strline, bool absoluteAfterAxisAdj);
    void parseCoordinates(const QString& received);
    void pollPosWaitForIdle();
    void setOldFormatMeasurementUnitControl();
    void setUnitsTypeDisplay(bool millimeters);
    void setConfigureMmMode(bool setGrblUnits);
    void setConfigureInchesMode(bool setGrblUnits);
    QStringList doZRateLimit(QString strline, QString& msg, bool& xyRateSet);
    void sendStatusList(QStringList& listToSend);
    void clearToHome();
    bool checkMarlin(const QString& result);
    void computeCoordinates(const QString& command);

private:


    ControlParams controlParams;
    int errorCount;
    QString currComPort;
    bool doubleDollarFormat;
    AtomicIntBool settingsItemCount;
    QString lastState;
    bool incorrectMeasurementUnits;
    bool incorrectLcdDisplayUnits;
    Coord3D machineCoord, workCoord;
    Coord3D machineCoordLastIdlePos, workCoordLastIdlePos;
    double maxZ;
    QList<CmdResponse> sendCount;

    bool motionOccurred;
    int sliderZCount;
    QStringList grblCmdErrors;
    QStringList grblFilteredCmds;

    float lastExplicitFeed;
    bool manualFeedSetted;
    QString lastGCommand;

    int sentI;
    int rcvdI;
    int numaxis;
};

#endif // GCODEMARLIN_H
