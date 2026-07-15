# M0 芯片工程开发规范

本文档用于沉淀一套可复用的 M0 芯片工程代码风格和分层框架。后续开发新的 M0/同类 MCU 项目时，优先按本文档建立目录、分层、命名、注释和硬件配置边界，让不同项目保持相似结构，方便阅读、维护和移植。

当前 `24_Test1` 工程是这套风格的参考实现之一。新项目可以按芯片、外设和业务复杂度裁剪，但不要随意打破核心分层、命名和注释习惯。

## 适用范围

适合以下类型项目：

- 裸机或轻量 RTOS 的 MCU 固件项目。
- 需要长期迭代、经常新增外设或迁移板子的项目。
- 希望把硬件配置、外设抽象、设备语义和应用策略分清楚的项目。
- MSPM0、STM32、ESP32、RISC-V 等常见 MCU 工程。

不适合把所有逻辑堆在 `main.c`、中断函数或厂商生成文件里。项目再小，也应保留清晰的职责边界。

## 核心目标

- 让不同项目的代码结构相似，方便快速阅读。
- 让硬件配置、设备语义、应用策略分开，降低移植成本。
- 让注释和命名形成固定习惯，减少“看不懂为什么这么写”的情况。
- 让新增模块时有明确位置，不把逻辑随手塞进 `main.c` 或中断里。
- 让以后同芯片换板、同业务换芯片时，知道优先改哪里。

## 总体分层

推荐所有类似裸机/轻量嵌入式固件按以下方向依赖：

```text
APP -> DAL -> BSP -> DriverLib/SysConfig
CPU -> APP/DAL/BSP/Test
Test -> DAL/BSP
Arithmetic 独立于硬件
```

固定原则：上层优先依赖下一层的语义接口；但对没有运行期状态、事务语义或错误恢复价值的简单板级 GPIO，不为了“封装”再增加一层 BSP 透传。GPIO 的方向、上下拉、PINCM 和输出使能由 SysConfig 生成代码完成；DAL/Driver 只直接使用 SysConfig 生成宏和 DriverLib GPIO API 做运行期读写或逻辑初始电平控制。APP 仍不直接触碰底层硬件。

## 新项目目录模板

推荐目录结构：

```text
Project/
├── User/
│   ├── APP/          应用策略和模式管理
│   ├── DAL/          设备语义、数据转换、控制闭环
│   ├── BSP/          MCU 外设抽象和 IRQ-backed I/O
│   ├── CPU/          初始化、调度、系统任务
│   ├── Arithmetic/   硬件无关算法
│   └── Test/         临时上板验证代码
├── main.c/empty.c    主入口，尽量保持薄
├── *.syscfg/.ioc     固定硬件资源配置
└── docs/spec         可选：需求、硬件锁、移植记录
```

最小项目可以暂时没有所有目录，但一旦出现对应职责，就按这个位置放置，不要混放。

## 工程层职责

| 层级 | 负责什么 | 不负责什么 |
| --- | --- | --- |
| `APP` | 应用策略、模式切换、业务状态机，例如循迹、直行、按键切模式 | 直接读 GPIO、写 PWM、调用 DriverLib |
| `DAL` | 设备语义、数据转换、设备输出和共享控制器实例，例如灰度解码、电机控制、IMU 状态机、`dal_pid`；可直接使用 SysConfig 生成的简单板级 GPIO 宏和 DriverLib GPIO 读写 | GPIO 固定配置、Timer compare、底层 DMA 细节、复杂外设事务状态机 |
| `BSP` | 有运行期抽象价值的 MCU 外设封装、IRQ 数据搬运、状态机和错误恢复，例如 PWM、I2C、UART、DMA、系统时间 | 灰度线位计算、编码器业务含义、APP 模式策略、无状态 GPIO 透传 |
| `CPU` | 芯片初始化、裸机调度、低功耗空闲、系统级任务表 | 具体业务算法、设备数据解释 |
| `Arithmetic` | 硬件无关算法和兼容组件，例如 PID、滤波、ringbuffer、数学工具 | 包含芯片头文件或访问硬件 |
| `Test` | 上板阶段的单项硬件验证，例如电机 PWM 阶梯测试、GPIO 翻转测试 | 长期业务逻辑、APP 策略、复杂闭环 |

