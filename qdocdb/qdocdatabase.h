#ifndef QDOCDATABASE_H
#define QDOCDATABASE_H

#include <QString>
#include <QList>
#include <QObject>
#include "qdoccollection.h"
#include "qdocdbcommonconfig.h"

class QDocDatabase : public QObject {

    Q_OBJECT

    bool isOpened;
    QString baseDir;
    QList<QDocCollection*> collList;

    QDocdbCommonConfig* commonConfig;

public:
    enum resultEnum {
        success = 0x0,
        errorDatabase = 0x1
    };

    int open(QString baseDir);
    void close();

    QDocCollection* collection(QString collName, bool inMemory = false);

    QDocDatabase(QDocdbCommonConfig* commonConfig);
    ~QDocDatabase();
};

#endif // QDOCDATABASE_H
