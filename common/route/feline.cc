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

#include "feline.h"
#include <queue>
#include "array_2d.h"
#include "feline_internal.h"
#include "log.h"
#include "nextpnr.h"

NEXTPNR_NAMESPACE_BEGIN

/*
 * FELINE -- Fast Estimate Led Incremental eNginE
 *
 * The aim is to apply VLSI style routing methodology to the FPGA context.
 *
 * Instead of going straight into exact detail/routing like router1/router2, the aim of FELINE is to generate a rough
 * global route based on grid locations only (and RMST decompositions of high-fanout nets) and use that to strictly
 * guide the detailed routing phase that finds an exact path of legal resources.
 *
 * cite:  A Survey on Steiner Tree Construction and Global Routing for VLSI Design
 *        https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=9057662
 */

namespace Feline {
BaseFelineAPI::BaseFelineAPI(Context *ctx, bool init_flat_wires) : ctx(ctx)
{
    if (init_flat_wires) {
        for (auto wire : ctx->getWires())
            wire2idx(wire);
    }
}

} // namespace Feline

void feline_route(Context *ctx, const FelineCfg &cfg, Feline::FelineAPI *api)
{
    using namespace Feline;
    NPNR_UNUSED(api);
    // EXPERIMENT: find the highest fanout unrouted, nonglobal net and dump the spanning/steiner/... tree of it
    const NetInfo *target = nullptr;
    for (auto &net : ctx->nets) {
        NetInfo *ni = net.second.get();
#ifdef ARCH_ECP5
        if (ni->is_global)
            continue;
#endif
        if (!ni->wires.empty())
            continue;
        if (!ni->driver.cell)
            continue;
        if (!target || (ni->users.entries() > target->users.entries()))
            target = ni;
    }
    log_info("using %s (%d) for tree experiments...\n", ctx->nameOf(target), int(target->users.entries()));
    STree tree = STree::init_nodes(ctx, api, target);
    tree.run_prim_djistrka(0.5);
    tree.dump_svg("stree_empty.svg");
    // tree.do_edge_flips(0.5);
    // tree.dump_svg("stree_flps.svg");
    tree.steinerise_hvw();
    tree.dump_svg("stree_hvw.svg");
    NPNR_ASSERT(false); // TODO
}

NEXTPNR_NAMESPACE_END
