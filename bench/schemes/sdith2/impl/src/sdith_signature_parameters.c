#include "sdith_signature.h"

const signature_parameters CAT1_SHORT_PARAMETERS = {
    //
    .lambda = 128,               //
    .kappa = 11,                 //
    .tau = 11,                   //
    .target_topen = 107,         //
    .proofow_w = 9,              //
    .rsd_w = 56,                 //
    .rsd_n = 10360,              //
    .mux_depth = 4,              //
    .mux_arities = {4, 4, 4, 3}  //
};

const signature_parameters CAT3_SHORT_PARAMETERS = {
    //
    .lambda = 192,               //
    .kappa = 12,                 //
    .tau = 16,                   //
    .target_topen = 157,         //
    .proofow_w = 2,              //
    .rsd_w = 73,                 //
    .rsd_n = 18396,              //
    .mux_depth = 4,              //
    .mux_arities = {4, 4, 4, 4}  //
};

const signature_parameters CAT5_SHORT_PARAMETERS = {
    //
    .lambda = 256,               //
    .kappa = 12,                 //
    .tau = 21,                   //
    .target_topen = 216,         //
    .proofow_w = 6,              //
    .rsd_w = 104,                //
    .rsd_n = 19864,              //
    .mux_depth = 4,              //
    .mux_arities = {4, 4, 4, 3}  //
};

const signature_parameters CAT1_FAST_PARAMETERS = {
    //
    .lambda = 128,               //
    .kappa = 8,                  //
    .tau = 16,                   //
    .target_topen = 101,         //
    .proofow_w = 2,              //
    .rsd_w = 56,                 //
    .rsd_n = 10360,              //
    .mux_depth = 4,              //
    .mux_arities = {4, 4, 4, 3}  //
};

const signature_parameters CAT3_FAST_PARAMETERS = {
    //
    .lambda = 192,               //
    .kappa = 8,                  //
    .tau = 24,                   //
    .target_topen = 153,         //
    .proofow_w = 2,              //
    .rsd_w = 73,                 //
    .rsd_n = 18396,              //
    .mux_depth = 4,              //
    .mux_arities = {4, 4, 4, 4}  //
};

const signature_parameters CAT5_FAST_PARAMETERS = {
    //
    .lambda = 256,               //
    .kappa = 8,                  //
    .tau = 32,                   //
    .target_topen = 207,         //
    .proofow_w = 2,              //
    .rsd_w = 104,                //
    .rsd_n = 19864,              //
    .mux_depth = 4,              //
    .mux_arities = {4, 4, 4, 3}  //
};
