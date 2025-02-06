QT = core

CONFIG += c++17 cmdline

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        websockets_client.cpp \
        websockets_server.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += /usr/local/include/opencv4 \
               /usr/local/include/boost \
               /usr/include/boost \
               /usr/include \
               /usr/include/gtest

LIBS += -L/usr/local/lib \  #for both Boost and OpenCV
        -L/usr/lib/x86_64-linux-gnu \#for OpenSSL
        -L/usr/lib \ #for gtest
        -lboost_system \
        -lboost_filesystem \
        -lboost_thread \
        -lssl \
        -lcrypto \
        -lgtest \
        -lgtest_main \
        -pthread

HEADERS += \
    ssl_conf.h \
    tests.h \
    websockets_client.h \
    websockets_server.h
