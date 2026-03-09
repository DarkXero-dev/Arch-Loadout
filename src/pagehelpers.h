#pragma once
#include <QWidget>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProcess>
#include <QMap>
#include <QString>
#include <QPushButton>
#include <QSizePolicy>
#include <QScrollArea>
#include <QScrollBar>
#include <QWheelEvent>
#include <QFrame>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QLinearGradient>
#include <QRadialGradient>
#include <cmath>

// ---- Smooth scrolling QScrollArea ----
class SmoothScrollArea : public QScrollArea
{
public:
    explicit SmoothScrollArea(QWidget *parent = nullptr) : QScrollArea(parent)
    {
        setFocusPolicy(Qt::NoFocus);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        verticalScrollBar()->setSingleStep(20);
    }

protected:
    void wheelEvent(QWheelEvent *e) override
    {
        QScrollBar *vbar = verticalScrollBar();
        int delta = e->angleDelta().y();
        vbar->setValue(vbar->value() - delta / 2);
        e->accept();
    }
};

// ---- Arch Linux logo pixmap ----
// Draws the Arch Linux logo using QPainterPath — no external files needed.
inline QPixmap makeArchLogoPixmap(int sz, const QColor &color = QColor("#1793d1"))
{
    QPixmap pm(sz, sz);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    auto s = [sz](float f) -> float { return f * sz / 100.0f; };

    // Outer arch shape (the Arch Linux logo silhouette)
    QPainterPath outer;
    outer.moveTo(s(50), s(2));
    outer.lineTo(s(98), s(96));
    outer.lineTo(s(67), s(96));
    outer.lineTo(s(50), s(73));
    outer.lineTo(s(33), s(96));
    outer.lineTo(s(2),  s(96));
    outer.closeSubpath();

    // Inner triangle cutout
    QPainterPath inner;
    inner.moveTo(s(50), s(18));
    inner.lineTo(s(78), s(83));
    inner.lineTo(s(22), s(83));
    inner.closeSubpath();

    p.fillPath(outer.subtracted(inner), color);
    return pm;
}

// ---- Internal helper: build the common header frame with a pre-made icon widget ----
inline QFrame* makePageHeaderImpl(QWidget *iconWidget,
                                   const QString &title,
                                   const QString &subtitle)
{
    auto *frame = new QFrame;
    frame->setFixedHeight(68);
    frame->setStyleSheet(
        "QFrame {"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 #4c1d95,"
        "    stop:0.35 #6d28d9,"
        "    stop:0.65 #2563eb,"
        "    stop:1 #1d4ed8);"
        "  border-radius: 8px;"
        "}"
    );

    auto *hlay = new QHBoxLayout(frame);
    hlay->setContentsMargins(16, 8, 16, 8);
    hlay->setSpacing(12);
    hlay->addWidget(iconWidget);

    auto *textWidget = new QWidget;
    textWidget->setStyleSheet("QWidget { background: transparent; }");
    auto *textLay = new QVBoxLayout(textWidget);
    textLay->setContentsMargins(0, 0, 0, 0);
    textLay->setSpacing(2);

    auto *titleLabel = new QLabel(title);
    titleLabel->setStyleSheet(
        "QLabel { background: transparent; color: white; font-weight: bold; font-size: 16px; }");
    textLay->addWidget(titleLabel);

    auto *subLabel = new QLabel(subtitle);
    subLabel->setStyleSheet(
        "QLabel { background: transparent; color: rgba(255,255,255,200); font-size: 11px; }");
    subLabel->setWordWrap(true);
    textLay->addWidget(subLabel);

    hlay->addWidget(textWidget, 1);
    return frame;
}

// ---- Gradient page header — emoji/text icon variant ----
inline QFrame* makePageHeader(const QString &icon,
                               const QString &title,
                               const QString &subtitle)
{
    auto *iconLabel = new QLabel(icon);
    iconLabel->setStyleSheet("QLabel { background: transparent; font-size: 28px; }");
    iconLabel->setFixedWidth(36);
    return makePageHeaderImpl(iconLabel, title, subtitle);
}

