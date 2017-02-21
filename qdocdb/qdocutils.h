#ifndef QDOCUTILS_H
#define QDOCUTILS_H

#include <QJsonValue>
#include <QByteArray>
#include <QString>

class QDocSerialize {
    enum levelType {
        typeValue = 0x0,
        typeJsonDocument = 0x1
    };
    enum Type {
        typeUndefined = 0x0,
        typeNull = 0x1,
        typeBool = 0x2,
        typeDouble = 0x3,
        typeString = 0x4,
        typeArray = 0x5,
        typeObject = 0x6
    };

public:
    static QByteArray marshal(QJsonValue value);
    static QJsonValue unmarshal(QByteArray& bytes);
};

class QDocIdGen {
    quint64 counter;
public:
    QString getId();
    QDocIdGen();
    ~QDocIdGen();
};

class QDocUtils {
public:
    static QJsonValue getJsonValueByPath(QJsonValue jsonValue, QString path);
    static QJsonArray multiply(QJsonArray& a, QJsonArray& b);
    static bool compare(QJsonValue& a, QJsonValue& b, QString oper);
};

#endif // QDOCUTILS_H
