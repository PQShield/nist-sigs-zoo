/* PATCH (nist-sigs-zoo, fix): Rhyme-SHAKE only. encode_z indexes the z1 rANS symbol table es1[RANS_NZ1] with
 * hi = (z1 + B0) >> RANS_L1 and no bound check. rans_freqs.h (identical in all trees) was sized for the SM3
 * parameters (B0 = 382/582/599/903), but the SHAKE params_tables.h has larger bounds (B0 = 388/598/605/916), so
 * the table has fewer z1 symbols than (2*B0 >> RANS_L1) + 1: 96 < 98 (128), 146 < 150 (256), 150 < 152 (384),
 * 113 < 115 (512). Sample_Z accepts every |z1| <= B0, so a z1 coefficient >= 380, 570, 595 or 892 reads a
 * RansEncSymbol past the end of es1 (ASan: global-buffer-overflow). Depending on what lies there, the
 * renormalisation loop in RansEncPutSymbol writes below the malloc'd rtmp buffer until it faults (SIGSEGV), or
 * the signer returns a signature that verify rejects (decode_z can never produce hi >= RANS_NZ1).
 * This copy treats such a z1 as unencodable: encode_z returns 0, pack_sig
 * fails and crypto_sign_signature moves on to its next iteration, as it already does for any encoding failure.
 * Signatures that encoded correctly before are unchanged (KAT PASS). Otherwise the submitted src/encoding.c. */
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "params.h"
#include "encoding.h"
#include "rans_byte.h"
#include "rans_freqs.h"

#define SCALE_BITS 16

/* Construction 4 ("cut-F") split coding, two tables:
 *   z1 (p=0):           v' = v + B0, hi = v' >> RANS_L1, lo raw.  no escape.
 *   z_rest rows 0..d-1: sigma_rest table (RANS_LS, RANS_CENTER_S) + ESC.
 * Every retained z_rest row is a SHORT-row output with identical statistics,
 * so there is no separate long F-row table.  (RANS_*B mirror RANS_*S for
 * format compatibility but are unused.)
 * ESC: 24 raw bits of (v + 2^23). */
#define ESC_BITS 24
#define ESC_OFF (1 << 23)

typedef struct {
    const uint16_t *freq;
    int nsym;
    RansEncSymbol *esyms;
    RansDecSymbol *dsyms;
    uint16_t *lut;
} rtable;
static RansEncSymbol es1[RANS_NZ1], ess[RANS_NZS];
static RansDecSymbol ds1[RANS_NZ1], dss[RANS_NZS];
static rtable T1 = { rans_freq_z1, RANS_NZ1, es1, ds1, 0 };
static rtable TS = { rans_freq_zs, RANS_NZS, ess, dss, 0 };
static int tables_ready;

static void build_one(rtable *t) {
    t->lut = malloc((size_t)(1u << SCALE_BITS) * sizeof(uint16_t));
    uint32_t start = 0;
    for (int s = 0; s < t->nsym; s++) {
        RansEncSymbolInit(&t->esyms[s], start, t->freq[s], SCALE_BITS);
        RansDecSymbolInit(&t->dsyms[s], start, t->freq[s]);
        for (uint32_t i = 0; i < t->freq[s]; i++) t->lut[start + i] = (uint16_t)s;
        start += t->freq[s];
    }
}
/* Build the rANS lookup tables once. Exposed via rhyme_encoding_init() so that
 * table construction (a one-time cost, not part of signing) is hoisted out of
 * the sign/verify hot path. The tables_ready guard keeps encode_z/decode_z
 * correct even if init is not called explicitly: the first call builds them,
 * every later call returns immediately. */
void RHYME_NAMESPACE(encoding_init)(void) {
    if (tables_ready) return;
    build_one(&T1); build_one(&TS);
    tables_ready = 1;
}

