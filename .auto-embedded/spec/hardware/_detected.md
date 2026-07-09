# 自动探测结果（草案，待人工确认）

> 由 `aemb init` 扫描工程文件名/扩展名推断，**可能误判**。
> 确认无误后，请把相关信息手工并入 `index.md` 与 `hw-lock.yaml`，再删除本文件。

- 疑似芯片: MSPM0G3507, MSPM0G350X
- 构建系统: Makefile, IAR EWARM, Keil MDK, 固件产物(.hex)

## 证据
- MSPM0G3507 ← empty_LP_MSPM0G3507_nortos_gcc.projectspec
- Makefile ← makefile
- IAR EWARM ← empty_LP_MSPM0G3507_nortos_iar.ewp
- MSPM0G350X ← startup_mspm0g350x_iar.c
- Keil MDK ← empty_LP_MSPM0G3507_nortos_keil.uvprojx
- 固件产物(.hex) ← empty_LP_MSPM0G3507_nortos_keil.hex
