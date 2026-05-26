#include "ggm.h"

#include <math.h>
#include <memory.h>

#include "sdith_prng.h"
#include "vole_private.h"

// private layout (move to a ggm-private.h if needed)
struct full_ggm_tree_t {
  uint32_t lambda;
  uint32_t tau;
  uint32_t kappa;
  uint32_t lambda_bytes;
  uint32_t depth;
  uint32_t num_leaves;
  uint64_t rng_data[4];            // here: global salt (lambda bits used)
  GGM_SEED_RNG_LR_F* seed_rng_lr;  // left and right in one call
  GGM_COMMIT_RNG_F* commit_rng;
  const seed_t* root_seed;  // shared with seeds_cache
  seed_t* seeds_cache;      // 2*lambda_bytes*depth bytes
  uint32_t cached_index;    //
  uint32_t* open_buf;       // 4*tau*depth bytes
  // space for the root seed copy + cache + open buffer
};

// in debug mode, this derives a single seed
void full_ggm_tree_single_seed_rng(const full_ggm_tree* tree, seed_t* out, const salt_t* salt, const seed_t* parent,
                                   uint64_t node_idx) {
  const uint32_t lambda_bytes = tree->lambda_bytes;
  uint8_t tmp[64];
  tree->seed_rng_lr(tmp, salt, parent, node_idx & UINT64_C(-2));
  memcpy(out, tmp + (node_idx & 1) * lambda_bytes, lambda_bytes);
}

/** @brief  if x==0, return 0, else floor(log2(x)) */
uint32_t floor_log2i(uint32_t x) {
  uint32_t res = 0;
  uint32_t t = x;
  if (t >> 16) {
    res += 16;
    t >>= 16;
  }
  if (t >> 8) {
    res += 8;
    t >>= 8;
  }
  if (t >> 4) {
    res += 4;
    t >>= 4;
  }
  if (t >> 2) {
    res += 2;
    t >>= 2;
  }
  if (t >> 1) {
    res += 1;
    t >>= 1;
  }
  return res;
}

/** @brief  if x==0, return 0, else ceil(log2(x)) */
uint32_t ceil_log2i(uint32_t x) {
  uint32_t res = floor_log2i(x);
  if (x == (UINT32_C(1) << res))
    return res;
  else
    return res + 1;
}

EXPORT uint32_t estimate_topen_tmp_bytes(uint32_t tau, uint32_t kappa) {
  uint32_t depth = ceil_log2i(tau) + kappa;  // depth = ceil(log2(tau)) + kappa.
  return 4 * tau * depth;
}

/**
 * GGM tree: number of bytes of the structure
 * This number of contiguous bytes shall be allocated to hold the structure.
 * No restriction is made on the location of those bytes: heap, stack, mmap, ...
 */
EXPORT uint64_t bytes_of_full_ggm_tree(uint64_t lambda, uint64_t tau, uint64_t kappa) {
  const uint64_t lambda_bytes = lambda >> 3;
  const uint64_t depth = ceil_log2i(tau) + kappa;
  return sizeof(struct full_ggm_tree_t)     // base struct
         + 2 * lambda_bytes * (depth + 1)   // root seed + cache (2 per depth)
         + tau * depth * sizeof(uint32_t);  // tree_open buffer;
}

/**
 *  @brief GGM tree: constructor
 * initializes a GGM tree structure the first time
 */
