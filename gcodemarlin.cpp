
/****************************************************************
 * gcode.cpp
 * GrblHoming - zapmaker fork on github
 *
 * 15 Nov 2012
 * GPL License (see LICENSE file)
 * Software is provided AS-IS
 ****************************************************************/

#include "gcodemarlin.h"

#include <QObject>
#include <iostream>

using std::cout;
using std::endl;

GCodeMarlin::GCodeMarlin()
    : errorCount(0), doubleDollarFormat(false),
      incorrectMeasurementUnits(false), incorrectLcdDisplayUnits(false),
      maxZ(0), motionOccurred(false),
      sliderZCount(0),
      numaxis(DEFAULT_AXIS_COUNT),
      manualFeedSetted(false),
      interpolator(NULL)
{
    // use base class's timer - use it to capture random text from the controller
    startTimer(1000);
}

void GCodeMarlin::openPort(QString commPortStr, QString baudRate)
{
    numaxis = controlParams.useFourAxis ? MAX_AXIS_COUNT : DEFAULT_AXIS_COUNT;

    clearToHome();

    currComPort = commPortStr;

    port.setCharSendDelayMs(controlParams.charSendDelayMs);

    if (port.OpenComport(commPortStr, baudRate))
    {
        emit portIsOpen(true);
    }
    else
    {
        emit portIsClosed(false);
        QString msg = tr("Can't open COM port ") + commPortStr;
        sendMsg(msg);
        addList(msg);
        warn("%s", qPrintable(msg));

        addList(tr("-Is hardware connected to USB?") );
        addList(tr("-Is correct port chosen?") );
        addList(tr("-Does current user have sufficient permissions?") );
#if defined(Q_OS_LINUX)
        addList("-Is current user in sudoers group?");
#endif
        //QMessageBox(QMessageBox::Critical,"Error","Could not open port.",QMessageBox::Ok).exec();
    }
}

// Slot for interrupting current operation or doing a clean reset of grbl without changing position values
void GCodeMarlin::sendControllerReset()
{
    sendGcodeLocal("M999", true, SHORT_WAIT_SEC);
}

void GCodeMarlin::sendControllerUnlock()
{
    sendGcodeLocal(SET_UNLOCK_STATE_V08c);
}

// Slot for gcode-based 'zero out the current position values without motion'
void GCodeMarlin::controllerSetHome()
{
    clearToHome();
    gotoXYZFourth("G92 X0 Y0 Z0");
}

void GCodeMarlin::goToHome()
{
    sendGcodeLocal("G28");

    pollPosWaitForIdle();
}

// Slot called from other threads (i.e. main window, grbl dialog, etc.)
void GCodeMarlin::sendGcode(QString line)
{


    // empty line means we have just opened the com port
    if (line.length() == 0)
    {
        resetState.set(false);

        QString result;

        if (shutdownState.get() || resetState.get())
            return;
        // it is possible that we are already connected and missed the
        // signon banner. Force a reset (is this ok?) to get the banner

        emit addListOut("M115");

        const char * m115 = "M115\n";

        diag(qPrintable(tr("SENDING: M115) to check presence of Marlin\n")))  ;
        emit addList(QString(m115));
        if (!port.SendBuf(m115, 5))
        {
            QString msg = tr("Sending to port failed");
            err("%s", qPrintable(msg));
            emit addList(msg);
            emit sendMsg(msg);
            return;
        }

        if (!waitForStartupBanner(result, SHORT_WAIT_SEC, true))
            return;



    }

    else
    {
        pollPosWaitForIdle();
        // normal send of actual commands
        sendGcodeLocal(line, false);
    }

    pollPosWaitForIdle();
}

// keep polling our position and state until we are done running
void GCodeMarlin::pollPosWaitForIdle()
{
    for (int i = 0; i < 10000; i++)
    {
        bool ret = sendGcodeLocal(REQUEST_CURRENT_POS);
        if (!ret)
        {
            break;
        }

        if (machineCoordLastIdlePos == machineCoord
            && workCoordLastIdlePos == workCoord)
        {
            break;
        }

        machineCoordLastIdlePos = machineCoord;
        workCoordLastIdlePos = workCoord;


        if (shutdownState.get())
            return;
    }

}

// Slot called from other thread that returns whatever text comes back from the controller
void GCodeMarlin::sendGcodeAndGetResult(int id, QString line)
{
    QString result;

    emit sendMsg("");
    resetState.set(false);
    if (!sendGcodeInternal(line, result, false, SHORT_WAIT_SEC, false))
        result.clear();

    emit gcodeResult(id, result);
}

// To be called only from this class, not from other threads. Use above two methods for that.
// Wraps sendGcodeInternal() to allow proper handling of failure cases, etc.
bool GCodeMarlin::sendGcodeLocal(QString line, bool recordResponseOnFail /* = false */, int waitSec /* = -1 */, int currLine /* = 0 */)
{
    QString result;
    sendMsg("");
    resetState.set(false);

    bool ret = sendGcodeInternal(line, result, recordResponseOnFail, waitSec, currLine);
    if (shutdownState.get())
        return false;

    if (!ret && (!recordResponseOnFail || resetState.get()))
    {
        if (!resetState.get())
            emit stopSending();

        if (!ret && resetState.get())
        {
            resetState.set(false);
            port.Reset();
        }
    }

    resetState.set(false);
    return ret;
}

bool GCodeMarlin::checkMarlin(const QString& result)
{
    if (result.contains("Marlin"))
    {
        QRegExp rx("FIRMWARE_NAME:(.*) FIRMWARE_URL:(.*) PROTOCOL_VERSION:(\\d+)\\.(\\d+) MACHINE_TYPE:(\\w*).*");

       if (rx.indexIn(result) != -1 && rx.captureCount() > 0)
        {

            QStringList list = rx.capturedTexts();
            if (list.size() >= 3)
            {
                int majorVer = list.at(3).toInt();
                int minorVer = list.at(4).toInt();
                QByteArray tmp = list.at(1).toLocal8Bit();
                //const char * url = tmp.constData();
                tmp = list.at(1).toLocal8Bit();
                const char * marlinStr = tmp.constData();
                tmp = list.at(5).toLocal8Bit();
                const char * machineType = tmp.constData();

                diag(qPrintable(tr("Firmware: %s\n")),marlinStr);
                //diag(qPrintable(tr("Url: %s\n")), url);
                diag(qPrintable(tr("Protocol version: %d.%d\n")), majorVer, minorVer);
                diag(qPrintable(tr("Machine: %s\n")), machineType);
            }

        }
        return true;
    }
    return false;
}

