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

#ifndef FELINE_H
#define FELINE_H

#include "nextpnr.h"

NEXTPNR_NAMESPACE_BEGIN

namespace Feline {

struct GCell
{
    int16_t x = std::numeric_limits<int16_t>::lowest(), y = std::numeric_limits<int16_t>::lowest();
    GCell(){};
    GCell(int16_t x, int16_t y) : x(x), y(y){};
    explicit GCell(Loc loc) : x(loc.x), y(loc.y){};
    bool operator==(const GCell &other) const { return x == other.x && y == other.y; }
    bool operator<(const GCell &other) const { return y < other.y || ((y == other.y) && (x < other.x)); }
    bool operator<=(const GCell &other) const { return (*this == other) || (*this < other); }
    bool operator>=(const GCell &other) const { return !(*this < other); }
    bool operator!=(const GCell &other) const { return x != other.x || y != other.y; }
    unsigned hash() const { return mkhash(x, y); }
    int mdist(GCell other) const { return std::abs(x - other.x) + std::abs(y - other.y); }
};

/*
We allow architectures to provide a highly abstracted model of their long-distance routing for initial congestion
estimates of different routing resources. This is roughly analagous to layer assignment in VLSI routing, with the use of
longer wires being equivalent to higher layers.
*/
enum class RRDir
{
    HORIZ,
    VERT,
};

struct RoutingResource
{
    int width;
    std::vector<int> hops; // delta from source
    RRDir dir;
    // Example:
    //     an interconnect that has a source position at Δ(0, 0), sinks at Δ(0, -1) and Δ(0, -4)
    //     would have a hops of {-1, -4} and a dir of VERT
};

struct FelineAPI
{
    virtual bool isInterconnect(int x, int y) const = 0; // returns true if general interconnect exists at a given tile
    virtual GCell getPinInterconLoc(BelId bel, IdString pin)
            const = 0; // gets the location of the general interconnect tile that a bel pin connects to (in many arches,
                       // this isn't always the same tile as the bel)
    virtual int32_t flatWireIndex(
            WireId wire) const = 0; // gets a fast flat index for a wire. this doesn't have to be fully contiguous, but
                                    // should not be any sparser than necessary to avoid wasting memory
    virtual int32_t flatWireSize() const = 0; // returns the exclusive upper bound of flatWireIndex
    virtual GCell approxWireLoc(WireId wire)
            const = 0; // gets an approximate location (any point on the wire) for partitioning and heuristic purposes
    virtual bool steinerSkipPort(const NetInfo *net, const PortRef &port)
            const = 0; // returns true if a port should go straight to detail routing without steinerisation
    virtual std::vector<RoutingResource>
    getChannels() const = 0; // gets the abstracted routing resource channel model for congestion estimation
};

struct BaseFelineAPI : FelineAPI
{
    BaseFelineAPI(Context *ctx, bool init_flat_wires = true);

    virtual bool isInterconnect(int x, int y) const override { return true; }
    virtual GCell getPinInterconLoc(BelId bel, IdString pin) const override { return GCell(ctx->getBelLocation(bel)); }
    virtual int32_t flatWireIndex(WireId wire) const override { return wire2idx.at(wire); };
    virtual int32_t flatWireSize() const override { return int32_t(wire2idx.size()); }
    virtual GCell approxWireLoc(WireId wire) const override
    {
        ArcBounds bb = ctx->getRouteBoundingBox(wire, wire);
        return GCell((bb.x0 + bb.x1) / 2, (bb.y0 + bb.y1) / 2);
    }
    virtual bool steinerSkipPort(const NetInfo *net, const PortRef &port) const override { return false; }
    virtual std::vector<RoutingResource> getChannels() const override { return {}; }
    Context *ctx;
    idict<WireId> wire2idx;
};
}; // namespace Feline

struct FelineCfg
{
    FelineCfg(Context *ctx){};
};

void feline_route(Context *ctx, const FelineCfg &cfg, Feline::FelineAPI *api);

NEXTPNR_NAMESPACE_END

#endif