EXPORT void full_ggm_tree_init(  //
    full_ggm_tree* tree,         //
    uint32_t lambda,
    uint32_t tau,                        // dimensions
    uint32_t kappa,                      // dimensions
    GGM_SEED_RNG_LR_F* ggm_seed_rng_lr,  // seed derivation function
    GGM_COMMIT_RNG_F* ggm_commit_rng,    // commit derivation function
    const void* salt,                    // global salt: lambda bits (used by the rng)
    const seed_t* root_seed)             // value of the root seed (will be copied)
{
  uint32_t depth = ceil_log2i(tau) + kappa;
  uint32_t lambda_bytes = lambda >> 3;
  uint8_t* seeds = ((uint8_t*)tree) + sizeof(struct full_ggm_tree_t);
  uint8_t* root_seed_addr = seeds + lambda_bytes;
  uint8_t* open_buf = seeds + 2 * lambda_bytes * (depth + 1);
  tree->lambda = lambda;
  tree->tau = tau;
  tree->kappa = kappa;
  tree->lambda_bytes = lambda_bytes;
  tree->depth = depth;
  tree->num_leaves = tau * UINT64_C(1) << kappa;
  memcpy(tree->rng_data, salt, lambda_bytes);
  tree->root_seed = root_seed_addr;
  tree->seeds_cache = seeds;
  tree->cached_index = 1;
  tree->open_buf = (uint32_t*)open_buf;
  tree->seed_rng_lr = ggm_seed_rng_lr;
  tree->commit_rng = ggm_commit_rng;
  memcpy(root_seed_addr, root_seed, lambda_bytes);
}

/** @brief  GGM tree: query a node leaf (naive stateless implementation) */
EXPORT void full_ggm_tree_get_node_seed_naive(  // naive stateless version
    const full_ggm_tree* tree,                  //
    uint32_t node_index,                        // requested node coordinates
    seed_t* node_seed)                          // output: leaf seed value
{
  CASSERT(node_index >= 1 && node_index < 2 * tree->num_leaves);
  uint64_t depth = floor_log2i(node_index);
  memcpy(node_seed, tree->root_seed, tree->lambda_bytes);
  if (depth == 0) return;
  uint8_t tmp[256];
  seed_t* cur = node_seed;
  seed_t* parent = (seed_t*)tmp;
  seed_t* t;
  for (uint64_t i = 1; i <= depth; ++i) {
    // swap parent <-> cur
    t = parent;
    parent = cur;
    cur = t;
    // derive cur from parent
    uint64_t cur_idx = node_index >> (depth - i);
    full_ggm_tree_single_seed_rng(tree, cur, tree->rng_data, parent, cur_idx);
  }
  // ensure the final answer is in node_seed
  if (cur != node_seed) {
    memcpy(node_seed, tmp, tree->lambda_bytes);
  }
}

/** @brief  GGM tree: query a node leaf (seed cache implementation) */
EXPORT void full_ggm_tree_get_node_seed(  // naive stateless version
    full_ggm_tree* tree,                  //
    uint32_t node_index,                  // requested node coordinates
    seed_t* node_seed)                    // output: leaf seed value
{
  CASSERT(node_index >= 1 && node_index < 2 * tree->num_leaves);
  // largest depth s.t.
  // (ni >> (nd - common_depth)) ^ (ci >> (cd - common_depth)) = 0 or 1
  uint32_t node_depth = floor_log2i(node_index);
  uint32_t cache_depth = floor_log2i(tree->cached_index);
  uint32_t common_depth;
  if (node_depth <= cache_depth) {
    uint32_t xor = node_index ^ (tree->cached_index >> (cache_depth - node_depth));
    common_depth = node_depth - floor_log2i(xor);
  } else {
    uint32_t xor = tree->cached_index ^ (node_index >> (node_depth - cache_depth));
    common_depth = cache_depth - floor_log2i(xor);
  }

  // update the cache until the requested index is here
  for (uint64_t i = common_depth; i < node_depth; ++i) {
    uint64_t p_idx = node_index >> (node_depth - i);
    seed_t* parent = tree->seeds_cache + (2 * i + (p_idx & 1)) * tree->lambda_bytes;
    seed_t* left = tree->seeds_cache + (2 * (i + 1)) * tree->lambda_bytes;
    tree->seed_rng_lr(left, tree->rng_data, parent, 2 * p_idx);
  }
  tree->cached_index = node_index;
  // copy the final result
  memcpy(node_seed,                                                                       //
         tree->seeds_cache + ((2 * node_depth) + (node_index & 1)) * tree->lambda_bytes,  //
         tree->lambda_bytes);
}

/**
 * @brief GGM tree: query a node leaf (naive implementation)
 */
