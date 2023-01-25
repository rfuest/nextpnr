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

#include "placer_static.h"
#include <Eigen/Core>
#include <Eigen/IterativeLinearSolvers>
#include <boost/optional.hpp>
#include <chrono>
#include <deque>
#include <fstream>
#include <numeric>
#include <queue>
#include <tuple>
#include "array2d.h"
#include "fast_bels.h"
#include "log.h"
#include "nextpnr.h"
#include "parallel_refine.h"
#include "place_common.h"
#include "placer1.h"
#include "scope_lock.h"
#include "timing.h"
#include "util.h"

NEXTPNR_NAMESPACE_BEGIN

namespace {

enum class Axis
{
    X,
    Y
};

struct RealLoc
{
    RealLoc() : x(0), y(0){};
    RealLoc(double x, double y) : x(x), y(y){};
    double x, y;
    RealLoc &operator+=(const RealLoc &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    RealLoc &operator/=(double factor)
    {
        x /= factor;
        y /= factor;
        return *this;
    }
    RealLoc operator/(double factor) const { return RealLoc(x / factor, y / factor); }
    // to simplify axis-generic code
    double &at(Axis axis) { return (axis == Axis::Y) ? y : x; }
    const double &at(Axis axis) const { return (axis == Axis::Y) ? y : x; }
};

struct PlacerGroup
{
    int total_bels = 0;
    double concrete_area = 0;
    double total_area = 0;
    array2d<float> loc_area;
};

// Could be an actual concrete netlist cell; or just a spacer
struct MoveCell
{
    StaticRect rect;
    double x, y;
    int16_t group;
    int16_t bx, by; // bins
    bool is_fixed : 1;
    bool is_spacer : 1;
};

// Extra data for cells that aren't spacers
struct ConcreteCell
{
    CellInfo *base_cell;
    // When cells are macros; we split them up into chunks
    // based on dx/dy location
    int32_t macro_root = -1;
    int16_t chunk_dx = 0, chunk_dy = 0;
};

struct ClusterGroupKey
{
    ClusterGroupKey(int dx = 0, int dy = 0, int group = -1) : dx(dx), dy(dy), group(group){};
    bool operator==(const ClusterGroupKey &other) const
    {
        return dx == other.dx && dy == other.dy && group == other.group;
    }
    unsigned hash() const { return mkhash(mkhash(dx, dy), group); }
    int16_t dx, dy, group;
};

struct PlacerMacro
{
    CellInfo *root;
    std::vector<int32_t> conc_cells;
    dict<ClusterGroupKey, std::vector<CellInfo *>> cells;
};

struct PlacerBin
{
    float density;
    // ...
};

struct PlacerNet
{
    NetInfo *ni;
};

class StaticPlacer
{
    Context *ctx;
    PlacerStaticCfg cfg;

    std::vector<MoveCell> mcells;
    std::vector<ConcreteCell> ccells;
    std::vector<PlacerMacro> macros;
    std::vector<PlacerGroup> groups;
    idict<ClusterId> cluster2idx;

    FastBels fast_bels;
    TimingAnalyser tmg;

    int width, height;
    void prepare_cells()
    {
        for (auto &cell : ctx->cells) {
            CellInfo *ci = cell.second.get();
            // process legacy-ish bel attributes
            if (ci->attrs.count(ctx->id("BEL")) && ci->bel == BelId()) {
                std::string loc_name = ci->attrs.at(ctx->id("BEL")).as_string();
                BelId bel = ctx->getBelByNameStr(loc_name);
                NPNR_ASSERT(ctx->isValidBelForCellType(ci->type, bel));
                NPNR_ASSERT(ctx->checkBelAvail(bel));
                ctx->bindBel(bel, ci, STRENGTH_USER);
            }
        }
    }

