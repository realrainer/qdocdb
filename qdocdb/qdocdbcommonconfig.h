#ifndef QDOCDBCOMMONCONFIG_H
#define QDOCDBCOMMONCONFIG_H

#include <QByteArray>
#include <QString>

class QDocIdGen {
    quint64 counter;
    int nextNumber;
protected:
    QByteArray getRandomBytes(int);
public:
    QByteArray getId();
    virtual QByteArray getUUID();
    int getNextNumber();
    QDocIdGen();
    virtual ~QDocIdGen();
};

class QDocdbCommonConfig {

    QDocIdGen* idGen;
    bool isCustomIdGen;

    QString readOnlyKey;
    bool readOnlyKeyValid;
    QString UUIDPath;
    bool UUIDPathValid;

public:
    QDocIdGen* getIdGen();
    QString getReadOnlyKey();
    QString getUUIDPath();
    bool isReadOnlyKeyValid();
    bool isUUIDPathValid();

    QDocdbCommonConfig(QDocIdGen* idGen = NULL, QString UUIDPath = "", QString readOnlyKey = "");
    ~QDocdbCommonConfig();
};

#endif // QDOCDBCOMMONCONFIG_H
