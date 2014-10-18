#ifndef GCOMMANDS_H
#define GCOMMANDS_H

#include <exception>
#include <QString>
#include <QMap>
#include "basicgeometry.h"

class CodeCommandException : public std::exception
{
public:
    CodeCommandException(const QString &msg) {this->msg = msg; }
    virtual ~CodeCommandException() throw () {}
    QString getMessage() { return msg; }
private:
    QString msg;
};

class CodeCommand
{
public:
    enum command_type_t {G_COMMAND, M_COMMAND};
    virtual ~CodeCommand() {}
    virtual QString toString() const = 0;
    virtual command_type_t getType() const = 0;
    int getCommand() const { return command; }
protected:
    int command;
};


class MCodeCommand : public CodeCommand
{
public:
    MCodeCommand(int command, const QString &parameters);
    MCodeCommand(const QString &commandLine) throw (CodeCommandException);
    ~MCodeCommand(){}

    QString toString() const;

    command_type_t getType() const {return M_COMMAND;}

    QString getParameters() const {return parameters;}
private:
    QString parameters;
};

class GCodeCommand : public CodeCommand
{
public:
    GCodeCommand(int command, const QString &parameters, const char fourthName = 'E');
    GCodeCommand(const GCodeCommand& command);
    ~GCodeCommand(){}

    command_type_t getType() const {return G_COMMAND;}

    QString toString() const;
    bool getX(double& x) const;
    bool getY(double& y) const;
    bool getZ(double& z) const;
    bool getX(float& x) const;
    bool getY(float& y) const;
    bool getZ(float& z) const;

    bool getF(double& f) const;
    bool getFourth(double &fourth) const;

    void setX(double x);
    void setY(double y);
    void setZ(double z);
    void setF(double f);
    void setFloatPrecission(int precission) const;
    void setPoint(const Point &p);
    void setFourth(double fourth);
    void setParameter(char param, double value);
    bool getParameter(char param, double &value) const;

    QString getParameters() const {return parameters;}

private:
    double x,y,z,f,fourth;
    mutable int floatPrecission;
    QString parameters;
    const char fourthName;
    QMap<char, double> parameterMap;
};

#endif // GCOMMANDS_H
