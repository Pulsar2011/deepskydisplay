//
//  QtFitsPloting.h
//  DeepSkyTools — public plotting utilities for the DSTdisplay library
//
//  Created by GILLARD William
//  Centre de Physique des Particules de Marseille
//  Licensed under CC BY-NC 4.0
//

#pragma once

#ifdef __WITH_QT__
#include <DSTdisplay/QtDisplay.h>   // QCPColorGradient, QColor
#include <DSTfits/FITSdata.h>   // QCPColorGradient, QColor

namespace QtFITS
{

    /**
     * @brief Build a Viridis-equivalent QCPColorGradient.
     *
     * QCustomPlot v2.1.1 does not include a @c gpViridis preset. This function
     * constructs an equivalent gradient from explicit colour stops matching the
     * Matplotlib Viridis palette (dark purple → blue → teal → green → yellow).
     *
     * @return A @c QCPColorGradient configured with 9 Viridis colour stops.
     */
    inline QCPColorGradient makeViridisGradient()
    {
        QCPColorGradient g;
        g.clearColorStops();
        g.setColorStops({
            {0.000, QColor( 68,   1,  84)},
            {0.125, QColor( 72,  40, 120)},
            {0.250, QColor( 62,  83, 140)},
            {0.375, QColor( 49, 104, 142)},
            {0.500, QColor( 38, 130, 142)},
            {0.625, QColor( 31, 158, 137)},
            {0.750, QColor( 53, 183, 121)},
            {0.875, QColor(110, 206,  88)},
            {1.000, QColor(253, 231,  37)},
        });
        return g;
    }

    /**
     * @brief Pack two @c DSL::dtype values into a single 32-bit key.
     *
     * @details Used internally by the plotting dispatchers to select a
     * type-specific code path from a @c std::unordered_map keyed on the
     * combination of the X and Y column types without requiring a nested
     * switch or a @c std::pair key.
     *
     * @param a Type of the first (e.g. x-axis) column.
     * @param b Type of the second (e.g. y-axis) column.
     * @return A @c uint32_t where the upper 16 bits hold @p a and the lower
     *         16 bits hold @p b.
     */
    constexpr uint32_t dtype_pair(DSL::dtype a, DSL::dtype b)
    {
        return (static_cast<uint32_t>(a) << 16) | static_cast<uint32_t>(b);
    }

} // namespace QtFITS
#endif // __WITH_QT__
