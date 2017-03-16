
#include <QDebug>
#include <QFile>

#include "service.h"

void MainService::onTimer() {
    this->doc["counter"] = this->doc["counter"].toInt() + 1;
    if (!this->dbclient->isConnected()) {
        this->dbclient->connectToServer("qdocdblocal");
    }
    if (this->dbclient->isConnected()) {
        QByteArray docId;
        int r = this->dbclient->insert("qdocdb://qdocdblocal/testdb?collection=coll&persistent=false", this->doc, docId, true);
        if (r == QDocdbClient::success) {
            if (this->doc["_id"].toString().isEmpty()) {
                this->doc["_id"] = QString::fromLatin1(docId);
            }
        }
    }
}

MainService::MainService() {
    this->dbserver = new QDocdbServer("qdocdblocal");
    this->dbUrl = "qdocdb://qdocdblocal/testdb?collection=coll&persistent=false";
    this->dbclient = new QDocdbClient();

    this->doc["docType"] = QVariant("exchanger1");

    timer = new QTimer();
    timer->setInterval(1000);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start();
}

MainService::~MainService() {
    timer->stop();
    delete timer;
    delete this->dbclient;
    if (this->dbserver != NULL) {
        delete this->dbserver;
    }
}
