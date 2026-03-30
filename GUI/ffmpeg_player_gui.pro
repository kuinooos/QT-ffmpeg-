QT += core gui qml quick quickcontrols2 multimedia
CONFIG += c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = ffmpeg_player_gui

SOURCES += \
    src/main.cpp \
    src/playerbridge.cpp \
    src/ffmpegengine.cpp \
    src/log.cpp \
    src/demuxthread.cpp \
    src/decodethread.cpp \
    src/avpacketqueue.cpp \
    src/avframequeue.cpp \
    src/audiooutput.cpp \
    src/queue.cpp \
    src/thread.cpp

HEADERS += \
    src/playerbridge.h \
    src/ffmpegengine.h \
    src/videorenderer.h \
    src/log.h \
    src/thread.h \
    src/queue.h \
    src/demuxthread.h \
    src/decodethread.h \
    src/avpacketqueue.h \
    src/avframequeue.h \
    src/audiooutput.h

RESOURCES += resources.qrc

FFMPEG_DIR =
exists($$PWD/../ffmpeg-x64) {
    FFMPEG_DIR = $$PWD/../ffmpeg-x64
} else: exists($$PWD/../ffmpeg-x86) {
    FFMPEG_DIR = $$PWD/../ffmpeg-x86
} else {
    error(Cannot find FFmpeg directory. Expected ../ffmpeg-x64 or ../ffmpeg-x86)
}

contains(QMAKE_TARGET.arch, x86_64) {
    contains(FFMPEG_DIR, ffmpeg-x86) {
        error(Detected 64-bit compiler but selected 32-bit FFmpeg libs (ffmpeg-x86). Please provide ffmpeg-x64.)
    }
}

INCLUDEPATH += \
    $$PWD/src \
    $$FFMPEG_DIR/include

LIBS += -L$$FFMPEG_DIR/lib
LIBS += -lavformat -lavcodec -lavutil -lswresample -lswscale

win32:QMAKE_LFLAGS += -Wl,-subsystem,windows
