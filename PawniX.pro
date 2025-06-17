QT += widgets core5compat core gui network
CONFIG += static
TARGET = PawniX
TEMPLATE = app
QMAKE_LFLAGS += -static -static-libgcc -static-libstdc++

LIBS += -L"C:/Qt/6.9.1/mingw_64/lib" \
        -lQt6Core5Compat \
        -lQt6Core \
        -lQt6Gui \
        -lQt6Widgets \
        -lQt6Network

win32 {
    QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
    DEFINES += _WIN32_WINNT=0x0601
}

SOURCES += main.cpp
RESOURCES += icons.qrc
RC_FILE = app.rc
HEADERS += includes.h