// Wrapped method. Should only be called from above method.
bool GCodeMarlin::sendGcodeInternal(QString line, QString& result, bool recordResponseOnFail, int waitSec, int currLine /* = 0 */)
{
    if (!port.isPortOpen())
    {
        QString msg = tr("Port not available yet")  ;
        err("%s", msg.toLocal8Bit().constData());
        emit addList(msg);
        emit sendMsg(msg);
        return false;
    }

    //bool ctrlX = line.size() > 0 ? (line.at(0).toLatin1() == CTRL_X) : false;

    bool sentReqForLocation = false;
    bool sentReqForSettings = false;

    if (!line.compare(REQUEST_CURRENT_POS))
    {
        sentReqForLocation = true;
    }

    else if (!line.compare(SETTINGS_COMMAND_V08a))
    {
        if (doubleDollarFormat)
            line = SETTINGS_COMMAND_V08c;

        sentReqForSettings = true;
    }
    else
        motionOccurred = true;

    // adds to UI list, but prepends a > indicating a sent command

    if (!sentReqForLocation)// if requesting location, don't add that "noise" to the output view
    {
        emit addListOut(line);
    }

    if (line.size() == 0 || (!line.endsWith('\r')))
        line.append('\r');

    char buf[BUF_SIZE + 1] = {0};
    if (line.length() >= BUF_SIZE)
    {
        QString msg = tr("Buffer size too small");
        err("%s", qPrintable(msg));
        emit addList(msg);
        emit sendMsg(msg);
        return false;
    }
    for (int i = 0; i < line.length(); i++)
        buf[i] = line.at(i).toLatin1();


    diag(qPrintable(tr("SENDING[%d]: %s\n")), currLine, buf);

    int waitSecActual = waitSec == -1 ? controlParams.waitTime : waitSec;

    if (!port.SendBuf(buf, line.length()))
    {
        QString msg = tr("Sending to port failed")  ;
        err("%s", qPrintable(msg));
        emit addList(msg);
        emit sendMsg(msg);
        return false;
    }
    else
    {
        sentI++;
        if (!waitForOk(result, waitSecActual, sentReqForLocation, false))
        {
            diag(qPrintable(tr("WAITFOROK FAILED\n")));
            if (shutdownState.get())
                return false;

            if (!recordResponseOnFail && !(resetState.get() || abortState.get()))
            {
                QString msg = tr("Wait for ok failed");
                emit addList(msg);
                emit sendMsg(msg);
            }

            return false;
        }
        else
        {
            if (sentReqForSettings)
            {
                QStringList list = result.split("$");
                for (int i = 0; i < list.size(); i++)
                {
                    QString item = list.at(i);
                    const QRegExp rx(REGEXP_SETTINGS_LINE);

                    if (rx.indexIn(item, 0) != -1 && rx.captureCount() == 3)
                    {
                        QStringList capList = rx.capturedTexts();
                        if (!capList.at(1).compare("13"))
                        {
                            if (!capList.at(2).compare("0"))
                            {
                                if (!controlParams.useMm)
                                    incorrectLcdDisplayUnits = true;
                            }
                            else
                            {
                                if (controlParams.useMm)
                                    incorrectLcdDisplayUnits = true;
                            }
                            break;
                        }
                    }
                }

                settingsItemCount.set(list.size());
            }
        }
    }
    return true;
}

bool GCodeMarlin::waitForOk(QString& result, int waitSec, bool sentReqForLocation, bool finalize)
{
    char tmp[BUF_SIZE + 1] = {0};

    bool status = true;
    result.clear();
    while (!result.contains(RESPONSE_OK) && !result.contains(RESPONSE_ERROR) && !resetState.get())
    {
        int n = port.PollComportLine(tmp, BUF_SIZE);
        if (n < 0)
        {
            QString Mes(tr("Error reading data from COM port\n"))  ;
            err(qPrintable(Mes));

        }
        else if (n > 0)
        {
            tmp[n] = 0;
            result.append(tmp);

            QString tmpTrim(tmp);
            int pos = tmpTrim.indexOf(port.getDetectedLineFeed());
            if (pos != -1)
                tmpTrim.remove(pos, port.getDetectedLineFeed().size());
            QString received(tmp);

            if (!received.isEmpty()){
                diag(qPrintable(tr("GOT:%s\n")), tmpTrim.toLocal8Bit().constData());

                if (!received.contains(RESPONSE_OK) && !received.contains(RESPONSE_ERROR))
                {
                    parseCoordinates(received);
                }
            }
        } else {
            //Limit CPU usage while waiting for data.
            usleep(4000);
        }


        //TODO Use a qtimer in single shot mode to control the timeout. (Is this really necesary in Marlin?)
       /* if (count > waitCount)
        {
            std::cout << "MABURRI porque count is " << count << " and waitCount is " << waitCount << std::endl;
            // waited too long for a response, fail
            status = false;
            break;
        }*/
    }

    if (shutdownState.get())
    {
        return false;
    }

    if (status)
    {

        if (resetState.get())
        {
            QString msg(tr("Wait interrupted by user"));
            err("%s", qPrintable(msg));
            emit addList(msg);
        }
    }

    if (result.contains(RESPONSE_ERROR))
    {
        errorCount++;
        // skip over errors
        //status = false;
    }

    QStringList list = QString(result).split(port.getDetectedLineFeed());
    QStringList listToSend;
    for (int i = 0; i < list.size(); i++)
    {
        if (list.at(i).length() > 0 && list.at(i) != RESPONSE_OK && !sentReqForLocation && !list.at(i).startsWith("MPos:["))
            listToSend.append(list.at(i));
    }

    sendStatusList(listToSend);

    if (resetState.get())
    {
        // we have been told by the user to stop.
        status = false;
    }

    return status;
}

