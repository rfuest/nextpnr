/*
 *  nextpnr -- Next Generation Place and Route
 *
 *  Copyright (C) 2021-23  gatecat <gatecat@ds0.me>
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

#include "fab_cfg.h"
#include "log.h"

#include <string>
#include <vector>

NEXTPNR_NAMESPACE_BEGIN

namespace {
	LogicConfig::LutType parse_lut_type(const parser_view &t) {
		if (t == "SINGLE_LUT")
			return LogicConfig::SINGLE_LUT;
		else if (t == "HALF_TAP_LUT")
			return LogicConfig::HALF_TAP_LUT;
		else if (t == "FULL_SPLIT_LUT")
			return LogicConfig::FULL_SPLIT_LUT;
		else
			log_error("unknown lut_type value encountered!\n");
	}


	LogicConfig::CarryType parse_carry_type(const parser_view &t) {
		if (t == "NO_CARRY")
			return LogicConfig::NO_CARRY;
		else if (t == "HA_PRE_LUT")
			return LogicConfig::HA_PRE_LUT;
		else if (t == "PG_POST_LUT")
			return LogicConfig::PG_POST_LUT;
		else if (t == "FA_POST_LUT")
			return LogicConfig::FA_POST_LUT;
		else
			log_error("unknown carry_type value encountered!\n");
	}
}

void LogicConfig::read_csv(CsvParser &csv) {
	while (csv.fetch_next_line()) {
		auto cmd = csv.next_field();
		if (cmd.empty())
			continue;
		if (cmd == "lc_per_clb") {
			lc_per_clb = csv.next_field().to_int();
		} else if (cmd == "split_lc") {
			split_lc = csv.next_field().to_int();
		} else if (cmd == "lut_k") {
			lut_k = csv.next_field().to_int();
		} else if (cmd == "lut_type") {
			lut_type = parse_lut_type(csv.next_field());
		} else if (cmd == "carry_type") {
			carry_type = parse_carry_type(csv.next_field());
		} else if (cmd == "carry_lut_frac") {
			carry_lut_frac = csv.next_field().to_int();
		} else if (cmd == "ff_per_lc") {
			ff_per_lc = csv.next_field().to_int();
		} else if (cmd == "dedi_ff_input") {
			dedi_ff_input = csv.next_field().to_int();
		} else if (cmd == "dedi_ff_output") {
			dedi_ff_output = csv.next_field().to_int();
		}
	}
}

NEXTPNR_NAMESPACE_END
