/**
 * Copyright 2013-2023 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "srsran/phy/modem/mod.h"
#include "srsran/phy/utils/bit.h"
#include "srsran/phy/utils/debug.h"

/** Low-level API */

int srsran_mod_modulate(const srsran_modem_table_t* q, uint8_t* bits, cf_t* symbols, uint32_t nbits)
{
  uint32_t i, j, idx;
  uint8_t* b_ptr = (uint8_t*)bits;
  j              = 0;
  for (i = 0; i < nbits; i += q->nbits_x_symbol) {
    idx = srsran_bit_pack(&b_ptr, q->nbits_x_symbol);
    if (idx < q->nsymbols) {
      symbols[j] = q->symbol_table[idx];
    } else {
      return SRSRAN_ERROR;
    }
    j++;
  }
  return j;
}

static void mod_bpsk_bytes(const srsran_modem_table_t* q, const uint8_t* bits, cf_t* symbols, uint32_t nbits)
{
  uint8_t mask_bpsk[8]  = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
  uint8_t shift_bpsk[8] = {7, 6, 5, 4, 3, 2, 1, 0};

  for (int i = 0; i < nbits / 8; i++) {
    memcpy(&symbols[8 * i], &q->symbol_table_bpsk[bits[i]], sizeof(bpsk_packed_t));
  }
  for (int i = 0; i < nbits % 8; i++) {
    symbols[8 * (nbits / 8) + i] = q->symbol_table[(bits[8 * (nbits / 8) + i] & mask_bpsk[i]) >> shift_bpsk[i]];
  }
}

static void mod_qpsk_bytes(const srsran_modem_table_t* q, const uint8_t* bits, cf_t* symbols, uint32_t nbits)
{
  uint8_t mask_qpsk[4]  = {0xc0, 0x30, 0x0c, 0x03};
  uint8_t shift_qpsk[4] = {6, 4, 2, 0};
  for (int i = 0; i < nbits / 8; i++) {
    memcpy(&symbols[4 * i], &q->symbol_table_qpsk[bits[i]], sizeof(qpsk_packed_t));
  }
  // Encode last 1, 2 or 3 bit pairs if not multiple of 8
  for (int i = 0; i < (nbits % 8) / 2; i++) {
    symbols[4 * (nbits / 8) + i] = q->symbol_table[(bits[nbits / 8] & mask_qpsk[i]) >> shift_qpsk[i]];
  }
}

static void mod_16qam_bytes(const srsran_modem_table_t* q, const uint8_t* bits, cf_t* symbols, uint32_t nbits)
{
  for (int i = 0; i < nbits / 8; i++) {
    memcpy(&symbols[2 * i], &q->symbol_table_16qam[bits[i]], sizeof(qam16_packed_t));
  }
  // Encode last 4 bits if not multiple of 8
  if (nbits % 8) {
    symbols[2 * (nbits / 8)] = q->symbol_table[(bits[nbits / 8] & 0xf0) >> 4];
  }
}

static void mod_64qam_bytes(const srsran_modem_table_t* q, const uint8_t* bits, cf_t* symbols, uint32_t nbits)
{
  uint8_t  in0, in1, in2, in3;
  uint32_t in80, in81, in82;

  for (int i = 0; i < nbits / 24; i++) {
    in80 = bits[3 * i + 0];
    in81 = bits[3 * i + 1];
    in82 = bits[3 * i + 2];

    in0 = (in80 & 0xfc) >> 2;
    in1 = (in80 & 0x03) << 4 | ((in81 & 0xf0) >> 4);
    in2 = (in81 & 0x0f) << 2 | ((in82 & 0xc0) >> 6);
    in3 = in82 & 0x3f;

    symbols[i * 4 + 0] = q->symbol_table[in0];
    symbols[i * 4 + 1] = q->symbol_table[in1];
    symbols[i * 4 + 2] = q->symbol_table[in2];
    symbols[i * 4 + 3] = q->symbol_table[in3];
  }
  if (nbits % 24 >= 6) {
    in80 = bits[3 * (nbits / 24) + 0];
    in0  = (in80 & 0xfc) >> 2;

    symbols[4 * (nbits / 24) + 0] = q->symbol_table[in0];
  }
  if (nbits % 24 >= 12) {
    in81 = bits[3 * (nbits / 24) + 1];
    in1  = (in80 & 0x03) << 4 | ((in81 & 0xf0) >> 4);

    symbols[4 * (nbits / 24) + 1] = q->symbol_table[in1];
  }
  if (nbits % 24 >= 18) {
    in82 = bits[3 * (nbits / 24) + 2];
    in2  = (in81 & 0x0f) << 2 | ((in82 & 0xc0) >> 6);

    symbols[4 * (nbits / 24) + 2] = q->symbol_table[in2];
  }
}

