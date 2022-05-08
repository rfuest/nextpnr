/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2022  gatecat <gatecat@ds0.me>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <queue>
#include "feline.h"

NEXTPNR_NAMESPACE_BEGIN

namespace Feline {

struct InFlightArc
{
    GCell src_gcell;
    std::vector<GCell> global_path;

    int within_tol_of_cell(GCell target, GCell actual, int xtol, int ytol)
    {
        int xdist = std::abs(target.x - actual.x);
        int ydist = std::abs(target.y - actual.y);
        if (xdist <= xtol && ydist <= ytol)
            return (xdist + ydist);
        else
            return -1;
    }

    int within_tol_of_line(GCell line0, GCell line1, GCell actual, int xtol, int ytol)
    {
        bool horiz = (line1.y == line0.y);
        if (!horiz)
            NPNR_ASSERT(line1.x == line0.x);
        // Diagonal lines in global routing prohibited for now
        // The 'moving' axis of the line
        int m0 = (horiz ? line0.x : line0.y), m1 = (horiz ? line1.x : line1.y), ma = (horiz ? actual.x : actual.y),
            mtol = (horiz ? xtol : ytol);
        // The 'fixed' axis of the line
        int f = (horiz ? line0.y : line0.x), fa = (horiz ? actual.y : actual.x), ftol = (horiz ? ytol : xtol);
        // Never allow going backwards by more than the tolerance (abort further GCell checks if we find this)
        if ((m1 > m0 && ma < (m0 - mtol)) /* increasing m0->m1 case */ ||
            (m1 < m0 && ma > (m0 + mtol)) /* decreasing m0->m1 case */)
            return -2; // nyeh, not the nicest way to encode these results but it works
        // We've at least made none or some progress at this point. What we want to check is if we are on the line
        int df = std::abs(fa - f);
        if (df > ftol)
            return -1; // not on the line, but not a backwards step either
        // compute total delta, from the fixed axis plus the moving axis
        if (ma < std::min(m0, m1))
            return df + (std::min(m0, m1) - ma);
        else if (ma > std::max(m0, m1))
            return df + (ma - std::max(m0, m1));
        else
            return df;
    }

    int get_next_global_idx(GCell next, int curr, int xtol, int ytol)
    {
        //
        // Not on global path
        return -1;
    }
};

} // namespace Feline

NEXTPNR_NAMESPACE_END
