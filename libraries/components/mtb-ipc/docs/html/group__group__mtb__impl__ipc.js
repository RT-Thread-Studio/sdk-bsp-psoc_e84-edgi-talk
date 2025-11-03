var group__group__mtb__impl__ipc =
[
    [ "_mtb_ipc_event_callback_data_t", "group__group__mtb__impl__ipc.html#struct__mtb__ipc__event__callback__data__t", null ],
    [ "_mtb_ipc_queue_process_info_t", "group__group__mtb__impl__ipc.html#struct__mtb__ipc__queue__process__info__t", [
      [ "mask", "group__group__mtb__impl__ipc.html#acf203c80749230d0ed64ade3b7d304aa", null ],
      [ "flags", "group__group__mtb__impl__ipc.html#adc7b9701aee2f002403798f78aabb8f4", null ]
    ] ],
    [ "_mtb_ipc_core_irq_data", "group__group__mtb__impl__ipc.html#struct__mtb__ipc__core__irq__data", [
      [ "sema_irq", "group__group__mtb__impl__ipc.html#adc2260cd7337902a8a5fb7256ed9b5f6", null ],
      [ "queue_irq", "group__group__mtb__impl__ipc.html#a6402acaddeecd6123f24cf2eee5626cf", null ]
    ] ],
    [ "mtb_ipc_semaphore_data_t", "group__group__mtb__impl__ipc.html#structmtb__ipc__semaphore__data__t", [
      [ "sema_preemptable", "group__group__mtb__impl__ipc.html#af0418ed2abe8b62f8e22fd0af47d9320", null ],
      [ "sema_number", "group__group__mtb__impl__ipc.html#ab42f64d53ad6d7c728e10d46355dc904", null ],
      [ "next_sema", "group__group__mtb__impl__ipc.html#ae011386dee589fad494e516c35677592", null ]
    ] ],
    [ "mtb_ipc_semaphore_t", "group__group__mtb__impl__ipc.html#structmtb__ipc__semaphore__t", [
      [ "sema_obj", "group__group__mtb__impl__ipc.html#a6bb0edafbb869436719e12e8da086177", null ],
      [ "instance", "group__group__mtb__impl__ipc.html#af80bcebac4eb07e2810bf63ce2a6eac0", null ]
    ] ],
    [ "mtb_ipc_mbox_data_t", "group__group__mtb__impl__ipc.html#structmtb__ipc__mbox__data__t", [
      [ "mbox_idx", "group__group__mtb__impl__ipc.html#ab8f4f33a4c209b2507199880f9599723", null ],
      [ "payload", "group__group__mtb__impl__ipc.html#a4afac3c2d8e6cf1ce34d1fabfd4551eb", null ],
      [ "sema_read", "group__group__mtb__impl__ipc.html#a7f0a692d00f12b3a817b0bd9bd291e9c", null ],
      [ "sema_write", "group__group__mtb__impl__ipc.html#a1725e7fabf2c23dae19b14c6a74717a5", null ],
      [ "core_id_sender", "group__group__mtb__impl__ipc.html#a8b720d3a12bc290189ad2948a1e1d3eb", null ],
      [ "core_id_receiver", "group__group__mtb__impl__ipc.html#acae8ed9702bed7f8e2750c239d326189", null ]
    ] ],
    [ "mtb_ipc_mbox_t", "group__group__mtb__impl__ipc.html#structmtb__ipc__mbox__t", [
      [ "mbox_obj", "group__group__mtb__impl__ipc.html#a57359994d7526eda232ee955bef971e2", null ],
      [ "callback_data", "group__group__mtb__impl__ipc.html#acbc018aacae2ca3850095a4f9a590de6", null ],
      [ "sema_read_handle", "group__group__mtb__impl__ipc.html#add51f1b1317210157a022a8063cce5ad", null ],
      [ "sema_write_handle", "group__group__mtb__impl__ipc.html#a27d7c5d92a48830a68e151d5eadea54d", null ],
      [ "instance", "group__group__mtb__impl__ipc.html#a78e69590e2623a7bf7a72ce22d2b86c6", null ]
    ] ],
    [ "mtb_ipc_queue_data_t", "group__group__mtb__impl__ipc.html#structmtb__ipc__queue__data__t", [
      [ "channel_num", "group__group__mtb__impl__ipc.html#a3e77d2bad341f32f51b3f3581b1110c2", null ],
      [ "queue_num", "group__group__mtb__impl__ipc.html#af585d7d57d5fb0a9d3da6c7ac2ecd339", null ],
      [ "max_num_items", "group__group__mtb__impl__ipc.html#a2708c98d606d2eb9aee53af16f08696b", null ],
      [ "item_size", "group__group__mtb__impl__ipc.html#addca8a0781547ec4e749d07ce8c8a04f", null ],
      [ "queue_pool", "group__group__mtb__impl__ipc.html#a7b7019cad72f8db0017dd9b4de77a8a2", null ],
      [ "curr_items", "group__group__mtb__impl__ipc.html#a9d830f6f9f71ebc600045417661437ea", null ],
      [ "first_item", "group__group__mtb__impl__ipc.html#a067d504233788994b0e34d97d947d197", null ],
      [ "queue_semaphore", "group__group__mtb__impl__ipc.html#ae77efb58df3890c8c86ae78ace79c2d8", null ],
      [ "notifications", "group__group__mtb__impl__ipc.html#a664dbc6eee95a3f67c3f7716a0b9589e", null ],
      [ "next", "group__group__mtb__impl__ipc.html#acfcfa7607f4bf2922c94f63efd715c47", null ]
    ] ],
    [ "mtb_ipc_queue_t", "group__group__mtb__impl__ipc.html#structmtb__ipc__queue__t", [
      [ "callback_data", "group__group__mtb__impl__ipc.html#af4cf142427183b57d4d86bf3da47707c", null ],
      [ "queue_obj", "group__group__mtb__impl__ipc.html#a5713a521fecfa7674a4dbb9abff202fd", null ],
      [ "semaphore_handle", "group__group__mtb__impl__ipc.html#a9c0becee9be7dfd22250484221efe3f7", null ],
      [ "next", "group__group__mtb__impl__ipc.html#a4d61cb2b1962760d582f31e6cf45a381", null ],
      [ "instance", "group__group__mtb__impl__ipc.html#a7dd6e6286e2a81e1ef2fe0c575fe9433", null ],
      [ "events_fired", "group__group__mtb__impl__ipc.html#acf70d7c5c8fad871f296de8261115ed0", null ],
      [ "events_enabled", "group__group__mtb__impl__ipc.html#a3887b978f1db44a6ebea1c8f2153ae57", null ],
      [ "pending_reads", "group__group__mtb__impl__ipc.html#a327be1196ce5f44db33fef59c01411a3", null ],
      [ "pending_writes", "group__group__mtb__impl__ipc.html#aa6a033dab3e1cd856b0bfdb094f9d6f0", null ],
      [ "queue_rtos_event", "group__group__mtb__impl__ipc.html#a881c05feb42a032968da7e07733a9d55", null ]
    ] ],
    [ "_mtb_ipc_rtos_semaphore_data", "group__group__mtb__impl__ipc.html#struct__mtb__ipc__rtos__semaphore__data", [
      [ "active_count", "group__group__mtb__impl__ipc.html#a2235993666388e02c58a1504eedeb328", null ],
      [ "sema_idx", "group__group__mtb__impl__ipc.html#ac4522967c75ead404f82eaa2532c20dd", null ]
    ] ],
    [ "mtb_ipc_shared_t", "group__group__mtb__impl__ipc.html#structmtb__ipc__shared__t", [
      [ "ipc_driver_semaphore", "group__group__mtb__impl__ipc.html#aa600c689c6ff02bb575f06cb5d4d9f6b", null ],
      [ "ipc_semaphore_list", "group__group__mtb__impl__ipc.html#a6ad0fd57bb523315e565f4d5913e0849", null ],
      [ "ipc_queue_array", "group__group__mtb__impl__ipc.html#aba21cdfa804b49ef506d965281264544", null ],
      [ "irq_channels", "group__group__mtb__impl__ipc.html#a393311fd711466b571bdd8955da1b699", null ],
      [ "ipc_pdl_sema_array", "group__group__mtb__impl__ipc.html#a2f48585bc57fbdebc16ca0882f3fe629", null ],
      [ "ipc_mailbox_array", "group__group__mtb__impl__ipc.html#ad806596e662174b516b8d3a64899cefc", null ],
      [ "rtos_semaphore_idx", "group__group__mtb__impl__ipc.html#a4bf09b7e755b6cb66dc572a226866551", null ],
      [ "wait_semaphore_idx", "group__group__mtb__impl__ipc.html#a831c95f3bc274b42c2f08042ff79272d", null ]
    ] ],
    [ "mtb_ipc_t", "group__group__mtb__impl__ipc.html#structmtb__ipc__t", [
      [ "internal_channel_index", "group__group__mtb__impl__ipc.html#afbf420fea5c9390c67ba25adeade1996", null ],
      [ "semaphore_irq", "group__group__mtb__impl__ipc.html#a75107c2b45d892bd0eedf56fb16e9563", null ],
      [ "queue_irq", "group__group__mtb__impl__ipc.html#ae7c713016715a4099fdd99724b285ec3", null ],
      [ "trigger_mask", "group__group__mtb__impl__ipc.html#abef20847b750e4221b209a36f9cac18a", null ],
      [ "internal_ipc_semaphore", "group__group__mtb__impl__ipc.html#a7d961ee3f5bc70ed43f2e581522ae0e9", null ],
      [ "rtos_semaphore_base", "group__group__mtb__impl__ipc.html#a0c7d038ea91844cf5d3c85ec24d8c06c", null ],
      [ "wait_semaphore_idx", "group__group__mtb__impl__ipc.html#a1f3f5155f582a0c4700ea8382fd38c8a", null ],
      [ "ipc_shared_vars", "group__group__mtb__impl__ipc.html#aee680bdd3aa5998d49b876a5df1c4260", null ],
      [ "queue_array", "group__group__mtb__impl__ipc.html#ad1b01aca9179d0852c9f74666beb60b4", null ],
      [ "mbox_array", "group__group__mtb__impl__ipc.html#aa46ca7724f642ed014c185c5588aed49", null ]
    ] ],
    [ "MTB_IPC_IRQ_USER", "group__group__mtb__impl__ipc.html#ga1fadf74647efce059f473fd625787b17", null ],
    [ "MTB_IPC_SEMAPHORE_DATA_ALLOC", "group__group__mtb__impl__ipc.html#ga49aea375f838eeb23d2f71420410c442", null ],
    [ "MTB_IPC_MBOX_DATA_ALLOC", "group__group__mtb__impl__ipc.html#ga62857121b80bd6cc53438b013cbfaf78", null ],
    [ "MTB_IPC_QUEUE_POOL_ALLOC", "group__group__mtb__impl__ipc.html#ga82db85425839482443f455a7a7cec48f", null ],
    [ "MTB_IPC_QUEUE_DATA_ALLOC", "group__group__mtb__impl__ipc.html#gaac8ccfa76cd59a07320b3788cb95f073", null ],
    [ "MTB_IPC_SHARED_DATA_ALLOC", "group__group__mtb__impl__ipc.html#ga20855a0037157872e860a8e129ea362d", null ],
    [ "MTB_IPC_POLLING_INTERVAL_uS", "group__group__mtb__impl__ipc.html#gabcfb2c2640ed7c2bace0feecf539ddae", null ],
    [ "MTB_IPC_RTOS_SEMA_NUM", "group__group__mtb__impl__ipc.html#ga1660d9754edc6e84c57ed054887322ff", null ],
    [ "MTB_IPC_SEMAPHORE_WAIT_MASK", "group__group__mtb__impl__ipc.html#ga30c644ffbf0aa58b338b5bfe985ff318", null ],
    [ "MTB_IPC_MAX_MBOXES", "group__group__mtb__impl__ipc.html#gac0c225a813616672c6ff0587de187d43", null ],
    [ "_MTB_IPC_MAX_PROCESSES_PER_QUEUE", "group__group__mtb__impl__ipc.html#ga35808984012b25d3256fa348c0e07d64", null ],
    [ "_MTB_IPC_SEMA_COUNT", "group__group__mtb__impl__ipc.html#ga745f46533b66d4dc1132fd6f53cf457d", null ],
    [ "MTB_IPC_USER_SEMA_COUNT", "group__group__mtb__impl__ipc.html#gab757b4651e132a60dd5ddc95789a3d07", null ],
    [ "_MTB_IPC_UNUSED_SEMAPHORE", "group__group__mtb__impl__ipc.html#ga47bcdfed822289ca7d4421302d70ff32", null ],
    [ "mtb_ipc_mbox_sender_t", "group__group__mtb__impl__ipc.html#ga40774ba7e9a5960245f9d269d78fcffc", null ],
    [ "mtb_ipc_mbox_receiver_t", "group__group__mtb__impl__ipc.html#gab98223c22d617c699c85728236f84617", null ],
    [ "mtb_ipc_channel_t", "group__group__mtb__impl__ipc.html#ga791dec732a2251c395e8a2a913e70993", [
      [ "MTB_IPC_CHAN_0", "group__group__mtb__impl__ipc.html#gga791dec732a2251c395e8a2a913e70993a6c3119c9f51726a13c1dc2c1c95638d6", null ]
    ] ]
];