var group__group__hal__i2c =
[
    [ "I2C HAL Results", "group__group__hal__results__i2c.html", "group__group__hal__results__i2c" ],
    [ "mtb_hal_i2c_cfg_t", "group__group__hal__i2c.html#structmtb__hal__i2c__cfg__t", [
      [ "is_target", "group__group__hal__i2c.html#a8a8e44067a417fcac04d816808f7e0d4", null ],
      [ "address", "group__group__hal__i2c.html#ab21e1f81fa18e9ee6145b5718363b17f", null ],
      [ "frequency_hz", "group__group__hal__i2c.html#a451f2c5482c67752c1e51df895a01604", null ],
      [ "address_mask", "group__group__hal__i2c.html#aeb708d88c2936a44d06e044d9cf69e4c", null ],
      [ "enable_address_callback", "group__group__hal__i2c.html#a0f1e61a7a60f2606c85540e0fb7fba26", null ]
    ] ],
    [ "MTB_HAL_I2C_MODE_TARGET", "group__group__hal__i2c.html#gaf2168badd030c55381c470b849f0d9e8", null ],
    [ "MTB_HAL_I2C_MODE_CONTROLLER", "group__group__hal__i2c.html#ga7f2554a3095dc72de7b6eeb5bef228aa", null ],
    [ "MTB_HAL_I2C_DEFAULT_ADDR_MASK", "group__group__hal__i2c.html#ga4cd902f7ba1326902a2aa4f04173b77a", null ],
    [ "mtb_hal_i2c_event_callback_t", "group__group__hal__i2c.html#ga2176dab61482eae237d3c7d6f5d30d43", null ],
    [ "mtb_hal_i2c_address_callback_t", "group__group__hal__i2c.html#ga18ae3a199400fc5c7caccb21bcefdf5e", null ],
    [ "mtb_hal_i2c_byte_callback_t", "group__group__hal__i2c.html#gaf8483dfbb99126f5551712607c9f154c", null ],
    [ "mtb_hal_i2c_event_t", "group__group__hal__i2c.html#ga8e634b8ee008faefc71f4d7854976791", [
      [ "MTB_HAL_I2C_EVENT_NONE", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a9fdc57bf933a6137ac3073b817b6d7ba", null ],
      [ "MTB_HAL_I2C_TARGET_READ_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a13725e0f04c2c731ba5c3c548b635f1b", null ],
      [ "MTB_HAL_I2C_TARGET_WRITE_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791aeed7710b9fbfb825c5f9c170b74bfccf", null ],
      [ "MTB_HAL_I2C_TARGET_RD_IN_FIFO_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791ae3a6aa948e52f5aeac3f3594495d5a7c", null ],
      [ "MTB_HAL_I2C_TARGET_RD_BUF_EMPTY_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a6ef8dd93795afe0c3eadf50cc8bdef48", null ],
      [ "MTB_HAL_I2C_TARGET_RD_CMPLT_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791ab83e093ed163ac8e59f68ad073f07189", null ],
      [ "MTB_HAL_I2C_TARGET_WR_CMPLT_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a9e98f71ffa45e9312fdc82317674ba93", null ],
      [ "MTB_HAL_I2C_TARGET_ERR_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791ab4ff58efc8e2819f2f4f0329f39368b6", null ],
      [ "MTB_HAL_I2C_TARGET_RESTART_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a13d354ee07c9e3f2a550bf1182a483f5", null ],
      [ "MTB_HAL_I2C_TARGET_STOP_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a9f92076d3d1bdcb3d6a9129653d3bb55", null ],
      [ "MTB_HAL_I2C_TARGET_ARB_LOST_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791aafb45169abea46ab7b8cf426b97c2645", null ],
      [ "MTB_HAL_I2C_TARGET_TIMEOUT0_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791ad66e8e03d1bdc350f0b911c5cfc9df28", null ],
      [ "MTB_HAL_I2C_TARGET_TIMEOUT1_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a25b4903c893faceaa85f9cad5aa69612", null ],
      [ "MTB_HAL_I2C_TARGET_TIMEOUT2_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791afbaa27227ede850eda0e299c4fd7a22a", null ],
      [ "MTB_HAL_I2C_CONTROLLER_ERR_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a1b76717c61ba994e8ed604143f871325", null ],
      [ "MTB_HAL_I2C_CONTROLLER_ARB_LOST_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a4558c9d44d124a94d7747e3eedf9755e", null ],
      [ "MTB_HAL_I2C_CONTROLLER_TIMEOUT0_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791ad54e30dd823a630aed512cd82957c9f2", null ],
      [ "MTB_HAL_I2C_CONTROLLER_TIMEOUT1_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a4704be8d2c9da07ab3c044fc7d8a5e90", null ],
      [ "MTB_HAL_I2C_CONTROLLER_TIMEOUT2_EVENT", "group__group__hal__i2c.html#gga8e634b8ee008faefc71f4d7854976791a98c049404db1c171432b9f4e9eb486ad", null ]
    ] ],
    [ "mtb_hal_i2c_addr_event_t", "group__group__hal__i2c.html#gae0be6d796369cbf15cbb598801b74d42", [
      [ "MTB_HAL_I2C_ADDR_EVENT_NONE", "group__group__hal__i2c.html#ggae0be6d796369cbf15cbb598801b74d42ad39064cdeccdc77c441962e824eb823b", null ],
      [ "MTB_HAL_I2C_GENERAL_CALL_EVENT", "group__group__hal__i2c.html#ggae0be6d796369cbf15cbb598801b74d42a1b78bc2abf10d05fa2124093b9889c86", null ],
      [ "MTB_HAL_I2C_ADDR_MATCH_EVENT", "group__group__hal__i2c.html#ggae0be6d796369cbf15cbb598801b74d42a657868f03c276eada4a2af63889651e4", null ]
    ] ],
    [ "mtb_hal_i2c_fifo_type_t", "group__group__hal__i2c.html#gaebd4305fa37b4f0b80f3e8edc9772c9e", [
      [ "MTB_HAL_I2C_FIFO_RX", "group__group__hal__i2c.html#ggaebd4305fa37b4f0b80f3e8edc9772c9eae1c6dfca880e84bdc37859f2fddd6f60", null ],
      [ "MTB_HAL_I2C_FIFO_TX", "group__group__hal__i2c.html#ggaebd4305fa37b4f0b80f3e8edc9772c9ea008fc5f18bab01a0b655719b961df509", null ]
    ] ],
    [ "mtb_hal_i2c_command_rsp_t", "group__group__hal__i2c.html#gab6a1e408f6959f2b4fb0505acaa59571", [
      [ "MTB_HAL_I2C_CMD_ACK", "group__group__hal__i2c.html#ggab6a1e408f6959f2b4fb0505acaa59571ad257ce69207c5f0cf68de0103b3cc9e5", null ],
      [ "MTB_HAL_I2C_CMD_NAK", "group__group__hal__i2c.html#ggab6a1e408f6959f2b4fb0505acaa59571ae056a25e2011910de03977865bb38781", null ]
    ] ],
    [ "mtb_hal_i2c_output_t", "group__group__hal__i2c.html#ga540e89bfa8f8eb1c1cbf1f1039fba68f", [
      [ "MTB_HAL_I2C_OUTPUT_TRIGGER_RX_FIFO_LEVEL_REACHED", "group__group__hal__i2c.html#gga540e89bfa8f8eb1c1cbf1f1039fba68fa44bc795b85296d08aeb3bab1b8fd2ffa", null ],
      [ "MTB_HAL_I2C_OUTPUT_TRIGGER_TX_FIFO_LEVEL_REACHED", "group__group__hal__i2c.html#gga540e89bfa8f8eb1c1cbf1f1039fba68fa9dfbb9764265e28e16ad6931eca04cc0", null ]
    ] ],
    [ "mtb_hal_i2c_configure", "group__group__hal__i2c.html#ga3e6a19afa75a6bcdd6fb700c2c0e8748", null ],
    [ "mtb_hal_i2c_controller_write", "group__group__hal__i2c.html#gae11e1deef221b6d628c9e0c0727c1ae8", null ],
    [ "mtb_hal_i2c_controller_read", "group__group__hal__i2c.html#gadc139fce1bba59e5dcf5969b7ce8336d", null ],
    [ "mtb_hal_i2c_target_config_read_buffer", "group__group__hal__i2c.html#gaa70af44eda7574252b42f0309b11d133", null ],
    [ "mtb_hal_i2c_target_config_write_buffer", "group__group__hal__i2c.html#ga7ecbce89119cefaca18a27f1ef937fe0", null ],
    [ "mtb_hal_i2c_register_callback", "group__group__hal__i2c.html#ga61c290cdd41065107b55b395bbd87dfd", null ],
    [ "mtb_hal_i2c_register_address_callback", "group__group__hal__i2c.html#gaca10fd3fc422bf113f71409b2ec1ddfe", null ],
    [ "mtb_hal_i2c_register_byte_received_callback", "group__group__hal__i2c.html#ga6eb3eb37b10d3e258f84d8bce313dd1e", null ],
    [ "mtb_hal_i2c_enable_event", "group__group__hal__i2c.html#ga3728b432e01d66c3262b1e962ca4af62", null ],
    [ "mtb_hal_i2c_enable_address_event", "group__group__hal__i2c.html#ga7cae29ddae7ca97363b43a6e8607579b", null ],
    [ "mtb_hal_i2c_target_readable", "group__group__hal__i2c.html#ga09dddf41f9d9fb76137261c07e54a607", null ],
    [ "mtb_hal_i2c_target_writable", "group__group__hal__i2c.html#ga81b8391e21326905ce86744f4c8d616b", null ],
    [ "mtb_hal_i2c_target_read", "group__group__hal__i2c.html#gaf759556840ee7ca1f2c073ebab60b006", null ],
    [ "mtb_hal_i2c_target_write", "group__group__hal__i2c.html#gac2464f25bda991130f901761a3308681", null ],
    [ "mtb_hal_i2c_target_abort_read", "group__group__hal__i2c.html#ga23c14f27347735729166e5b189baee07", null ],
    [ "mtb_hal_i2c_clear", "group__group__hal__i2c.html#gaac6918b173a4bae3e27686385da4e0ee", null ],
    [ "mtb_hal_i2c_process_interrupt", "group__group__hal__i2c.html#gaf8f477fc9daf0ea1bd2213187a96cf1c", null ]
];