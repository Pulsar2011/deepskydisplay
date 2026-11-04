//
//  QCustomContours.cxx
//  DeepSkyTools
//
//  Implementation of the QCustomContours iso-contour generator.
//  See QCustomContours.h for the public API documentation.
//
//  Copyright (c) 2025, All rights reserved
//

#include <DSTdisplay/QCustomContours.h>

#include <algorithm>
#include <cmath>
#include <deque>
#include <future>
#include <limits>
#include <unordered_map>
#include <vector>

#include <QFont>
#include <QFontDatabase>
#include <QString>

// ===========================================================================
// Construction
// ===========================================================================

QCustomContours::QCustomContours(const std::vector<std::vector<double>>& Z, size_t nLevels)
    : pen_(QPen(Qt::green, 1.0))
    , labelFontSize_(8)
{
    levels_ = computeLevels(Z, nLevels);
    if (levels_.isEmpty()) return;

    lines_.resize(levels_.size());

    // Dispatch one std::async task per level — marchingSquares is a pure
    // static function with no shared mutable state, so concurrent calls are safe.
    std::vector<std::future<vertices_list_type>> futures;
    futures.reserve(static_cast<size_t>(levels_.size()));
    for (double lv : levels_)
        futures.push_back(std::async(std::launch::async,
                                     &QCustomContours::marchingSquares,
                                     std::cref(Z), lv));

    for (int k = 0; k < static_cast<int>(futures.size()); ++k)
        lines_[k] = futures[static_cast<size_t>(k)].get();
}

// ===========================================================================
// Rendering style — setters / getters
// ===========================================================================

/** @copydoc QCustomContours::setPen */
void QCustomContours::setPen(const QPen& pen) { pen_ = pen; }

/** @copydoc QCustomContours::pen */
QPen QCustomContours::pen() const { return pen_; }

/** @copydoc QCustomContours::setLabelFontSize */
void QCustomContours::setLabelFontSize(int pointSize) { labelFontSize_ = pointSize; }

/** @copydoc QCustomContours::labelFontSize */
int QCustomContours::labelFontSize() const { return labelFontSize_; }

// ===========================================================================
// Level-dependent colour gradient — setters / getters
// ===========================================================================

/** @copydoc QCustomContours::setColorGradient */
void QCustomContours::setColorGradient(const QCPColorGradient& gradient)
{
    gradient_    = gradient;
    useGradient_ = true;
}

/** @copydoc QCustomContours::colorGradient */
QCPColorGradient QCustomContours::colorGradient() const { return gradient_; }

/** @copydoc QCustomContours::clearColorGradient */
void QCustomContours::clearColorGradient() { useGradient_ = false; }

/** @copydoc QCustomContours::hasColorGradient */
bool QCustomContours::hasColorGradient() const { return useGradient_; }

// ===========================================================================
// Geometry accessors
// ===========================================================================

/** @copydoc QCustomContours::lines */
const QVector<QCustomContours::vertices_list_type>& QCustomContours::lines() const
{
    return lines_;
}

/** @copydoc QCustomContours::levels */
const QVector<double>& QCustomContours::levels() const { return levels_; }

// ===========================================================================
// Convenience renderer
// ===========================================================================

/**
 * @copydoc QCustomContours::render
 *
 * @details For each level @c k, a per-level QPen is derived from @c pen_:
 *          - When @c hasColorGradient() is @c false the pen is used as-is.
 *          - When @c hasColorGradient() is @c true the pen colour is replaced
 *            by @c gradient_.color(levels_[k], QCPRange(zmin, zmax)) so that
 *            each iso-level gets a distinct hue while width, style and caps
 *            are still governed by @c pen_.
 *
 *          The per-level pen is then used for every @c QCPCurve in that level
 *          and for the colour of any @c QCPItemText labels.
 */