`SysConfig/CubeMX/.ioc/Kconfig` 等配置文件是固定硬件资源的单一来源。当前 MSPM0 工程使用 `empty.syscfg`。

## 硬件配置边界

固定硬件资源优先放在芯片厂商配置工具里，例如 MSPM0 的 `empty.syscfg`、STM32 的 `.ioc`、Zephyr 的 devicetree/Kconfig。若项目没有配置工具，则集中放在 BSP 的板级描述表中。

应纳入配置工具或板级描述的内容：

- 引脚复用、PINCM、GPIO 端口和 pin mask。
- 外设实例绑定，例如 `UART0`、`I2C0`、`TIMG8`。
- 固定外设参数，例如 UART 波特率、I2C 速率、PWM 周期、FIFO 阈值、DMA 通道。
- 配置工具能生成的外设中断源。

不要手工修改生成文件，例如 `Debug/ti_msp_dl_config.c`、`Debug/ti_msp_dl_config.h`、CubeMX 生成的初始化代码。修改硬件配置后，通过对应工具重新生成。

BSP 可以包含厂商生成头文件并引用生成宏。DAL/Driver 在处理简单板级 GPIO 时也可以包含厂商生成头文件，并直接调用 `DL_GPIO_readPins()`、`DL_GPIO_setPins()`、`DL_GPIO_clearPins()` 这类无状态 GPIO 读写 API。应以当前 SysConfig 实际生成形式为准：跨端口引脚组可使用各功能的 `BOARD_GPIO_<功能>_PORT`，同一端口引脚组应使用组级 `BOARD_GPIO_PORT` 配合各功能的 `BOARD_GPIO_<功能>_PIN`；不得为兼容旧代码手工补写端口宏。APP 不应包含这些生成头文件，也不应直接调用底层 DriverLib/HAL/LL API。

## BSP 风格

BSP 负责把有运行期价值的 DriverLib/SysConfig 外设能力包成稳定接口。接口命名使用 `bsp_模块_动作`。

推荐风格：

- `bsp_pwm_set_duty()`：只暴露 PWM 通道和千分比占空比，不让 DAL 操作 Timer compare。
- `bsp_i2c_*()` / `bsp_spi_*()`：保留事务状态机、超时和错误恢复。
- `bsp_uart_*()`：保留 TX/RX ringbuffer、DMA/timeout 中断搬运、printf 封装。
- `bsp_encoder_*()`：读取两路 A/B 相电平、集中持有 GPIOB Group1 中断入口，并把 M1/M2 边沿分发给 DAL；不在 BSP 判断编码器方向。

不推荐为简单 GPIO 单独做 `bsp_gpio_read()` / `bsp_gpio_write()` 透传层。若 GPIO 只需要固定配置、读电平或写电平，固定配置放在 SysConfig，DAL/Driver 可直接使用 `BOARD_GPIO_*_PORT`、`BOARD_GPIO_*_PIN` 与 `DL_GPIO_readPins()`、`DL_GPIO_setPins()`、`DL_GPIO_clearPins()`。当 GPIO 涉及中断消抖、批量扫描 DMA、功耗唤醒策略或跨板兼容表时，再建立专用 BSP 或 DAL 表。

BSP 可以做：

- 初始化运行期缓冲区、状态机、NVIC 优先级。
- 处理 IRQ 中短小、确定的数据搬运。
- 做参数检查并返回 `bsp_status_e`。

BSP 不应做：

- 灰度线位计算、编码器方向判断、电机运动策略等设备语义。
- APP 模式切换、循迹策略、调参策略。

## DAL 风格

DAL 负责设备语义和快照数据。接口命名使用 `dal_模块_动作`。

推荐约定：

