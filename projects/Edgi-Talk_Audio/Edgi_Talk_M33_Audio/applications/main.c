#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define RECORD_CHUNK_FRAME     1024

#define LED_PIN_B                 GET_PIN(16, 5)
#define BUTTON_PIN                GET_PIN(8, 3)

static rt_device_t mic_dev;
static rt_device_t sound_dev;
rt_bool_t isplaying = RT_TRUE;

void button_handler(void *args)
{
    isplaying = !isplaying;
    if (isplaying)
    {
        rt_device_open(mic_dev, RT_DEVICE_OFLAG_RDONLY);
        rt_pin_write(LED_PIN_B, PIN_HIGH);
    }
    else
    {
        rt_device_close(mic_dev);
        rt_pin_write(LED_PIN_B, PIN_LOW);
    }
}

int main(void)
{
    rt_uint32_t length = 0;
    rt_uint8_t buffer[RECORD_CHUNK_FRAME] = {0};

    rt_kprintf("Hello RT-Thread\r\n");
    rt_kprintf("This core is cortex-m33\n");
    rt_pin_mode(LED_PIN_B, PIN_MODE_OUTPUT);
    rt_pin_write(LED_PIN_B, PIN_HIGH);

    sound_dev = rt_device_find("sound0");

    struct rt_audio_caps sound_dev_arg;
    sound_dev_arg.main_type = AUDIO_TYPE_MIXER;
    sound_dev_arg.sub_type = AUDIO_MIXER_VOLUME;
    sound_dev_arg.udata.value = 65;
    rt_device_control(sound_dev, AUDIO_CTL_CONFIGURE, &sound_dev_arg);

    rt_device_open(sound_dev, RT_DEVICE_OFLAG_WRONLY);

    mic_dev = rt_device_find("mic0");

    struct rt_audio_caps mic_dev_arg;
    mic_dev_arg.main_type = AUDIO_TYPE_MIXER;
    mic_dev_arg.sub_type = AUDIO_MIXER_VOLUME;
    mic_dev_arg.udata.value = 5;
    rt_device_control(mic_dev, AUDIO_CTL_CONFIGURE, &mic_dev_arg);

    rt_device_open(mic_dev, RT_DEVICE_OFLAG_RDONLY);

    rt_pin_mode(BUTTON_PIN, PIN_MODE_INPUT_PULLUP);
    rt_pin_attach_irq(BUTTON_PIN, PIN_IRQ_MODE_FALLING, button_handler, RT_NULL);
    rt_pin_irq_enable(BUTTON_PIN, PIN_IRQ_ENABLE);

    while (1)
    {
        length = rt_device_read(mic_dev, 0, &buffer, RECORD_CHUNK_FRAME);
        if (length > 0)
            rt_device_write(sound_dev, 0, &buffer, length);
    }
    return 0;
}

//Mos管控制
#define ES8388_CTRL                 GET_PIN(16, 2)  //ES8388 电源 Enable引脚
#define SPEAKER_OE_CTRL             GET_PIN(21, 6)  //功放 Enable引脚
#define WIFI_OE_CTRL                GET_PIN(16, 3)  //WIFI Enable引脚
#define WIFI_WL_REG_OE_CTRL         GET_PIN(11, 6)  //WiFi寄存器开关
#define CTRL                        GET_PIN(7, 2)   //底板 3V3 DCDC电源控制
int en_gpio(void)
{
    rt_pin_mode(WIFI_OE_CTRL, PIN_MODE_OUTPUT);
    rt_pin_write(WIFI_OE_CTRL, PIN_HIGH);

    rt_pin_mode(WIFI_WL_REG_OE_CTRL, PIN_MODE_OUTPUT);
    rt_pin_write(WIFI_WL_REG_OE_CTRL, PIN_HIGH);

    rt_pin_mode(ES8388_CTRL, PIN_MODE_OUTPUT);
    rt_pin_write(ES8388_CTRL, PIN_HIGH);

    rt_pin_mode(SPEAKER_OE_CTRL, PIN_MODE_OUTPUT);
    rt_pin_write(SPEAKER_OE_CTRL, PIN_HIGH);

    return 0;
}
INIT_BOARD_EXPORT(en_gpio);