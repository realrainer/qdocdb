#ifndef QDOCDBCOMMONCONFIG_H
#define QDOCDBCOMMONCONFIG_H

#include <QByteArray>
#include <QString>

class QDocIdGen {
    quint64 counter;
public:
    QByteArray getId();
    QDocIdGen();
    ~QDocIdGen();
};

class QDocdbCommonConfig {

    QDocIdGen* idGen;
    bool isCustomIdGen;

    QString readOnlyKey;
    bool readOnlyKeyValid;

public:
    QDocIdGen* getIdGen();
    QString getReadOnlyKey();
    bool isReadOnlyKeyValid();

    QDocdbCommonConfig(QDocIdGen* idGen = NULL, QString readOnlyKey = "");
    ~QDocdbCommonConfig();
};

#endif // QDOCDBCOMMONCONFIG_H
