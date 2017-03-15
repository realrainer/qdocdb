#ifndef QDOCDBCONNECTOR_H
#define QDOCDBCONNECTOR_H

#include "qdocdbclient.h"
#include "qdocdatabase.h"

#include <QVariantList>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>

class QDocdbConnector : public QObject {
    Q_OBJECT
    QString _url;
    bool urlValid;
    QJsonObject _query;
    QJsonObject _queryOptions;
    QJsonArray _value;

    QString lastError;

    int observeId;

    QDocdbClient* dbClient;

public:
    Q_PROPERTY(QString url READ url WRITE setUrl)
    Q_PROPERTY(QJsonObject query READ query WRITE setQuery)
    Q_PROPERTY(QJsonObject queryOptions READ queryOptions WRITE setQueryOptions)
    Q_PROPERTY(QJsonArray value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(QString err READ err NOTIFY errChanged)

    QString url() { return this->_url; }
    QJsonObject query() { return this->_query; }
    QJsonObject queryOptions() { return this->_queryOptions; }
    QJsonArray value() { return this->_value; }
    QString err() { return this->lastError; }

    void setUrl(QString);
    void setQuery(QJsonObject);
    void setQueryOptions(QJsonObject);
    void setValue(QJsonArray);

    void setLastError(QString);

    enum resultEnum {
        success,
        error
    };

    Q_ENUM(resultEnum);

    Q_INVOKABLE resultEnum removeQueryResults();

    Q_INVOKABLE resultEnum insert(QJsonObject doc);
    Q_INVOKABLE resultEnum remove(QJsonObject query);
    Q_INVOKABLE resultEnum removeId(QString id);
    Q_INVOKABLE resultEnum createIndex(QString field, QString indexName);

    Q_INVOKABLE resultEnum newSnapshot(QString snapshotName);
    Q_INVOKABLE resultEnum revertToSnapshot(QString snapshotName);
    Q_INVOKABLE resultEnum removeSnapshot(QString snapshotName);

    Q_INVOKABLE QVariantList find(QJsonObject query, QString snapshot = "__CURRENT");

    void observe();
    void unobserve();

    bool isValid();

    QDocdbConnector();
    ~QDocdbConnector();

signals:
    void valueChanged();
    void errChanged();
public slots:
    void observeQueryChanged(QJsonArray&);
};

#endif // QDOCDBCONNECTOR_H