bool GCodeMarlin::waitForStartupBanner(QString& result, int waitSec, bool failOnNoFound)
{
    char tmp[BUF_SIZE + 1] = {0};
    int count = 0;
    int waitCount = waitSec * 10;// multiplier depends on sleep values below
    bool status = true;
    result.clear();
    while (!resetState.get())
    {
        int n = port.PollComportLine(tmp, BUF_SIZE);
        if (n == 0)
        {
            count++;
            SLEEP(100);
        }
        else if (n < 0)
        {
            err(qPrintable(tr("Error reading data from COM port\n")) );
        }
        else
        {
            tmp[n] = 0;
            result.append(tmp);

            QString tmpTrim(tmp);
            int pos = tmpTrim.indexOf(port.getDetectedLineFeed());
            if (pos != -1)
                tmpTrim.remove(pos, port.getDetectedLineFeed().size());
            diag(qPrintable(tr("GOT:%s\n")), tmpTrim.toLocal8Bit().constData());

            if (tmpTrim.length() > 0)
            {
                if (!checkMarlin(tmpTrim))
                {
                    if (failOnNoFound)
                    {
                        QString msg(tr("Expecting Marlin version string. Unable to parse response."));
                        emit addList(msg);
                        emit sendMsg(msg);

                        closePort(false);
                    }
                    status = false;
                }
                else
                {
                    pollPosWaitForIdle();
                    emit enableGrblDialogButton();
                }
                break;
            }
        }

        SLEEP(100);

        if (count > waitCount)
        {
            if (failOnNoFound)
            {
                // waited too long for a response, fail

                QString msg(tr("No data from COM port after connect. Expecting Grbl version string."));
                emit addList(msg);
                emit sendMsg(msg);

                closePort(false);
            }

            status = false;
            break;
        }
    }

    if (shutdownState.get())
    {
        return false;
    }

    if (status)
    {
        if (resetState.get())
        {
            QString msg(tr("Wait interrupted by user (startup)"));
            err("%s", qPrintable(msg));
            emit addList(msg);
        }
    }

    if (result.contains(RESPONSE_ERROR))
    {
        errorCount++;
        // skip over errors
        //status = false;
    }

    QStringList list = QString(result).split(port.getDetectedLineFeed());
    QStringList listToSend;
    for (int i = 0; i < list.size(); i++)
    {
        if (list.at(i).length() > 0 && list.at(i) != RESPONSE_OK)
            listToSend.append(list.at(i));
    }

    sendStatusList(listToSend);

    if (resetState.get())
    {
        // we have been told by the user to stop.
        status = false;
    }

    return status;
}

void GCodeMarlin::parseCoordinates(const QString& received)
{

    std::cout << "parsing coordinates" << std::endl;
    bool good = false;

    QString format(".*X:(-*\\d+\\.\\d+)Y:(-*\\d+\\.\\d+)Z:(-*\\d+\\.\\d+).*");
    QRegExp rxWPos = QRegExp(format);

    if (rxWPos.indexIn(received, 0) != -1)
    {
        good = true;
    }

    if (good) {  /// naxis contains number axis

        //numaxis = naxis;
        QStringList list = rxWPos.capturedTexts();
        //int index = 1;



        machineCoord.x = list.at(1).toFloat();
        machineCoord.y = list.at(2).toFloat();
        machineCoord.z = list.at(3).toFloat();
        //if (numaxis == MAX_AXIS_COUNT)
        //    machineCoord.c = list.at(index++).toFloat();

        workCoord.x = list.at(1).toFloat();
        workCoord.y = list.at(2).toFloat();
        workCoord.z = list.at(3).toFloat();
        if (numaxis == MAX_AXIS_COUNT)
            workCoord.fourth = list.at(4).toFloat();
        /*if (state != "Run")
            workCoord.stoppedZ = true;
        else
            workCoord.stoppedZ = false;
            */

        workCoord.sliderZIndex = sliderZCount;
        if (numaxis == DEFAULT_AXIS_COUNT)
            diag(qPrintable(tr("Decoded: MPos: %f,%f,%f WPos: %f,%f,%f\n")),
                 machineCoord.x, machineCoord.y, machineCoord.z,
                 workCoord.x, workCoord.y, workCoord.z
                 );
        else if (numaxis == MAX_AXIS_COUNT)
            diag(qPrintable(tr("Decoded:MPos: %f,%f,%f,%f WPos: %f,%f,%f,%f\n")),
                 machineCoord.x, machineCoord.y, machineCoord.z, machineCoord.fourth,
                 workCoord.x, workCoord.y, workCoord.z, workCoord.fourth
                 );

        if (workCoord.z > maxZ)
            maxZ = workCoord.z;

        emit updateCoordinates(machineCoord, workCoord);
        emit setLivePoint(workCoord.x, workCoord.y, controlParams.useMm);
        //emit setLastState(state);

        //lastState = state;
        return;
    }
    // TODO fix to print
    //if (!good /*&& received.indexOf("MPos:") != -1*/)
    //    err(qPrintable(tr("Error decoding position data! [%s]\n")), qPrintable(received));

    lastState = "";
}

void GCodeMarlin::sendStatusList(QStringList& listToSend)
{
    if (listToSend.size() > 1)
    {
        emit addListFull(listToSend);
    }
    else if (listToSend.size() == 1)
    {
        emit addList(listToSend.at(0));
    }
}

// called once a second to capture any random strings that come from the controller
#pragma GCC diagnostic ignored "-Wunused-parameter" push
void GCodeMarlin::timerEvent(QTimerEvent *event)
{
    if (port.isPortOpen())
    {
        char tmp[BUF_SIZE + 1] = {0};
        QString result;

        for (int i = 0; i < 10 && !shutdownState.get() && !resetState.get(); i++)
        {
            int n = port.PollComport(tmp, BUF_SIZE);
            if (n == 0)
                break;

            tmp[n] = 0;
            result.append(tmp);
            diag(qPrintable(tr("GOT-TE:%s\n")), tmp);
        }

        if (shutdownState.get())
        {
            return;
        }

        QStringList list = QString(result).split(port.getDetectedLineFeed());
        QStringList listToSend;
        for (int i = 0; i < list.size(); i++)
        {
            if (list.at(i).length() > 0 && (list.at(i) != "ok" || (list.at(i) == "ok" && abortState.get())))
                listToSend.append(list.at(i));
        }

        sendStatusList(listToSend);
    }
}
#pragma GCC diagnostic ignored "-Wunused-parameter" pop