EXPORT void full_ggm_tree_get_leaf_seed_commit_naive(  //
    full_ggm_tree* tree,                               //
    uint32_t leaf_index,                               // requested leaf coordinates
    seed_t* leaf_seed,                                 // output: leaf seed value
    commit_t* leaf_commit)                             // output: leaf commitment value
{
  CASSERT(leaf_index < tree->num_leaves);
  uint32_t node_index = leaf_index + tree->num_leaves;
  full_ggm_tree_get_node_seed_naive(tree, node_index, leaf_seed);
  tree->commit_rng(leaf_commit, tree->rng_data, leaf_seed, node_index);
}

/**
 * @brief GGM tree: query a node leaf (cached implementation)
 *  optimized version, ensures that the node seeds are cached up to node_depth-1
 *  This way, querying all leaf seeds one by one will reuse cached node seeds.
 */
EXPORT void full_ggm_tree_get_leaf_seed_commit(  //
    full_ggm_tree* tree,                         //
    uint32_t leaf_index,                         // requested leaf coordinates
    seed_t* leaf_seed,                           // output: leaf seed value
    commit_t* leaf_commit)                       // output: leaf commitment value
{
  CASSERT(leaf_index < tree->num_leaves);
  uint32_t node_index = leaf_index + tree->num_leaves;
  full_ggm_tree_get_node_seed(tree, node_index, leaf_seed);
  tree->commit_rng(leaf_commit, tree->rng_data, leaf_seed, node_index);
}

/** decode "delta" into the indexes of the hidden leaves */
//  hidden_nodes = [delta[i.kappa, (i+1).kappa) * tau + i  for i in range(tau)]
//  qsort hidden_leaves
// uint64_t hidden_leaves_indexes(uint64_t kappa, uint64_t tau,  // dims
//                                uint32_t* hidden_leaves,       // output: node indexes
//                                const bitvec_t* delta);        // input: bits of delta

// swap two elements - used in partition
void swap(uint32_t* a, uint32_t* b) {
  uint32_t t = *a;
  *a = *b;
  *b = t;
}

// partioning - used in quick sort
int partition(uint32_t arr[], int low, int high) {
  uint32_t pivot = arr[high];
  int hi = high - 1;
  int lo = low;
  while (1) {
    while (lo <= hi && arr[lo] <= pivot) ++lo;
    while (lo <= hi && arr[hi] >= pivot) --hi;
    if (lo > hi) break;
    swap(arr + lo, arr + hi);
    ++lo;
    --hi;
  }
  swap(arr + lo, arr + high);
  return lo;
}

// QuickSort - used in hidden_leaves_indexes
void quickSort(uint32_t arr[], int low, int high) {
  if (low < high) {
    int pi = partition(arr, low, high);

    quickSort(arr, low, pi - 1);
    quickSort(arr, pi + 1, high);
  }
}

__always_inline uint32_t extract_kappabit_uint_inline(uint64_t kappa, uint64_t bitpos, const uint64_t* data) {
  const uint64_t kappa_mask = (UINT64_C(1) << kappa) - 1;
  uint64_t limb_pos = bitpos >> 6;
  uint64_t limb_rem = bitpos & UINT64_C(63);
  if (limb_rem + kappa <= 64) {
    return (data[limb_pos] >> limb_rem) & kappa_mask;
  } else {
    return ((data[limb_pos] >> limb_rem) | (data[limb_pos + 1] << (64 - limb_rem))) & kappa_mask;
  }
}
__always_inline void xorto_kappabit_uint_inline(uint64_t kappa, uint64_t bitpos, uint64_t* data, uint64_t value) {
  const uint64_t kappa_mask = (UINT64_C(1) << kappa) - 1;
  uint64_t limb_pos = bitpos >> 6;
  uint64_t limb_rem = bitpos & UINT64_C(63);
  value &= kappa_mask;
  if (limb_rem + kappa <= 64) {
    data[limb_pos] ^= value << limb_rem;
  } else {
    data[limb_pos] ^= value << limb_rem;
    data[limb_pos + 1] ^= value >> (64 - limb_rem);
  }
}

EXPORT uint32_t extract_kappabit_uint(uint64_t kappa, uint64_t bitpos, const void* data) {
  return extract_kappabit_uint_inline(kappa, bitpos, (uint64_t*)data);
}
EXPORT void xorto_kappabit_uint(uint64_t kappa, uint64_t bitpos, const void* data, uint64_t value) {
  xorto_kappabit_uint_inline(kappa, bitpos, (uint64_t*)data, value);
}

