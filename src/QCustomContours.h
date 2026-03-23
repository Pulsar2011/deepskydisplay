//
//  QCustomContours.h
//  DeepSkyTools
//
//  Created by GILLARD William.
//  Self-contained iso-contour line generator using the marching squares algorithm.
//
//  Copyright (c) 2025, All rights reserved
//

#pragma once

#include <vector>

#include <DSTdisplay/QtDisplay.h>   // QCustomPlot, QCPColorGradient, QPen, QVector, …

/**
 * @class QCustomContours
 * @brief Self-contained iso-contour line generator using the marching squares algorithm.
 *
 * @details Computes iso-contour polylines from a 2D scalar field without any external
 *          plotting library. The scalar field is supplied as a row-major nested vector
 *          @c Z[row][col]. Level values are evenly spaced between the field minimum and
 *          maximum (endpoints excluded). Per-level computation is parallelised via
 *          @c std::async so the wall-clock cost scales as @c O(N×M) regardless of the
 *          number of levels requested.
 *
 *          Each level's geometry is stored as a NaN-separated polyline pair compatible
 *          with @c QCPCurve::addData(). QVector is used throughout to avoid an extra
 *          copy at the rendering stage.
 *
 *          After construction the object is immutable with respect to geometry, but the
 *          rendering style (@c pen, @c labelFontSize, @c colorGradient) may be changed
 *          at any time before calling @c render().
 *
 * @par Typical usage — flat colour
 * @code
 *   QCustomContours c(Z, 8);
 *   c.setPen(QPen(Qt::white, 1.2));
 *   c.setLabelFontSize(9);
 *   c.render(myPlot);
 *   myPlot->replot();
 * @endcode
 *
 * @par Typical usage — level-dependent colour
 * @code
 *   QCustomContours c(Z, 12);
 *   QPen basePen(Qt::black, 1.5);           // width + style; colour is overridden
 *   basePen.setStyle(Qt::DashLine);
 *   c.setPen(basePen);
 *   c.setColorGradient(QCPColorGradient::gpSpectrum);
 *   c.setLabelFontSize(8);
 *   c.render(myPlot);
 *   myPlot->replot();
 * @endcode
 */
class QCustomContours
{
public:
    /**
     * @brief Type alias for one level's contour data.
     *
     * @c first holds x-coordinates, @c second holds y-coordinates.
     * Individual polylines within a level are separated by @c NaN sentinels.
     */
    using vertices_list_type = std::pair<QVector<double>, QVector<double>>;

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct and compute iso-contour geometry.
     *
     * @param Z       2D scalar field in row-major order: @c Z[row][col].
     *                Must have at least 2 rows and 2 columns; otherwise no
     *                levels are generated and @c lines() / @c levels() are empty.
     * @param nLevels Number of evenly spaced iso-levels to compute (default: 4).
     *                Levels are placed at @c zmin + k*(zmax-zmin)/(nLevels+1)
     *                for @c k = 1 … nLevels.
     */
    explicit QCustomContours(const std::vector<std::vector<double>>& Z, size_t nLevels = 4);

    // -----------------------------------------------------------------------
    // Rendering style
    // -----------------------------------------------------------------------

    /**
     * @brief Set the pen used to draw contour lines.
     *
     * Controls width, dash pattern, and cap/join style of every @c QCPCurve
     * created by @c render().  The pen colour is used directly when no colour
     * gradient is active; otherwise it is overridden per level by the gradient.
     *
     * Default: @c QPen(Qt::green, 1.0).
     */
    void setPen(const QPen& pen);

    /** @brief Return the current contour line pen. */
    QPen pen() const;

    /**
     * @brief Set the point size of contour value labels.
     *
     * Labels are placed at 1/8 of the arc length of polylines that are longer
     * than 50 points and are rotated to follow the local tangent direction.
     * Set to @c 0 (or any negative value) to suppress labels entirely.
     * Default: @c 8.
     */
    void setLabelFontSize(int pointSize);

    /** @brief Return the current label font point size (0 = no labels). */
    int labelFontSize() const;

    // -----------------------------------------------------------------------
    // Level-dependent colour gradient
    // -----------------------------------------------------------------------