void GCodeMarlin::sendFile(QString path)
{

    std::cout << " ---------- INIT SENDING FILE -------------- " << std::endl;
    addList(QString(tr("Sending file '%1'")).arg(path));

    // send something to be sure the controller is ready
    //sendGcodeLocal("", true, SHORT_WAIT_SEC);

    //Set absolute coordinates
    sendGcodeLocal("G90\r");

    setProgress(0);
    emit setQueuedCommands(0, false);
    grblCmdErrors.clear();
    grblFilteredCmds.clear();
    errorCount = 0;
    abortState.set(false);
    QFile file(path);
    if (file.open(QFile::ReadOnly))
    {

        QTime totalTime(0,0,0);
        totalTime.start();

        float totalLineCount = 0;
        QTextStream code(&file);
        while ((code.atEnd() == false))
        {
            totalLineCount++;
            code.readLine();
        }
        if (totalLineCount == 0)
            totalLineCount = 1;

        code.seek(0);

        sentI = 0;
        rcvdI = 0;
        emit resetTimer(true);

        int currLine = 0;
        bool xyRateSet = false;

        do
        {
            QString strline = code.readLine();

            emit setVisCurrLine(currLine + 1);

            if (controlParams.filterFileCommands)
            {
                trimToEnd(strline, '(');
                trimToEnd(strline, ';');
                trimToEnd(strline, '%');
            }

            strline = strline.trimmed();

            if (strline.size() > 0)
            {
                if (controlParams.filterFileCommands)
                {
                    strline = strline.toUpper();
                    strline.replace(QRegExp("([A-Z])"), " \\1");
                    strline = removeUnsupportedCommands(strline);
                }

                strline = makeLineMarlinFriendly(strline);
                computeCoordinates(strline);

                if (strline.size() != 0)
                {
                    if (controlParams.reducePrecision)
                    {
                        strline = reducePrecision(strline);
                    }

                    QString rateLimitMsg;
                    QStringList outputList;
                    if (controlParams.zRateLimit)
                    {
                        outputList = doZRateLimit(strline, rateLimitMsg, xyRateSet);
                    }
                    else
                    {
                        outputList.append(strline);
                    }

                    bool ret = false;
                    if (outputList.size() == 1)
                    {
                        ret = sendGcodeLocal(outputList.at(0), false, -1, currLine + 1);
                    }
                    else
                    {
                        foreach (QString outputLine, outputList)
                        {
                            ret = sendGcodeLocal(outputLine, false, -1, currLine + 1);

                            if (!ret)
                                break;
                        }
                    }

                    if (rateLimitMsg.size() > 0)
                        addList(rateLimitMsg);

                    if (!ret)
                    {
                        abortState.set(true);
                        break;
                    }
                }
            }

            float percentComplete = (currLine * 100.0) / totalLineCount;
            setProgress((int)percentComplete);

            currLine++;
        } while ((code.atEnd() == false) && (!abortState.get()));
        file.close();

        sendGcodeLocal(REQUEST_CURRENT_POS);

        emit resetTimer(false);

        if (shutdownState.get())
        {
            return;
        }

        QString msg;
        if (!abortState.get())
        {
            setProgress(100);
            if (errorCount > 0)
            {
                msg = QString(tr("Code sent successfully with %1 error(s):")).arg(QString::number(errorCount));
                emit sendMsg(msg);
                emit addList(msg);

                foreach(QString errItem, grblCmdErrors)
                {
                    emit sendMsg(errItem);
                }
                emit addListFull(grblCmdErrors);
            }
            else
            {
                msg = tr("Code sent successfully with no errors.");
                emit sendMsg(msg);
                emit addList(msg);
            }

            if (grblFilteredCmds.size() > 0)
            {
                msg = QString(tr("Filtered %1 commands:")).arg(QString::number(grblFilteredCmds.size()));
                emit sendMsg(msg);
                emit addList(msg);

                foreach(QString errItem, grblFilteredCmds)
                {
                    emit sendMsg(errItem);
                }
                emit addListFull(grblFilteredCmds);
            }
        }
        else
        {
            msg = tr("Process interrupted.");
            emit sendMsg(msg);
            emit addList(msg);
        }

        int ms = totalTime.elapsed();
        int hours = ms / 1000 / 3600;
        ms = ms - (hours * 3600 * 1000);
        int min = ms / 60 / 1000;
        ms = ms - (min * 60 * 1000);
        int s = ms / 1000;
        ms = ms - (s * 1000);
        msg = QString(tr("Time elapsed: %1h, %2m, %3s, %4ms")).arg(QString::number(hours), QString::number(min), QString::number(s), QString::number(ms));
        emit sendMsg(msg);
        emit addList(msg);

    }

    pollPosWaitForIdle();

    if (!resetState.get())
    {
        emit stopSending();
    }

    emit setQueuedCommands(0, false);
}

