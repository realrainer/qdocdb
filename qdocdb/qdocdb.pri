
DEPENDPATH += $$PWD
SOURCE_DIR = $$PWD
INCLUDEPATH += $$SOURCE_DIR

SOURCES += \
    $$SOURCE_DIR/qdoccollection.cpp \
    $$SOURCE_DIR/qdocdatabase.cpp \
    $$SOURCE_DIR/qdocutils.cpp \
    $$PWD/qdocdbconnector.cpp \
    $$PWD/qdockvleveldb.cpp \
    $$PWD/qdockvinterface.cpp

HEADERS += \
    $$SOURCE_DIR/qdoccollection.h \
    $$SOURCE_DIR/qdocdatabase.h \
    $$SOURCE_DIR/qdocutils.h \
    $$PWD/qdocdbconnector.h \
    $$PWD/qdockvinterface.h \
    $$PWD/qdockvleveldb.h

include(leveldb/leveldb.pri)
