#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class FFmpegEngine; // 前向声明

class PlayerBridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(int playbackState READ playbackState WRITE setPlaybackState NOTIFY playbackStateChanged)
    Q_PROPERTY(qint64 positionMs READ positionMs WRITE setPositionMs NOTIFY positionMsChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs WRITE setDurationMs NOTIFY durationMsChanged)
    Q_PROPERTY(qreal bufferedRatio READ bufferedRatio WRITE setBufferedRatio NOTIFY bufferedRatioChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(QString mediaTitle READ mediaTitle WRITE setMediaTitle NOTIFY mediaMetaChanged)
    Q_PROPERTY(QString mediaArtist READ mediaArtist WRITE setMediaArtist NOTIFY mediaMetaChanged)
    Q_PROPERTY(QString mediaPath READ mediaPath WRITE setMediaPath NOTIFY mediaMetaChanged)
    Q_PROPERTY(bool hasMedia READ hasMedia WRITE setHasMedia NOTIFY hasMediaChanged)
    Q_PROPERTY(int playbackMode READ playbackMode WRITE setPlaybackMode NOTIFY playbackModeChanged)
    Q_PROPERTY(qreal playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)

public:
    enum PlaybackState {
        Idle = 0,
        Playing = 1,
        Paused = 2,
        Seeking = 3,
        Stopped = 4,
        Buffering = 5,
        Error = 6
    };
    Q_ENUM(PlaybackState)

    enum PlaybackMode {
        Sequential = 0,
        RepeatOne = 1,
        RepeatAll = 2,
        Shuffle = 3
    };
    Q_ENUM(PlaybackMode)

    explicit PlayerBridge(QObject* parent = nullptr);

    // --- 给 FFmpegEngine 提供访问入口 ---
    FFmpegEngine* engine() const { return engine_; }

    int playbackState() const;
    void setPlaybackState(int state);

    qint64 positionMs() const;
    void setPositionMs(qint64 value);

    qint64 durationMs() const;
    void setDurationMs(qint64 value);

    qreal bufferedRatio() const;
    void setBufferedRatio(qreal value);

    qreal volume() const;
    void setVolume(qreal value);

    bool muted() const;
    void setMuted(bool value);

    QString mediaTitle() const;
    void setMediaTitle(const QString& value);

    QString mediaArtist() const;
    void setMediaArtist(const QString& value);

    QString mediaPath() const;
    void setMediaPath(const QString& value);

    bool hasMedia() const;
    void setHasMedia(bool value);

    int playbackMode() const;
    void setPlaybackMode(int mode);

    qreal playbackRate() const;
    void setPlaybackRate(qreal value);

    Q_INVOKABLE void openMedia(const QString& fileOrUrl);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void seekTo(qint64 positionMs);
    Q_INVOKABLE void seekBy(qint64 deltaMs);
    Q_INVOKABLE void playNext();
    Q_INVOKABLE void playPrevious();
    Q_INVOKABLE void fastForward();
    Q_INVOKABLE void rewind();
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void toggleFullscreen();
    Q_INVOKABLE void cyclePlaybackMode();

signals:
    void playbackStateChanged();
    void positionMsChanged();
    void durationMsChanged();
    void bufferedRatioChanged();
    void volumeChanged();
    void mutedChanged();
    void mediaMetaChanged();
    void hasMediaChanged();
    void playbackModeChanged();
    void playbackRateChanged();

    void commandToggleFullscreen();

    void eventError(int errorCode, const QString& message);
    void eventInfo(const QString& message);

private:
    QString fallbackTitleFromPath(const QString& path) const;

    // 连接 FFmpegEngine 的信号
    void connectEngine();

    FFmpegEngine* engine_ = nullptr;

    int playbackState_ = Idle;
    qint64 positionMs_ = 0;
    qint64 durationMs_ = 0;
    qreal bufferedRatio_ = 0.0;
    qreal volume_ = 0.8;
    bool muted_ = false;
    QString mediaTitle_;
    QString mediaArtist_;
    QString mediaPath_;
    bool hasMedia_ = false;
    int playbackMode_ = Sequential;
    qreal playbackRate_ = 1.0;
};
