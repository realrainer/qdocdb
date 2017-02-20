#ifndef QDOCKVINTERFACE_H
#define QDOCKVINTERFACE_H

#include <QByteArray>
#include <QString>

class QDocKVIterator {
public:
    virtual void seekToFirst() = 0;
    virtual void seek(QByteArray key) = 0;
    virtual bool isValid() = 0;
    virtual void next() = 0;
    virtual QByteArray key() = 0;
    virtual QByteArray value() = 0;

    virtual ~QDocKVIterator() {}
};

class QDocKVInterface {

protected:
    QString lastError;
    bool valid;

public:
    enum resultEnum {
        success = 0x0,
        errorIsNotTransaction = 0x1,
        errorOperation = 0x2
    };

    QString getLastError();
    bool isValid();

    virtual int getValue(QByteArray key, QByteArray& value) = 0;
    virtual int putValue(QByteArray key, QByteArray value) = 0;
    virtual int deleteValue(QByteArray key) = 0;

    virtual QDocKVInterface* newTransaction() = 0;
    virtual QDocKVIterator* newIterator() = 0;
    virtual int writeTransaction() = 0;

    virtual ~QDocKVInterface() {}
};


#endif // QDOCKVINTERFACE_H