- DAL 周期刷新出来的最新数据用 `extern` 快照暴露给 APP 只读访问。
- 快照结构体保留 `sequence` 字段，`sequence == 0U` 表示尚未完成刷新。
- 不再新增只为读取最新快照的 `dal_xxx_get_latest()` / `dal_xxx_get_sample()`。
- 编码器测速这类设备语义在 DAL 快照中统一维护：`bsp_encoder` 在 M1/M2 任一 A/B 相双边沿到来时分发通道，DAL 使用四相转移表更新累计计数和快照；CPU 仅每 10 ms 调用 `dal_encoder_update_speed()` 刷新 `dal_encoder_sample_t.speed_cp`，其单位为 `counts/speed-period`。APP/Test 不重复保存上一帧累计计数再自行差分。
- PID 控制器实例统一由 `User/DAL/dal_pid.[ch]` 暴露；APP 组合控制环时直接调用 PID 算法库函数并使用 `pid_gather[]` 中的实例。
- DAL 内部滤波状态、设备状态机阶段、上次计数等运行态保持 `static` 私有。

DAL 可以持有设备语义映射，例如：

- 灰度 D1~D8 顺序、权重、raw mask bit 顺序。
- 编码器 A/B 相关系、方向修正、解码表。
- 电机左右轮、方向脚、矢量占空比输出命令含义。

DAL 不应直接持有复杂外设细节，例如 `MOTOR_PWM_INST`、Timer compare、I2C FIFO/DMA 状态等。简单板级 GPIO 例外：DAL/Driver 可直接持有 SysConfig 生成的 `BOARD_GPIO_*_PORT` / `BOARD_GPIO_*_PIN` 宏和 `DL_GPIO_*Pins()` 读写调用，用来表达传感器输入脚、按键脚、电机方向脚等固定硬件连接；GPIO 方向、上下拉和 PINCM 不在 DAL/Driver 重新初始化。

## APP 风格

APP 只做策略，不做硬件采样和底层控制细节。

推荐约定：

- APP 读取 DAL 快照，例如 `g_dal_gray_sample`、`g_dal_mpu6050_sample`。
- APP 可组合 DAL 快照、`dal_pid` 实例和 DAL 输出 API，例如循迹模式读取灰度/编码器快照后调用 `dal_motor_set_output()`。
- `dal_motor_set_output()` 的电机输出命令统一使用 `-1000~1000` 有符号千分比；该值表示矢量占空比，不表示编码器测速结果。
- DAL 快照可以直接复用收敛后的驱动层状态枚举，例如编码器快照使用 `EncoderState_t state`，不再为同语义状态重复定义 DAL 枚举。
- APP 不直接读 GPIO、不直接写 PWM、不直接调用 DriverLib。
- APP 不负责基础传感器周期刷新，周期刷新由 CPU 调度 DAL 完成。
- 同一应用模块需要支持多个独立赛题/流程时，可以在模块内以“当前任务选择 + 各任务独立状态机”组织；按键、停车计时、灰度边沿确认等共用机制可复用，但不得把不同任务的路线状态交叉混写为单一巨型状态机。单个任务状态机使 `app.c` 接近 800 行上限时，应拆为 `app_taskN.[ch]`，由 `app.c` 保留任务选择和调度入口。

例如循迹 APP 的职责是选择循迹模式、读取灰度和编码器快照、调用 `dal_pid` 中的循迹外环与速度内环 PID 实例、生成左右轮速度目标并通过 `dal_motor_set_output()` 输出矢量占空比；PID 参数集中在 `dal_pid` 修改。

## CPU 调度风格

当前工程的裸机周期任务统一由 `User/CPU/scheduler.[ch]` 管理。调度器以 SysTick 毫秒计数 `uwTick` 为唯一时基：`scheduler_init()` 依次完成 SysConfig、BSP 和 DAL/APP 初始化，`scheduler_run()` 在主循环中按周期调用非阻塞任务并在空闲时进入 `bsp_board_idle()`。不得重新引入 `cpu_scheduler` 或 MultiTimer 作为并行调度入口。

