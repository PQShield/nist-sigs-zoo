#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "loader.h"

typedef const bench_scheme_info_t *(*info_fn_t)(void);

bench_scheme_t *bench_load(const char *so_path) {
    void *h = dlopen(so_path, RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        fprintf(stderr, "bench_load: %s: %s\n", so_path, dlerror());
        return NULL;
    }

    info_fn_t info_fn = (info_fn_t)(uintptr_t)dlsym(h, "bench_info");
    if (!info_fn) {
        fprintf(stderr, "bench_load: %s: bench_info not found\n", so_path);
        dlclose(h);
        return NULL;
    }

    bench_scheme_t *s = calloc(1, sizeof(*s));
    if (!s) { dlclose(h); return NULL; }

    const bench_scheme_info_t *info = info_fn();
    s->name      = info->name;
    s->pk_bytes  = info->pk_bytes;
    s->sk_bytes  = info->sk_bytes;
    s->sig_bytes = info->sig_bytes;
    s->iters     = info->iters;
    s->dl_handle = h;

    s->keygen_fn = (int (*)(uint8_t *, uint8_t *))
                   (uintptr_t)dlsym(h, "crypto_sign_keypair");
    s->sign_fn   = (int (*)(uint8_t *, size_t *, const uint8_t *, size_t,
                             const uint8_t *))
                   (uintptr_t)dlsym(h, "crypto_sign_signature");
    s->verify_fn = (int (*)(const uint8_t *, size_t, const uint8_t *, size_t,
                             const uint8_t *))
                   (uintptr_t)dlsym(h, "crypto_sign_verify");

    if (!s->keygen_fn || !s->sign_fn || !s->verify_fn) {
        fprintf(stderr, "bench_load: %s: missing crypto_sign_* symbol(s)\n",
                so_path);
        free(s);
        dlclose(h);
        return NULL;
    }
    return s;
}

void bench_unload(bench_scheme_t *s) {
    if (!s) return;
    dlclose(s->dl_handle);
    free(s);
}
