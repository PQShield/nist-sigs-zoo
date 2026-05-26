#ifndef AQ_VOLE_IMPL_GGM_H
#define AQ_VOLE_IMPL_GGM_H

// This file contains the declaration of all algebra functions over vectors and matrices over F2

#include "commons.h"

typedef struct full_ggm_tree_t full_ggm_tree;
typedef void GGM_SEED_RNG_LR_F(void* lr_out, const void* salt, const void* key, uint64_t node_idx);
typedef void GGM_COMMIT_RNG_F(void* out, const void* salt, const void* key, uint64_t node_idx);

/**
 * GGM tree: number of bytes of the structure
 * This number of contiguous bytes shall be allocated to hold the structure.
 * No restriction is made on the location of those bytes: heap, stack, mmap, ...
 */
EXPORT uint64_t bytes_of_full_ggm_tree(uint64_t lambda, uint64_t tau, uint64_t kappa);

/**
 *  @brief GGM tree: constructor
 * initializes a GGM tree structure the first time
 */
EXPORT void full_ggm_tree_init(  //
    full_ggm_tree* tree,         //
    uint32_t lambda,
    uint32_t tau,              // dimensions
    uint32_t kappa,            // dimensions
    GGM_SEED_RNG_LR_F* ggm_seed_rng_lr,  // seed derivation function
    GGM_COMMIT_RNG_F* ggm_commit_rng,    // commit derivation function
    const salt_t* salt,        // tweak (used by the rng)
    const seed_t* root_seed);  // value of the root seed (will be copied)

/** @brief  GGM tree: query a node leaf (naive stateless implementation) */
EXPORT void full_ggm_tree_get_node_seed_naive(  // naive stateless version
    const full_ggm_tree* tree,                  //
    uint32_t node_index,                        // requested node coordinates
    seed_t* node_seed);                         // output: leaf seed value

/** @brief  GGM tree: query a node leaf (seed cache implementation) */
EXPORT void full_ggm_tree_get_node_seed(full_ggm_tree* tree,  //
                                        uint32_t node_index,  // requested node coordinates
                                        seed_t* node_seed);   // output: leaf seed value

/**
 * @brief GGM tree: query a node leaf (naive)
 *  optimized version, ensures that the node seeds are cached up to node_depth-1
 *  This way, querying all leaf seeds one by one will reuse cached node seeds.
 */
EXPORT void full_ggm_tree_get_leaf_seed_commit_naive(  //
    full_ggm_tree* tree,                               //
    uint32_t leaf_index,                               // requested leaf coordinates
    seed_t* leaf_seed,                                 // output: leaf seed value
    commit_t* leaf_commit);                            // output: leaf commitment value

/**
 * @brief GGM tree: query a node leaf (cached implementation)
 *  optimized version, ensures that the node seeds are cached up to node_depth-1
 *  This way, querying all leaf seeds one by one will reuse cached node seeds.
 */
EXPORT void full_ggm_tree_get_leaf_seed_commit(  //
    full_ggm_tree* tree,                         //
    uint32_t leaf_index,                         // requested leaf coordinates
    seed_t* leaf_seed,                           // output: leaf seed value
    commit_t* leaf_commit);                      // output: leaf commitment value

/** decode "delta" into the indexes of the hidden leaves */
//  hidden_nodes = [delta[i.kappa, (i+1).kappa) * tau + i  for i in range(tau)]
//  qsort hidden_leaves
EXPORT void hidden_leaves_indexes(uint64_t kappa, uint64_t tau,  // dims
                                  uint32_t* hidden_leaves,       // output: node indexes
                                  const bitvec_t* delta);        // input: bits of delta
/** decode "delta" into the indexes of the hidden leaves:
 * the v2 version returns node indexes instead of leave indexes */
EXPORT void hidden_leaves_indexes2(uint64_t kappa, uint64_t tau,  // dims
                                   uint32_t* hidden_leaves,       // output: node indexes
                                   const bitvec_t* delta);        // input: bits of delta

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
EXPORT uint32_t estimate_topen_tmp_bytes(
    uint32_t tau, 
    uint32_t kappa
    );

