# AP3216C 驱动测试计划

对照手册：`docs/AP3216C.pdf`（Lite-On Rev 0.86）  
对照代码：`src/ap3216c/ap3216c.c`、`ap3216c.h`  
目的：用板上可操作的步骤判断**已实现功能是否工作**，并用预期失败区分**尚未实现**的功能。

约定：

- `IIO` 表示 `/sys/bus/iio/devices/iio:deviceN`（`N` 以 `name` 为 `ap3216c` 的节点为准）。
- 芯片上电默认 `0x00 = 0`（Power Down）。probe **不会**自动开测量，测试前必须写 mode。
- 连续 ALS 一轮约 100 ms；PS+IR 约 12.5 ms；ALS+PS+IR 约 112.5 ms。读 raw 前至少等一轮。
- 必须先读低字节再读高字节：由驱动保证，测试时不要用 `i2cget` 插在驱动读的中间。

---

## 0. 编译与加载（无芯片也可做）

| ID | 步骤 | 期望 |
|----|------|------|
| C1 | 用**目标内核**（RK3568 为 5.10）`make M=src` 编 `ap3216c.ko` | 无 error。当前源码在 **5.1 上不能编过**，见文末「内核 5.1」。 |
| C2 | `insmod` / `modprobe ap3216c` | `dmesg` 无 probe 失败。DT 无 `interrupts` 时仍应成功（只是没有事件）。 |
| C3 | `ls /sys/bus/iio/devices/*/name` 含 `ap3216c` | 存在 `mode`、`als_persistence`、`in_illuminance_raw`、`in_illuminance_scale`、`in_proximity_raw`、`in_intensity_ir_raw`。 |
| C4 | `rmmod ap3216c` | 成功。有芯片时可用 `i2cget` 看 `0x00` 是否为 0（remove 写 power down）。 |

---

## 1. System mode（0x00）— 已实现

先确认 I2C 从地址 **0x1E**。

| ID | 步骤 | 期望 |
|----|------|------|
| M0 | `cat $IIO/mode`（未写过） | `0`（PD）。 |
| M1 | `echo 1 > $IIO/mode` 后 `cat $IIO/mode` | `1`。约 100 ms 后 `in_illuminance_raw` 非全 0（有光环境）。`in_proximity_raw` 手册规定此 mode 下 PS 不工作，勿当接近值用。 |
| M2 | `echo 2 > $IIO/mode` | mode=2。约 12.5 ms 后可读 `in_proximity_raw`、`in_intensity_ir_raw`。ALS 此 mode 不工作。 |
| M3 | `echo 3 > $IIO/mode` | mode=3。约 112.5 ms 后 ALS/PS/IR 都能读。 |
| M4 | `echo 0 > $IIO/mode` | 立刻返回（不应再睡 250 ms）。随后 raw 被芯片清掉（手册：PD 会清 ALS/PS/IR）。 |
| M5 | `echo 4 > $IIO/mode`（SW reset） | 各寄存器回默认。复位后 mode 应变回 0。当前驱动仍会 `msleep(250)`，长于手册 10 ms，功能上应仍成功。 |
| M6 | `echo 5 > $IIO/mode`（ALS once） | 约 250 ms 后 ALS 有数；芯片自动 PD。再 `cat mode` 可能已是 0，数据保持到下一次测量。 |
| M7 | `echo 8 > $IIO/mode` | 失败（`-EINVAL`）。合法为 0–7。 |

未覆盖：按 mode 精确等待（见缺口清单 6.3）。M1–M3 只要「能读到数」即通过；不要用 250 ms 去证明时序正确。

---

## 2. ALS 数据与量程（0x0C/0x0D、0x10 bit5:4）— 已实现

mode=1 或 3。lux ≈ `raw * scale`（scale 单位 microlux/count，sysfs 为 `0.xxxxxx`）。

