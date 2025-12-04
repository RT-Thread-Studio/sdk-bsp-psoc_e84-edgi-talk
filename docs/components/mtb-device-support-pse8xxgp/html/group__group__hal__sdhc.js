var group__group__hal__sdhc =
[
    [ "SDHC HAL Results", "group__group__hal__results__sdhc.html", "group__group__hal__results__sdhc" ],
    [ "mtb_hal_sdhc_config_t", "group__group__hal__sdhc.html#structmtb__hal__sdhc__config__t", [
      [ "enableLedControl", "group__group__hal__sdhc.html#a4f4627f58e2b5d443cf275b546d1fb49", null ],
      [ "lowVoltageSignaling", "group__group__hal__sdhc.html#a92db331670779296757119f37036334e", null ],
      [ "isEmmc", "group__group__hal__sdhc.html#a9b48f47c1cccc6fb1c695317c83fa73f", null ],
      [ "busWidth", "group__group__hal__sdhc.html#a60a911871d189693aa355b9a810a5417", null ]
    ] ],
    [ "mtb_hal_sdhc_data_config_t", "group__group__hal__sdhc.html#structmtb__hal__sdhc__data__config__t", [
      [ "data_ptr", "group__group__hal__sdhc.html#ac607986412cb0323d977c08d3c55e714", null ],
      [ "block_size", "group__group__hal__sdhc.html#a1b08389c1121e404ee7f466829909c07", null ],
      [ "number_of_blocks", "group__group__hal__sdhc.html#a1361c49846f19e9b07b9d404dffbd1a1", null ],
      [ "is_read", "group__group__hal__sdhc.html#aab20ebc57e99f79bf47db0a71fc02ba3", null ]
    ] ],
    [ "mtb_hal_sdhc_cmd_config_t", "group__group__hal__sdhc.html#structmtb__hal__sdhc__cmd__config__t", [
      [ "command_index", "group__group__hal__sdhc.html#a1d0c90ff1bb92c01976ab861bf2b1110", null ],
      [ "command_argument", "group__group__hal__sdhc.html#afc967fcd264044a92c2d10bff152cb02", null ],
      [ "enable_crc_check", "group__group__hal__sdhc.html#a7df24b2e4504de6e92d7a1e9c23d8f20", null ],
      [ "response_type", "group__group__hal__sdhc.html#aec1328016916cc3cba09686634798bea", null ],
      [ "enable_idx_check", "group__group__hal__sdhc.html#aa9ef6a6f193d9b69c2ee6e020677c0d5", null ],
      [ "command_type", "group__group__hal__sdhc.html#af2074b151f8e710e10722f061d6d428f", null ],
      [ "data_config", "group__group__hal__sdhc.html#aa1c6303707bf00e229d10dbe4c29c966", null ]
    ] ],
    [ "mtb_hal_sdhc_card_type_t", "group__group__hal__sdhc.html#ga4668fb47762fbfae2421ec83795e1130", [
      [ "MTB_HAL_SDHC_SD", "group__group__hal__sdhc.html#gga4668fb47762fbfae2421ec83795e1130a726c36658a1e925ec95bd03454296ec9", null ],
      [ "MTB_HAL_SDHC_SDIO", "group__group__hal__sdhc.html#gga4668fb47762fbfae2421ec83795e1130a7791de4a1b8d0324d14069c43363d795", null ],
      [ "MTB_HAL_SDHC_EMMC", "group__group__hal__sdhc.html#gga4668fb47762fbfae2421ec83795e1130af7d4029dbc9bb00f4d02d6f144947305", null ],
      [ "MTB_HAL_SDHC_COMBO", "group__group__hal__sdhc.html#gga4668fb47762fbfae2421ec83795e1130ae99e0715a5144116e4f1e6cd4427a581", null ],
      [ "MTB_HAL_SDHC_UNUSABLE", "group__group__hal__sdhc.html#gga4668fb47762fbfae2421ec83795e1130a8c0469560e00760da344d9e761cee1b7", null ],
      [ "MTB_HAL_SDHC_NOT_EMMC", "group__group__hal__sdhc.html#gga4668fb47762fbfae2421ec83795e1130a309fdede832508da45757e0162bc8a89", null ]
    ] ],
    [ "mtb_hal_sdhc_io_voltage_t", "group__group__hal__sdhc.html#ga15b482186bf55288cc0b934a7adbf586", [
      [ "MTB_HAL_SDHC_IO_VOLTAGE_3_3V", "group__group__hal__sdhc.html#gga15b482186bf55288cc0b934a7adbf586acba6aace39cc7a703136253b8334d440", null ],
      [ "MTB_HAL_SDHC_IO_VOLTAGE_1_8V", "group__group__hal__sdhc.html#gga15b482186bf55288cc0b934a7adbf586a4937063d4f10ec33941bfced042d6a03", null ]
    ] ],
    [ "mtb_hal_sdhc_io_volt_action_type_t", "group__group__hal__sdhc.html#gabd6d36cc44dd2f3ca2b3e74d452112c5", [
      [ "MTB_HAL_SDHC_IO_VOLT_ACTION_NEGOTIATE", "group__group__hal__sdhc.html#ggabd6d36cc44dd2f3ca2b3e74d452112c5a567491dfc731e10eb48cc70a86ab32e6", null ],
      [ "MTB_HAL_SDHC_IO_VOLT_ACTION_SWITCH_SEQ_ONLY", "group__group__hal__sdhc.html#ggabd6d36cc44dd2f3ca2b3e74d452112c5a902ef680c3cbcf8e1d40269f2bda6d24", null ],
      [ "MTB_HAL_SDHC_IO_VOLT_ACTION_NONE", "group__group__hal__sdhc.html#ggabd6d36cc44dd2f3ca2b3e74d452112c5a0389e6ee55aaba2b35d3eefc1d05b131", null ]
    ] ],
    [ "mtb_hal_sdhc_cmd_response_type_t", "group__group__hal__sdhc.html#ga12d3432fec217686cd014e700cb661da", [
      [ "MTB_HAL_SDHC_RESPONSE_NONE", "group__group__hal__sdhc.html#gga12d3432fec217686cd014e700cb661daa67fcc49ed1c7ef92a686c4d3a6538f88", null ],
      [ "MTB_HAL_SDHC_RESPONSE_LEN_136", "group__group__hal__sdhc.html#gga12d3432fec217686cd014e700cb661daafbaa9502da7b97a07b5e99313020b1b5", null ],
      [ "MTB_HAL_SDHC_RESPONSE_LEN_48", "group__group__hal__sdhc.html#gga12d3432fec217686cd014e700cb661daa87ba8ba3c298709e2e7150c4602129e0", null ],
      [ "MTB_HAL_SDHC_RESPONSE_LEN_48B", "group__group__hal__sdhc.html#gga12d3432fec217686cd014e700cb661daa3d1e85590c06e2b00134bc46e8a4464f", null ]
    ] ],
    [ "mtb_hal_sdhc_auto_cmd_t", "group__group__hal__sdhc.html#ga299b4c428eba88c5aecbb39139c85b75", [
      [ "MTB_HAL_SDHC_AUTO_CMD_NONE", "group__group__hal__sdhc.html#gga299b4c428eba88c5aecbb39139c85b75a9d057e05a9bf2eea6a1fb9a73c011152", null ],
      [ "MTB_HAL_SDHC_AUTO_CMD_12", "group__group__hal__sdhc.html#gga299b4c428eba88c5aecbb39139c85b75a092450e8a8b79bd9eac8c53fad313766", null ],
      [ "MTB_HAL_SDHC_AUTO_CMD_23", "group__group__hal__sdhc.html#gga299b4c428eba88c5aecbb39139c85b75aeecbb78eddb425c2a7d8717c2dee8b9d", null ],
      [ "MTB_HAL_SDHC_AUTO_CMD_AUTO", "group__group__hal__sdhc.html#gga299b4c428eba88c5aecbb39139c85b75af3f14272b2611e609646cfbd81e3ab22", null ]
    ] ],
    [ "mtb_hal_sdhc_cmd_type_t", "group__group__hal__sdhc.html#gabd5cc309612b07abc2ce588bc6544152", [
      [ "MTB_HAL_SDHC_CMD_NORMAL", "group__group__hal__sdhc.html#ggabd5cc309612b07abc2ce588bc6544152a3a88922b647a59d69f3817d3aa3f1023", null ],
      [ "MTB_HAL_SDHC_CMD_SUSPEND", "group__group__hal__sdhc.html#ggabd5cc309612b07abc2ce588bc6544152add27b0ba8ddc1e525c2b025bbe4acd30", null ],
      [ "MTB_HAL_SDHC_CMD_RESUME", "group__group__hal__sdhc.html#ggabd5cc309612b07abc2ce588bc6544152a51ce0b481a8482c10e2c59fa946c3d34", null ],
      [ "MTB_HAL_SDHC_CMD_ABORT", "group__group__hal__sdhc.html#ggabd5cc309612b07abc2ce588bc6544152aafc0cdff6dcf7a5ba4274f05b27c0b99", null ]
    ] ],
    [ "mtb_hal_sdhc_error_type_t", "group__group__hal__sdhc.html#ga17eaa6a5f9b555e167399040a2867511", [
      [ "MTB_HAL_SDHC_NO_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511a1e49b6af0789af53afcb8fab6786d12f", null ],
      [ "MTB_HAL_SDHC_CMD_TOUT_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511a2bcb5ff43999e655c2dd770c36813991", null ],
      [ "MTB_HAL_SDHC_CMD_CRC_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511ace05aa940b12bfbc91551ce0c209870d", null ],
      [ "MTB_HAL_SDHC_CMD_END_BIT_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511a0344a834fc8c979da4918bd329b468b6", null ],
      [ "MTB_HAL_SDHC_CMD_IDX_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511a158f6730bf022ac4915d291a2a64c506", null ],
      [ "MTB_HAL_SDHC_DATA_TOUT_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511afcef521cd81a1e95fbb6d7da6db2eb9c", null ],
      [ "MTB_HAL_SDHC_DATA_CRC_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511acc37d2a788b9929d2449597a4bf6b4cb", null ],
      [ "MTB_HAL_SDHC_DATA_END_BIT_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511ae9538a06ef015ebc1742a96a8e32a5e7", null ],
      [ "MTB_HAL_SDHC_CUR_LMT_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511acb81f4f0054799528c20c6c26a589f29", null ],
      [ "MTB_HAL_SDHC_AUTO_CMD_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511a75724beb5596da7083168c98ec187e10", null ],
      [ "MTB_HAL_SDHC_ADMA_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511a7a60893969a1524a9f8677e251a4b148", null ],
      [ "MTB_HAL_SDHC_TUNNING_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511a750557d9ada1983bd328db725b7a7c86", null ],
      [ "MTB_HAL_SDHC_RESP_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511a8745a823826d94cf9bd536a83afffed1", null ],
      [ "MTB_HAL_SDHC_BOOT_ACK_ERR", "group__group__hal__sdhc.html#gga17eaa6a5f9b555e167399040a2867511a606499d9b856301ff7bf35ad56b45e6c", null ]
    ] ],
    [ "mtb_hal_sdhc_setup", "group__group__hal__sdhc.html#gad88058dc1388490241feeebb3e05f8fe", null ],
    [ "mtb_hal_sdhc_erase", "group__group__hal__sdhc.html#gabf453125b3f194ba6cba5296abc4d576", null ],
    [ "mtb_hal_sdhc_read_async", "group__group__hal__sdhc.html#ga18e8a67d81239ca19675eb849483cbf9", null ],
    [ "mtb_hal_sdhc_write_async", "group__group__hal__sdhc.html#gaf795bdfcbe02977569ab126cf33f0b00", null ],
    [ "mtb_hal_sdhc_is_card_inserted", "group__group__hal__sdhc.html#gae86fb18ca41342e46c8b7ecb6dc8f122", null ],
    [ "mtb_hal_sdhc_is_card_mech_write_protected", "group__group__hal__sdhc.html#gacb6d096fa35646584c361da45568da5b", null ],
    [ "mtb_hal_sdhc_get_block_count", "group__group__hal__sdhc.html#ga065ebd1e67d6d00425867c079a058987", null ],
    [ "mtb_hal_sdhc_set_frequency", "group__group__hal__sdhc.html#ga99dff4cc56b7e0a4044b9d227d0b5336", null ],
    [ "mtb_hal_sdhc_get_frequency", "group__group__hal__sdhc.html#ga5b73cd8ecca03e77b3047163c07bea24", null ],
    [ "mtb_hal_sdhc_set_data_read_timeout", "group__group__hal__sdhc.html#ga351ef63fd11aba67eeebb8911de932a1", null ],
    [ "mtb_hal_sdhc_config_data_transfer", "group__group__hal__sdhc.html#gaddc9dd1330120090ecb7390cf8c069be", null ],
    [ "mtb_hal_sdhc_send_cmd", "group__group__hal__sdhc.html#ga098e46f2ef37099db1a7eefadd14013c", null ],
    [ "mtb_hal_sdhc_get_response", "group__group__hal__sdhc.html#ga4da32ac6ef2ac298aef978cd1d4647cb", null ],
    [ "mtb_hal_sdhc_wait_transfer_complete", "group__group__hal__sdhc.html#gafe7b1794fd1c9beca10a7ecce465b9ae", null ],
    [ "mtb_hal_sdhc_set_io_voltage", "group__group__hal__sdhc.html#ga5e9195fb7f6070f1f13001e4ab69bdd8", null ],
    [ "mtb_hal_sdhc_get_io_voltage", "group__group__hal__sdhc.html#gab7603e61ed05f8727e4f5367845cc5d8", null ],
    [ "mtb_hal_sdhc_set_bus_width", "group__group__hal__sdhc.html#ga1c6eb964590bb613d7697543fcfe99f4", null ],
    [ "mtb_hal_sdhc_get_bus_width", "group__group__hal__sdhc.html#gaf05b54d50f1b34c533d2e1a967f1588e", null ],
    [ "mtb_hal_sdhc_get_last_command_errors", "group__group__hal__sdhc.html#gab69062cd126add0f8cb210ef63fa4e94", null ],
    [ "mtb_hal_sdhc_clear_errors", "group__group__hal__sdhc.html#ga7f266deb004491df2d3654580572c2ed", null ],
    [ "mtb_hal_sdhc_software_reset", "group__group__hal__sdhc.html#gac4ac03228308834d3a5427fd51c956ac", null ],
    [ "mtb_hal_sdhc_enable_card_power", "group__group__hal__sdhc.html#gac05ff57b6080cb6c24115ce38f2d14ec", null ],
    [ "mtb_hal_sdhc_process_interrupt", "group__group__hal__sdhc.html#ga6260817655bb44f7c090f5dd6102e917", null ]
];