void QCustomContours::render(QCustomPlot* plot) const
{
    if (!plot) return;

    static constexpr int MIN_LABEL_PTS = 50;
    static constexpr double PI = 3.14159265358979323846;

    // Pre-compute gradient range once (only meaningful when useGradient_ is true).
    // QCPColorGradient::color() is not const, so we work on a local copy.
    const bool useGrad = useGradient_ && levels_.size() >= 2;
    QCPColorGradient gradient = gradient_;                       // mutable copy
    const QCPRange gradRange = useGrad
        ? QCPRange(levels_.first(), levels_.last())
        : QCPRange(0.0, 1.0);

    for (int k = 0; k < lines_.size(); ++k)
    {
        // Build the effective pen for this level.
        QPen levelPen = pen_;
        if (useGrad)
            levelPen.setColor(QColor(gradient.color(levels_[k], gradRange)));

        const QVector<double>& xs = lines_[k].first;
        const QVector<double>& ys = lines_[k].second;

        QVector<double> segX, segY;

        // Iterate one position past the end to flush the last segment without
        // duplicating the flush logic.
        for (int i = 0; i <= xs.size(); ++i)
        {
            const bool flush = (i == xs.size()) || std::isnan(xs[i]);

            if (flush)
            {
                if (segX.size() >= 2)
                {
                    // --- Contour curve ---
                    QCPCurve* curve = new QCPCurve(plot->xAxis, plot->yAxis);
                    curve->addData(segX, segY);
                    curve->setPen(levelPen);
                    curve->setScatterStyle(QCPScatterStyle::ssNone);

                    // --- Optional level label ---
                    if (labelFontSize_ > 0
                        && segX.size() > MIN_LABEL_PTS
                        && k < levels_.size())
                    {
                        const int pos   = static_cast<int>(segX.size()) / 8;
                        const int prev  = std::max(pos - 1, 0);
                        const int next  = std::min(pos + 1, static_cast<int>(segX.size()) - 1);

                        const double ang = std::atan2(segY[next] - segY[prev],
                                                      segX[next] - segX[prev]);

                        QCPItemText* label = new QCPItemText(plot);
                        label->position->setCoords(segX[pos], segY[pos]);
                        label->setRotation(ang * 180.0 / PI);
                        label->setText(QString::number(levels_[k], 'f', 3));
                        QFont labelFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
                        labelFont.setPointSize(labelFontSize_);
                        label->setFont(labelFont);
                        label->setColor(levelPen.color());
                    }
                }

                segX.clear();
                segY.clear();
            }
            else
            {
                segX.push_back(xs[i]);
                segY.push_back(ys[i]);
            }
        }
    }
}

// ===========================================================================
// Private static helpers
// ===========================================================================

/**
 * @copydoc QCustomContours::computeLevels
 *
 * @details Scans every finite value in @p Z to find the global minimum and
 *          maximum.  Returns @p n levels placed at
 *          @code zmin + k * (zmax-zmin) / (n+1) @endcode for k = 1 … n,
 *          i.e., evenly spaced and strictly between @p zmin and @p zmax.
 */
QVector<double> QCustomContours::computeLevels(
    const std::vector<std::vector<double>>& Z, size_t n)
{
    if (Z.size() < 2 || Z[0].size() < 2 || n == 0) return {};

    double zmin =  std::numeric_limits<double>::infinity();
    double zmax = -std::numeric_limits<double>::infinity();
    for (const auto& row : Z)
        for (double v : row)
            if (std::isfinite(v)) { zmin = std::min(zmin, v); zmax = std::max(zmax, v); }

    if (!std::isfinite(zmin) || zmin >= zmax) return {};

    QVector<double> lv;
    lv.reserve(static_cast<int>(n));
    const double step = (zmax - zmin) / static_cast<double>(n + 1);
    for (size_t k = 1; k <= n; ++k)
        lv.push_back(zmin + static_cast<double>(k) * step);
    return lv;
}

/**
 * @copydoc QCustomContours::encodePoint
 *
 * @details The encoding is @c xi * 1 000 000 000 + yi, where @c xi and @c yi
 *          are the coordinates rounded to 4 decimal places.  This is lossless
 *          for pixel-scale coordinates (up to 65 535 in either dimension).
 */