EXPORT uint64_t estimate_topen(  //
    const uint32_t tau, 
    const uint32_t kappa, 
    uint64_t max_topen,                        // TODO ignored for now
    const uint32_t* hidden_leaves_idx,
    uint8_t* tmp_space
    );        //

EXPORT void full_ggm_tree_open_sibling_path(  //
    full_ggm_tree* tree,                      // tree
    seed_t* sibling_seeds,                    // sibling seeds (in the same order as estimate_topen)
    commit_t* hidden_leaves_commits,          // sibling seeds (in leaf order)
    const uint32_t* hidden_leaves_idx);       // index of hidden leaves (node index)

typedef enum node_type_t { NORMAL_NODE = 0, HIDDEN_NODE = 1, SIBLING_ROOT = 2, HIDDEN_LEAF = 3 } node_type_t;

/** GGM Multi-Sibling tree:
 * this structure is opaque, and non memcpy-able.
 * The byte size of the structure is provided by the bytes_of_ggm_sibling_tree function
 * ggm_sibling_tree_init must be called to set-up the structure the first time
 */
typedef struct ggm_multi_sibling_tree_t ggm_multi_sibling_tree;

/**
 * GGM multi-sibling tree: number of bytes of the structure
 * This number of contiguous bytes shall be allocated to hold the structure.
 * No restriction is made on the location of those bytes: heap, stack, mmap, ...
 */
EXPORT uint64_t bytes_of_ggm_multi_sibling_tree(uint64_t lambda, uint64_t tau, uint64_t kappa);

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
    const uint32_t* hidden_leaves,           // seed_size and depth
    const seed_t* sibling_seeds,             // tweak (used by the rng)
    const commit_t* hidden_leaves_commits);  // sibling path seeds (need to remain available)

/**
 * @brief  GGM sibling tree: query a node leaf (naive stateless implementation)
 */
EXPORT void ggm_multi_sibling_tree_get_node_data_naive(  // naive stateless version
    const ggm_multi_sibling_tree* tree,                  //
    uint32_t node_index,                                 // requested node index
    enum node_type_t* out_type,                          // node type
    seed_t* out_seed,                                    // node seed (must be lambda bits wide)
    const commit_t** out_commit);                        // node commit (ptr will be set)

/**
 * @brief GGM sibling tree: query a node leaf (cached implementation)
 * This function may abort if queried on the hidden leaf index.
 *  optimized version, ensures that the node seeds are cached up to node_depth-1
 *  This way, querying all leaf seeds one by one will reuse cached node seeds.
 */
EXPORT void ggm_multi_sibling_tree_get_node_data(  // naive stateless version
    ggm_multi_sibling_tree* tree,                  //
    uint32_t node_index,                           // requested node index
    enum node_type_t* out_type,                    // node type
    seed_t* out_seed,                              // node seed (must be lambda bits wide)
    const commit_t** out_commit);                  // node commit (ptr will be set)

/**
 * @brief GGM  sibling tree: query a leaf (naive)
 * Note: the output leaf seed and commit will be pointed by out_leaf_seed and commit.
 * They may live either in the tree's cache, or on the provided output buffer (i.e. not thread safe)
 */
EXPORT void ggm_multi_sibling_tree_get_leaf_seed_commit_naive(  //
    const ggm_multi_sibling_tree* tree,                         //
    uint32_t leaf_index,                                        // requested leaf coordinates
    const seed_t** out_leaf_seed,                               // output: leaf seed value (if not hidden)
    const commit_t** out_leaf_commit,                           // output: leaf commitment value
    void* out_buf);                                             // output buffer size (3.lambda)

/**
 * @brief GGM tree: query a node leaf (cached implementation)
 * Note: the output leaf seed and commit will be pointed by out_leaf_seed and commit.
 * They may live either in the tree's cache, or on the provided output buffer (i.e. not thread safe)
 */
EXPORT void ggm_multi_sibling_tree_get_leaf_seed_commit(  //
    ggm_multi_sibling_tree* tree,                         //
    uint32_t leaf_index,                                  // requested leaf coordinates
    const seed_t** out_leaf_seed,                         // output: leaf seed value (if not hidden)
    const commit_t** out_leaf_commit,                     // output: leaf commitment value
    void* out_buf);                                       // output buffer size (3.lambda)

#endif  // AQ_VOLE_IMPL_GGM_H
