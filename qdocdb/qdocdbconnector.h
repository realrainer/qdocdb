#ifndef QDOCDBCONNECTOR_H
#define QDOCDBCONNECTOR_H

#include <QVariantList>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QQuickItem>

#include "qdocdbclient.h"

class QDocdbConnector : public QQuickItem  {
    Q_OBJECT
    QString _url;
    bool urlValid;
    bool initComplete;

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
    Q_PROPERTY(QJsonObject valueOne READ valueOne WRITE setValueOne NOTIFY valueOneChanged)
    Q_PROPERTY(bool valid READ valid NOTIFY validChanged)
    Q_PROPERTY(QString err READ err NOTIFY errChanged)

    QString url() { return this->_url; }
    QJsonObject query() { return this->_query; }
    QJsonObject queryOptions() { return this->_queryOptions; }
    QJsonArray value() { return this->_value; }
    QJsonObject valueOne();
    QString err() { return this->lastError; }
    bool valid();

    void setUrl(QString);
    void setQuery(QJsonObject);
    void setQueryOptions(QJsonObject);
    void setValue(QJsonArray);
    void setValueOne(QJsonObject);

    void setLastError(QString);

    enum resultEnum {
        success,
        error
    };

    Q_ENUM(resultEnum);

    Q_INVOKABLE resultEnum removeQueryResults();

    Q_INVOKABLE resultEnum insert(QJsonObject doc, int transactionId = -1);
    Q_INVOKABLE resultEnum remove(QJsonObject query, int transactionId = -1);
    Q_INVOKABLE resultEnum removeId(QString id, int transactionId = -1);
    Q_INVOKABLE resultEnum createIndex(QString field, QString indexName);

    Q_INVOKABLE int newTransaction();
    Q_INVOKABLE resultEnum writeTransaction(int transactionId);
    Q_INVOKABLE resultEnum discardTransaction(int transactionId);

    Q_INVOKABLE resultEnum newSnapshot(QString snapshotName);
    Q_INVOKABLE resultEnum revertToSnapshot(QString snapshotName);
    Q_INVOKABLE resultEnum removeSnapshot(QString snapshotName);

    Q_INVOKABLE QVariantList find(QJsonObject query, QString snapshot = "__CURRENT");
    Q_INVOKABLE QVariantMap findOne(QJsonObject query, QString snapshot = "__CURRENT");
    Q_INVOKABLE int count(QJsonObject query, QString snapshot = "__CURRENT");

    void observe();
    void unobserve();

    QDocdbConnector();
    ~QDocdbConnector();

protected:
    void componentComplete();

signals:
    void valueChanged();
    void valueOneChanged();
    void errChanged();
    void validChanged();
public slots:
    void observeQueryChanged(int, QJsonArray&);
};

#endif // QDOCDBCONNECTOR_H