EXPORT void hidden_leaves_indexes(uint64_t kappa, uint64_t tau, uint32_t* hidden_leaves, const bitvec_t* delta) {
  for (uint64_t k = 0; k < tau; ++k) {
    hidden_leaves[k] = extract_kappabit_uint_inline(kappa, k * kappa, delta) * tau + k;
  }
  quickSort(hidden_leaves, 0, tau - 1);
}
EXPORT void hidden_leaves_indexes2(uint64_t kappa, uint64_t tau, uint32_t* hidden_leaves, const bitvec_t* delta) {
  const uint64_t start_index = tau * (UINT64_C(1) << kappa);
  for (uint64_t k = 0; k < tau; ++k) {
    hidden_leaves[k] = extract_kappabit_uint_inline(kappa, k * kappa, delta) * tau + k + start_index;
  }
  quickSort(hidden_leaves, 0, tau - 1);
}

/** estimate the number of nodes to open for this choice of delta: */
// algorithm:
//  sibbling_path_indexes = []
//  new nodes_list = []
//  for (depth = tau*kappa-1 down to 0)
//    while hidden_leave is not empty {
//       if (hidden_leaves have >= 2 elements and hidden_leave[0] ^ hidden_leave[1] == 1) {
//          append hidden_leave[0] >> 1 to new nodes_list
//          pop the two first nodes of hidden_nodes
//       } else {
//          append hidden_leave[0] ^ 1 to sibbling_path_indexes
//          append hidden_leave[0] >> 1 to new nodes_list
//          pop the first nodes of hidden_nodes
//       }
//    }
// }
uint64_t estimate_topen(const uint32_t tau, const uint32_t kappa, uint64_t max_topen, const uint32_t* hidden_leaves_idx,
                        uint8_t* tmp_space) {
  // CASSERT(max_topen < tree->tau * tree->kappa, "topen bug");
  uint32_t sibling_path_size = 0;
  uint32_t* bstart = (uint32_t*)tmp_space;
  uint32_t* bend = bstart + tau;
#ifndef NDEBUG
  uint32_t num_leaves = tau * UINT64_C(1) << kappa;
  uint32_t last_leave_index = num_leaves - 1;
  for (uint32_t i = 0; i < tau; ++i) {
    CREQUIRE(hidden_leaves_idx[i] > last_leave_index, "incorrect leave node index");
    last_leave_index = hidden_leaves_idx[i];
  }
#endif
  // copy the leaf indexes in reversed order
  for (uint32_t i = 0; i < tau; ++i) {
    bstart[tau - i - 1] = hidden_leaves_idx[i];
  }
  while (bend - bstart >= 2) {
    uint32_t first_idx = *(bstart++);
    uint32_t second_idx = *(bstart);
    uint32_t next_idx = first_idx >> 1;
    CASSERT(first_idx > 1, "bug1");
    CASSERT(*(bend - 1) > next_idx, "bug2");
    if ((first_idx ^ second_idx) == 1) {
      // two hidden nodes with the same parent
      ++bstart;
    } else {
      // single child out of at least 2
      ++sibling_path_size;
    }
    *(bend++) = next_idx;
  }
  while (*bstart != 1) {
    ++sibling_path_size;
    *bstart >>= 1;
  }
  return sibling_path_size;
}

