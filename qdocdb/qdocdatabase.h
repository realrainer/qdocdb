#ifndef QDOCDATABASE_H
#define QDOCDATABASE_H

#include <QString>
#include <QList>
#include <QHash>
#include <QObject>
#include "qdoccollection.h"
#include "qdocutils.h"

class QDocDatabase : public QObject {

    Q_OBJECT

    bool isOpened;
    QString baseDir;
    QList<QDocCollection*> collList;
    QDocIdGen idGen;

public:
    enum resultEnum {
        success = 0x0,
        errorDatabase = 0x1
    };

    int open(QString baseDir);
    void close();

    QDocCollection* collection(QString collName, bool inMemory = false);

    QDocDatabase();
    ~QDocDatabase();
};

#endif // QDOCDATABASE_H
