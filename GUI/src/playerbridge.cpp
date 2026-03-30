#include "playerbridge.h"
#include "ffmpegengine.h"

#include <QFileInfo>
#include <QtGlobal>
#include <QDebug>

PlayerBridge::PlayerBridge(QObject* parent) : QObject(parent) {
    // 创建 FFmpeg 引擎
    engine_ = new FFmpegEngine(this);
    connectEngine();
}

void PlayerBridge::connectEngine() {
    // 引擎状态 → Bridge 属性
    connect(engine_, &FFmpegEngine::stateChanged, this, [this](int state) {
        setPlaybackState(state);
    });

    connect(engine_, &FFmpegEngine::positionChanged, this, [this](qint64 pos) {
        setPositionMs(pos);
    });

    connect(engine_, &FFmpegEngine::durationChanged, this, [this](qint64 dur) {
        setDurationMs(dur);
    });

    connect(engine_, &FFmpegEngine::errorOccurred, this, [this](const QString& msg) {
        qWarning() << "[PlayerBridge] Error:" << msg;
        setPlaybackState(Error);
        emit eventError(2000, msg);
    });
}

// ========== Property implementations (unchanged logic) ==========

int PlayerBridge::playbackState() const { return playbackState_; }
void PlayerBridge::setPlaybackState(int state) {
    if (playbackState_ == state) return;
    playbackState_ = state;
    emit playbackStateChanged();
}

qint64 PlayerBridge::positionMs() const { return positionMs_; }
void PlayerBridge::setPositionMs(qint64 value) {
    const qint64 clamped = qMax<qint64>(0, qMin(value, durationMs_ > 0 ? durationMs_ : value));
    if (positionMs_ == clamped) return;
    positionMs_ = clamped;
    emit positionMsChanged();
}

qint64 PlayerBridge::durationMs() const { return durationMs_; }
void PlayerBridge::setDurationMs(qint64 value) {
    const qint64 clamped = qMax<qint64>(0, value);
    if (durationMs_ == clamped) return;
    durationMs_ = clamped;
    emit durationMsChanged();
}

qreal PlayerBridge::bufferedRatio() const { return bufferedRatio_; }
void PlayerBridge::setBufferedRatio(qreal value) {
    const qreal clamped = qBound(0.0, value, 1.0);
    if (qFuzzyCompare(bufferedRatio_, clamped)) return;
    bufferedRatio_ = clamped;
    emit bufferedRatioChanged();
}

qreal PlayerBridge::volume() const { return volume_; }
void PlayerBridge::setVolume(qreal value) {
    const qreal clamped = qBound(0.0, value, 1.0);
    if (qFuzzyCompare(volume_, clamped)) return;
    volume_ = clamped;
    if (engine_) {
        engine_->setVolume(volume_);
    }
    emit volumeChanged();
}

bool PlayerBridge::muted() const { return muted_; }
void PlayerBridge::setMuted(bool value) {
    if (muted_ == value) return;
    muted_ = value;
    if (engine_) {
        engine_->setMuted(muted_);
    }
    emit mutedChanged();
}

QString PlayerBridge::mediaTitle() const { return mediaTitle_; }
void PlayerBridge::setMediaTitle(const QString& value) {
    if (mediaTitle_ == value) return;
    mediaTitle_ = value;
    emit mediaMetaChanged();
}

QString PlayerBridge::mediaArtist() const { return mediaArtist_; }
void PlayerBridge::setMediaArtist(const QString& value) {
    if (mediaArtist_ == value) return;
    mediaArtist_ = value;
    emit mediaMetaChanged();
}

QString PlayerBridge::mediaPath() const { return mediaPath_; }
void PlayerBridge::setMediaPath(const QString& value) {
    if (mediaPath_ == value) return;
    mediaPath_ = value;
    emit mediaMetaChanged();
}

bool PlayerBridge::hasMedia() const { return hasMedia_; }
void PlayerBridge::setHasMedia(bool value) {
    if (hasMedia_ == value) return;
    hasMedia_ = value;
    emit hasMediaChanged();
}

int PlayerBridge::playbackMode() const { return playbackMode_; }
void PlayerBridge::setPlaybackMode(int mode) {
    const int clamped = qBound(static_cast<int>(Sequential), mode, static_cast<int>(Shuffle));
    if (playbackMode_ == clamped) return;
    playbackMode_ = clamped;
    emit playbackModeChanged();
}

qreal PlayerBridge::playbackRate() const { return playbackRate_; }
void PlayerBridge::setPlaybackRate(qreal value) {
    const qreal clamped = qBound(0.25, value, 4.0);
    if (qFuzzyCompare(playbackRate_, clamped)) return;
    playbackRate_ = clamped;
    emit playbackRateChanged();
}

// ========== 控制方法：委托给 FFmpegEngine ==========

void PlayerBridge::openMedia(const QString& fileOrUrl) {
    if (fileOrUrl.isEmpty()) {
        emit eventError(1001, QStringLiteral("openMedia received empty input"));
        return;
    }

    setMediaPath(fileOrUrl);
    setMediaTitle(fallbackTitleFromPath(fileOrUrl));
    setMediaArtist(QStringLiteral("Unknown artist"));

    // 调用 FFmpeg 引擎打开文件
    bool ok = engine_->openFile(fileOrUrl);
    if (ok) {
        engine_->setVolume(volume_);
        engine_->setMuted(muted_);
        setHasMedia(true);
        setPositionMs(0);
        setBufferedRatio(0.0);
        setPlaybackState(Idle);
        emit eventInfo(QStringLiteral("Media opened successfully"));
    } else {
        setHasMedia(false);
        setPlaybackState(Error);
    }
}

void PlayerBridge::play() {
    if (!hasMedia_) {
        emit eventError(1002, QStringLiteral("No media loaded"));
        return;
    }
    engine_->play();
}

void PlayerBridge::pause() {
    if (!hasMedia_) return;
    engine_->pause();
}

void PlayerBridge::stop() {
    engine_->stop();
    setPositionMs(0);
    setHasMedia(false);
}

void PlayerBridge::seekTo(qint64 positionMs) {
    // TODO: 实现 FFmpeg seek 功能
    setPositionMs(positionMs);
}

void PlayerBridge::seekBy(qint64 deltaMs) {
    seekTo(positionMs_ + deltaMs);
}

void PlayerBridge::playNext() {
    emit eventInfo(QStringLiteral("playNext requested"));
}

void PlayerBridge::playPrevious() {
    emit eventInfo(QStringLiteral("playPrevious requested"));
}

void PlayerBridge::fastForward() { seekBy(10 * 1000); }
void PlayerBridge::rewind() { seekBy(-10 * 1000); }
void PlayerBridge::toggleMute() { setMuted(!muted_); }
void PlayerBridge::toggleFullscreen() { emit commandToggleFullscreen(); }
void PlayerBridge::cyclePlaybackMode() { setPlaybackMode((playbackMode_ + 1) % 4); }

QString PlayerBridge::fallbackTitleFromPath(const QString& path) const {
    const QFileInfo info(path);
    if (info.fileName().isEmpty()) return QStringLiteral("Untitled media");
    return info.fileName();
}
