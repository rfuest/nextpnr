
/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2021 gatecat <gatecat@ds0.me>
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

#ifndef HIMBAECHEL_CHIPDB_H
#define HIMBAECHEL_CHIPDB_H

#include "archdefs.h"
#include "nextpnr_namespaces.h"
#include "relptr.h"

NEXTPNR_NAMESPACE_BEGIN

NPNR_PACKED_STRUCT(struct BelPinPOD {
    int32_t name;
    int32_t wire;
    int32_t type;
});

NPNR_PACKED_STRUCT(struct BelDataPOD {
    int32_t name;
    int32_t bel_type;
    // These are really 96-bits of general data, with some names intended to be vaguely helpful...
    int32_t site;
    int32_t checker_idx;
    int32_t flags;
    // General placement
    int16_t z;
    int16_t bucket;
    RelSlice<BelPinPOD> pins;
});

NPNR_PACKED_STRUCT(struct BelPinRefPOD {
    int32_t bel;
    int32_t pin;
});

NPNR_PACKED_STRUCT(struct TileWireDataPOD {
    int32_t name;
    int32_t wire_type;
    int32_t flags; // 32 bits of arbitrary data
    RelSlice<int32_t> pips_uphill;
    RelSlice<int32_t> pips_downhill;
    RelSlice<BelPinRefPOD> bel_pins;
});

NPNR_PACKED_STRUCT(struct PipDataPOD {
    int32_t src_wire;
    int32_t dst_wire;

    uint32_t type;
    uint32_t flags;
});

NPNR_PACKED_STRUCT(struct RelTileWireRefPOD {
    int16_t dx;
    int16_t dy;
    int16_t wire;
});

NPNR_PACKED_STRUCT(struct NodeShapePOD { RelSlice<RelTileWireRefPOD> tile_wires; });

NPNR_PACKED_STRUCT(struct TileTypePOD {
    int32_t type_name;
    RelSlice<BelDataPOD> bels;
    RelSlice<TileWireDataPOD> wires;
    RelSlice<PipDataPOD> pips;
    RelSlice<uint8_t> extra_data;
});

NPNR_PACKED_STRUCT(struct RelNodeRefPOD {
    // wire is entirely internal to a single tile
    static constexpr int16_t MODE_TILE_WIRE = 0x7000;
    // where this is the root {wire, dy} form the node shape index
    static constexpr int16_t MODE_IS_ROOT = 0x7001;
    // special cases for the global constant nets
    static constexpr int16_t MODE_ROW_CONST = 0x7002;
    static constexpr int16_t MODE_GLB_CONST = 0x7003;
    // special cases where the user needs to outsmart the deduplication
    static constexpr int16_t MODE_USR_BEGIN = 0x7010;
    int16_t dx_mode; // relative X-coord, or a special value
    int16_t dy;      // normally, relative Y-coord
    uint16_t wire;   // normally, node index in tile (x+dx, y+dy)
});

NPNR_PACKED_STRUCT(struct RoutingShapePOD { RelSlice<RelNodeRefPOD> wire_to_node; });

NPNR_PACKED_STRUCT(struct PadInfoPOD {
    // package pin name
    int32_t package_pin;
    // reference to corresponding bel
    int32_t tile;
    int32_t bel;
    // function name
    int32_t pad_function;
    // index of pin bank
    int32_t pad_bank;
    // extra pad flags
    uint32_t flags;
    RelSlice<uint8_t> extra_data;
});

NPNR_PACKED_STRUCT(struct PackageInfoPOD {
    int32_t name;
    RelSlice<PadInfoPOD> pads;
});

NPNR_PACKED_STRUCT(struct TileInstPOD {
    int32_t type;
    int32_t shape;

    RelSlice<uint8_t> extra_data;
});

NPNR_PACKED_STRUCT(struct ChipInfoPOD {
    RelSlice<char> uarch;
    RelSlice<char> name;
    RelSlice<char> generator;

    int32_t version;
    int32_t width, height;
    RelSlice<TileTypePOD> tile_types;
    RelSlice<TileInstPOD> tile_insts;
    RelSlice<NodeShapePOD> node_shapes;
    RelSlice<RoutingShapePOD> tile_shapes;

    RelSlice<PackageInfoPOD> packages;

    RelPtr<ConstIDDataPOD> extra_constids;

    RelSlice<uint8_t> extra_data;
});

NEXTPNR_NAMESPACE_END
#endif