| ID | 步骤 | 期望 |
|----|------|------|
| A1 | `cat $IIO/in_illuminance_raw` | 0…65535 的整数。遮光应变小，照灯应变大。 |
| A2 | `cat $IIO/in_illuminance_scale` | 默认 range 00：`0.350000`（0.35 lux/count）。 |
| A3 | 依次写入 scale：`0.350000`、`0.078800`、`0.019700`、`0.004900` | 写回后再读 scale 一致。非法值（如 `0.1`）返回错误。 |
| A4 | 固定光照下改到更大量程（0.35）再读 raw | raw 应变小（分辨率变粗）。 |

---

## 3. ALS persist（0x10 bit3:0）— 已实现

用户写 **value**（1/4/8/12/16/60），不是寄存器码。

| ID | 步骤 | 期望 |
|----|------|------|
| P1 | `echo 1 > $IIO/als_persistence && cat $IIO/als_persistence` | `1`。 |
| P2 | 对 4、8、12、16、60 各做一次写读 | 读出与写入相同。 |
| P3 | `echo 2 > $IIO/als_persistence` | `-EINVAL`。 |
| P4 | 用 `i2cget` 看 0x10 低 4 位 | 1→`0x0`，4→`0x1`，16→`0x4`，60→`0xf`。 |

---

## 4. ALS 阈值与中断（0x1A–0x1D、0x01 bit0）— 已实现（方向不完整）

需要 DT `interrupts`。事件：`iio_event_monitor $IIO_DEV` 或读 `/dev/iio:deviceN`。

| ID | 步骤 | 期望 |
|----|------|------|
| E1 | `cat $IIO/events/in_illuminance_thresh_rising_value` | 默认高阈 0xFFFF。 |
| E2 | `cat $IIO/events/in_illuminance_thresh_falling_value` | 默认低阈 0。 |
| E3 | 写入新的 rising/falling，再读回 | 16-bit，一致。 |
| E4 | persist=1；把 **高阈** 调到当前 raw 以下，保持光照 | INT 拉低；`iio_event_monitor` 收到 thresh 事件。当前驱动 ALS 方向为 `either`，**不要**断言 rising。 |
| E5 | 把阈调回「当前值在窗内」，确认不再连报 | 清中断后应安静（persist 生效时更明显）。 |

---

## 5. PS / IR 数据（0x0A–0x0F）— 已实现

mode=2 或 3。PS 为 10-bit count，越近越大。手册无 mm 公式。

| ID | 步骤 | 期望 |
|----|------|------|
| S1 | `cat $IIO/in_proximity_raw` | 0…1023。手靠近变大，移开变小。 |
| S2 | `cat $IIO/in_intensity_ir_raw` | 0…1023 的 IR count。 |
| S3 | 强红外/阳光导致溢出 | `in_proximity_raw` 失败（驱动返回 `-ENODATA`）。可用 `i2cget` 看 0x0E bit6（IR_OF）。 |
| S4 | 确认无 `in_proximity_scale` | 预期**没有**该文件（未实现，且手册 TBD）。 |

---

## 6. PS 阈值与中断（0x2A–0x2D，默认 Mode 2）— 已实现

阈值公式：`value = high_byte * 4 + (low_byte & 0x3)`，10-bit。默认 Mode 2：OBJ 近/远翻转才中断。

| ID | 步骤 | 期望 |
|----|------|------|
| T1 | 读写 `events/in_proximity_thresh_rising_value`（高阈）和 `falling`（低阈） | 读回一致。建议 high > low，留滞回区。 |
| T2 | 手从远处贴近，越过**高阈** | 出 PS 事件。Mode 2 下 OBJ=1 为近。 |
| T3 | 手拿开，越过**低阈** | 再出 PS 事件。 |
| T4 | 在高低阈之间小幅晃动 | 不应出中断（滞回）。 |
| T5 | 写 `1024` 或更大（10-bit 最大 1023） | 当前驱动上限写成了 `0x3fff`，**1024 可能被错误接受**；记录实际行为，不作为通过条件。 |

已知问题（测试时记下，不阻断「有事件」）：IRQ 里 OBJ=0 被标成 RISING、OBJ=1 标成 FALLING，与「近=rising」惯例相反。T2/T3 用「有事件」判定，不要用方向 bit 当近/远。

---

## 7. remove / 掉电 — 已实现

