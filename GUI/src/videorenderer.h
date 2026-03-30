#pragma once

#include <QQuickPaintedItem>
#include <QImage>
#include <QPainter>

/**
 * VideoRenderer: 用于在 QML 中渲染视频帧的自定义 Item。
 *
 * 使用方式（QML）:
 *   VideoRenderer { id: videoSurface; anchors.fill: parent }
 *
 * 接收 QImage 帧 → 在 QML 控件内绘制。
 */
class VideoRenderer : public QQuickPaintedItem {
    Q_OBJECT
public:
    explicit VideoRenderer(QQuickItem* parent = nullptr)
        : QQuickPaintedItem(parent)
    {
        setRenderTarget(QQuickPaintedItem::FramebufferObject);
    }

public slots:
    /**
     * 接收一帧新图片。由 FFmpegEngine::frameReady 信号连接。
     */
    void updateFrame(const QImage& frame) {
        currentFrame_ = frame;
        update(); // 触发重绘
    }

protected:
    void paint(QPainter* painter) override {
        if (currentFrame_.isNull()) {
            // 没有帧时画黑色背景
            painter->fillRect(0, 0, width(), height(), Qt::black);
            return;
        }

        // 保持宽高比地缩放到控件大小
        QImage scaled = currentFrame_.scaled(
            static_cast<int>(width()),
            static_cast<int>(height()),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        // 居中绘制
        int x = static_cast<int>((width()  - scaled.width())  / 2);
        int y = static_cast<int>((height() - scaled.height()) / 2);

        painter->fillRect(0, 0, width(), height(), Qt::black);
        painter->drawImage(x, y, scaled);
    }

private:
    QImage currentFrame_;
};
