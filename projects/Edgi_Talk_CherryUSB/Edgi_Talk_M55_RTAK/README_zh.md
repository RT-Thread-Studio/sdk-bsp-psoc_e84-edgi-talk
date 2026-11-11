# Edgi_Talk_M55_RTAK RT-AK 部署工程

**中文** | [**English**](./README.md)

用于 RT-AK 部署模型的 M55 工程，由 `Edgi_Talk_M55_USB_UVC` 复制而来，保留 UVC 摄像头与 LCD。与原工程相比：

* LCD 旋转固定为 0 度（`M55_BSP_LCD_ROTATION_0`），在 RT-Thread Settings 中保存不会变为 90 度；
* 默认开启 C++（`RT_USING_CPLUSPLUS`）与 cache（`RT_USING_CACHE`），由工程 `Kconfig` 中的 `RTAK_BSP` 选中。

链接脚本、驱动和应用代码与原工程相同。

## 运行效果

示例模型为 YOLOv8n 目标检测（Ultralytics 预训练，COCO 80 类，输入 192×192）：由 RT-AK 量化为 INT8、经 Vela 编译后在 Ethos-U55 NPU 上推理。
USB 摄像头画面显示在 LCD 上，检测到的目标用彩色框标出，框上显示类别名和置信度，左上角显示帧率：

<img src="figures/yolov8-detect.jpg" style="zoom:60%;" />

操作步骤见第 5 节。

## 1. 获取工程

任选一种：

* RT-Thread Studio：新建工程 → 基于开发板 → PSOC_E84-EDGI-TALK → 示例 `Edgi_Talk_M55_RTAK`；
* 克隆本仓库，在本目录双击运行 `mklinks.bat`，然后在 Studio 中：文件 → 导入 → RT-Thread Studio 项目到工作空间，选择本目录。

## 2. 部署模型

在 RT-AK 仓库根目录的 PowerShell 中执行（`$bsp` 改为本工程的实际目录）：

```powershell
$bsp = 'C:\work\sdk-bsp-psoc_e84-edgi-talk\projects\Edgi_Talk_CherryUSB\Edgi_Talk_M55_RTAK'
rtak convert --model <模型.tflite> --board edgi_talk_uvc_vendor --project "$bsp" --dataset <校准数据>
```

## 3. 编译与下载

1. Studio 中右键工程 → 刷新；
2. 打开 RT-Thread Settings，不改任何选项，直接保存；
3. 构建工程；
4. 只下载 M55 固件，复位开发板。

## 4. 上板确认

串口终端（115200 8N1）执行：

```text
msh />rtak_run
```

输出 `[rtak] check=PASS` 即部署成功。

插入 USB 摄像头后执行 `usbh_uvc_start 0 320 240`（0 = YUYV，1 = MJPEG；最大 432x240），LCD 显示画面：

<img src="figures/usb-lcd.png" style="zoom:80%;" />

## 5. 摄像头目标检测（YOLOv8n）

1. 按 RT-AK 仓库 `examples/yolov8/README.md` 第 1、2 节准备 `yolov8n.tflite` 和校准图片目录 `calib_images`。
2. 部署：

   ```powershell
   rtak convert --model yolov8n.tflite --board edgi_talk_uvc_vendor --project "$bsp" --dataset calib_images
   ```

3. 把 RT-AK 仓库 `examples/yolov8/` 下的 `yolov8_app.c`、`yolov8_app.h`、`yolov8_uvc.c` 复制到本工程 `applications/`：

   ```powershell
   Copy-Item RT-AK/examples/yolov8/yolov8_app.c, RT-AK/examples/yolov8/yolov8_app.h, RT-AK/examples/yolov8/yolov8_uvc.c "$bsp\applications\"
   ```

4. 按第 3 节刷新、保存 RT-Thread Settings、构建、只下载 M55。
5. 插入 USB 摄像头，串口执行：

   ```text
   msh />usbh_uvc_start 0 320 240
   msh />yolov8_start
   ```

   串口每秒打印一行帧率与耗时，LCD 显示检测框。执行 `yolov8_stop` 停止。

## 6. 启动顺序

M55 由 M33 启动。M55 无法运行时，先编译并下载 `Edgi_Talk_M33_Template`，并在 M33 工程中开启 CM55：

```
RT-Thread Settings -> 硬件 -> select SOC Multi Core Mode -> Enable CM55 Core
```

![config](figures/config.png)