long long QCustomContours::encodePoint(double x, double y) noexcept
{
    const long long xi = static_cast<long long>(std::round(x * 1e4));
    const long long yi = static_cast<long long>(std::round(y * 1e4));
    return xi * 1000000000LL + yi;
}

/**
 * @copydoc QCustomContours::marchingSquares
 *
 * @details
 * **Corner bit encoding** (cell at column @c i, row @c j):
 * - bit 0: @c Z[j][i]     ≥ level  (BL — bottom-left)
 * - bit 1: @c Z[j][i+1]   ≥ level  (BR — bottom-right)
 * - bit 2: @c Z[j+1][i+1] ≥ level  (TR — top-right)
 * - bit 3: @c Z[j+1][i]   ≥ level  (TL — top-left)
 *
 * **Edge numbering** for interpolation:
 * - Edge 0 (bottom): BL → BR
 * - Edge 1 (right):  BR → TR
 * - Edge 2 (top):    TR → TL
 * - Edge 3 (left):   TL → BL
 *
 * Saddle cases 5 (BL+TR above) and 10 (BR+TL above) produce two segments
 * each, using a fixed consistent resolution (no centre-value disambiguation).
 *
 * After collecting all 2-point segments for the level, an endpoint hash map
 * is used to chain them into maximal polylines.  Each polyline is appended
 * to the result followed by a NaN sentinel.
 */
