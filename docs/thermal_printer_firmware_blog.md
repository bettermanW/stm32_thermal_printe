# 从零解析 STM32 热敏打印机固件

在嵌入式圈子里，迷你热敏打印机项目可谓"麻雀虽小，五脏俱全"。本文就以仓库中的 `stm32_thermal_printe` 工程为例，串起从上电自检、蓝牙传输、队列缓存、热敏头驱动再到步进电机送纸的完整链路，帮助你快速看懂这一套固件架构，并为后续的二次开发打下基础。

---

## 项目速览

- **MCU**：STM32F103（外部 8 MHz 晶振，经 PLL 倍频至 72 MHz 主频）。【F:Core/Src/main.c†L69-L108】
- **固件框架**：基于 HAL 库，辅以 FreeRTOS，但当前主循环仍以裸机轮询形式运行。【F:Core/Src/main.c†L44-L130】
- **核心外设**：SPI1 驱动热敏打印头、TIM1 提供定时、ADC1 读取电池/温度、USART2 连接蓝牙模块、GPIO 控制步进电机和纸检传感器等。【F:Core/Src/main.c†L96-L112】
- **打印规格**：单行 384 点（48 Byte），支持 6 路 STB 选通，热密度可调。【F:HAL/Inc/dr_printer.h†L9-L33】

---

## 启动流程：从上电到主循环

`main.c` 里的控制流非常清晰：

1. **HAL 初始化 & 时钟配置**：`HAL_Init()` 完成底层寄存器复位，随后 `SystemClock_Config()` 选用外部 HSE，设置 72 MHz 系统时钟，并分频生成 ADC 时钟。【F:Core/Src/main.c†L69-L108】
2. **外设装配**：依次初始化 GPIO、DMA、UART、ADC、TIM、SPI 等驱动资源，为后续的打印和通信打好基础。【F:Core/Src/main.c†L96-L112】
3. **系统服务初始化**：调用 `device_state_init()`、`init_timer()`、`sys_queue_init()`、`adc_init()` 完成设备状态、软件定时器、打印缓冲队列和模拟量采样的准备工作。【F:Core/Src/main.c†L114-L122】
4. **主循环**：暂未启用 RTOS 调度，`while(1)` 中每 100 ms 执行一次 `read_all_hal()`，持续采集电量、温度、缺纸状态等信息。【F:Core/Src/main.c†L125-L139】

> 如果后续需要多任务调度，只需放开 `MX_FREERTOS_Init()` 和 `osKernelStart()` 相关代码，即可切换为 RTOS 模式。

---

## 设备抽象层：状态、传感与反馈

### 全局状态快照

`SYSTEM/Src/sys_device.c` 维护了一个 `device_state_t` 结构体，集中记录打印机的运行状态（打印机状态、剩余电量、温度、缺纸标志以及蓝牙是否完成数据读取）。系统初始化时会给出一组合理的缺省值，供 UI 或上位机查询。【F:SYSTEM/Src/sys_device.c†L1-L38】

### 传感器与指示灯

`sys_hal.c` 将硬件读取封装为高层接口：

- `read_all_hal()` 统一调度电池电压、热敏头温度与缺纸传感器，形成主循环的轮询入口。【F:SYSTEM/Src/sys_hal.c†L14-L33】
- `led_run_state()` 根据业务状态执行常亮、熄灭或不同节奏的闪烁，用于提示蓝牙配置、打印异常等事件。【F:SYSTEM/Src/sys_hal.c†L19-L33】
- 缺纸检测既支持轮询，也通过 `HAL_GPIO_EXTI_Callback` 响应外部中断，保证插纸/缺纸状态能够及时反馈。【F:SYSTEM/Src/sys_hal.c†L35-L69】
- 温度、电池读数来自 ADC，内部做了范围校验、线性映射和分压补偿，避免异常数据污染设备状态。【F:SYSTEM/Src/sys_hal.c†L71-L114】

---

## 打印数据流水线：从蓝牙到热敏头

### 环形缓冲队列

蓝牙数据以 48 Byte 为单位（单行）写入 `sys_queue.c` 实现的环形缓冲区。该模块使用 `xSemaphoreTakeFromISR`/`xSemaphoreGiveFromISR` 保护共享内存，因此既能在中断态写入，又能在任务态读取，避免了数据竞争。`get_ble_rx_left_line()` 则提供了实时行数统计，便于打印任务判断何时收尾。【F:SYSTEM/Inc/sys_queue.h†L1-L25】【F:SYSTEM/Src/sys_queue.c†L1-L72】

