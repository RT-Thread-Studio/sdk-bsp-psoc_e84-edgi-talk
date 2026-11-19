# Edgi_Talk_M55_RTAK RT-AK 部署工程

**中文** | [**English**](./README.md)

用于 RT-AK 部署模型的 M55 工程，由 `Edgi_Talk_M55_USB_UVC` 复制而来，保留 UVC 摄像头与 LCD。与原工程相比：

* LCD 旋转固定为 0 度（`M55_BSP_LCD_ROTATION_0`），在 RT-Thread Settings 中保存不会变为 90 度；
* 默认开启 C++（`RT_USING_CPLUSPLUS`）与 cache（`RT_USING_CACHE`），由工程 `Kconfig` 中的 `RTAK_BSP` 选中。

链接脚本、驱动和应用代码与原工程相同。

## 示例模型

本工程的演示模型为 **YOLOv8n 目标检测**：

| 项目 | 说明 |
| --- | --- |
| 模型 | Ultralytics `yolov8n.pt` 预训练权重，未重新训练 |
| 类别 | COCO 80 类（person、cup、laptop、keyboard、chair 等） |
| 导出 | `yolo export model=yolov8n.pt format=litert imgsz=192`（或导出 ONNX） |
| 输入 | `[1, 3, 192, 192]`，RGB，letterbox 缩放 |
| 输出 | `[1, 84, 756]`：4 个已归一化的框坐标 + 80 个类别分数，不含 NMS（NMS 在板上做） |
| 量化与编译 | `rtak convert` 用 100–300 张校准图片量化为全 INT8，再经 Arm Vela 编译 |
| 运行位置 | Cortex-M55 + Ethos-U55 NPU；模型放在 XIP Flash，推理工作区放在 SOCMEM |

模型导出和校准数据的准备见 RT-AK 仓库 `examples/yolov8/README.md`。

## 运行效果

USB 摄像头（320×240）的画面实时显示在 LCD 上。每一帧缩放到 192×192 后送进 NPU 推理，在板上解码并做 NMS，结果画回原画面：

* 每个检测到的目标都有一个彩色框，框上显示类别名和置信度（例如 `cup 84%`）；
* LCD 左上角显示当前帧率；
* 画面等本帧检测完再显示，框与画面对齐。某一帧漏检的目标，框最多再保留 3 次检测，减少闪烁。

下图为办公桌场景的实拍，同时检测出 `chair`、`cup`、`laptop`、`keyboard`。左上角 `4.9fps 86ms` 表示每秒约检测 4.9 帧，单次推理（NPU 推理加解码和 NMS）平均 86 ms：

<img src="figures/yolov8-detect.jpg" style="zoom:60%;" />

串口每秒打印一行帧率和耗时拆分，并逐个打印检测框（类别、置信度 ×1000、坐标）：

```text
[yolov8] x.x fps, N ms/frame = prepare N + infer N + other N ms, boxes N-N
person 873/1000 [102,40,298,371]
```

`prepare` 是缩放和量化，`infer` 是 NPU 推理加解码和 NMS，`other` 是 LCD 显示和等待下一帧摄像头画面的时间。

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
