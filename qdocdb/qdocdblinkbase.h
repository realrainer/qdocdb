#ifndef QDOCDBLINKBASE_H
#define QDOCDBLINKBASE_H

#include <QVariantMap>
#include <QVariantList>
#include <QTcpSocket>
#include <QSignalSpy>
#include <QMap>

const QLatin1String QDOCDB_ORGANIZATION = QLatin1String("com.example");
const QLatin1String QDOCDB_APPLICATION = QLatin1String("qdocdb");

class QDocdbLinkObject : public QObject {
    Q_OBJECT
    QVariantMap fMap;

    void assignType();

public:
    enum LinkObjectType {
        typeFind,
        typeFindReply,
        typeCreateIndex,
        typeNewTransaction,
        typeNewTransactionReply,
        typeWriteTransaction,
        typeDiscardTransaction,
        typeInsert,
        typeInsertReply,
        typeRemove,
        typeRemoveId,
        typeSet,
        typeNewSnapshot,
        typeRevertToSnapshot,
        typeRemoveSnapshot,
        typeObserve,
        typeObserveReply,
        typeObserveData,
        typeUnobserve,
        typeUnknown
    };

    static const int waitForDoneTimeout = 10000;

    static QDocdbLinkObject* newLinkObject(int id, QDocdbLinkObject::LinkObjectType type);

    void marshal(QByteArray&);
    void unmarshal(QByteArray&);
    bool isEmpty();

    void set(QString key, QVariant value);
    QVariant get(QString key);

    QVariantMap& map();

    LinkObjectType getType();

    QSignalSpy* signalSpy;
    bool waitForDone();

    QDocdbLinkObject(QVariantMap& fMap);
    QDocdbLinkObject();
    ~QDocdbLinkObject();

protected:
    LinkObjectType type;

signals:
    void done();
};

const QMap<QDocdbLinkObject::LinkObjectType, QString> QDOCDB_FUNCMAP({
    { QDocdbLinkObject::typeFind, "find" },
    { QDocdbLinkObject::typeFindReply, "findReply" },
    { QDocdbLinkObject::typeCreateIndex ,"createIndex" },
    { QDocdbLinkObject::typeNewTransaction, "newTransaction" },
    { QDocdbLinkObject::typeNewTransactionReply, "newTransactionReply" },
    { QDocdbLinkObject::typeWriteTransaction, "writeTransaction" },
    { QDocdbLinkObject::typeDiscardTransaction, "discardTransaction" },
    { QDocdbLinkObject::typeInsert, "insert" },
    { QDocdbLinkObject::typeInsertReply, "insertReply" },
    { QDocdbLinkObject::typeRemove, "remove" },
    { QDocdbLinkObject::typeRemoveId, "removeId" },
    { QDocdbLinkObject::typeSet, "set" },
    { QDocdbLinkObject::typeNewSnapshot, "newSnapshot" },
    { QDocdbLinkObject::typeRevertToSnapshot, "revertToSnapshot" },
    { QDocdbLinkObject::typeRemoveSnapshot, "removeSnapshot" },
    { QDocdbLinkObject::typeObserve, "observe" },
    { QDocdbLinkObject::typeObserveReply, "observeReply" },
    { QDocdbLinkObject::typeObserveData, "observeData" },
    { QDocdbLinkObject::typeUnobserve, "unobserve" },
    { QDocdbLinkObject::typeUnknown, "unknown" }
});

class QDocdbLinkBase : public QObject {
    Q_OBJECT

    QTcpSocket* socket;
    QByteArray rxData;
    int rxDataLength;

public:
    QTcpSocket* getSocket();
    void send(QDocdbLinkObject* linkObject);
    void parse(QByteArray* data);
    void close();
    QDocdbLinkBase(QTcpSocket* socket);
    ~QDocdbLinkBase();

public slots:
    void readyRead();
    void disconnected();

signals:
    void receive(QDocdbLinkObject*);
    void clientRemoved();
};

#endif // QDOCDBLINKBASE_H
