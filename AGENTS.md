<!-- TRELLIS:START -->
# Trellis Instructions

These instructions are for AI assistants working in this project.

Use the `/trellis:start` command when starting a new session to:
- Initialize your developer identity
- Understand current project context
- Read relevant guidelines

Use `@/.trellis/` to learn:
- Development workflow (`workflow.md`)
- Project structure guidelines (`spec/`)
- Developer workspace (`workspace/`)

If you're using Codex, project-scoped helpers may also live in:
- `.agents/skills/` for reusable Trellis skills
- `.codex/agents/` for optional custom subagents

Keep this managed block so 'trellis update' can refresh the instructions.

<!-- TRELLIS:END -->

。

## M0 芯片工程开发规范维护

`CODE_STYLE_AND_ARCHITECTURE.md` 是本工程沉淀出来的 M0 芯片工程开发规范，用于约束后续项目的框架分层、代码风格、命名、注释和移植习惯。

在本工程开发过程中，如果框架分层、目录职责、SysConfig/BSP/DAL/APP 边界、命名规范、注释规范或可复用代码风格发生变化，必须同步更新 `CODE_STYLE_AND_ARCHITECTURE.md`，确保该文档始终反映当前工程真实采用的规范。

## 嵌入式 C 注释与命名规范

对本工程 `.c`/`.h` 生产代码进行新增或修改时，必须遵守以下规则；`User/Arithmetic/ringbuffer.c` 和
`User/Arithmetic/ringbuffer.h` 为外部/兼容代码，除非用户明确要求，否则不要改。

1. **中文注释**：新增和修改的注释必须使用简体中文。
2. **文件头**：每个 `.c`/`.h` 必须有 Doxygen 文件头，包含 `@file` 和 `@brief`。
3. **魔法数字**：除 `0`、`1`、`-1` 外，代码中的数字必须提取为头文件宏，放在 `/* ======== 可调参数宏定义 ======== */` 分隔区；每个可调参数宏都必须有中文注释说明用途、单位或约束。
4. **函数注释**：头文件每个公开函数必须有 `@brief` 和 `@param`；`.c` 文件内部 `static` 函数定义处也必须有 Doxygen 注释，至少包含 `@brief`、`@param`，有返回值时写 `@return`。
5. **成员注释**：结构体和枚举的每个成员必须使用 `/**< */` 行内注释。
6. **命名规范**：结构体类型使用 `_t` 后缀，枚举类型使用 `_e` 后缀，枚举成员使用 `模块_描述` 全大写风格，普通函数使用 `模块_动作` 小写下划线风格。硬件向量表要求的中断入口名可保留芯片库规定名称。
7. **代码段分隔**：用 `/* ======== xxx ======== */` 分隔代码段，推荐顺序为：文件头、include、可调参数宏定义、类型定义、全局实例、内部变量、内部函数声明、内部函数、公开 API。
8. **行内注释**：使用 `/* */`，不要使用 `//`；行内说明只写 WHY，不写显而易见的 WHAT。
9. **静态变量命名**：静态变量禁止使用单字母或无意义名称，必须使用描述性名称，例如 `last_can_send_tick`。
10. **Include Guard**：头文件 include guard 必须全大写，并与文件名一致。

## DAL 快照数据访问规则

在本工程中，DAL 层周期刷新出来的“最新数据快照”应通过 `extern` 全局变量暴露给 APP 层直接只读访问，不再为这类快照额外封装 `get_latest()`、`get_sample()` 之类的拷贝读取函数。

要求：

1. DAL 负责刷新快照变量，例如 `g_dal_gray_sample`、`g_dal_encoder_sample[]`、`g_dal_speed_control_sample[]`、`g_dal_mpu6050_sample`。
2. APP 层可以直接包含对应 DAL 头文件并读取这些 `extern` 快照变量。
3. APP 层不得写这些快照变量。
4. DAL 内部 PID 状态、滤波状态、上次计数、状态机阶段等运行态仍应保持 `static` 私有；只暴露刷新后的数据快照。
5. 快照结构体内应保留 `sequence` 或等价有效性字段；`sequence == 0U` 表示尚未完成过刷新。
6. 不新增 `dal_xxx_get_latest()`、`dal_xxx_get_sample()` 这类仅用于读取最新快照的函数。

