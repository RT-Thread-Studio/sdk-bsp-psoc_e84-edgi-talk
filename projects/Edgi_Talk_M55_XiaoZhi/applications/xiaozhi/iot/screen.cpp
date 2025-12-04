/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-01-01     RT-Thread    First version - Screen IoT device
 */

#include "thing.h"

extern "C"
{
#include "../ui/xiaozhi_ui.h"
extern void set_brightness(rt_uint8_t percent);
extern rt_uint8_t get_brightness(void);
}

#define TAG "Screen"

namespace iot
{

    // 屏幕状态
    static int screen_brightness = 80;
    static std::string current_emoji = "neutral";

    class Screen : public Thing
    {
    public:
        Screen() : Thing("Screen", "屏幕")
        {
            // 属性：亮度
            properties_.AddNumberProperty("brightness", "亮度", [this]() -> int
                                          { return get_brightness(); });

            // 方法：设置表情
            methods_.AddMethod("SetEmoji", "设置表情", ParameterList({Parameter("emoji", "表情名称", kValueTypeString, true)}), [this](const ParameterList &parameters)
                               {
            current_emoji = parameters["emoji"].string();
            xiaozhi_ui_set_emoji(current_emoji.c_str());
            rt_kprintf("[%s] SetEmoji: %s\n", TAG, current_emoji.c_str()); });

            // 方法：设置亮度
            methods_.AddMethod("SetBrightness", "设置亮度", ParameterList({Parameter("brightness", "0-100", kValueTypeNumber, true)}), [this](const ParameterList &parameters)
                               {
            int brightness = parameters["brightness"].number();
            if (brightness < 0) brightness = 0;
            if (brightness > 100) brightness = 100;
            set_brightness(brightness);
            rt_kprintf("[%s] SetBrightness: %d\n", TAG, screen_brightness); });
        }
    };

} // namespace iot

DECLARE_THING(Screen);