    bool lookup_group(IdString type, int &group, StaticRect &rect)
    {
        for (size_t i = 0; i < cfg.cell_groups.size(); i++) {
            const auto &g = cfg.cell_groups.at(i);
            if (g.cell_area.count(type)) {
                group = i;
                rect = g.cell_area.at(type);
                return true;
            }
        }
        return false;
    }

    void init_bels()
    {
        log_info("⌁ initialising bels...\n");
        width = 0;
        height = 0;
        for (auto bel : ctx->getBels()) {
            Loc loc = ctx->getBelLocation(bel);
            width = std::max(width, loc.x + 1);
            height = std::max(height, loc.y + 1);
        }
        dict<IdString, int> beltype2group;
        for (int i = 0; i < int(groups.size()); i++) {
            groups.at(i).loc_area.reset(width, height);
            for (const auto &bel_type : cfg.cell_groups.at(i).cell_area)
                beltype2group[bel_type.first] = i;
        }
        for (auto bel : ctx->getBels()) {
            Loc loc = ctx->getBelLocation(bel);
            IdString type = ctx->getBelType(bel);
            auto fnd = beltype2group.find(type);
            if (fnd == beltype2group.end())
                continue;
            float area = cfg.cell_groups.at(fnd->second).bel_area.at(type).area(); // TODO: do we care about dimensions too
            auto &group = groups.at(fnd->second);
            group.loc_area.at(loc.x, loc.y) += area;
            group.total_area += area;
            group.total_bels += 1;
        }
    }

    int add_cell(StaticRect rect, int group, double x, double y, CellInfo *ci = nullptr)
    {
        int idx = mcells.size();
        mcells.emplace_back();
        auto &m = mcells.back();
        m.rect = rect;
        m.group = group;
        m.x = x;
        m.y = y;
        if (ci) {
            // Is a concrete cell (might be a macro, in which case ci is just one of them...)
            // Can't add concrete cells once we have spacers (we define it such that indices line up between mcells and
            // ccells; spacer cells only exist in mcells)
            NPNR_ASSERT(idx == int(ccells.size()));
            ccells.emplace_back();
            auto &c = ccells.back();
            c.base_cell = ci;
            groups.at(group).concrete_area += rect.area();
        } else {
            // Is a spacer cell
            m.is_spacer = true;
        }
        return idx;
    }

