
#include "qdockvinterface.h"

QString QDocKVInterface::getLastError() {
    return this->lastError;
}

bool QDocKVInterface::isValid() {
    return this->valid;
}