CPU 层负责裸机协作式任务调度。若项目使用 RTOS，则 CPU 层可替换为系统任务创建和调度入口，但职责仍然是“系统组织”，不是业务策略。

推荐初始化顺序：

1. 厂商配置初始化，例如 `SYSCFG_DL_init()`。
2. BSP 运行期初始化，例如 `bsp_board_init()`。
3. CPU 周期任务初始化，例如 `MultiTimer` 或 RTOS task。
4. 进入主循环或调度器。

调度约束：

- 任务函数必须非阻塞。
- ISR 只做短小数据搬运或状态更新。
- ISR 不做日志、不做复杂策略、不做长时间循环等待。
- 传感器刷新、按键刷新、速度闭环等作为独立周期任务运行。
- 执行单项 Test 时，CPU 调度器只登记该测试及其必需的基础任务；凡是会写入同一执行器的 APP/DAL 控制任务必须停用，防止覆盖测试输出。

## C 代码格式

生产 `.c` / `.h` 文件遵守以下风格：

- 文件头使用 Doxygen，包含 `@file` 和 `@brief`。
- 注释使用简体中文。
- 代码段用 `/* ======== xxx ======== */` 分隔。
- 推荐顺序：文件头、include、可调参数宏定义、类型定义、全局实例、内部变量、内部函数声明、内部函数、公开 API。
- 头文件公开函数写 `@brief`、`@param`，有返回值写 `@return`。
- `.c` 内部 `static` 函数定义处也写 Doxygen 注释。
- 结构体和枚举成员使用 `/**< */` 行内注释。
- 行内注释使用 `/* */`，不使用 `//`。
- 行内说明优先解释 WHY，不重复显而易见的 WHAT。

命名约定：

- 普通函数：`模块_动作` 小写下划线，例如 `dal_gray_refresh`。
- 结构体类型：`_t` 后缀。
- 枚举类型：`_e` 后缀。
- 枚举成员：`模块_描述` 全大写风格。
- 文件内静态变量：描述性名称，避免单字母和无意义缩写。
- 头文件 include guard：全大写，并与文件名一致。

## 可调参数

除 `0`、`1`、`-1` 外，生产代码中的数字优先提取为宏，并放在头文件或模块顶部的 `/* ======== 可调参数宏定义 ======== */` 区域。

每个宏需要中文注释说明：

- 用途。
- 单位。
- 约束或调参影响。

硬件固定参数若由 SysConfig/CubeMX 等工具管理，不要再在 BSP/DAL 手写一份。例如 PWM 周期、UART 分频、I2C 速率应来自生成配置或运行期寄存器读取。

控制量、速度、角度等浮点量转整数时默认直接强转截断，不额外加 `0.5f` / `0.5` 做四舍五入；本工程优先保持转换逻辑简单、确定。

## 新项目初始化 Checklist

新开一个项目时，按这个顺序搭框架：

1. 建立目录：`APP`、`DAL`、`BSP`、`CPU`、`Arithmetic`，需要上板分项验证时增加 `Test`。
2. 配好芯片厂商配置文件：时钟、GPIO、UART、调试口、基础外设。
3. 先写 BSP：time、uart log、board init，以及 PWM/I2C/SPI/DMA 等有状态外设；简单 GPIO 先在 SysConfig 配好，DAL/Driver 直接使用生成宏和 `DL_GPIO_*Pins()` 做读写。
4. 再写 CPU：初始化顺序、周期任务表、空闲处理。
5. 再写 DAL：每个传感器/执行器一个模块，暴露快照或控制 API。
6. 最后写 APP：只组合 DAL 快照和目标命令，不直接碰硬件。
7. 跑一次最小构建，确认新增源文件已进入构建系统。

## 新增模块 Checklist

每新增一个外设、传感器、执行器或业务模块，先回答这几个问题：

1. 固定硬件资源是否已经放进 `*.syscfg`、`.ioc` 或板级描述表？
2. BSP 是否只提供外设动作接口，而不是设备业务语义？
3. DAL 是否负责设备语义、数据转换、快照或控制闭环？
4. APP 是否只组合 DAL 快照和控制目标？
5. 是否有中文注释解释复杂表驱动、索引含义和调参宏？
6. 是否避免 APP/DAL 直接包含厂商生成头文件？
7. 新增 `.c` 文件是否已经加入构建清单？

