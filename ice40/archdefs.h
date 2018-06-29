/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2018  Clifford Wolf <clifford@symbioticeda.com>
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

#ifndef NEXTPNR_H
#error Include "archdefs.h" via "nextpnr.h" only.
#endif

NEXTPNR_NAMESPACE_BEGIN

typedef int delay_t;

struct DelayInfo
{
    delay_t delay = 0;

    delay_t raiseDelay() const { return delay; }
    delay_t fallDelay() const { return delay; }
    delay_t avgDelay() const { return delay; }

    DelayInfo operator+(const DelayInfo &other) const
    {
        DelayInfo ret;
        ret.delay = this->delay + other.delay;
        return ret;
    }
};

// -----------------------------------------------------------------------

enum BelType : int32_t
{
    TYPE_NONE,
    TYPE_ICESTORM_LC,
    TYPE_ICESTORM_RAM,
    TYPE_SB_IO,
    TYPE_SB_GB,
    TYPE_ICESTORM_PLL,
    TYPE_SB_WARMBOOT,
    TYPE_SB_MAC16,
    TYPE_ICESTORM_HFOSC,
    TYPE_ICESTORM_LFOSC,
    TYPE_SB_I2C,
    TYPE_SB_SPI,
    TYPE_IO_I3C,
    TYPE_SB_LEDDA_IP,
    TYPE_SB_RGBA_DRV,
    TYPE_ICESTORM_SPRAM,
};

enum PortPin : int32_t
{
    PIN_NONE,
#define X(t) PIN_##t,
#include "portpins.inc"
#undef X
    PIN_MAXIDX
};

enum WireType : int8_t
{
    WIRE_TYPE_NONE = 0,
    WIRE_TYPE_LOCAL = 1,
    WIRE_TYPE_GLOBAL = 2,
    WIRE_TYPE_SP4_VERT = 3,
    WIRE_TYPE_SP4_HORZ = 4,
    WIRE_TYPE_SP12_HORZ = 5,
    WIRE_TYPE_SP12_VERT = 6
};

struct BelId
{
    int32_t index = -1;

    bool operator==(const BelId &other) const { return index == other.index; }
    bool operator!=(const BelId &other) const { return index != other.index; }
};

struct WireId
{
    int32_t index = -1;

    bool operator==(const WireId &other) const { return index == other.index; }
    bool operator!=(const WireId &other) const { return index != other.index; }
};

struct PipId
{
    int32_t index = -1;

    bool operator==(const PipId &other) const { return index == other.index; }
    bool operator!=(const PipId &other) const { return index != other.index; }
};

NEXTPNR_NAMESPACE_END

namespace std {
template <> struct hash<NEXTPNR_NAMESPACE_PREFIX BelId>
{
    std::size_t operator()(const NEXTPNR_NAMESPACE_PREFIX BelId &bel) const noexcept { return hash<int>()(bel.index); }
};

template <> struct hash<NEXTPNR_NAMESPACE_PREFIX WireId>
{
    std::size_t operator()(const NEXTPNR_NAMESPACE_PREFIX WireId &wire) const noexcept
    {
        return hash<int>()(wire.index);
    }
};

template <> struct hash<NEXTPNR_NAMESPACE_PREFIX PipId>
{
    std::size_t operator()(const NEXTPNR_NAMESPACE_PREFIX PipId &pip) const noexcept { return hash<int>()(pip.index); }
};

template <> struct hash<NEXTPNR_NAMESPACE_PREFIX BelType> : hash<int>
{
};

template <> struct hash<NEXTPNR_NAMESPACE_PREFIX PortPin> : hash<int>
{
};
} // namespace std