QCustomContours::vertices_list_type QCustomContours::marchingSquares(
    const std::vector<std::vector<double>>& Z, double level)
{
    const size_t nrows = Z.size();
    if (nrows < 2) return {};
    const size_t ncols = Z[0].size();
    if (ncols < 2) return {};

    // 16-case lookup table.
    // Each row: [edge_a, edge_b, edge_c, edge_d] where {a,b} and {c,d} are
    // the two edge indices of one (or two, for saddle cases) line segments.
    // A value of -1 means "unused".
    static constexpr int8_t lut[16][4] = {
        {-1,-1,-1,-1}, // 0  – no crossing
        { 3, 0,-1,-1}, // 1  – BL above
        { 0, 1,-1,-1}, // 2  – BR above
        { 3, 1,-1,-1}, // 3  – BL+BR above
        { 1, 2,-1,-1}, // 4  – TR above
        { 3, 2, 0, 1}, // 5  – BL+TR above  (saddle)
        { 0, 2,-1,-1}, // 6  – BR+TR above
        { 3, 2,-1,-1}, // 7  – BL+BR+TR above
        { 2, 3,-1,-1}, // 8  – TL above
        { 0, 2,-1,-1}, // 9  – BL+TL above
        { 1, 2, 3, 0}, // 10 – BR+TL above  (saddle)
        { 1, 2,-1,-1}, // 11 – BL+BR+TL above
        { 1, 3,-1,-1}, // 12 – TR+TL above
        { 0, 1,-1,-1}, // 13 – BL+TR+TL above
        { 3, 0,-1,-1}, // 14 – BR+TR+TL above
        {-1,-1,-1,-1}, // 15 – all above
    };

    struct Seg { double x0, y0, x1, y1; };
    std::vector<Seg> segs;
    segs.reserve((nrows - 1) * (ncols - 1) / 2);

    for (size_t j = 0; j < nrows - 1; ++j)
    {
        const auto& rowJ  = Z[j];
        const auto& rowJ1 = Z[j + 1];
        for (size_t i = 0; i < ncols - 1; ++i)
        {
            const double vBL = rowJ[i],      vBR = rowJ[i + 1];
            const double vTR = rowJ1[i + 1], vTL = rowJ1[i];

            // NaN is treated as below level to avoid artefacts over masked pixels.
            const int caseIdx =
                (std::isfinite(vBL) && vBL >= level ? 1 : 0) |
                (std::isfinite(vBR) && vBR >= level ? 2 : 0) |
                (std::isfinite(vTR) && vTR >= level ? 4 : 0) |
                (std::isfinite(vTL) && vTL >= level ? 8 : 0);

            const int8_t* row = lut[caseIdx];
            if (row[0] < 0) continue;

            const double di = static_cast<double>(i);
            const double dj = static_cast<double>(j);

            // Interpolate the (x,y) crossing point on edge e of cell (i,j).
            auto edgePt = [&](int e) -> std::pair<double,double>
            {
                switch (e)
                {
                    case 0: { double t = (level - vBL) / (vBR - vBL);
                              return { di + t, dj }; }
                    case 1: { double t = (level - vBR) / (vTR - vBR);
                              return { di + 1.0, dj + t }; }
                    case 2: { double t = (level - vTR) / (vTL - vTR);
                              return { di + 1.0 - t, dj + 1.0 }; }
                    case 3: { double t = (level - vTL) / (vBL - vTL);
                              return { di, dj + 1.0 - t }; }
                    default: return { di, dj };
                }
            };

            auto [x0, y0] = edgePt(row[0]);
            auto [x1, y1] = edgePt(row[1]);
            segs.push_back({ x0, y0, x1, y1 });

            if (row[2] >= 0) // saddle: emit second segment
            {
                auto [x2, y2] = edgePt(row[2]);
                auto [x3, y3] = edgePt(row[3]);
                segs.push_back({ x2, y2, x3, y3 });
            }
        }
    }

    if (segs.empty()) return {};

    // ---- Chain raw 2-point segments into maximal polylines ----------------
    // Build maps: quantised endpoint → list of segment indices that start / end there.
    std::unordered_map<long long, std::vector<int>> byStart, byEnd;
    byStart.reserve(segs.size() * 2);
    byEnd  .reserve(segs.size() * 2);
    for (int k = 0; k < static_cast<int>(segs.size()); ++k)
    {
        byStart[encodePoint(segs[k].x0, segs[k].y0)].push_back(k);
        byEnd  [encodePoint(segs[k].x1, segs[k].y1)].push_back(k);
    }

    // Return the index of the first unused segment at the given endpoint, or -1.
    auto pickUnused = [](
        const std::unordered_map<long long, std::vector<int>>& m,
        long long key, const std::vector<bool>& used) -> int
    {
        auto it = m.find(key);
        if (it == m.end()) return -1;
        for (int k : it->second)
            if (!used[k]) return k;
        return -1;
    };

    std::vector<bool> used(segs.size(), false);
    vertices_list_type result;

    for (int start = 0; start < static_cast<int>(segs.size()); ++start)
    {
        if (used[start]) continue;
        used[start] = true;

        std::deque<std::pair<double,double>> chain;
        chain.push_back({ segs[start].x0, segs[start].y0 });
        chain.push_back({ segs[start].x1, segs[start].y1 });

        // Extend forward: find the segment that starts at the current chain tail.
        for (;;)
        {
            long long key = encodePoint(chain.back().first, chain.back().second);
            int k = pickUnused(byStart, key, used);
            if (k < 0) break;
            used[k] = true;
            chain.push_back({ segs[k].x1, segs[k].y1 });
        }
        // Extend backward: find the segment that ends at the current chain head.
        for (;;)
        {
            long long key = encodePoint(chain.front().first, chain.front().second);
            int k = pickUnused(byEnd, key, used);
            if (k < 0) break;
            used[k] = true;
            chain.push_front({ segs[k].x0, segs[k].y0 });
        }

        // Append chain to the result, followed by a NaN sentinel.
        result.first .reserve(result.first .size() + static_cast<int>(chain.size()) + 1);
        result.second.reserve(result.second.size() + static_cast<int>(chain.size()) + 1);
        for (auto& [x, y] : chain)
        {
            result.first .push_back(x);
            result.second.push_back(y);
        }
        result.first .push_back(std::numeric_limits<double>::quiet_NaN());
        result.second.push_back(std::numeric_limits<double>::quiet_NaN());
    }

    return result;
}
