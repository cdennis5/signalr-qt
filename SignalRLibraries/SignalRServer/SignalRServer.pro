#-------------------------------------------------
#
# Project created by QtCreator 2014-01-16T10:42:23
#
#-------------------------------------------------

QT       += core network qhttpserver
QT       -= gui

TARGET = SignalRServer
TEMPLATE = lib

DEFINES += SIGNALRSERVER_LIBRARY

SOURCES += SignalRServer.cpp \
    PersistentConnection.cpp \
    Infrastructure/EmptyProtectionData.cpp \
    Configuration/ConfigurationManager.cpp \
    Infrastructure/Purpose.cpp \
    Transport/TransportManager.cpp \
    Transport/ITransport.cpp

HEADERS += SignalRServer.h\
        signalrserver_global.h \
    PersistentConnection.h \
    Infrastructure/Purpose.h \
    Infrastructure/EmptyProtectionData.h \
    Infrastructure/IProtectedData.h \
    Configuration/ConfigurationManager.h \
    Transport/TransportManager.h \
    Transport/ITransport.h

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../ThirdParty/QtExtJson/release/ -lQextJson
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../ThirdParty/QtExtJson/debug/ -lQextJson
else:unix: LIBS += -L$$OUT_PWD/../../ThirdParty/QtExtJson/ -lQextJson

INCLUDEPATH += $$PWD/../../ThirdParty/QtExtJson
DEPENDPATH += $$PWD/../../ThirdParty/QtExtJson

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../ThirdParty/QHttpServer/release/ -lQHttpServer
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../ThirdParty/QHttpServer/debug/ -lQHttpServer
else:unix: LIBS += -L$$OUT_PWD/../../ThirdParty/QHttpServer/ -lQHttpServer

INCLUDEPATH += $$PWD/../../ThirdParty/QHttpServer
DEPENDPATH += $$PWD/../../ThirdParty/QHttpServer