// ---- Gradient page header — QPixmap icon variant ----
inline QFrame* makePageHeader(const QPixmap &icon,
                               const QString &title,
                               const QString &subtitle)
{
    auto *iconLabel = new QLabel;
    iconLabel->setPixmap(icon.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setFixedWidth(36);
    iconLabel->setStyleSheet("QLabel { background: transparent; }");
    return makePageHeaderImpl(iconLabel, title, subtitle);
}

// ---- Purple circuit decoration widget (animated) ----
// Animated PCB-style circuit pattern that fades in from the top.
// Data packets travel along horizontal buses at staggered phases.
class CircuitWidget : public QWidget
{
public:
    explicit CircuitWidget(QWidget *parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(60);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);

        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, [this]() {
            m_phase += 0.0055f;
            if (m_phase >= 1.0f) m_phase -= 1.0f;
            update();
        });
        m_timer->start(33); // ~30 fps
    }

    ~CircuitWidget() { m_timer->stop(); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const int W = width(), H = height();
        if (W < 2 || H < 2) return;

        QPixmap pm(W, H);
        pm.fill(Qt::transparent);

        QPainter pp(&pm);
        pp.setRenderHint(QPainter::Antialiasing);

        // -- Color palette --
        const QColor traceGlow (139,  92, 246,  45);
        const QColor traceMain (139,  92, 246, 175);
        const QColor nodeCol   (167, 139, 250, 230);
        const QColor chipFill  ( 76,  29, 149, 105);
        const QColor chipBorder(139,  92, 246, 190);

        // -- Bus positions (normalized 0..1 of height) --
        const float B1=0.16f, B2=0.38f, B3=0.61f, B4=0.83f;

        auto hline = [&](float y) {
            pp.drawLine(QPointF(0, y*H), QPointF(W, y*H));
        };
        auto vline = [&](float x, float ya, float yb) {
            pp.drawLine(QPointF(x*W, ya*H), QPointF(x*W, yb*H));
        };

        // Glow pass
        pp.setPen(QPen(traceGlow, 7.0f, Qt::SolidLine, Qt::RoundCap));
        hline(B1); hline(B2); hline(B3); hline(B4);
        for (float x : {0.08f,0.20f,0.32f,0.44f,0.56f,0.68f,0.80f,0.92f})
            vline(x, 0.0f, 1.0f);

        // Main buses
        pp.setPen(QPen(traceMain, 1.5f, Qt::SolidLine, Qt::RoundCap));
        hline(B1); hline(B2); hline(B3); hline(B4);

        // Vertical traces
        struct VT { float x, ya, yb; };
        const QList<VT> verts = {
            {0.08f, B1, B2},  {0.20f, B2, B3},  {0.32f, B1, B3},
            {0.44f, B2, B4},  {0.56f, B1, B2},  {0.68f, B3, B4},
            {0.80f, B1, B2},  {0.92f, B2, B4},
            {0.14f, 0.00f, B1}, {0.38f, 0.00f, B2},
            {0.62f, 0.00f, B1}, {0.86f, 0.00f, B2},
            {0.08f, B4, 1.00f}, {0.26f, B4, 1.00f},
            {0.50f, B4, 1.00f}, {0.74f, B4, 1.00f}, {0.98f, B4, 1.00f},
        };
        for (const auto &v : verts)
            vline(v.x, v.ya, v.yb);

        // Junction nodes
        pp.setPen(Qt::NoPen);
        pp.setBrush(nodeCol);
        const float NR = 3.5f;
        auto node = [&](float x, float y) {
            pp.drawEllipse(QPointF(x*W, y*H), NR, NR);
        };
        node(0.08f,B1); node(0.08f,B2);
        node(0.20f,B2); node(0.20f,B3);
        node(0.32f,B1); node(0.32f,B3);
        node(0.44f,B2); node(0.44f,B4);
        node(0.56f,B1); node(0.56f,B2);
        node(0.68f,B3); node(0.68f,B4);
        node(0.80f,B1); node(0.80f,B2);
        node(0.92f,B2); node(0.92f,B4);
        node(0.14f,B1); node(0.38f,B2); node(0.62f,B1); node(0.86f,B2);
        node(0.08f,B4); node(0.26f,B4); node(0.50f,B4); node(0.74f,B4); node(0.98f,B4);

        // IC chips
        pp.setPen(QPen(chipBorder, 1.0f));
        pp.setBrush(chipFill);
        auto chip = [&](float x1, float y1, float x2, float y2) {
            QRectF r(x1*W, y1*H, (x2-x1)*W, (y2-y1)*H);
            if (r.width() > 8 && r.height() > 6) pp.drawRect(r);
        };
        chip(0.09f, B1+0.02f, 0.19f, B2-0.02f);
        chip(0.33f, B2+0.02f, 0.43f, B3-0.02f);
        chip(0.57f, B1+0.02f, 0.67f, B2-0.02f);
        chip(0.21f, B3+0.02f, 0.31f, B4-0.02f);
        chip(0.81f, B2+0.02f, 0.91f, B3-0.02f);

        // ── Animated data packets ───────────────────────────────────────
        // Each bus has a packet traveling left→right at a different phase.
        // Phase offsets are staggered so they don't all line up.
        struct BusAnim { float y; float offset; };
        const BusAnim busAnims[] = {
            {B1, 0.00f}, {B2, 0.27f}, {B3, 0.54f}, {B4, 0.73f}
        };
        const float TAIL = 0.16f * W;   // tail length in pixels

        for (const auto &ba : busAnims) {
            float px = std::fmod(m_phase + ba.offset, 1.0f);
            float cx = px * W;
            float cy = ba.y * H;

            // Glowing tail (gradient line behind the head)
            float tailStart = cx - TAIL;
            QLinearGradient tailGrad(tailStart, cy, cx, cy);
            tailGrad.setColorAt(0.0f, QColor(139, 92, 246, 0));
            tailGrad.setColorAt(1.0f, QColor(196, 167, 255, 200));
            pp.setPen(QPen(QBrush(tailGrad), 2.5f, Qt::SolidLine, Qt::RoundCap));
            pp.drawLine(QPointF(tailStart, cy), QPointF(cx, cy));

            // Bright head dot with soft radial glow
            float gr = TAIL * 0.28f;
            QRadialGradient rg(cx, cy, gr);
            rg.setColorAt(0.0f, QColor(255, 255, 255, 230));
            rg.setColorAt(0.35f, QColor(216, 180, 254, 160));
            rg.setColorAt(1.0f, QColor(139,  92, 246,   0));
            pp.setPen(Qt::NoPen);
            pp.setBrush(rg);
            pp.drawEllipse(QPointF(cx, cy), gr, gr * 0.55f);
        }

        // Fade mask: transparent at top → opaque at 35%
        QLinearGradient fadeGrad(0, 0, 0, H * 0.35f);
        fadeGrad.setColorAt(0.0, Qt::transparent);
        fadeGrad.setColorAt(1.0, Qt::black);
        pp.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        pp.fillRect(0, 0, W, H, fadeGrad);
        pp.end();

        QPainter p(this);
        p.drawPixmap(0, 0, pm);
    }

