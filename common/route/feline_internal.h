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

#ifndef FELINE_INTERNAL_H
#define FELINE_INTERNAL_H
#include "feline.h"
#include "hashlib.h"
#include "nextpnr.h"

#include <limits>

NEXTPNR_NAMESPACE_BEGIN
namespace Feline {
// TODO: do we also need a wire type field

struct GBox
{
    int16_t x0 = std::numeric_limits<int16_t>::max(), y0 = std::numeric_limits<int16_t>::max();
    int16_t x1 = std::numeric_limits<int16_t>::lowest(), y1 = std::numeric_limits<int16_t>::lowest();
    GBox(){};
    GBox(int16_t x, int16_t y) : x0(x), y0(y), x1(x), y1(y){};
    GBox(int16_t x0, int16_t y0, int16_t x1, int16_t y1) : x0(x0), y0(y0), x1(x1), y1(y1){};
    bool operator==(const GBox &other) const
    {
        return x0 == other.x0 && y0 == other.y0 && x1 == other.x1 && y1 == other.y1;
    }
    bool operator!=(const GBox &other) const { return !(*this == other); }
    inline void extend(GCell p)
    {
        x0 = std::min(x0, p.x);
        y0 = std::min(y0, p.y);
        x1 = std::max(x1, p.x);
        y1 = std::max(y1, p.y);
    }
};

// A sorted set of GCells
struct GCellSet
{
    std::vector<GCell> cells;
    bool dirty = false;
    void clear();
    void push(GCell cell);
    void do_sort();
    // get previous and next cell, if any
    GCell prev_cell(GCell c) const;
    GCell next_cell(GCell c) const;
    // next non-empty row in either direction
    int16_t prev_y(int16_t y) const;
    int16_t next_y(int16_t y) const;
};

// A {Steiner, Spanning} tree structure
struct STreeNode
{
    GCell uphill;          // singly linked, at least for now...
    int port_count = 0;    // port_count==0 means this is a Steiner node
    float criticality = 0; // timing criticality in [0, 1]
};
struct STree
{
    GCell source;
    dict<GCell, STreeNode> nodes;
    GCellSet ports;
    GBox box;
    static STree init_nodes(const Context *ctx, const FelineAPI *api, const NetInfo *net);
    void dump_svg(const std::string &filename) const;
    void iterate_neighbours(GCell cell, std::function<void(GCell)> func) const;
    void run_prim_djistrka(float alpha);
    void get_leaves(dict<GCell, pool<GCell>> &leaves) const;
    std::vector<GCell> topo_sorted() const;
    int get_altitudes(dict<GCell, int> &altitudes) const; // altitude=0 is **leaf-most** nodes. returns max altitude
    void do_edge_flips(float alpha);
    void steinerise_hvw();
};

// TODO: think about caching et al.
struct PerWireData
{
    int32_t curr_cong;
    float hist_cong;
    IdString reserved;
    uint16_t flags;
    uint16_t quad;
};

struct SinkIdx
{
    int32_t user_idx;
    int32_t phys_port;
};

struct PerSinkData
{
    GCell sink_gcell;
};

struct PerNetData
{
    NetInfo *net_info;
    std::vector<SSOArray<PerSinkData, 2>> sink_data; // [user idx][phys port idx]
    dict<int32_t, PipId> bwd_route_tree;             // wire -> driving pip
    dict<int32_t, std::vector<SinkIdx>> wire2sinks;  // wire -> sink user and phys port
    std::vector<int32_t> sink_order;                 // nearest sinks in steiner order are routed first (TODO: timing)
    STree steiner_tree;
};

struct SteinerProgress
{
    int steiner_idx;
    int alongness;
};

struct VisitedWire
{
    PipId prev_pip;
    float total_cost;
    SteinerProgress progress;
};

struct QueuedWire
{
    WireId wire;
};

struct DetailRouter
{
    // TODO: multithreading
    dict<WireId, VisitedWire> visit_fwd, visit_bwd;
};
struct FelineState
{
    std::vector<PerNetData> nets;
    std::vector<PerWireData> wires;
};

} // namespace Feline
NEXTPNR_NAMESPACE_END
#endif