void full_ggm_tree_open_sibling_path(   //
    full_ggm_tree* tree,                // tree
    seed_t* sibling_seeds,              // sibling seeds (in the same order as estimate_topen)
    commit_t* hidden_leaves_commits,    // sibling seeds (in leaf order)
    const uint32_t* hidden_leaves_idx)  // index of hidden leaves (node index)
{
  uint8_t buf[256];
  uint32_t sibling_path_size = 0;
  uint32_t* bstart = tree->open_buf;
  uint32_t* bend = bstart + tree->tau;
#ifndef NDEBUG
  uint32_t last_leave_index = tree->num_leaves - 1;
  for (uint32_t i = 0; i < tree->tau; ++i) {
    CREQUIRE(hidden_leaves_idx[i] > last_leave_index, "incorrect leave node index");
    last_leave_index = hidden_leaves_idx[i];
  }
#endif
  // copy the leaf indexes in reversed order
  for (uint32_t i = 0; i < tree->tau; ++i) {
    bstart[tree->tau - i - 1] = hidden_leaves_idx[i];
    full_ggm_tree_get_leaf_seed_commit(tree,
                                       hidden_leaves_idx[i] - tree->num_leaves,  // leaf index
                                       buf,                                      // ignore the seed
                                       ((uint8_t*)hidden_leaves_commits) + i * 2 * tree->lambda_bytes);  // commitment
  }
  while (bend - bstart >= 2) {
    uint32_t first_idx = *(bstart++);
    uint32_t second_idx = *(bstart);
    uint32_t next_idx = first_idx >> 1;
    CASSERT(first_idx > 1, "bug1");
    CASSERT(*(bend - 1) > next_idx, "bug2");
    if ((first_idx ^ second_idx) == 1) {
      // two hidden nodes with the same parent
      ++bstart;
    } else {
      // single child out of at least 2
      full_ggm_tree_get_node_seed(                                              //
          tree, first_idx ^ 1,                                                  //
          ((uint8_t*)sibling_seeds) + sibling_path_size * tree->lambda_bytes);  //
      ++sibling_path_size;
    }
    *(bend++) = next_idx;
  }
  while (*bstart != 1) {
    full_ggm_tree_get_node_seed(                                              //
        tree, (*bstart) ^ 1,                                                  //
        ((uint8_t*)sibling_seeds) + sibling_path_size * tree->lambda_bytes);  //
    ++sibling_path_size;
    *bstart >>= 1;
  }
}

struct ggm_multi_item_t {
  uint16_t type : 2;
  uint16_t addr : 14;
};

// private layout
struct ggm_multi_sibling_tree_t {
  uint32_t lambda;
  uint32_t tau;
  uint32_t kappa;
  uint32_t num_leaves;
  uint32_t lambda_bytes;
  uint64_t rng_data[4];
  GGM_SEED_RNG_LR_F* seed_rng_lr;
  GGM_COMMIT_RNG_F* commit_rng;
  const seed_t* sibling_seeds;           // weak pointer
  const commit_t* hidde_leaves_commit;   // weak pointer
  struct ggm_multi_item_t* tree_layout;  // layout of the entire tree (2*2*tau.2^kappa)
  uint32_t* init_buf;                    // 4*tau*depth (shared with seed_cache)
  seed_t* seeds_cache;                   // 2*lambda_b*depth bytes
  uint32_t cached_index;                 // index of the last seed cached
};

/**
 * GGM multi-sibling tree: number of bytes of the structure
 * This number of contiguous bytes shall be allocated to hold the structure.
 * No restriction is made on the location of those bytes: heap, stack, mmap, ...
 */
EXPORT uint64_t bytes_of_ggm_multi_sibling_tree(uint64_t lambda, uint64_t tau, uint64_t kappa) {
  uint32_t depth = kappa + ceil_log2i(tau);
  uint32_t lambda_bytes = lambda >> 3;
  uint32_t num_leaves = tau * (UINT64_C(1) << kappa);
  uint32_t init_buf_size = sizeof(uint32_t) * tau * depth;
  uint32_t seeds_cache_size = 2 * lambda_bytes * (depth + 1);
  uint32_t seeds_or_init_size = seeds_cache_size < init_buf_size ? init_buf_size : seeds_cache_size;
  return sizeof(struct ggm_multi_sibling_tree_t)             //
         + 2 * num_leaves * sizeof(struct ggm_multi_item_t)  // tree layout
         + seeds_or_init_size;                               // init_buf & seeds cache
}

/**
 *  @brief GGM sibling tree: constructor
 * initializes a GGM sibling tree structure the first time
 */