    void init_cells()
    {
        log_info("⌁ initialising cells...\n");
        // Process non-clustered cells and find clusters
        for (auto &cell : ctx->cells) {
            CellInfo *ci = cell.second.get();
            int cell_group;
            StaticRect rect;
            // Mismatched group case
            if (!lookup_group(ci->type, cell_group, rect)) {
                // TODO: what is the best thing to do here? singletons/odd cells we can probably mostly randomly place
                continue;
            }
            if (ci->cluster != ClusterId()) {
                // Defer processing of macro clusters
                int c_idx = cluster2idx(ci->cluster);
                if (c_idx >= int(macros.size())) {
                    macros.emplace_back();
                    macros.back().root = ctx->getClusterRootCell(ci->cluster);
                }
                auto &m = macros.at(c_idx);
                Loc delta = ctx->getClusterOffset(ci);
                m.cells[ClusterGroupKey(delta.x, delta.y, cell_group)].push_back(ci);
            } else {
                // Non-clustered cells can be processed already
                int idx = add_cell(rect, cell_group, ctx->rngf(width), ctx->rngf(height), ci);
                if (ci->bel != BelId()) {
                    // Currently; treat all ready-placed cells as fixed (eventually we might do incremental ripups
                    // here...)
                    auto &mc = mcells.at(idx);
                    Loc loc = ctx->getBelLocation(ci->bel);
                    mc.x = loc.x + 0.5;
                    mc.y = loc.y + 0.5;
                    mc.is_fixed = true;
                }
            }
        }
        // Process clustered cells
        for (int i = 0; i < int(macros.size()); i++) {
            auto &m = macros.at(i);
            for (auto &kv : m.cells) {
                const auto &g = cfg.cell_groups.at(kv.first.group);
                // Only treat zero-area cells as zero-area; if this cluster also contains non-zero area cells
                bool has_nonzero = std::any_of(kv.second.begin(), kv.second.end(),
                                               [&](const CellInfo *ci) { return !g.zero_area_cells.count(ci->type); });
                StaticRect cluster_size;
                for (auto ci : kv.second) {
                    if (has_nonzero && g.zero_area_cells.count(ci->type))
                        continue;
                    // Compute an equivalent-area stacked rectange for cells in this cluster group.
                    // There are probably some ugly cases this handles badly.
                    StaticRect r = g.cell_area.at(ci->type);
                    if (r.w > r.h) {
                        // Long and thin, "stack" vertically
                        // Compute height we add to stack
                        if (cluster_size.w < r.w) {
                            cluster_size.h *= (cluster_size.w / r.w);
                            cluster_size.w = r.w;
                        }
                        cluster_size.h += ((r.w * r.h) / cluster_size.w);
                    } else {
                        // "stack" horizontally
                        if (cluster_size.h < r.h) {
                            cluster_size.w *= (cluster_size.h / r.h);
                            cluster_size.h = r.h;
                        }
                        cluster_size.w += ((r.w * r.h) / cluster_size.h);
                    }
                }
                // Now add the moveable cell
                if (cluster_size.area() > 0) {
                    int idx = add_cell(cluster_size, kv.first.group, ctx->rngf(width), ctx->rngf(height),
                                       kv.second.front());
                    if (kv.second.front()->bel != BelId()) {
                        // Currently; treat all ready-placed cells as fixed (eventually we might do incremental ripups
                        // here...)
                        auto &mc = mcells.at(idx);
                        Loc loc = ctx->getBelLocation(kv.second.front()->bel);
                        mc.x = loc.x + 0.5;
                        mc.y = loc.y + 0.5;
                        mc.is_fixed = true;
                    }
                }
            }
        }
    }

    const double target_util = 0.8;

    void insert_spacer()
    {
        log_info("⌁ inserting spacers...\n");
        int inserted_spacers = 0;
        for (int group = 0; group < int(groups.size()); group++) {
            const auto &cg = cfg.cell_groups.at(group);
            const auto &g = groups.at(group);
            double util = g.concrete_area / g.total_area;
            log_info("⌁   group %s pre-spacer utilisation %.02f%% (target %.02f%%)\n", ctx->nameOf(cg.name), (util*100.0), (target_util*100.0));
            // TODO: better computation of spacer size and placement?
            int spacer_count = (g.total_area * target_util - g.concrete_area) / cg.spacer_rect.area();
            if (spacer_count <= 0)
                continue;
            for (int i = 0; i < spacer_count; i++) {
                add_cell(cg.spacer_rect, group, ctx->rngf(width), ctx->rngf(height), nullptr /*spacer*/);
                ++inserted_spacers;
            }
        }
        log_info("⌁   inserted a total of %d spacers\n", inserted_spacers);
    }

  public:
    StaticPlacer(Context *ctx, PlacerStaticCfg cfg) : ctx(ctx), cfg(cfg), fast_bels(ctx, true, 8), tmg(ctx){
        groups.resize(cfg.cell_groups.size());
    };
    void place()
    {
        log_info("Running Static placer...\n");
        init_bels();
        prepare_cells();
        init_cells();
        insert_spacer();
    }
};
}; // namespace

bool placer_static(Context *ctx, PlacerStaticCfg cfg)
{
    StaticPlacer(ctx, cfg).place();
    return true;
}

PlacerStaticCfg::PlacerStaticCfg(Context *ctx)
{
    timing_driven = ctx->setting<bool>("timing_driven");

    hpwl_scale_x = 1;
    hpwl_scale_y = 1;
}

NEXTPNR_NAMESPACE_END