QString GCodeMarlin::makeLineMarlinFriendly(const QString &line)
{

    std::cout << "Making line " << line.toStdString() << " Marlin compatible " << std::endl;

    //TODO make this value a config option.
    float g0feed = 300;

    QString tmp = line.trimmed();
    tmp = tmp.toUpper();
    //Marlin does not suppor the F command, it must be inside a G0 or G1 command, so prepend G1 to the command.
    if (tmp.at(0) == 'F')
    {
        QRegExp rx("F(\\d+)");
        if (rx.indexIn(tmp) != -1 && rx.captureCount() > 0)
        {
            QStringList list = rx.capturedTexts();
            bool ok = false;
            float feed = list.at(1).toFloat(&ok);
            if (ok)
            {
               lastExplicitFeed = feed;
            }
        }
        return QString("G1 ") + tmp;
    }

    if (tmp.at(0) == 'X' || tmp.at(0) == 'Y' || tmp.at(0) == 'Z')
    {
        //This is a modal command. Marlin does not support modal command, so prepend the last Gcommand seen.
        return lastGCommand + QString(" ") + tmp;
    }


    if (tmp.at(0) == 'G')
    {
        //A few modifications must be done to G0 and G1 commands.
        //Marling treats G0 as if it were G1, so the machine will not move at the maximun speed but at the last one.
        //This can cause some problems, so we define a "G0 feed speed", and we append this speed to every G0 command.
        //This has a side efect, in the next G1 command, Marlin will use the G0 feed speed if no speed is specified, so
        //we need to save the last non G0 speed, and manually append it to every G1 command that has no F parameter.

        std::cout << "We have a G command" << std::endl;
        QRegExp rx("G(\\d+)(.*)");

        if (rx.indexIn(tmp) != -1 && rx.captureCount() > 0)
        {
             QStringList list = rx.capturedTexts();
             std::cout << "list size: " << list.size() << std::endl;
             bool ok = false;
             int commandCode = list.at(1).toInt(&ok);

             lastGCommand = "G" + QString::number(commandCode);
             std::cout << "G code: " << lastGCommand.toStdString() << std::endl;

               QString parameters = list.at(2);
             if (commandCode == 0)
             {
                 if (parameters.indexOf("F") < 0)
                 {
                     //There is no F parameter, just append it.
                     manualFeedSetted = true;
                     return tmp + " F" + QString().setNum(g0feed);
                 }

             } else if (commandCode == 1)
             {
                 int i = parameters.indexOf("F");
                 if (i < 0 && manualFeedSetted)
                 {
                    //Restore the last saved feed
                     manualFeedSetted = false;
                     return tmp + " F" + QString().setNum(lastExplicitFeed);
                 } else if (i >= 0)
                 {
                    QRegExp exp("F(\\d+\\.*\\d*)");//Can feed speed be negative? if so, then change the regex by "F(-*\\d+\\.*\\d*)"
                    if (exp.indexIn(parameters) != -1 && rx.captureCount() > 0)
                    {
                        QStringList lst = exp.capturedTexts();
                        ok = false;
                        float feed = lst.at(1).toFloat(&ok);
                        if (ok)
                            lastExplicitFeed = feed;
                    }
                 }
             }
        }


    }

    return line;
}

void GCodeMarlin::computeCoordinates(const QString &command)
{
    QString tmp = command.trimmed();
    tmp = tmp.toUpper();

    QRegExp rx("G(\\d+)(.*)");

    if (rx.indexIn(tmp) != -1 && rx.captureCount() > 0)
    {
         QStringList list = rx.capturedTexts();
         bool ok = false;
         int commandCode = list.at(1).toInt(&ok);
         QString parameters = list.at(2);

         //Because in marlin we can not pull for position, because that means to break the command buffer in the firmware,
         //and that breaks the look ahead functionality, we manually parse the GCode, and guess the coordinates from there.

         //TODO Add coordinate parsing for G2 and G3 commands. I guess that the simplest way to do so is to calculate the arc end coordinate.
         if (commandCode == 0 || commandCode == 1)
         {
            QStringList components = parameters.split(" ", QString::SkipEmptyParts);
            QString c;
            foreach (c, components)
            {
                float num = c.right(c.size()-1).toFloat();
                if (c.at(0) == 'X')
                {
                    machineCoord.x = num;
                    workCoord.x = num;
                }
                else if (c.at(0) == 'Y')
                {
                    machineCoord.y = num;
                    workCoord.y = num;
                }
                else if (c.at(0) == 'Z')
                {
                    machineCoord.z = num;
                    workCoord.z = num;
                }
            }

            emit updateCoordinates(machineCoord, workCoord);
            emit setLivePoint(workCoord.x, workCoord.y, controlParams.useMm);
         }


    }
}



QString GCodeMarlin::removeUnsupportedCommands(QString line)
{
    return line;
    QStringList components = line.split(" ", QString::SkipEmptyParts);
    QString tmp;
    QString s;
    QString following;
    bool toEndOfLine = false;
    foreach (s, components)
    {
        if (toEndOfLine)
        {
            QString msg(QString(tr("Removed unsupported command '%1' part of '%2'")).arg(s).arg(following));
            warn("%s", qPrintable(msg));
            grblFilteredCmds.append(msg);
            emit addList(msg);
            continue;
        }

        if (s.at(0) == 'G')
        {
            float value = s.mid(1,-1).toFloat();
            if (isGCommandValid(value, toEndOfLine))
                tmp.append(s).append(" ");
            else
            {
                if (toEndOfLine)
                    following = s;
                QString msg(QString(tr("Removed unsupported G command '%1'")).arg(s));
                warn("%s", qPrintable(msg));
                grblFilteredCmds.append(msg);
                emit addList(msg);
            }
        }
        else if (s.at(0) == 'M')
        {
            float value = s.mid(1,-1).toFloat();
            if (isMCommandValid(value))
                tmp.append(s).append(" ");
            else
            {
                QString msg(QString(tr("Removed unsupported M command '%1'")).arg(s));
                warn("%s", qPrintable(msg));
                grblFilteredCmds.append(msg);
                emit addList(msg);
            }
        }
        else if (s.at(0) == 'N')
        {
            // skip line numbers
        }
        else if (s.at(0) == 'X' || s.at(0) == 'Y' || s.at(0) == 'Z'
                 || s.at(0) == 'E'
                 || s.at(0) == 'A' || s.at(0) == 'B' || s.at(0) == 'C'
                 || s.at(0) == 'I' || s.at(0) == 'J' || s.at(0) == 'K'
                 || s.at(0) == 'F' || s.at(0) == 'L' || s.at(0) == 'S')
        {
            tmp.append(s).append(" ");
        }
        else
        {
            QString msg(QString(tr("Removed unsupported command '%1'")).arg(s));
            warn("%s", qPrintable(msg));
            grblFilteredCmds.append(msg);
            emit addList(msg);
        }
    }

    return tmp.trimmed();
}

