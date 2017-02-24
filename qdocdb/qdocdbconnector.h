#ifndef QDOCDBCONNECTOR_H
#define QDOCDBCONNECTOR_H

#include "qdocdatabase.h"

#include <QVariantList>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>

class QDocdbConnector : public QObject {
    Q_OBJECT
    QString _database;
    QString _collection;
    QJsonObject _query;
    QJsonArray _value;
    QDocDatabase* pDatabase;

    QString lastError;

public:
    Q_PROPERTY(QString database READ database WRITE setDatabase)
    Q_PROPERTY(QString collection READ collection WRITE setCollection)
    Q_PROPERTY(QJsonObject query READ query WRITE setQuery)
    Q_PROPERTY(QJsonArray value READ value WRITE setValue NOTIFY valueChanged)

    QString database() { return this->_database; }
    QString collection() { return this->_collection; }
    QJsonObject query() { return this->_query; }
    QJsonArray value() { return this->_value; }

    void setDatabase(QString);
    void setCollection(QString);
    void setQuery(QJsonObject);
    void setValue(QJsonArray);

    enum resultEnum {
        success,
        error
    };

    Q_ENUM(resultEnum);

    Q_INVOKABLE resultEnum insert(QJsonObject query);
    Q_INVOKABLE resultEnum remove(QJsonObject query);
    Q_INVOKABLE resultEnum removeId(QString id);
    Q_INVOKABLE resultEnum removeQueryResults();
    Q_INVOKABLE resultEnum createIndex(QString field, QString indexName);

    Q_INVOKABLE resultEnum newSnapshot(QString snapshotName);
    Q_INVOKABLE resultEnum revertToSnapshot(QString snapshotName);
    Q_INVOKABLE resultEnum removeSnapshot(QString snapshotName);

    void observe();
    void unobserve();

    bool isValid();

    QDocdbConnector();
    ~QDocdbConnector();

signals:
    void valueChanged();
public slots:
    void observeQueryChanged(QJsonArray&);
};

#endif // QDOCDBCONNECTOR_H
