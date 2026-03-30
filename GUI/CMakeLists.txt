cmake_minimum_required(VERSION 3.21)

project(FFmpegPlayerGui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Multimedia)

qt_standard_project_setup(REQUIRES 6.5)

set(FFMPEG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ffmpeg-x64")

# ============================================================
# 可执行文件：GUI 源码 + logic 后端源码
# ============================================================
qt_add_executable(app_ffmpeg_player_gui
    # --- GUI 前端 ---
    src/main.cpp
    src/playerbridge.cpp
    src/ffmpegengine.cpp
    # --- FFmpeg 后端 (来自 logic) ---
    src/log.cpp
    src/demuxthread.cpp
    src/decodethread.cpp
    src/avpacketqueue.cpp
    src/avframequeue.cpp
    src/audiooutput.cpp
    src/queue.cpp
    src/thread.cpp
)

qt_add_qml_module(app_ffmpeg_player_gui
    URI PlayerGui
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/GlassCard.qml
        qml/components/ControlButton.qml
        qml/components/TimelineSlider.qml
    SOURCES
        src/playerbridge.h
        src/ffmpegengine.h
        src/videorenderer.h
        src/log.h
        src/thread.h
        src/queue.h
        src/demuxthread.h
        src/decodethread.h
        src/avpacketqueue.h
        src/avframequeue.h
        src/audiooutput.h
)

# ============================================================
# 头文件搜索路径
# ============================================================
target_include_directories(app_ffmpeg_player_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_DIR}/include
)

# ============================================================
# 链接库
# ============================================================
# FFmpeg 库
target_link_directories(app_ffmpeg_player_gui PRIVATE
    ${FFMPEG_DIR}/lib
)

target_link_libraries(app_ffmpeg_player_gui PRIVATE
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    # FFmpeg 核心库
    avformat
    avcodec
    avutil
    swresample
    swscale
)

set_target_properties(app_ffmpeg_player_gui PROPERTIES
    WIN32_EXECUTABLE TRUE
)

install(TARGETS app_ffmpeg_player_gui
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
