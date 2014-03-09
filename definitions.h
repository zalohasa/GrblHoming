/****************************************************************
 * definitions.h
 * GrblHoming - zapmaker fork on github
 *
 * 15 Nov 2012
 * GPL License (see LICENSE file)
 * Software is provided AS-IS
 ****************************************************************/

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <QtGlobal>
#include <QString>
#include <QDateTime>
#include "atomicintbool.h"

#define DEFAULT_WAIT_TIME_SEC   100

#define DEFAULT_Z_JOG_RATE      260.0
#define DEFAULT_Z_LIMIT_RATE    100.0
#define DEFAULT_XY_RATE         2000.0

#define DEFAULT_GRBL_LINE_BUFFER_LEN    50
#define DEFAULT_CHAR_SEND_DELAY_MS      0

#define MM_IN_AN_INCH           25.4
#define PRE_HOME_Z_ADJ_MM       5.0

#define SETTINGS_COMMAND_V08a           "$"
#define SETTINGS_COMMAND_V08c           "$$"

#define SET_UNLOCK_STATE_V08c           "$X"

#define REGEXP_SETTINGS_LINE    "(\\d+)\\s*=\\s*([\\w\\.]+)\\s*\\(([^\\)]*)\\)"

#define OPEN_BUTTON_TEXT                "Open"
#define CLOSE_BUTTON_TEXT               "Close / Reset"

#define LOG_MSG_TYPE_DIAG       "DIAG"
#define LOG_MSG_TYPE_STATUS     "STATUS"

extern AtomicIntBool g_enableDebugLog;

void status(const char *str, ...);
void diag(const char *str, ...);
void err(const char *str, ...);
void warn(const char *str, ...);
void info(const char *str, ...);

#endif // DEFINITIONS_H
