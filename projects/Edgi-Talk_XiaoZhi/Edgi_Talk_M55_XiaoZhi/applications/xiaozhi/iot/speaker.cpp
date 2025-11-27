#include "thing.h"

// 添加 C 接口头文件
extern "C" {
#include "drv_es8388.h"
}


#define TAG "Speaker"

namespace iot {

class Speaker : public Thing {
public:
    Speaker() : Thing("Speaker", "扬声器") {
        // 定义属性：volume（当前音量值）
        properties_.AddNumberProperty("volume", "当前音量值(0到100之间)", [this]() -> int {
            return es8388_volume_get();
        });

        // 定义方法：SetVolume（设置音量）
        methods_.AddMethod("SetVolume", "设置音量", ParameterList({
            Parameter("volume", "0到100之间的整数", kValueTypeNumber, true)
        }), [this](const ParameterList& parameters) {
            uint8_t volume = static_cast<uint8_t>(parameters["volume"].number());
            if(volume > 100) {
                volume = 100;
            }
            es8388_volume_set(volume);
        });

        // 新增方法：GetVolume（获取音量）
        methods_.AddMethod("GetVolume", "获取当前音量", ParameterList(),
            [this](const ParameterList&) {
                return es8388_volume_get(); // 直接返回音频服务获取的值
            });
    }
};

} // namespace iot

DECLARE_THING(Speaker);