## 移植同芯片板子的 Checklist

同芯片、不同 PCB 移植时，优先按这个顺序改：

1. 修改 `empty.syscfg` 或同类配置文件：调整 GPIO、PWM、I2C、UART、DMA、Timer 等固定硬件资源。
2. 重新生成并构建，确认生成宏存在。
3. 修改 DAL/Driver 中引用的 SysConfig GPIO 宏或局部 GPIO 描述表；只有存在中断/低功耗/批量扫描等运行期策略时，才新增 BSP GPIO 抽象。
4. 修改 DAL 设备语义：例如灰度顺序、编码器方向修正、电机左右轮映射。
5. 修改 APP 控制参数：例如速度、循迹增益、直行增益。
6. 运行构建和分层检查。
7. 上板验证 GPIO、编码器方向、灰度 bit 顺序、电机方向、UART/I2C 通信。

不要一开始就新增新的框架层。当前规模下，保持 `配置文件 + BSP 语义枚举 + DAL 设备语义` 更直观，也更容易排查。

## 阅读友好性要求

为了让代码后续容易阅读，新增模块时尽量做到：

- 文件名一眼看出层级和对象，例如 `dal_gray.c`、`bsp_uart.c`、`app_line_follow.c`。
- 每个 `.c` 顶部先看宏和类型，就能知道这个模块的可调项和状态结构。
- 复杂表驱动必须解释索引含义，例如灰度权重表、编码器转移表。
- APP 的函数名体现业务动作，DAL 的函数名体现设备动作，BSP 的函数名体现外设动作。
- 临时测试代码放 `User/Test/`，验证完删除或明确标注；测试启用时由 CPU 调度器显式接管测试任务表，不与生产控制任务并行写同一执行器。

## 不要做的事

- 不要把业务策略写进 BSP。
- 不要让 APP 直接调用 DriverLib/HAL/LL；DAL/Driver 只允许对简单板级 GPIO 直接使用 `DL_GPIO_*`，复杂外设仍通过 BSP 或专用驱动接口。
- 不要手工修改厂商生成文件。
- 不要把长期逻辑放进临时测试文件。
- 不要在 ISR 里写复杂策略、日志打印或阻塞等待。
- 不要为“读取最新快照”额外封装无意义的 `get_latest()`。
- 不要为当前规模不足的问题提前增加新框架层。
- 不要在生产代码里留下没有注释的复杂数组、查表或魔法数字。

## 当前项目参考实现

本 MSPM0 工程的参考做法：

- 固定硬件资源放在 `empty.syscfg`。
- 简单板级 GPIO 由 `dal_gray`、`dal_key`、`dal_motor` 和相应 `drv_*` 直接使用 SysConfig 生成宏与 `DL_GPIO_*`，不再保留通用 `bsp_gpio` 透传层。
- BSP 通过 `bsp_pwm`、`bsp_i2c`、`bsp_uart`、`bsp_encoder` 等模块封装有运行期状态、中断分发或错误恢复价值的 DriverLib/SysConfig 外设能力。
- DAL 通过 `dal_gray`、`dal_encoder`、`dal_motor`、`dal_pid` 等模块表达设备语义、设备输出和共享 PID 实例。
- APP 通过循迹、直行等模块组合 DAL 快照和控制目标。
- CPU 通过周期任务驱动传感器刷新、按键刷新和速度闭环。

验证命令示例：

```powershell
D:\TI\ccs_20.3\ccs\utils\bin\gmake.exe -C Debug all
python .\.auto-embedded\scripts\check.py
```

建议搜索确认 APP/DAL 没有越层访问底层硬件：

```powershell
rg "ti_msp_dl_config|DL_" User\APP
rg "MOTOR_PWM_INST|DL_Timer|DL_I2C|DL_UART" User\APP User\DAL
```
