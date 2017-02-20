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

    Q_INVOKABLE void insert(QJsonObject);
    Q_INVOKABLE void remove(QJsonObject);
    Q_INVOKABLE void removeId(QString);
    Q_INVOKABLE void removeQueryResults();
    Q_INVOKABLE QString createIndex(QString, QString);

    void observe();
    void unobserve();

    QDocdbConnector();
    ~QDocdbConnector();

signals:
    void valueChanged();
public slots:
    void observeQueryChanged(QJsonArray&);
};

#endif // QDOCDBCONNECTOR_H