static const rtable *row_table(int p) {
    return (p == 0) ? &T1 : &TS;          /* cut-F: every z_rest row uses TS */
}
static unsigned row_L(int p)     { return p == 0 ? RANS_L1 : RANS_LS; }
static int32_t row_center(int p) { return p == 0 ? B0 : RANS_CENTER_S; }
static int row_has_esc(int p)    { return p != 0; }

/* ---------------- raw bit stream (LSB first) ---------------- */
typedef struct { uint8_t *p; size_t cap, bytes; uint32_t acc; unsigned nbits; } raww;
static void rw_init(raww *w, uint8_t *p, size_t cap) { w->p = p; w->cap = cap; w->bytes = 0; w->acc = 0; w->nbits = 0; }
static int rw_put(raww *w, uint32_t v, unsigned bits) {
    w->acc |= (v & ((bits < 32 ? (1u << bits) : 0u) - 1u)) << w->nbits;
    w->nbits += bits;
    while (w->nbits >= 8) {
        if (w->bytes >= w->cap) return -1;
        w->p[w->bytes++] = (uint8_t)w->acc;
        w->acc >>= 8;
        w->nbits -= 8;
    }
    return 0;
}
static int rw_flush(raww *w) {
    if (w->nbits) {
        if (w->bytes >= w->cap) return -1;
        w->p[w->bytes++] = (uint8_t)w->acc;
        w->acc = 0; w->nbits = 0;
    }
    return 0;
}
typedef struct { const uint8_t *p; size_t len, pos; uint32_t acc; unsigned nbits; } rawr;
static void rr_init(rawr *r, const uint8_t *p, size_t len) { r->p = p; r->len = len; r->pos = 0; r->acc = 0; r->nbits = 0; }
static int rr_get(rawr *r, unsigned bits, uint32_t *out) {
    while (r->nbits < bits) {
        if (r->pos >= r->len) return -1;
        r->acc |= (uint32_t)r->p[r->pos++] << r->nbits;
        r->nbits += 8;
    }
    *out = r->acc & ((bits < 32 ? (1u << bits) : 0u) - 1u);
    r->acc >>= bits;
    r->nbits -= bits;
    return 0;
}

/* ---------------- encode ---------------- */
size_t encode_z(uint8_t *buf, size_t cap, const poly *z1, const poly z_rest[D_REST]) {
    RHYME_NAMESPACE(encoding_init)();
    size_t tmpcap = (size_t)(1 + D_REST) * N * 4 + 256;
    uint8_t *rtmp = malloc(tmpcap);
    uint8_t *rawbuf = malloc(tmpcap);
    if (!rtmp || !rawbuf) { free(rtmp); free(rawbuf); return 0; }

    /* raw low bits in FORWARD coefficient order */
    raww rw; rw_init(&rw, rawbuf, tmpcap);
    int err = 0;
    for (int p = 0; p < 1 + D_REST && !err; p++) {
        const poly *zp = (p == 0) ? z1 : &z_rest[p - 1];
        unsigned lbits = row_L(p);
        int32_t cen = row_center(p);
        for (int i = 0; i < N && !err; i++) {
            int32_t v = zp->coeffs[i];
            int64_t vp = (int64_t)v + cen;
            if (!row_has_esc(p) || (vp >= 0 && vp <= 2 * (int64_t)cen))
                err |= rw_put(&rw, (uint32_t)vp & ((1u << lbits) - 1), lbits);
            else
                err |= rw_put(&rw, (uint32_t)(v + ESC_OFF), ESC_BITS);
        }
    }
    if (!err) err |= rw_flush(&rw);

    /* rANS symbols pushed in REVERSE order so decode pops forward */
    RansState rans;
    RansEncInit(&rans);
    uint8_t *ptr = rtmp + tmpcap;
    for (int p = D_REST; p >= 0 && !err; p--) {
        const poly *zp = (p == 0) ? z1 : &z_rest[p - 1];
        const rtable *T = row_table(p);
        unsigned lbits = row_L(p);
        int32_t cen = row_center(p);
        int esc_sym = T->nsym - 1;
        for (int i = N - 1; i >= 0; i--) {
            int32_t v = zp->coeffs[i];
            int64_t vp = (int64_t)v + cen;
            uint32_t hi;
            if (!row_has_esc(p) || (vp >= 0 && vp <= 2 * (int64_t)cen))
                hi = (uint32_t)(vp >> lbits);
            else
                hi = (uint32_t)esc_sym;
            /* PATCH (nist-sigs-zoo): no symbol for this value in the table (see header) */
            if (vp < 0 || hi >= (uint32_t)T->nsym) { err = 1; break; }
            RansEncPutSymbol(&rans, &ptr, &T->esyms[hi]);
            if (ptr < rtmp + 8) { err = 1; break; }
        }
    }
    if (!err) RansEncFlush(&rans, &ptr);
    size_t rans_len = (size_t)((rtmp + tmpcap) - ptr);
    size_t total = 2 + rans_len + rw.bytes;
    if (err || rans_len > 0xFFFF || total > cap) {
        free(rtmp); free(rawbuf);
        return 0;
    }
    buf[0] = (uint8_t)rans_len;
    buf[1] = (uint8_t)(rans_len >> 8);
    memcpy(buf + 2, ptr, rans_len);
    memcpy(buf + 2 + rans_len, rawbuf, rw.bytes);
    free(rtmp); free(rawbuf);
    return total;
}