EXPORT void ggm_multi_sibling_tree_init(            //
    ggm_multi_sibling_tree* tree,                   //
    uint32_t lambda, uint32_t tau, uint32_t kappa,  // num of leaves: tau.2^kappa
    GGM_SEED_RNG_LR_F* ggm_seed_rng_lr,             // seed derivation function
    GGM_COMMIT_RNG_F* ggm_commit_rng,               // commit derivation function
    const void* global_salt,
    const uint32_t* hidden_leaves_idx,      // seed_size and depth
    const seed_t* sibling_seeds,            // tweak (used by the rng)
    const commit_t* hidden_leaves_commits)  // sibling path seeds (need to remain available)
{
  uint64_t num_leaves = tau * (UINT64_C(1) << kappa);
  uint8_t* tree_struct = ((uint8_t*)tree) + sizeof(struct ggm_multi_sibling_tree_t);
  uint8_t* cache = tree_struct + 2 * num_leaves * sizeof(struct ggm_multi_item_t);
  tree->lambda = lambda;
  tree->tau = tau;
  tree->kappa = kappa;
  tree->sibling_seeds = sibling_seeds;
  tree->hidde_leaves_commit = hidden_leaves_commits;
  tree->num_leaves = num_leaves;
  tree->lambda_bytes = lambda >> 3;
  tree->cached_index = 0;
  memcpy(tree->rng_data, global_salt, tree->lambda_bytes);
  tree->seed_rng_lr = ggm_seed_rng_lr;
  tree->commit_rng = ggm_commit_rng;
  tree->tree_layout = (struct ggm_multi_item_t*)tree_struct;
  tree->seeds_cache = cache;
  tree->init_buf = (uint32_t*)cache;
  memset(tree->tree_layout, 0, 2 * num_leaves * sizeof(struct ggm_multi_item_t));
  // populate the layout
  uint32_t sibling_path_size = 0;
  uint32_t* bstart = tree->init_buf;
  uint32_t* bend = bstart + tree->tau;
#ifndef NDEBUG
  uint32_t last_leave_index = tree->num_leaves - 1;
  for (uint32_t i = 0; i < tree->tau; ++i) {
    CREQUIRE(hidden_leaves_idx[i] > last_leave_index, "incorrect leave node index");
    last_leave_index = hidden_leaves_idx[i];
  }
#endif
  // copy the leaf indexes in reversed order
  for (uint32_t i = 0; i < tree->tau; ++i) {
    bstart[tree->tau - i - 1] = hidden_leaves_idx[i];
    tree->tree_layout[hidden_leaves_idx[i]].type = HIDDEN_LEAF;
    tree->tree_layout[hidden_leaves_idx[i]].addr = i;
  }
  while (bend - bstart >= 2) {
    uint32_t first_idx = *(bstart++);
    uint32_t second_idx = *(bstart);
    uint32_t next_idx = first_idx >> 1;
    CASSERT(first_idx > 1, "bug1");
    CASSERT(*(bend - 1) > next_idx, "bug2");
    if ((first_idx ^ second_idx) == 1) {
      // two hidden nodes with the same parent
      ++bstart;
    } else {
      // single child out of at least 2
      tree->tree_layout[first_idx ^ 1].type = SIBLING_ROOT;
      tree->tree_layout[first_idx ^ 1].addr = sibling_path_size;
      ++sibling_path_size;
    }
    tree->tree_layout[next_idx].type = HIDDEN_NODE;
    *(bend++) = next_idx;
  }
  while (*bstart != 1) {
    tree->tree_layout[(*bstart) ^ 1].type = SIBLING_ROOT;
    tree->tree_layout[(*bstart) ^ 1].addr = sibling_path_size;
    ++sibling_path_size;
    *bstart >>= 1;
    tree->tree_layout[(*bstart)].type = HIDDEN_NODE;
  }
}

// in debug mode, this derives a single seed
void ggm_multi_sibling_tree_single_seed_rng(const ggm_multi_sibling_tree* stree, seed_t* out, const salt_t* salt,
                                            const seed_t* parent, uint64_t node_idx) {
  const uint32_t lambda_bytes = stree->lambda_bytes;
  uint8_t tmp[64];
  stree->seed_rng_lr(tmp, salt, parent, node_idx & UINT64_C(-2));
  memcpy(out, tmp + (node_idx & 1) * lambda_bytes, lambda_bytes);
}