QString GCodeMarlin::reducePrecision(QString line)
{
    // first remove all spaces to determine what are line length is
    QStringList components = line.split(" ", QString::SkipEmptyParts);
    QString result;
    foreach(QString token, components)
    {
        result.append(token);
    }

    if (result.length() == 0)
        return line;// nothing to do

    if (!result.at(0).isLetter())
        return line;// leave as-is if not a command

    // find first comment and eliminate
    int pos = result.indexOf('(');
    if (pos >= 0)
        result = result.left(pos);

    int charsToRemove = result.length() - controlParams.grblLineBufferLen;

    if (charsToRemove > 0)
    {
        // ok need to do something with precision
        // split apart based on letter
        pos = 0;
        components.clear();
        int i;
        for (i = 1; i < result.length(); i++)
        {
            if (result.at(i).isLetter())
            {
                components.append(result.mid(pos, i - pos));
                pos = i;
            }
        }

        if (pos == 0)
        {
            // we get here if only one command
            components.append(result);
        }
        else
        {
            // add last item
            components.append(result.mid(pos, i));
        }

        QList<DecimalFilter> items;
        foreach (QString tmp, components)
        {
            items.append(DecimalFilter(tmp));
        }

        int totalDecCount = 0;
        int eligibleArgumentCount = 0;
        int largestDecCount = 0;
        for (int j = 0; j < items.size(); j++)
        {
            DecimalFilter& item = items[j];
            pos = item.token.indexOf('.');
            if ((item.token.at(1).isDigit() || item.token.at(1) == '-' || item.token.at(1) == '.') && pos >= 0)
            {
                // candidate to modify
                // count number of decimal places
                int decPlaceCount = 0;
                for (i = pos + 1; i < item.token.length(); i++, decPlaceCount++)
                {
                    if (!item.token.at(i).isDigit())
                        break;
                }

                // skip commands that have a single decimal place
                if (decPlaceCount > 1)
                {
                    item.decimals = decPlaceCount;
                    totalDecCount += decPlaceCount - 1;// leave at least the last decimal place
                    eligibleArgumentCount++;
                    if (decPlaceCount > largestDecCount)
                        largestDecCount = decPlaceCount;
                }
            }
        }

        bool failRemoveSufficientDecimals = false;
        if (totalDecCount < charsToRemove)
        {
            // remove as many as possible, then grbl will truncate
            charsToRemove = totalDecCount;
            failRemoveSufficientDecimals = true;
        }

        if (eligibleArgumentCount > 0)
        {
            for (int k = largestDecCount; k > 1 && charsToRemove > 0; k--)
            {
                for (int j = 0; j < items.size() && charsToRemove > 0; j++)
                {
                    DecimalFilter& item = items[j];
                    if (item.decimals == k)
                    {
                        item.token.chop(1);
                        item.decimals--;
                        charsToRemove--;
                    }
                }
            }

            //chk.clear();
            //chk.append("CORRECTED:");
            result.clear();
            foreach (DecimalFilter item, items)
            {
                result.append(item.token);

                //chk.append("[");
                //chk.append(item.token);
                //chk.append("]");
            }
            //diag(chk.toLocal8Bit().constData());

            err(qPrintable(tr("Unable to remove enough decimal places for command (will be truncated): %s")), line.toLocal8Bit().constData());

            QString msg;
            if (failRemoveSufficientDecimals)
                msg = QString(tr("Error, insufficent reduction '%1'")).arg(result);
            else
                msg = QString(tr("Precision reduced '%1'")).arg(result);

            emit addList(msg);
            emit sendMsg(msg);
        }
    }

    return result;
}

bool GCodeMarlin::isGCommandValid(float value, bool& toEndOfLine)
{
    toEndOfLine = false;
    // supported values obtained from https://github.com/grbl/grbl/wiki
    const static float supported[] =
    {
        0,    1,    2,    3,    4,
        10,    11,    28,    29,    30,    31,
        90,    91,    92
    };

    int len = sizeof(supported) / sizeof(float);
    for (int i = 0; i < len; i++)
    {
        if (value == supported[i])
            return true;
    }

    return false;
}

bool GCodeMarlin::isMCommandValid(float value)
{
    // supported values obtained from https://github.com/ErikZalm/Marlin

    const static float supported[] =
    {
        0, 1, 17, 18, 20, 21, 22, 24, 25, 26, 27, 28, 29, 30, 31, 23, 42, 80, 81, 82, 83, 84,
        85, 92, 104, 105, 106, 107, 109, 114, 115, 117, 119, 126, 127, 128, 129, 140, 150, 190,
        200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 218, 220, 221, 226, 240, 250, 280, 300,
        301, 302, 303, 304, 350, 351, 400, 401, 402, 500, 501, 502, 503, 540, 600, 665, 666, 605,
        907, 908, 928, 999
    };

    int len = sizeof(supported) / sizeof(float);
    for (int i = 0; i < len; i++)
    {
        if (value == supported[i])
            return true;
    }
    return false;
}

