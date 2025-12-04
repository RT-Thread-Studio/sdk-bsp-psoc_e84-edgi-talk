var group__group__nn__kernel__data__structures =
[
    [ "cy_nn_conv_params_t", "group__group__nn__kernel__data__structures.html#structcy__nn__conv__params__t", [
      [ "padValue", "group__group__nn__kernel__data__structures.html#af30ea8425efa4be981cb0ada275e6d08", null ],
      [ "padWidth", "group__group__nn__kernel__data__structures.html#afd78dd918b3944dba219b8b870b5743e", null ],
      [ "padHeight", "group__group__nn__kernel__data__structures.html#a459d9a8a62922b14fc7ae2ac77a1af61", null ],
      [ "strideCol", "group__group__nn__kernel__data__structures.html#af2b3319b831b98599a02518e1b3ba3ae", null ],
      [ "strideRow", "group__group__nn__kernel__data__structures.html#a11ddcc5e138f112d56a171c14cba83ef", null ],
      [ "inputOffset", "group__group__nn__kernel__data__structures.html#a5cb043310069a1fadad3d70cfa560cd7", null ],
      [ "outputOffset", "group__group__nn__kernel__data__structures.html#a23a895023daf3c142b9505e17b7b047d", null ],
      [ "filterOffset", "group__group__nn__kernel__data__structures.html#aa0ccb6d338a67b4528cf7f3700fa60ce", null ],
      [ "outScalingFactor", "group__group__nn__kernel__data__structures.html#a3d0a95bd4c07e553455966c2870b2299", null ],
      [ "outClipping", "group__group__nn__kernel__data__structures.html#ae02403730c265e07ae08dd1a33720d68", null ],
      [ "act_size", "group__group__nn__kernel__data__structures.html#a901d25098cd31fe8a0a863d20cce0749", null ],
      [ "sparseWeights", "group__group__nn__kernel__data__structures.html#a8f8866306b5e2c314703d612ca0af8b6", null ],
      [ "scratchBuf", "group__group__nn__kernel__data__structures.html#a709a6963ccecbad38f82aef893c40830", null ]
    ] ],
    [ "cy_nn_fc_params_t", "group__group__nn__kernel__data__structures.html#structcy__nn__fc__params__t", [
      [ "inputOffset", "group__group__nn__kernel__data__structures.html#a54a625454d2977f648a7e96b74a1542b", null ],
      [ "outputOffset", "group__group__nn__kernel__data__structures.html#aeda5df8724a29e91144d8014b7404b97", null ],
      [ "filterOffset", "group__group__nn__kernel__data__structures.html#a31b2508c6832d1b413436b53ccf92a33", null ],
      [ "outScalingFactor", "group__group__nn__kernel__data__structures.html#a9cd6112d0e878df05578ec238420e384", null ],
      [ "outClipping", "group__group__nn__kernel__data__structures.html#a32755a7dfb93354b573acfb7da3ed664", null ],
      [ "inputSize", "group__group__nn__kernel__data__structures.html#af14b1c10946c0dc0f6a416c86e6bc4b4", null ],
      [ "outputSize", "group__group__nn__kernel__data__structures.html#aafc7b39740ce4fb577ddcc68b609dfa4", null ],
      [ "sparseWeights", "group__group__nn__kernel__data__structures.html#afe811bdd1975ed8b053fb093390a922d", null ],
      [ "scratchBuf", "group__group__nn__kernel__data__structures.html#abc595e3d0a41dac7d98565d43d2819be", null ]
    ] ],
    [ "cy_nn_pool_params_t", "group__group__nn__kernel__data__structures.html#structcy__nn__pool__params__t", [
      [ "strideCol", "group__group__nn__kernel__data__structures.html#a4d4f57d447e26e02274568c8b818e380", null ],
      [ "strideRow", "group__group__nn__kernel__data__structures.html#a40cecb9c59a36042b835d49ff53705ca", null ],
      [ "outClipping", "group__group__nn__kernel__data__structures.html#a0215fcfa36172fe33431cc6d78c490fc", null ],
      [ "act_size", "group__group__nn__kernel__data__structures.html#abc10308403950f9a8de5da9b92b9a7d1", null ],
      [ "scratchBuf", "group__group__nn__kernel__data__structures.html#abb7cd7d3b3e037467ee37a03a6d95d7d", null ]
    ] ],
    [ "cy_nn_scaling_t", "group__group__nn__kernel__data__structures.html#structcy__nn__scaling__t", [
      [ "pre_mac_scale", "group__group__nn__kernel__data__structures.html#a87f5a84f4f61aa6da117468d581e93f1", null ],
      [ "pre_act_scale", "group__group__nn__kernel__data__structures.html#afa2afd590efdcc2e140fa406313a6336", null ],
      [ "pre_mac_scaling_mode", "group__group__nn__kernel__data__structures.html#a1cbfb7244af1a012ae06f5773d91c410", null ],
      [ "pre_act_scaling_mode", "group__group__nn__kernel__data__structures.html#a6753b93af7d944bd3ec47fba76a7912c", null ],
      [ "post_act_scale", "group__group__nn__kernel__data__structures.html#a9ec3aec1762461c9217c6a774c810769", null ],
      [ "scratchBuf", "group__group__nn__kernel__data__structures.html#a77e0e7bf144a588b64225b37878eee79", null ]
    ] ],
    [ "cy_nn_pwise_binary_params_t", "group__group__nn__kernel__data__structures.html#structcy__nn__pwise__binary__params__t", [
      [ "lhsOffset", "group__group__nn__kernel__data__structures.html#ab15fe16711674d4284e9b3a1647668f7", null ],
      [ "rhsOffset", "group__group__nn__kernel__data__structures.html#a207734ae7400ded306de31a010fa33b0", null ],
      [ "outputOffset", "group__group__nn__kernel__data__structures.html#a542b63e2cbc9f215fa693652be2bd3b0", null ],
      [ "outClipping", "group__group__nn__kernel__data__structures.html#a9fd5fce7af639ed659878dfedbad0d43", null ],
      [ "inputSize", "group__group__nn__kernel__data__structures.html#a07199d043ea2abe6afda44f957457916", null ],
      [ "outputSize", "group__group__nn__kernel__data__structures.html#aa9fdbdf8d01d9c624f8b07a266d7ae16", null ],
      [ "scaling", "group__group__nn__kernel__data__structures.html#a9c1ddc8ef0d10f503271d5da218d419b", null ],
      [ "scratchBuf", "group__group__nn__kernel__data__structures.html#aa8f52819e751cbb2410dcf2751224103", null ]
    ] ],
    [ "cy_nn_pwise_unary_params_t", "group__group__nn__kernel__data__structures.html#structcy__nn__pwise__unary__params__t", [
      [ "inScale", "group__group__nn__kernel__data__structures.html#ae8e5de2172d38c4276b769854af3ab54", null ],
      [ "inOffset", "group__group__nn__kernel__data__structures.html#ab7118b959ba67c4b9b182e2032e9db89", null ],
      [ "outScale", "group__group__nn__kernel__data__structures.html#a4702861c9d1b57ee04c08b0653dd2003", null ],
      [ "outOffset", "group__group__nn__kernel__data__structures.html#a46acfce05f8d4f3041a7ff7a3631eb72", null ],
      [ "outClipping", "group__group__nn__kernel__data__structures.html#a264c9c07c6fd1f054fcb36dd174321fb", null ],
      [ "inputSize", "group__group__nn__kernel__data__structures.html#a460fe1caa24b74bd2ca16b33e8965a2d", null ],
      [ "outputSize", "group__group__nn__kernel__data__structures.html#accd0c38ad875e20c4da9e8e7f170c41c", null ],
      [ "scratchBuf", "group__group__nn__kernel__data__structures.html#a7d7c322575ede43b7248c69aaf22171b", null ]
    ] ],
    [ "cy_nn_layernorm_params_t", "group__group__nn__kernel__data__structures.html#structcy__nn__layernorm__params__t", [
      [ "inScale", "group__group__nn__kernel__data__structures.html#af2c2f092cebd22106262593a52204f04", null ],
      [ "inOffset", "group__group__nn__kernel__data__structures.html#a931fa7689f38f147ce64f2efdca89ef6", null ],
      [ "outScale", "group__group__nn__kernel__data__structures.html#a75c0f8503bfcd4e162a900c8e01a7d12", null ],
      [ "outOffset", "group__group__nn__kernel__data__structures.html#aed0e560af1b443857ecaea0e10fbf784", null ],
      [ "epsilon", "group__group__nn__kernel__data__structures.html#a53f8eecb22097d20958b9b77403dfd29", null ],
      [ "outClipping", "group__group__nn__kernel__data__structures.html#a45da60272286fe4e4eb346aa0558d908", null ],
      [ "inputSize", "group__group__nn__kernel__data__structures.html#a4cbca798ebaeb826ce8228b4835c33e7", null ],
      [ "outputSize", "group__group__nn__kernel__data__structures.html#a0215b247ff72d53481428ab22a1461c0", null ],
      [ "scratchBuf", "group__group__nn__kernel__data__structures.html#a18d022497e44edd4e8ace0841e9090cc", null ]
    ] ],
    [ "cy_nn_dims_t", "group__group__nn__kernel__data__structures.html#structcy__nn__dims__t", [
      [ "dims", "group__group__nn__kernel__data__structures.html#a20d2d657274ca02d74613c2c954b68c5", null ]
    ] ],
    [ "cy_nn_act_intrpl_param_t", "group__group__nn__kernel__data__structures.html#structcy__nn__act__intrpl__param__t", [
      [ "segment", "group__group__nn__kernel__data__structures.html#a928bdf5fc1cae357e522e213533716f6", null ]
    ] ],
    [ "cy_nnlite_dma_queue_config_t", "group__group__nn__kernel__data__structures.html#structcy__nnlite__dma__queue__config__t", [
      [ "channelConfig", "group__group__nn__kernel__data__structures.html#a41b27f1b57d99a808b64e6700a2bdb45", null ],
      [ "currCfgDesc", "group__group__nn__kernel__data__structures.html#adb07a579d926497c02ccd7f6edd948a1", null ],
      [ "currDesc", "group__group__nn__kernel__data__structures.html#ad126ed4d39146e9d10d3ba6ceef88176", null ],
      [ "currDescType", "group__group__nn__kernel__data__structures.html#ae8b3290c4475fb32cd4ded4f308c385f", null ],
      [ "headDesc", "group__group__nn__kernel__data__structures.html#a836d5f2fd8ed6eb4f898aa855b850567", null ],
      [ "dmaQDepth", "group__group__nn__kernel__data__structures.html#a9b410560f700ebb33d20d26ad6d757e5", null ],
      [ "dmaQState", "group__group__nn__kernel__data__structures.html#a445daf497a4cac90476f93db12c965a4", null ]
    ] ],
    [ "cy_kernel_config_t", "group__group__nn__kernel__data__structures.html#structcy__kernel__config__t", [
      [ "completionCbFunc", "group__group__nn__kernel__data__structures.html#ab35ca5debb9ce95c4fc4a33f82ca7e06", null ],
      [ "cbArg", "group__group__nn__kernel__data__structures.html#a3689211e27d1bce0e01cb78b261f3aba", null ],
      [ "nnliteMutex", "group__group__nn__kernel__data__structures.html#a7625a2fe1158fb79b9d13e1118cff9c8", null ],
      [ "nnliteSem", "group__group__nn__kernel__data__structures.html#af97d5667657f0253ecc459d1c1973a40", null ],
      [ "mutexCreateFunc", "group__group__nn__kernel__data__structures.html#ab8193b9782c526254e2afb9e3623edf1", null ],
      [ "mutexDeleteFunc", "group__group__nn__kernel__data__structures.html#a80ac76d2fbdd9834b0f71b3943e472d4", null ],
      [ "mutexLockFunc", "group__group__nn__kernel__data__structures.html#af37bddae37ef45ca550c46c54558755e", null ],
      [ "mutexUnlockFunc", "group__group__nn__kernel__data__structures.html#a5188bc95d9834de712f414d3faab507d", null ],
      [ "SemCreateFunc", "group__group__nn__kernel__data__structures.html#a1ec9d73484f39e2f3e765d176cf38ac2", null ],
      [ "SemDeleteFunc", "group__group__nn__kernel__data__structures.html#afc3cf1b3093ed61d39ab403e6e999721", null ],
      [ "SemWaitFunc", "group__group__nn__kernel__data__structures.html#a96460b8a224b26c5cd50149b64534dce", null ],
      [ "SemGiveFunc", "group__group__nn__kernel__data__structures.html#afb8dacf4869a856b5f3b0219d5120f21", null ],
      [ "LpmLockFunc", "group__group__nn__kernel__data__structures.html#a997e1beb12e8729680fc445545c57231", null ],
      [ "LpmUnlockFunc", "group__group__nn__kernel__data__structures.html#a47cdd85cc7ba07c55aea10907ab485cc", null ],
      [ "profStart", "group__group__nn__kernel__data__structures.html#ab193e388a2f18edbf65878fe7429c773", null ],
      [ "profGetCount", "group__group__nn__kernel__data__structures.html#acc040605fcd32dc8178f26d0a278b9d0", null ],
      [ "profStop", "group__group__nn__kernel__data__structures.html#aec37373127cb59be1bf067ba3afd0f30", null ],
      [ "profArg", "group__group__nn__kernel__data__structures.html#a8a914fcd0902d0d73f30e5809b9eb53d", null ],
      [ "intrPriority", "group__group__nn__kernel__data__structures.html#acf85398ef51ee342f198e5eb52f9008c", null ]
    ] ],
    [ "Cy_NNLite_CompletionCallback", "group__group__nn__kernel__data__structures.html#gac81f0ce6912c797aa6df92dd6c438774", null ],
    [ "Cy_NNLite_MutexCreate", "group__group__nn__kernel__data__structures.html#ga7a8512d30f4b8c36f50425988b73fecd", null ],
    [ "Cy_NNLite_MutexDelete", "group__group__nn__kernel__data__structures.html#ga89043f9425e02665f583a789fb3a62ce", null ],
    [ "Cy_NNLite_MutexLock", "group__group__nn__kernel__data__structures.html#ga33db551ecf819049de4ba48e94d14a8d", null ],
    [ "Cy_NNLite_MutexUnlock", "group__group__nn__kernel__data__structures.html#ga564261d7f240fcf0c4d7c5505967bcff", null ],
    [ "Cy_NNLite_SemCreate", "group__group__nn__kernel__data__structures.html#ga3c5c510841865b1d8d6fffa58d71a948", null ],
    [ "Cy_NNLite_SemDelete", "group__group__nn__kernel__data__structures.html#gaa8c46d3e14d30bb1afaef15cc5892ce9", null ],
    [ "Cy_NNLite_SemWait", "group__group__nn__kernel__data__structures.html#ga251b8a02e4f1fb287d36b347fb2b0b37", null ],
    [ "Cy_NNLite_SemGive", "group__group__nn__kernel__data__structures.html#ga20e212ac1c7c48582fa7f76462ccbda0", null ],
    [ "Cy_NNLite_LpmLock", "group__group__nn__kernel__data__structures.html#ga3687863e094762f1dfed9d7dce73e8e3", null ],
    [ "Cy_NNLite_LpmUnlock", "group__group__nn__kernel__data__structures.html#gaaf30b306dd88c64c879f8c5464c28b7e", null ],
    [ "Cy_NNLite_ProfileStart", "group__group__nn__kernel__data__structures.html#ga55fa49a6df041a3f98e1a1409d7dab63", null ],
    [ "Cy_NNLite_ProfileGetCount", "group__group__nn__kernel__data__structures.html#ga5231a85ad78515663c36ebaee1abec6b", null ],
    [ "Cy_NNLite_ProfileStop", "group__group__nn__kernel__data__structures.html#gaa619d9c4b82629171a63f303b8debaca", null ]
];