/**
 * @brief  GGM sibling tree: query a node leaf (naive stateless implementation)
 * This function may abort if queried on the hidden leaf index.
 */
EXPORT void ggm_multi_sibling_tree_get_node_data_naive(  // naive stateless version
    const ggm_multi_sibling_tree* tree,                  //
    uint32_t node_index,                                 // requested node index
    enum node_type_t* out_type,                          // node type
    seed_t* out_seed,                                    // node seed (must be lambda bits wide)
    const commit_t** out_commit)                         // node commit (ptr will be set)
{
  const uint32_t lambda_bytes = tree->lambda_bytes;
  uint32_t idx = node_index;
  struct ggm_multi_item_t node = tree->tree_layout[idx];
  if (node.type == HIDDEN_NODE) {
    *out_type = HIDDEN_NODE;
    return;
  } else if (node.type == HIDDEN_LEAF) {
    *out_type = HIDDEN_LEAF;
    *out_commit = ((uint8_t*)tree->hidde_leaves_commit) + 2 * node.addr * lambda_bytes;
    return;
  }
  uint32_t d = 0;
  while (tree->tree_layout[idx].type == NORMAL_NODE) {
    idx >>= 1;
    ++d;
  }
  CASSERT(tree->tree_layout[idx].type == SIBLING_ROOT);
  memcpy(out_seed, ((uint8_t*)tree->sibling_seeds) + lambda_bytes * tree->tree_layout[idx].addr, lambda_bytes);
  for (uint32_t i = 0; i < d; ++i) {
    ggm_multi_sibling_tree_single_seed_rng(tree, out_seed, tree->rng_data, out_seed, (node_index >> (d - i - 1)));
  }
  *out_type = NORMAL_NODE;
}

/**
 * @brief GGM  sibling tree: query a leaf (naive)
 */
EXPORT void ggm_multi_sibling_tree_get_leaf_seed_commit_naive(  //
    const ggm_multi_sibling_tree* tree,                         //
    uint32_t leaf_index,                                        // requested leaf coordinates
    const seed_t** out_leaf_seed,                               // output: leaf seed value
    const commit_t** out_leaf_commit,                           // output: leaf commitment value
    void* out_buf)                                              // output buffer size (3.lambda)
{
  CASSERT(leaf_index < tree->num_leaves, "invalid leaf index: %d", leaf_index);
  enum node_type_t node_type;
  const commit_t* commit;
  const uint64_t node_index = leaf_index + tree->num_leaves;
  ggm_multi_sibling_tree_get_node_data_naive(  //
      tree, node_index, &node_type, out_buf, &commit);
  switch (node_type) {
    case HIDDEN_LEAF:
      *out_leaf_commit = commit;
      *out_leaf_seed = NULL;
      break;
    case NORMAL_NODE:
      *out_leaf_seed = out_buf;
      *out_leaf_commit = ((uint8_t*)out_buf) + tree->lambda_bytes;
      tree->commit_rng((commit_t*)*out_leaf_commit, tree->rng_data, *out_leaf_seed, node_index);
      break;
    default:
      CREQUIRE(0, "bad node type: invalid index?");
  }
}

/**
 * @brief GGM tree: query a node leaf (cached implementation)
 */
EXPORT void ggm_multi_sibling_tree_get_leaf_seed_commit(  //
    ggm_multi_sibling_tree* tree,                         //
    uint32_t leaf_index,                                  //
    const seed_t** out_leaf_seed,                         //
    const commit_t** out_leaf_commit,                     //
    void* out_buf) {
  CASSERT(leaf_index < tree->num_leaves, "invalid leaf index: %d", leaf_index);
  enum node_type_t node_type;
  const commit_t* commit;
  const uint64_t node_index = leaf_index + tree->num_leaves;
  ggm_multi_sibling_tree_get_node_data(  //
      tree, node_index, &node_type, out_buf, &commit);
  switch (node_type) {
    case HIDDEN_LEAF:
      *out_leaf_commit = commit;
      *out_leaf_seed = NULL;
      break;
    case NORMAL_NODE:
      *out_leaf_seed = out_buf;
      *out_leaf_commit = ((uint8_t*)out_buf) + tree->lambda_bytes;
      tree->commit_rng((commit_t*)*out_leaf_commit, tree->rng_data, *out_leaf_seed, node_index);
      break;
    default:
      CREQUIRE(0, "bad node type: invalid index?");
  }
}

