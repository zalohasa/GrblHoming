#include "gcommands.h"
#include "basicgeometry.h"

#include <QRegExp>
#include <QStringList>

#define X_PARAMETER 'X'
#define Y_PARAMETER 'Y'
#define Z_PARAMETER 'Z'
#define F_PARAMETER 'F'

MCodeCommand::MCodeCommand(int command, const QString &parameters)
{
    this->command = command;
    this->parameters = parameters;
}

MCodeCommand::MCodeCommand(const QString &commandLine) throw (CodeCommandException)
{
    QRegExp rx("M(\\d+)(.*)");

    if (rx.indexIn(commandLine) != -1 && rx.captureCount() > 0)
    {
        QStringList list = rx.capturedTexts();
        bool ok = false;
        int commandCode = list.at(1).toInt(&ok);
        QString parameters = "";
        if (list.size()>1)
            parameters = list.at(2);
        if (ok){
            this->command = commandCode;
            this->parameters = parameters;
        } else {
            throw CodeCommandException("Error parsing command code");
        }
    } else {
        throw CodeCommandException("Error parsing command line");
    }
}

QString MCodeCommand::toString() const
{
    return "M" + QString::number(command).append(" ").append(parameters);
}

GCodeCommand::GCodeCommand(int command, const QString &parameters, const char fourthName) : x(0), y(0), z(0), fourth(0),fourthName(fourthName)
{
    this->floatPrecission = 4;
    this->command = command;
    this->parameters = parameters;
    QStringList components = parameters.split(" ", QString::SkipEmptyParts);
    QString c;
    foreach (c, components)
    {
        bool ok = false;
        double num = c.right(c.size()-1).toDouble(&ok);
        //Skip bad formatted parameters.
        if (!ok)
            continue;

        char p = c.at(0).toLatin1();

        if (p == X_PARAMETER)
        {
            parameterMap.insert(X_PARAMETER, num);
            x = num;
        }
        else if (p == Y_PARAMETER)
        {
            parameterMap.insert(Y_PARAMETER, num);
            y = num;
        }
        else if (p == Z_PARAMETER)
        {
            parameterMap.insert(Z_PARAMETER, num);
            z = num;
        }
        else if (p == fourthName)
        {
            parameterMap.insert(fourthName, num);
            fourth = num;
        }
        else if (p == F_PARAMETER)
        {
            parameterMap.insert(F_PARAMETER, num);
            f = num;
        }
        else
        {
            parameterMap.insert(p, num);
        }
    }
}

GCodeCommand::GCodeCommand(const GCodeCommand &command) : fourthName(command.fourthName)
{
    this->command = command.command;
    x = command.x;
    y = command.y;
    z = command.z;
    fourth = command.fourth;
    f = command.f;
    parameterMap = command.parameterMap;
    parameters = command.parameters;
}

QString GCodeCommand::toString() const
{
    QString c = "G";
    c.append(QString::number(command)).append(" ");
    //Clone the parameters map so we can remove elements.
    QMap<char, double> p = this->parameterMap;

    if (p.contains(X_PARAMETER))
    {
        c.append("X").append(QString::number(x/*, 'f', floatPrecission*/)).append(" ");
        p.remove(X_PARAMETER);
    }
    if (p.contains(Y_PARAMETER))
    {
        c.append("Y").append(QString::number(y/*, 'f', floatPrecission*/)).append(" ");
        p.remove(Y_PARAMETER);
    }
    if (p.contains(Z_PARAMETER))
    {
        c.append("Z").append(QString::number(z/*, 'f', floatPrecission*/)).append(" ");
        p.remove(Z_PARAMETER);
    }
    if (p.contains(fourthName))
    {
        c.append(QString(fourthName)).append(QString::number(fourth/*, 'f', floatPrecission*/)).append(" ");
        p.remove(fourthName);
    }
    QString fparam = "";
    if (p.contains(F_PARAMETER))
    {
        fparam.append("F").append(QString::number(f/*, 'f', floatPrecission*/)).append(" ");
        p.remove(F_PARAMETER);
    }
    QMap<char,double>::const_iterator i = p.constBegin();
    while (i != p.constEnd())
    {
        c.append(i.key()).append(QString::number(i.value())).append(" ");
        i++;
    }
    c.append(fparam);

    return c;
}

void GCodeCommand::setFloatPrecission(int precission) const
{
    this->floatPrecission = precission;
}

bool GCodeCommand::getX(double &x) const
{
    if (parameterMap.contains(X_PARAMETER))
    {
        x = this->x;
        return true;
    }
    return false;
}

bool GCodeCommand::getY(double &y) const
{
    if (parameterMap.contains(Y_PARAMETER))
    {
        y = this->y;
        return true;
    }
    return false;
}


bool GCodeCommand::getZ(double &z) const
{
    if (parameterMap.contains(Z_PARAMETER))
    {
        z = this->z;
        return true;
    }
    return false;
}

bool GCodeCommand::getX(float &x) const
{
    if (parameterMap.contains(X_PARAMETER))
    {
        x = float(this->x);
        return true;
    }
    return false;
}

bool GCodeCommand::getY(float &y) const
{
    if (parameterMap.contains(Y_PARAMETER))
    {
        y = float(this->y);
        return true;
    }
    return false;
}


bool GCodeCommand::getZ(float &z) const
{
    if (parameterMap.contains(Z_PARAMETER))
    {
        z = float(this->z);
        return true;
    }
    return false;
}


bool GCodeCommand::getF(double &f) const
{
    if (parameterMap.contains(F_PARAMETER))
    {
        f = this->f;
        return true;
    }
    return false;
}

bool GCodeCommand::getFourth(double &fourth) const
{
    if (parameterMap.contains(fourthName))
    {
        fourth = this->fourth;
        return true;
    }
    return false;
}

bool GCodeCommand::getParameter(char param, double &value) const
{
    if (parameterMap.contains(param))
    {
        value = parameterMap.value(param);
        return true;
    }
    return false;
}

void GCodeCommand::setX(double x)
{
    parameterMap.insert(X_PARAMETER, x);
    this->x = x;
}

void GCodeCommand::setY(double y)
{
    parameterMap.insert(Y_PARAMETER, y);
    this->y = y;
}

void GCodeCommand::setZ(double z)
{
    parameterMap.insert(Z_PARAMETER, z);
    this->z = z;
}

void GCodeCommand::setF(double f)
{
    parameterMap.insert(F_PARAMETER, f);
    this->f = f;
}

void GCodeCommand::setFourth(double fourth)
{
    parameterMap.insert(fourthName, fourth);
    this->fourth = fourth;
}

void GCodeCommand::setPoint(const Point &p)
{
    parameterMap.insert(X_PARAMETER, p.x);
    parameterMap.insert(Y_PARAMETER, p.y);
    parameterMap.insert(Z_PARAMETER, p.z);
    x = p.x;
    y = p.y;
    z = p.z;
}

void GCodeCommand::setParameter(char param, double value)
{
    parameterMap.insert(param, value);
    if (param == X_PARAMETER)
    {
        x = value;
    }
    else if (param == Y_PARAMETER)
    {
        y = value;
    }
    else if (param == Z_PARAMETER)
    {
        z = value;
    }
    else if (param == fourthName)
    {
        fourth = value;
    }
    else if (param == F_PARAMETER)
    {
        f = value;
    }
}