static void mod_256qam_bytes(const srsran_modem_table_t* q, const uint8_t* bits, cf_t* symbols, uint32_t nbits)
{
  for (int i = 0; i < nbits / 8; i++) {
    symbols[i] = q->symbol_table[bits[i]];
  }
}

static void mod_1024qam_bytes(const srsran_modem_table_t* q, const uint8_t* bits, cf_t* symbols, uint32_t nbits)
{
  uint32_t nblocks = nbits / 40;
  for (uint32_t i = 0; i < nblocks; i++) {
    uint8_t in0 = bits[5 * i + 0];
    uint8_t in1 = bits[5 * i + 1];
    uint8_t in2 = bits[5 * i + 2];
    uint8_t in3 = bits[5 * i + 3];
    uint8_t in4 = bits[5 * i + 4];

    uint16_t idx0 = ((uint16_t)in0 << 2) | (in1 >> 6);
    uint16_t idx1 = ((uint16_t)(in1 & 0x3f) << 4) | (in2 >> 4);
    uint16_t idx2 = ((uint16_t)(in2 & 0x0f) << 6) | (in3 >> 2);
    uint16_t idx3 = ((uint16_t)(in3 & 0x03) << 8) | in4;

    symbols[i * 4 + 0] = q->symbol_table[idx0];
    symbols[i * 4 + 1] = q->symbol_table[idx1];
    symbols[i * 4 + 2] = q->symbol_table[idx2];
    symbols[i * 4 + 3] = q->symbol_table[idx3];
  }

  uint32_t rem_bits = nbits % 40;
  if (rem_bits) {
    const uint8_t* in = &bits[5 * nblocks];
    uint32_t       rem_bytes = (rem_bits + 7) / 8;
    uint32_t       out_idx = 4 * nblocks;
    const uint32_t shift_base = 14; // 24-bit window minus 10-bit symbol.

    for (uint32_t symbol = 0; symbol < rem_bits / 10; symbol++) {
      uint32_t bit_offset = symbol * 10;
      uint32_t byte_idx   = bit_offset / 8;
      uint32_t bit_idx    = bit_offset % 8;
      uint8_t  b0         = in[byte_idx];
      uint8_t  b1         = (byte_idx + 1 < rem_bytes) ? in[byte_idx + 1] : 0;
      uint8_t  b2         = (byte_idx + 2 < rem_bytes) ? in[byte_idx + 2] : 0;
      uint32_t value      = ((uint32_t)b0 << 16) | ((uint32_t)b1 << 8) | b2;
      uint32_t shift      = shift_base - bit_idx;
      uint16_t idx        = (uint16_t)((value >> shift) & 0x3ff);

      symbols[out_idx++] = q->symbol_table[idx];
    }
  }
}

/* Assumes packet bits as input */
int srsran_mod_modulate_bytes(const srsran_modem_table_t* q, const uint8_t* bits, cf_t* symbols, uint32_t nbits)
{
  if (!q->byte_tables_init) {
    ERROR("Error need to initiated modem tables for packeted bits before calling srsran_mod_modulate_bytes()");
    return SRSRAN_ERROR;
  }
  if (nbits % q->nbits_x_symbol) {
    ERROR("Error modulator expects number of bits (%d) to be multiple of %d", nbits, q->nbits_x_symbol);
    return -1;
  }
  switch (q->nbits_x_symbol) {
    case 1:
      mod_bpsk_bytes(q, bits, symbols, nbits);
      break;
    case 2:
      mod_qpsk_bytes(q, bits, symbols, nbits);
      break;
    case 4:
      mod_16qam_bytes(q, bits, symbols, nbits);
      break;
    case 6:
      mod_64qam_bytes(q, bits, symbols, nbits);
      break;
    case 8:
      mod_256qam_bytes(q, bits, symbols, nbits);
      break;
    case 10:
      mod_1024qam_bytes(q, bits, symbols, nbits);
      break;
    default:
      ERROR("srsran_mod_modulate_bytes() accepts BPSK/QPSK/16QAM/64QAM/256QAM/1024QAM modulations only");
      return SRSRAN_ERROR;
  }
  return nbits / q->nbits_x_symbol;
}