private:
    QTimer *m_timer  = nullptr;
    float   m_phase  = 0.0f;
};

// ---- Install status checks ----
inline bool isPacmanInstalled(const QString &pkg)
{
    static const QDir db("/var/lib/pacman/local");
    return !db.entryList({pkg + "-[0-9]*"}, QDir::Dirs).isEmpty();
}

inline bool isPacmanInstalledAny(const QStringList &pkgs)
{
    static const QDir db("/var/lib/pacman/local");
    for (const QString &pkg : pkgs)
        if (!db.entryList({pkg + "-[0-9]*"}, QDir::Dirs).isEmpty())
            return true;
    return false;
}

inline bool isFlatpakInstalled(const QString &appId)
{
    if (QFile::exists("/var/lib/flatpak/app/" + appId))
        return true;
    for (const QString &user : QDir("/home").entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (QFile::exists("/home/" + user + "/.local/share/flatpak/app/" + appId))
            return true;
    return false;
}

inline bool isKwinScriptInstalled(const QString &name, const QString &user)
{
    return QFile::exists("/home/" + user + "/.local/share/kwin/scripts/" + name)
        || QFile::exists("/usr/share/kwin/scripts/" + name);
}

inline bool isPlasmaAppletInstalled(const QString &name, const QString &user)
{
    return QFile::exists("/home/" + user + "/.local/share/plasma/plasmoids/" + name)
        || QFile::exists("/usr/share/plasma/plasmoids/" + name);
}

inline void runChecksAsync(
    QObject *context,
    QList<QPair<QString, std::function<bool()>>> checks,
    std::function<void(QMap<QString,bool>)> onDone)
{
    QTimer::singleShot(0, context, [checks, onDone]() {
        QMap<QString,bool> results;
        for (const auto &check : checks)
            results[check.first] = check.second();
        onDone(results);
    });
}

inline QCheckBox* makeItemRow(QWidget *parent, QLayout *layout,
                              const QString &label,
                              bool installed,
                              bool showBadge = true)
{
    auto *row    = new QWidget(parent);
    auto *rowLay = new QHBoxLayout(row);
    rowLay->setContentsMargins(0, 0, 0, 0);
    rowLay->setSpacing(8);

    auto *cb = new QCheckBox(label, row);
    rowLay->addWidget(cb, 1);

    if (showBadge) {
        auto *badge = new QLabel(installed ? "[Installed]" : "[Not Installed]", row);
        badge->setMinimumWidth(110);
        badge->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        badge->setStyleSheet(installed
            ? "color: #3db03d; font-weight: bold; font-size: 8pt;"
            : "color: #cc7700; font-weight: bold; font-size: 8pt;");
        rowLay->addWidget(badge);
    }

    layout->addWidget(row);
    return cb;
}

inline QPushButton* makeToolbarBtn(const QString &text, QWidget *parent = nullptr)
{
    auto *btn = new QPushButton(text, parent);
    btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    btn->setMinimumWidth(btn->fontMetrics().horizontalAdvance(text) + 24);
    return btn;
}

inline QLabel* makeDescLabel(QWidget *parent, const QString &desc)
{
    auto *dl = new QLabel("    " + desc, parent);
    dl->setWordWrap(true);
    auto pal = dl->palette();
    auto c = pal.color(QPalette::WindowText);
    c.setAlphaF(0.55f);
    pal.setColor(QPalette::WindowText, c);
    dl->setPalette(pal);
    return dl;
}

inline void updateBadge(QWidget *row, bool installed)
{
    if (!row) return;
    auto *lbl = row->findChild<QLabel*>();
    if (!lbl) return;
    lbl->setText(installed ? "[Installed]" : "[Not Installed]");
    lbl->setStyleSheet(installed
        ? "color: #3db03d; font-weight: bold; font-size: 8pt;"
        : "color: #cc7700; font-weight: bold; font-size: 8pt;");
}
