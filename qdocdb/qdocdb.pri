
QT += testlib

DEPENDPATH += $$PWD
SOURCE_DIR = $$PWD
INCLUDEPATH += $$SOURCE_DIR

SOURCES += \
    $$SOURCE_DIR/qdoccollection.cpp \
    $$SOURCE_DIR/qdocdatabase.cpp \
    $$SOURCE_DIR/qdocutils.cpp \
    $$PWD/qdocdbconnector.cpp \
    $$PWD/qdockvleveldb.cpp \
    $$PWD/qdockvinterface.cpp \
    $$PWD/qdockvmap.cpp \
    $$PWD/qdocdbserver.cpp \
    $$PWD/qdocdblinkbase.cpp \
    $$PWD/qdocdbclient.cpp \
    $$PWD/qdocdbcommonconfig.cpp

HEADERS += \
    $$SOURCE_DIR/qdoccollection.h \
    $$SOURCE_DIR/qdocdatabase.h \
    $$SOURCE_DIR/qdocutils.h \
    $$PWD/qdocdbconnector.h \
    $$PWD/qdockvinterface.h \
    $$PWD/qdockvleveldb.h \
    $$PWD/qdockvmap.h \
    $$PWD/qdocdbserver.h \
    $$PWD/qdocdblinkbase.h \
    $$PWD/qdocdbclient.h \
    $$PWD/qdocdbcommonconfig.h

include(leveldb/leveldb.pri)