    /**
     * @brief Enable level-dependent colouring using @p gradient.
     *
     * When a colour gradient is active, each iso-level @c k is assigned the
     * gradient colour at position @c levels()[k] within the range
     * @c [levels().first(), levels().last()].  All other pen properties
     * (width, dash pattern, …) are still taken from @c pen().
     *
     * @p gradient can be constructed from any @c QCPColorGradient::GradientPreset
     * (e.g. @c gpSpectrum, @c gpHot, @c gpCold, @c gpJet, @c gpViridis, …)
     * or built as a fully custom @c QCPColorGradient.
     *
     * @note Call @c clearColorGradient() to revert to flat pen colour.
     */
    void setColorGradient(const QCPColorGradient& gradient);

    /**
     * @brief Return the currently active colour gradient.
     *
     * Only meaningful when @c hasColorGradient() is @c true.
     */
    QCPColorGradient colorGradient() const;

    /**
     * @brief Disable level-dependent colouring and revert to flat pen colour.
     *
     * After this call @c hasColorGradient() returns @c false and @c render()
     * uses @c pen().color() for all levels.
     */
    void clearColorGradient();

    /**
     * @brief Return @c true when a colour gradient is currently active.
     */
    bool hasColorGradient() const;

    // -----------------------------------------------------------------------
    // Geometry accessors
    // -----------------------------------------------------------------------

    /**
     * @brief Return the computed contour polylines, one entry per level.
     *
     * @c lines()[k] corresponds to @c levels()[k].
     * Within each entry, individual polylines are separated by @c NaN pairs.
     */
    const QVector<vertices_list_type>& lines()  const;

    /**
     * @brief Return the computed iso-level values in ascending order.
     *
     * @c levels().size() == nLevels requested at construction, or 0 when the
     * grid is too small or has no dynamic range.
     */
    const QVector<double>& levels() const;

    // -----------------------------------------------------------------------
    // Convenience renderer
    // -----------------------------------------------------------------------

    /**
     * @brief Add @c QCPCurve and @c QCPItemText objects to @p plot.
     *
     * @details Iterates over all levels and their NaN-separated polylines.
     *          For each polyline segment a @c QCPCurve is created.  The pen
     *          applied to it is derived from @c pen() with the colour replaced
     *          by the gradient colour for that level when @c hasColorGradient()
     *          is @c true.  When @c labelFontSize() > 0 and a segment contains
     *          more than 50 points, a @c QCPItemText label showing the level
     *          value is placed at 1/8 of the segment, rotated to align with
     *          the local tangent, and coloured to match the contour pen.
     *
     * @note This method does @b not call @c plot->replot() — the caller is
     *       responsible for triggering a repaint.
     *
     * @param plot Target @c QCustomPlot widget.  Must not be @c nullptr.
     */
    void render(QCustomPlot* plot) const;

private:
    // -----------------------------------------------------------------------
    // Data
    // -----------------------------------------------------------------------
    QVector<vertices_list_type> lines_;
    QVector<double>             levels_;
    QPen                        pen_;
    int                         labelFontSize_;
    bool                        useGradient_  { false };
    QCPColorGradient            gradient_;

    // -----------------------------------------------------------------------
    // Static helpers (implemented in QCustomContours.cxx)
    // -----------------------------------------------------------------------

    /**
     * @brief Compute @p n evenly spaced iso-levels for the scalar field @p Z.
     *
     * Returns an empty vector when the grid has fewer than 2 rows or 2 columns,
     * when @p n is 0, or when all finite values in @p Z are equal.
     * NaN and infinite values are ignored when computing the min/max range.
     */
    static QVector<double> computeLevels(const std::vector<std::vector<double>>& Z, size_t n);

    /**
     * @brief Encode the floating-point grid coordinate @p (x,y) as a 64-bit key.
     *
     * Coordinates are quantised to 4 decimal places before encoding, which is
     * sufficient precision for grid coordinates up to 65 535 pixels.
     */
    static long long encodePoint(double x, double y) noexcept;

    /**
     * @brief Run the marching squares algorithm for a single iso-@p level.
     *
     * @details Scans every grid cell, classifies its four corners against
     *          @p level using the 16-case lookup table, interpolates edge
     *          crossing points, and chains the resulting 2-point segments into
     *          maximal polylines via an endpoint hash map.  The returned
     *          @c vertices_list_type contains all polylines for this level
     *          concatenated and each terminated by a NaN sentinel.
     *
     * This function is a pure static method with no shared mutable state and
     * is therefore safe to call concurrently from multiple threads.
     *
     * @param Z     Row-major scalar field (requires at least 2 rows and 2 columns).
     * @param level The iso-value to trace.
     * @return      NaN-terminated polyline data, or an empty pair if no crossings.
     */
    static vertices_list_type marchingSquares(
        const std::vector<std::vector<double>>& Z, double level);
};