### 热敏头驱动核心

`HAL/Src/dr_printer.c` 则承担了最关键的打印逻辑：

- `start_printing()`/`start_printing_by_queue_buf()` 将打印流程拆成“取数 → SPI 推送 → STB 加热 → 步进送纸”，并在循环内调用 `printing_error_check()` 捕获缺纸、过温、超时等异常。【F:HAL/Src/dr_printer.c†L127-L215】
- `send_one_line_data()` 会在发送前统计每路加热脉冲数量，通过平方函数和可配置的 `heat_density` 动态拉长加热时间，从而平衡灰度与速度。【F:HAL/Src/dr_printer.c†L52-L107】
- `move_and_start_std()` 把 6 个 STB 通道的加热与步进电机的运动交错执行，减少停顿和卡纸风险。【F:HAL/Src/dr_printer.c†L109-L170】
- `init_printing()`/`stop_printing()` 负责电源开关、锁存控制与超时监控，保证打印头在正确的热管理策略下工作。【F:HAL/Src/dr_printer.c†L24-L51】

整体来看，这一套设计既考虑到了热敏头的能耗（通过 `heat_density` 控制），也兼顾了纸路的平滑性（打印过程中插入 `motor_run()`）。

---

## 蓝牙通信状态机

蓝牙模块通过 UART2 连接，`dr_ble.c` 使用一个细粒度状态机引导 AT 配置流程：

1. **进入 AT 模式**：发送 `+++`，等待模块响应。
2. **关闭状态广播**：`AT+STATUS=0` 减少串口噪声。
3. **查询并设置设备名**：若名称不符合预期，则通过 `AT+NAME=Mini-Printer` 写入并标记需重启。
4. **退出 AT 模式**：`AT+EXIT` 结束配置。

整个流程循环执行直到 `BLE_INIT_FINISH`，并在过程中驱动状态 LED 做出反馈。接收侧通过 `uart_cmd_handle()` 累积串口数据，根据当前状态机阶段解析 `OK`、`+CONN` 等响应。【F:HAL/Src/dr_ble.c†L1-L170】

此外，模块还维护了一个数据包计数器 `pack_count`，配合打印队列一起衡量蓝牙链路的吞吐量。【F:HAL/Src/dr_ble.c†L9-L46】

---

## 步进电机与纸路控制

驱动纸张的核心在 `dr_motor.c`：

- 采用 **半步细分**，使用 8×4 的查找表驱动 ULN2003/ULQ2003 类芯片，实现更平滑的进纸动作。【F:HAL/Src/dr_motor.c†L13-L33】
- `motor_start()` 构建一个 2 ms 周期的 FreeRTOS 软件定时器，持续刷新四相线圈；`motor_stop()` 则在停机时同时清零 GPIO 和关闭定时器。【F:HAL/Src/dr_motor.c†L35-L66】
- 为了与打印循环配合，还提供了 `motor_run()` 与 `motor_run_step()` 两个同步 API，方便在非 RTOS 环境下一步一步推进纸张。【F:HAL/Src/dr_motor.c†L68-L97】

这些 API 与 `dr_printer.c` 中的 `move_and_start_std()` 紧密配合，共同决定打印速度与纸张张力。

---

## 调试与自检钩子

在没有实际图像数据时，可以调用 `testSTB()` 对 6 路 STB 选通逐一做方波测试，确认驱动电路是否正常工作。函数内部会构造多组 0x55 的测试数据，并依次调用 `start_printing_by_one_stb()`，非常适合用于产线或维修自检。【F:HAL/Src/dr_printer.c†L217-L266】

---

## 后续可拓展方向

1. **启用 FreeRTOS**：把蓝牙接收、打印排队、状态上报拆成独立任务，并合理设置优先级，可显著提升系统响应能力。
2. **闭环电源管理**：结合 ADC 读数动态调整 `heat_density` 与 `PRINT_TIME`，在低电量时主动降低功耗。
3. **图像预处理**：在 PC 或移动端预先抖动、压缩图像，减少 MCU 端的计算压力。
4. **固件 OTA**：复用蓝牙链路实现固件升级，进一步提升设备的可维护性。

---

通过以上梳理，相信你已经对这款 STM32 热敏打印机固件的整体架构有了清晰的认识。无论是添加新的打印命令、接入屏幕交互，还是改造成物联网打印终端，都可以在现有模块的基础上快速展开。希望这篇“技术博客”式的解析能成为你探索项目的起点！