| ID | 步骤 | 期望 |
|----|------|------|
| R1 | mode=3 工作时 `rmmod` | 成功。`i2cget 0x00` 为 0。再次 `insmod` 后 mode 仍为 0。 |

---

## 8. 未实现功能：预期「没有节点 / 不生效」

下列失败表示与当前实现一致，**不要当成回归**。若某项意外能用，再核对是否新做了功能。

| ID | 检查 | 当前期望 |
|----|------|----------|
| N1 | `$IIO` 下无 ALS calibration 属性；`i2cget 0x19` 保持默认 `0x40`（除非手工 i2cset） | 0x19 未实现 |
| N2 | 无 PS gain / 积分时间 / LED 电流 / mean time / wait / PS persist / PS cal 的 sysfs | 0x20–0x24、0x28–0x29 保持默认 |
| N3 | `i2cget 0x22` 为 `0x01`（Mode 2） | Mode 1 未开放 |
| N4 | 无 `scan_elements/`、`buffer/enable`，或 enable 失败 | 无 IIO buffer |
| N5 | 无 `in_illuminance_sampling_frequency` | 无 samp_freq |
| N6 | 无 `in_illuminance_calibscale` 一类 cal 属性 | ALS cal 未接 IIO |
| N7 | 无 `events/in_illuminance_thresh_rising_en`（或写 en 无效） | 无 event enable |
| N8 | ALS 事件方向不是稳定的 rising/falling | 6.3，push `either` |

---

## 9. 建议的最小冒烟集（板上 15 分钟）

1. C2、C3  
2. M1、A1、A2、A3  
3. P1、P2  
4. M3、S1、S2  
5. E3、E4（有 INT 脚时）  
6. T1、T2（有 INT 脚时）  
7. R1  

无 INT 脚：跳过第 4、5、6 节的中断项，sysfs 读写仍要做。

---

## 附录：Linux 5.1 编译结论

本机**没有** 5.1 内核树，无法执行 `make -C <linux-5.1> M=src modules`。结论来自：

- 已核对 [linux v5.1 `include/linux/sysfs.h`](https://raw.githubusercontent.com/torvalds/linux/v5.1/include/linux/sysfs.h)：无 `sysfs_emit`  
- 已核对 v5.1 `include/linux/device.h`：无 `dev_err_probe`  
- Linux 5.1 Kbuild：`-std=gnu89`，并打开 `-Werror=declaration-after-statement`  
- 用 `gcc -std=gnu89` 对与驱动相同的写法做了片段编译，确定会失败  

**在 5.1 上不能编译成功。** 至少会碰到：

| 问题 | 位置 | 原因 |
|------|------|------|
| `sysfs_emit` 未声明 | `ap3216c_show_mode`、`ap3216c_show_als_persistence` | 5.10 才引入；5.1 应改用 `scnprintf(buf, PAGE_SIZE, …)` |
| `dev_err_probe` 未声明 | `ap3216c_probe` | 5.6 才引入；5.1 应改用 `dev_err` + `return ret` |
| `for (int i = …)` | `write_als_scale_microlux`、`ap3216c_store_als_persistence` | gnu89 不允许 for 内声明 |
| `if (…) { return; }` 之后再 `int ret;` | `ap3216c_probe` | gnu89 不允许声明夹在语句后 |

`FIELD_GET` / `FIELD_PREP` / `FIELD_FIT` 在 5.1 的 `bitfield.h` 里已有。  
`probe(struct i2c_client *, const struct i2c_device_id *)` 和 `int remove(...)` 与 **5.1 匹配**。

RK3568 树是 **5.10.160**（同样 `gnu89`）：`sysfs_emit` / `dev_err_probe` 可用，但 **`for (int i)` 和 probe 中途声明仍会编不过**。要在 5.10 上编过，至少先改这两处 C90 问题。

本机 6.12 头文件、`gnu11` 下对 `ap3216c.c` 做了语法检查：驱动逻辑能过；但 6.12 的 `i2c_driver.probe` 已改为单参数、`remove` 改为 `void`，与当前代码不兼容（那是 6.x，不是 5.1）。