## SysConfig 与分层边界规则

本工程使用 `empty.syscfg` 作为固定硬件资源配置的单一来源。修改外设、引脚或固定硬件参数时，优先修改 `empty.syscfg`，通过 CCS/SysConfig CLI 重新生成 `Debug/ti_msp_dl_config.c` 和 `Debug/ti_msp_dl_config.h`；不得手工编辑这些生成文件。

必须纳入 `empty.syscfg` 的内容：

1. **引脚复用与 PINCM**：GPIO、PWM、I2C、UART 等外设的 pinmux、端口、pin mask、PINCM 编号。
2. **外设实例绑定**：例如 `TIMG8`、`I2C0`、`UART0` 这类硬件实例选择。
3. **固定外设参数**：PWM 周期、PWM 输出通道、I2C 总线速率、UART 波特率、UART 分频、FIFO 阈值、外设时钟源等。
4. **中断源使能**：由 SysConfig 能生成的外设中断源配置，例如 I2C NACK/RX/TX 事件、UART TX 事件。
5. **板级普通 GPIO 清单**：按功能命名生成 `BOARD_GPIO_*` 宏，生产代码不得重新手写 `GPIOA/GPIOB`、`DL_GPIO_PIN_x`、`IOMUX_PINCMx` 映射表。
6. **调试接口和系统基础配置**：SWD、SYSCTL、SysConfig 已管理的基础时钟配置。

应保留在 BSP/DAL 中的内容：

1. **BSP 运行期抽象**：GPIO 读写接口、I2C 异步事务状态机、UART 环形缓冲、PWM 占空比接口、NVIC 优先级、超时和错误恢复。
2. **DAL 设备语义**：电机方向/速度、灰度线位计算、编码器采样、MPU6050 数据刷新、PID 状态等业务含义。
3. **APP 策略逻辑**：循迹、直行、驱动模式选择等应用层决策。

分层要求：

1. 生产代码依赖方向保持 `APP -> DAL -> BSP -> DriverLib/SysConfig`。
2. APP 层不得包含 `ti_msp_dl_config.h`，也不得直接调用 `DL_*` DriverLib API。
3. DAL 层原则上不得直接包含 `ti_msp_dl_config.h` 或直接操作 `DL_*` 外设；如需要硬件动作，应先在 BSP 增加抽象接口。例如电机 PWM 必须经 `bsp_pwm_*`，不能在 `dal_motor.c` 直接使用 `MOTOR_PWM_INST` 或 `DL_TimerG_*`。
4. BSP 层可以包含 `ti_msp_dl_config.h`，但应优先引用 SysConfig 生成宏和生成初始化结果，不重复手写固定硬件配置。
5. CPU/板级启动入口可以调用 `SYSCFG_DL_init()`，随后再调用 BSP 初始化；BSP 初始化只补运行期状态、NVIC 优先级、缓冲区和动态中断策略。
6. 新增固定硬件资源后，必须用构建或 SysConfig CLI 验证生成宏存在，再让生产代码引用。

已知容易出错的点：

1. 不要选择带内部回环的 UART SysConfig profile 作为调试串口配置；调试 UART 应显式关闭 `enableInternalLoopback`。
2. 不要在 BSP/DAL 中双写 PWM 周期、UART 分频、I2C timer period 等由 SysConfig 生成的固定数值。
3. 新增 `.c` 文件后，必须确认 CCS/Debug 构建清单已纳入该源文件，否则命令行构建可能没有链接新模块。

### 更改代码的时候不要更改用户的人话注释 你可以优化 不可以删掉

