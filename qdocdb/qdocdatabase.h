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
    QHash<int, QObject*> observeHash;

public:
    enum resultEnum {
        success = 0x0,
        errorDatabase = 0x1
    };

    int open(QString baseDir);
    void close();

    int observe(QDocCollection* pColl, QJsonObject& query, QObject* pObject);
    int unobserve(QDocCollection* pColl, QObject* pObject);

    QDocCollection* collection(QString collName);

    QDocDatabase();
    ~QDocDatabase();
public slots:
    void observeQueryChanged(int, QJsonArray&);

};

#endif // QDOCDATABASE_H