/* ---------------- decode ---------------- */
int decode_z(poly *z1, poly z_rest[D_REST], const uint8_t *buf, size_t len) {
    RHYME_NAMESPACE(encoding_init)();
    if (len < 2 + 4) return 1;
    size_t rans_len = (size_t)buf[0] | ((size_t)buf[1] << 8);
    if (2 + rans_len > len) return 2;
    const uint8_t *rstart = buf + 2;
    const uint8_t *rend = rstart + rans_len;
    rawr rr; rr_init(&rr, rend, len - 2 - rans_len);

    RansState rans;
    uint8_t *ptr = (uint8_t *)rstart;
    if (RansDecInit(&rans, &ptr)) return 3;

    for (int p = 0; p < 1 + D_REST; p++) {
        poly *zp = (p == 0) ? z1 : &z_rest[p - 1];
        const rtable *T = row_table(p);
        unsigned lbits = row_L(p);
        int32_t cen = row_center(p);
        int esc_sym = T->nsym - 1;
        for (int i = 0; i < N; i++) {
            uint32_t slot = RansDecGet(&rans, SCALE_BITS);
            uint32_t hi = T->lut[slot];
            RansDecAdvanceSymbol(&rans, &ptr, rend, &T->dsyms[hi], SCALE_BITS);
            if (row_has_esc(p) && hi == (uint32_t)esc_sym) {
                uint32_t raw;
                if (rr_get(&rr, ESC_BITS, &raw)) return 6;
                int32_t v = (int32_t)raw - ESC_OFF;
                int64_t vp = (int64_t)v + cen;
                if (vp >= 0 && vp <= 2 * (int64_t)cen) return 7;  /* canonicity */
                zp->coeffs[i] = v;
            } else {
                uint32_t lo;
                if (rr_get(&rr, lbits, &lo)) return 8;
                int64_t vp = ((int64_t)hi << lbits) | lo;
                if (vp > 2 * (int64_t)cen) return 9;
                zp->coeffs[i] = (int32_t)(vp - cen);
            }
            if (ptr > rend) return 10;
        }
    }
    if (RansDecVerify(&rans)) return 11;
    if (ptr != rend) return 12;
    if (rr.pos != rr.len) return 13;
    if (rr.nbits && (rr.acc & ((1u << rr.nbits) - 1)) != 0) return 14;
    return 0;
}