QStringList GCodeMarlin::doZRateLimit(QString inputLine, QString& msg, bool& xyRateSet)
{
    // i.e.
    //G00 Z1 => G01 Z1 F100
    //G01 Z1 F260 => G01 Z1 F100
    //G01 Z1 F30 => G01 Z1 F30
    //G01 X1 Y1 Z1 F200 -> G01 X1 Y1 & G01 Z1 F100
    QStringList list;
    QString line = inputLine.toUpper();

    if (line.count("Z") > 0)
    {
        QStringList components = line.split(" ", QString::SkipEmptyParts);
        QString s;
        bool foundFeed = false;
        bool didLimit = false;

        // We need to build one or two command strings depending on input.
        QString x, y, g, f;

        // First get all component parts
        bool inLimit = false;
        foreach (s, components)
        {
            if (s.at(0) == 'G')
            {
                g = s;
            }
            else if (s.at(0) == 'F')
            {
                f = s;

                double value = s.mid(1,-1).toDouble();
                if (value > controlParams.zRateLimitAmount)
                    inLimit = true;
            }
            else if (s.at(0) == 'X')
            {
                x = s;
            }
            else if (s.at(0) == 'Y')
            {
                y = s;
            }
        }

        // Determine whether we want to have one or two command lins
        // 1 string: Have !G0 and F but not in limit
        // 1 string: Have Z only
        // 2 strings: All other conditions
        QString line1;
        QString line2;
        if ((g != "G0" && f.size() > 0 && !inLimit)
                || (x.size() == 0 && y.size() == 0))
        {
            foreach (s, components)
            {
                if (s.at(0) == 'G')
                {
                    int value = s.mid(1,-1).toInt();
                    if (value == 0)
                        line1.append("G1");
                    else
                        line1.append(s);
                }
                else if (s.at(0) == 'F')
                {
                    double value = s.mid(1,-1).toDouble();
                    if (value > controlParams.zRateLimitAmount)
                    {
                        line1.append("F").append(QString::number(controlParams.zRateLimitAmount));
                        didLimit = true;
                    }
                    else
                        line1.append(s);

                    foundFeed = true;
                }
                else
                {
                    line1.append(s);
                }
                line1.append(" ");
            }
        }
        else
        {
            // two lines
            foreach (s, components)
            {
                if (s.at(0) == 'G')
                {
                    int value = s.mid(1,-1).toInt();
                    if (value != 1)
                        line1.append("G1").append(" ");
                    else
                        line1.append(s).append(" ");

                    line2.append(s).append(" ");
                }
                else if (s.at(0) == 'F')
                {
                    double value = s.mid(1,-1).toDouble();
                    if (value > controlParams.zRateLimitAmount)
                    {
                        line1.append("F").append(QString::number(controlParams.zRateLimitAmount));
                        didLimit = true;
                    }
                    else
                        line1.append(s).append(" ");

                    line2.append(s).append(" ");

                    foundFeed = true;
                }
                else if (s.at(0) == 'Z')
                {
                    line1.append(s).append(" ");
                }
                else
                {
                    line2.append(s).append(" ");
                }
            }
        }

        if (!foundFeed)
        {
            line1.append("F").append(QString::number(controlParams.zRateLimitAmount));
            didLimit = true;
        }

        line1 = line1.trimmed();
        line2 = line2.trimmed();

        if (didLimit)
        {
            if (line2.size() == 0)
            {
                msg = QString(tr("Z-Rate Limit: [%1]=>[%2]")).arg(inputLine).arg(line1);
                xyRateSet = true;
            }
            else
            {
                msg = QString(tr("Z-Rate Limit: [%1]=>[%2,%3]")).arg(inputLine).arg(line1).arg(line2);
                line2.append(QString(" F").append(QString::number(controlParams.xyRateAmount)));
            }
        }

        list.append(line1);
        if (line2.size() > 0)
            list.append(line2);
        return list;
    }
    else if (xyRateSet)
    {
        QStringList components = line.split(" ", QString::SkipEmptyParts);
        QString s;

        bool addRateG = false;
        bool addRateXY = false;
        bool gotF = false;
        foreach (s, components)
        {
            if (s.at(0) == 'G')
            {
                int value = s.mid(1,-1).toInt();
                if (value != 0)
                {
                    addRateG = true;
                }
            }
            else if (s.at(0) == 'F')
            {
                gotF = true;
            }
            else
            if (s.at(0) == 'X' || s.at(0) == 'Y' || s.at(0) == 'C')
            {
                addRateXY = true;
            }
        }

        if (addRateG && addRateXY)
        {
            if (!gotF)
            {
                QString line = inputLine;
                line.append(QString(" F").append(QString::number(controlParams.xyRateAmount)));
                msg = QString(tr("XY-Rate Limit FIX: [%1]=>[%2]")).arg(inputLine).arg(line);
                list.append(line);
            }
            else
            {
                list.append(inputLine);
            }

            xyRateSet = false;

            return list;
        }
    }

    list.append(inputLine);
    return list;

}

void GCodeMarlin::gotoXYZFourth(QString line)
{
    pollPosWaitForIdle();

    if (sendGcodeLocal(line))
    {
        line = line.toUpper();

        bool moveDetected = false;

        QStringList list = line.split(QRegExp("[\\s]+"));
        for (int i = 0; i < list.size(); i++)
        {
            QString item = getMoveAmountFromString("X", list.at(i));
            moveDetected = item.length() > 0;

            item = getMoveAmountFromString("Y", list.at(i));
            moveDetected = item.length() > 0;

            item = getMoveAmountFromString("Z", list.at(i));
            moveDetected = item.length() > 0 ;
            if (numaxis == MAX_AXIS_COUNT)  {
                item = getMoveAmountFromString(QString(controlParams.fourthAxisType), list.at(i));
                moveDetected = item.length() > 0;
            }
        }

        if (!moveDetected)
        {
            //emit addList("No movement expected for command.");
        }

        pollPosWaitForIdle();
    }
    else
    {
        QString msg(QString(tr("Bad command: %1")).arg(line));
        warn("%s", qPrintable(msg));
        emit addList(msg);
    }

    emit setCommandText("");
}


QString GCodeMarlin::getMoveAmountFromString(QString prefix, QString item)
{
    int index = item.indexOf(prefix);
    if (index != -1)
        return item.mid(index + 1);

    return "";
}

void GCodeMarlin::axisAdj(char axis, float coord, bool inv, bool absoluteAfterAxisAdj, int sZC)
{
    if (inv)
    {
        coord =- coord;
    }

    QString cmd = QString("G01 ").append(axis)
            .append(QString::number(coord));

    if (axis == 'Z')
    {
        cmd.append(" F").append(QString::number(controlParams.zJogRate));
    }

    SendJog(cmd, absoluteAfterAxisAdj);

    if (axis == 'Z')
        sliderZCount = sZC;

    emit adjustedAxis();
}

bool GCodeMarlin::SendJog(QString cmd, bool absoluteAfterAxisAdj)
{
    pollPosWaitForIdle();

    // G91 = distance relative to previous
    bool ret = sendGcodeLocal("G91\r");

    bool result = ret && sendGcodeLocal(cmd.append("\r"));

    if (result)
    {
        pollPosWaitForIdle();
    }

    if (absoluteAfterAxisAdj)
        sendGcodeLocal("G90\r");

    return result;
}

// settings change calls here
void GCodeMarlin::setResponseWait(ControlParams controlParamsIn)
{
    bool oldMm = controlParams.useMm;

    controlParams = controlParamsIn;

    controlParams.useMm = oldMm;

    port.setCharSendDelayMs(controlParams.charSendDelayMs);

    if ((oldMm != controlParamsIn.useMm) && isPortOpen() && doubleDollarFormat)
    {
        if (controlParamsIn.useMm)
        {
            setConfigureMmMode(true);
        }
        else
        {
            setConfigureInchesMode(true);
        }
    }

    controlParams.useMm = controlParamsIn.useMm;
    numaxis = controlParams.useFourAxis ? MAX_AXIS_COUNT : DEFAULT_AXIS_COUNT;

    setUnitsTypeDisplay(controlParams.useMm);
}