uint32_t largest_prefix_length_sib(uint32_t query_idx, ggm_multi_sibling_tree* tree, uint64_t depth) {
  uint32_t xor = ceil_log2i(tree->cached_index ^ query_idx);

  uint32_t prefix_len = depth - xor;
  if (xor> depth) prefix_len = 0;
  if (tree->cached_index == 1) prefix_len = 0;
  if (xor== 0) prefix_len -= 1;

  return prefix_len;
}

/**
 * @brief  GGM sibling tree: query a node leaf (cached seed implementation)
 * This function may abort if queried on the hidden leaf index.
 */
EXPORT void ggm_multi_sibling_tree_get_node_data(ggm_multi_sibling_tree* tree,
                                                 uint32_t node_index,          // requested node index
                                                 enum node_type_t* out_type,   // node type
                                                 seed_t* out_seed,             // node seed (must be lambda bits wide)
                                                 const commit_t** out_commit)  // node commit (ptr will be set)
{
  const uint32_t lambda_bytes = tree->lambda_bytes;
  CASSERT(node_index >= 1 && node_index < 2 * tree->num_leaves);
  // largest depth s.t.
  // (ni >> (nd - common_depth)) ^ (ci >> (cd - common_depth)) = 0 or 1
  uint32_t node_depth = floor_log2i(node_index);
  uint32_t cache_depth = floor_log2i(tree->cached_index);
  uint32_t common_depth;
  if (node_depth <= cache_depth) {
    uint32_t xor = node_index ^ (tree->cached_index >> (cache_depth - node_depth));
    common_depth = node_depth - floor_log2i(xor);
  } else {
    uint32_t xor = tree->cached_index ^ (node_index >> (node_depth - cache_depth));
    common_depth = cache_depth - floor_log2i(xor);
  }
  // update the cache until the requested index is here
  uint8_t* const scache = (uint8_t*)tree->seeds_cache;
  for (uint64_t i = common_depth; i < node_depth; ++i) {
    uint64_t p_idx = node_index >> (node_depth - i);
    struct ggm_multi_item_t p = tree->tree_layout[p_idx];
    seed_t* cached_parent = scache + (2 * i + (p_idx & 1)) * lambda_bytes;
    seed_t* cached_left = scache + (2 * (i + 1)) * lambda_bytes;
    switch (p.type) {
      case HIDDEN_NODE:
        break;  // nothing to derive
      case SIBLING_ROOT:
        memcpy(                                                       //
            cached_parent,                                            // cache location
            ((uint8_t*)tree->sibling_seeds) + lambda_bytes * p.addr,  // sibling seed
            lambda_bytes);
        // and then, we continue as normal
      case NORMAL_NODE:
        tree->seed_rng_lr(cached_left, tree->rng_data, cached_parent, 2 * p_idx);
        break;
      default:
        CREQUIRE(0, "bug: parent index is a leaf?");
    }
  }
  tree->cached_index = node_index;
  // copy the final result
  struct ggm_multi_item_t node = tree->tree_layout[node_index];
  switch (node.type) {
    case HIDDEN_NODE:
      *out_type = HIDDEN_NODE;
      break;
    case HIDDEN_LEAF:
      *out_type = HIDDEN_LEAF;
      *out_commit = ((uint8_t*)tree->hidde_leaves_commit) + 2 * node.addr * lambda_bytes;
      break;
    case SIBLING_ROOT:
      memcpy(out_seed, ((uint8_t*)tree->sibling_seeds) + lambda_bytes * node.addr, lambda_bytes);
      *out_type = NORMAL_NODE;
      break;
    case NORMAL_NODE:
      *out_type = NORMAL_NODE;
      memcpy(out_seed,                                                       //
             scache + ((2 * node_depth) + (node_index & 1)) * lambda_bytes,  //
             lambda_bytes);
      break;
    default:
      CREQUIRE(0, "bug: bad node type: invalid index?");
  }
}
