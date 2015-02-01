#ifndef GCODEMARLIN_H
#define GCODEMARLIN_H

#include "gcodecontroller.h"
#include "rs232.h"
#include "coord3d.h"
#include "controlparams.h"
#include "gcommands.h"

#include <QString>
#include <QFile>
#include <QThread>
#include <QTextStream>
#include "definitions.h"


#define TX_BUF_SIZE 96 //In marlin there are four buffers of 96 bytes.
#define RX_BUF_SIZE 300

#define RESPONSE_OK "ok"
#define RESPONSE_ERROR "error"

// as defined in the grbl project on github...
#define GRBL_RX_BUFFER_SIZE     128

#define CTRL_X '\x18'
#define REQUEST_CURRENT_POS             "M114"

#define DEFAULT_AXIS_COUNT      3
#define MAX_AXIS_COUNT          4

#define MM_PER_ARC_SEGMENT 0.5



class GCodeMarlin : public GCodeController
{
    Q_OBJECT

public:
    GCodeMarlin();

    int getSettingsItemCount();
    int getNumaxis();

public slots:
    void openPort(QString commPortStr, QString baudRate);
    void sendGcode(QString line);
    void sendGcodeAndGetResult(int id, QString line);
    void sendFile(QString path);
    void gotoXYZFourth(QString line);
    void axisAdj(char axis, float coord, bool inv, bool absoluteAfterAxisAdj, int sliderZCount);
    void setResponseWait(ControlParams controlParams);
    void controllerSetHome();
    void sendControllerReset();
    void sendControllerUnlock();
    void performZLeveling(int levelingAlgorithm, QRect extent, int xSteps, int ySteps, double zStarting, double speed, double zSafe, double offset);
    void goToHome();
    bool isZInterpolatorReady();
    void clearLevelingData();
    const Interpolator * getInterpolator();
    void changeInterpolator(int index);
    void recomputeOffset(double speed, double zStarting);

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
    CodeCommand *makeLineMarlinFriendly(const QString& line);
    QList<CodeCommand *> levelLine(CodeCommand *line);

    bool SendJog(QString strline, bool absoluteAfterAxisAdj);
    void parseCoordinates(const QString& received);
    void pollPosWaitForIdle();
    void setOldFormatMeasurementUnitControl();
    void setUnitsTypeDisplay(bool millimeters);
    void setConfigureMmMode(bool setGrblUnits);
    void setConfigureInchesMode(bool setGrblUnits);
    QStringList doZRateLimit(QString strline, QString& msg, bool& xyRateSet);
    void sendStatusList(QStringList& listToSend);
    bool checkMarlin(const QString& result);
    void computeCoordinates(const CodeCommand &command);

    bool probeResultToValue(const QString & result, double &zCoord);

private:


    ControlParams controlParams;
    int errorCount;
    QString currComPort;
    bool doubleDollarFormat;
    AtomicIntBool settingsItemCount;
    bool incorrectMeasurementUnits;
    bool incorrectLcdDisplayUnits;
    Coord3D machineCoord, workCoord;
    Coord3D machineCoordLastIdlePos, workCoordLastIdlePos;
    QList<CmdResponse> sendCount;
    Point lastLevelingPoint;

    int sliderZCount;
    QStringList grblCmdErrors;
    QStringList grblFilteredCmds;

    Interpolator * interpolator;

    float lastExplicitFeed;
    bool manualFeedSetted;
    int lastGCommand;
    double lastZCoord;

    int rcvdI;
    int numaxis;
};

#endif // GCODEMARLIN_H