int GCodeMarlin::getSettingsItemCount()
{
    return settingsItemCount.get();
}

void GCodeMarlin::setOldFormatMeasurementUnitControl()
{
    //Marlin only supports mm
    /*if (controlParams.useMm)
        sendGcodeLocal("G21");
    else
        sendGcodeLocal("G20");*/
}

void GCodeMarlin::setConfigureMmMode(bool setGrblUnits)
{
    sendGcodeLocal("$13=0");
    if (setGrblUnits)
        sendGcodeLocal("G21");
    sendGcodeLocal(REQUEST_CURRENT_POS);
}

void GCodeMarlin::setConfigureInchesMode(bool setGrblUnits)
{
    sendGcodeLocal("$13=1");
    if (setGrblUnits)
        sendGcodeLocal("G20");
    sendGcodeLocal(REQUEST_CURRENT_POS);
}

void GCodeMarlin::setUnitsTypeDisplay(bool millimeters)
{
    if (millimeters)
    {
        emit setUnitsWork(tr("(mm)"));
        emit setUnitsMachine(tr("(mm)"));
    }
    else
    {
        emit setUnitsWork(tr("(in)"));
        emit setUnitsMachine(tr("(in)"));
    }
}

void GCodeMarlin::clearToHome()
{
    maxZ = 0;
    motionOccurred = false;
}

int GCodeMarlin::getNumaxis()
{
    return numaxis;
}

bool GCodeMarlin::probeResultToValue(const QString & result, double &zCoord)
{
    QRegExp rx("Z:(-*\\d+\\.\\d+)");
    if (rx.indexIn(result) != -1 && rx.captureCount() > 0)
    {
        QStringList list = rx.capturedTexts();
        bool ok = false;
        zCoord = list.at(1).toDouble(&ok);
        if (ok)
        {
           return true;
        } else {
            return false;
        }
    }

}

void GCodeMarlin::performZLeveling(QRect extent, int xSteps, int ySteps, double zSafe)
{
    cout << "Starting Z Leveling procedure" << endl;
    if (interpolator != NULL)
    {
        delete interpolator;

    }

    double * xValues = new double[xSteps];
    double * yValues = new double[ySteps];
    double * zValues = new double[xSteps * ySteps];

    double xLenght = extent.right() - extent.left();
    double yLenght = extent.bottom() - extent.top();

    cout << "xLenght: " <<xLenght << "ylen: " << yLenght << endl;

    double xInterval = xLenght / (xSteps - 1);
    double yInterval = yLenght / (ySteps - 1);

    cout << "xInterval: " << xInterval << " yInterval: " << yInterval << endl;

    for (int i = 0; i<xSteps; i++)
    {
        xValues[i] = i*xInterval;
    }

    for (int j = 0; j<ySteps; j++)
    {
        yValues[j] = j*yInterval;
    }

    //We do not have to home XY here, and we assume that the coordinates X0 and Y0 are already setted as the good starting point for the leveling.
    //TODO Load the F speed for G0 commands
    sendGcodeLocal("G90\r");
    sendGcodeLocal("G28 Z0\r");
    sendGcodeLocal("G0 X0 Y0 F300\r");
    //Get the first ZDepth in the 0,0 coordinate.
    QString res;
    double zCoord = 0.0d;
    //TODO BORRAR
    sendGcodeInternal("G0 Z25 F200", res, false, 0);


    sendGcodeInternal("G30", res, false, 0);
    cout << "Result: " << res.toStdString() << endl;

    if (!probeResultToValue(res, zCoord))
    {
        cout << "ERROR CONVERTING ZCOORDINATE" << endl;
        return;
    }

    double zSafeCoord = zCoord + zSafe;
    cout << "Safe coordinate: " << zSafeCoord << endl;
    sendGcodeLocal(QString("G1 Z").append(QString::number(zSafeCoord)));

    //sendGcodeLocal(QString("G1 Z").append(QString::number(zSafe)).append(" F100\r"));

    pollPosWaitForIdle();

    for (int i = 0; i < xSteps; i++)
    {
        for (int j = 0; j < ySteps; j++)
        {

            cout << "Probing in: " << xValues[i] << " - " << yValues[j] << endl;
            sendGcodeInternal(QString("G1 X").
                           append(QString::number(xValues[i])).
                                  append(" Y").
                                  append(QString::number(yValues[j])).append(" F300"), res, false, 0);
            sendGcodeInternal("G30", res, false, 0);
            if (!probeResultToValue(res, zCoord))
            {
                cout << "ERROR CONVERTING ZCOORDINATE AT " << xValues[i] <<  " - " << yValues[j] << endl;
                return;
            }
            zValues[j*xSteps + i] = zCoord;
            zSafeCoord = zCoord + zSafe;
            cout << "----Probe: " << xValues[i] << " - " << yValues[j] << " - " << zValues[j*xSteps + i] << endl;
            sendGcodeInternal(QString("G1 Z").append(QString::number(zSafeCoord)).append(" F100"), res, false, 0);
        }
        i++;
        if (i >= xSteps)
        {
            break;
        }
        for (int j = ySteps - 1; j >= 0; j--)
        {
            cout << "Probing in: " << xValues[i] << " - " << yValues[j] << endl;
            sendGcodeInternal(QString("G1 X").
                           append(QString::number(xValues[i])).
                                  append(" Y").
                                  append(QString::number(yValues[j])).append(" F300"), res, false, 0);
            sendGcodeInternal("G30", res, false, 0);
            if (!probeResultToValue(res, zCoord))
            {
                cout << "ERROR CONVERTING ZCOORDINATE AT " << xValues[i] <<  " - " << yValues[j] << endl;
                return;
            }
            zValues[j*xSteps + i] = zCoord;
            zSafeCoord = zCoord + zSafe;
            cout << "----Probe: " << xValues[i] << " - " << yValues[j] << " - " << zValues[j*xSteps + i] << endl;
            sendGcodeInternal(QString("G1 Z").append(QString::number(zSafeCoord)).append(" F100"), res, false, 0);
        }

    }

    interpolator = new SpilineInterpolate3D(xValues, xSteps, yValues, ySteps, zValues);

}
