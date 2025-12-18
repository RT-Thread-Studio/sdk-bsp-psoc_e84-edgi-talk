#include "thing.h"

// 添加 C 接口头文件
extern "C" {
#include "drv_es8388.h"
#include <rtdevice.h>
}

/* Device name configurations */
#ifndef BSP_XIAOZHI_SOUND_DEVICE_NAME
#define BSP_XIAOZHI_SOUND_DEVICE_NAME "sound0"
#endif

#define TAG "Speaker"

namespace iot {

class Speaker : public Thing {
public:
    Speaker() : Thing("Speaker", "扬声器"), current_volume_(60) {

        sound_dev_ = rt_device_find(BSP_XIAOZHI_SOUND_DEVICE_NAME);

        // 定义属性：volume（当前音量值）
        properties_.AddNumberProperty("volume", "当前音量值(0到100之间)", [this]() -> int {
            return current_volume_;
        });

        // 定义方法：SetVolume（设置音量）
        methods_.AddMethod("SetVolume", "设置音量", ParameterList({
            Parameter("volume", "0到100之间的整数", kValueTypeNumber, true)
        }), [this](const ParameterList& parameters) {
            uint8_t volume = static_cast<uint8_t>(parameters["volume"].number());
            if(volume > 100) {
                volume = 100;
            }
            struct rt_audio_caps caps;
            caps.main_type = AUDIO_TYPE_MIXER;
            caps.sub_type = AUDIO_MIXER_VOLUME;
            caps.udata.value = volume;
            rt_device_control(sound_dev_, AUDIO_CTL_CONFIGURE, &caps);
            current_volume_ = volume;
        });

        // 新增方法：GetVolume（获取音量）
        methods_.AddMethod("GetVolume", "获取当前音量", ParameterList(),
            [this](const ParameterList&) {
                return current_volume_;
            });
    }

private:
    rt_device_t sound_dev_;
    int current_volume_;
};

} // namespace iot

DECLARE_THING(Speaker);
