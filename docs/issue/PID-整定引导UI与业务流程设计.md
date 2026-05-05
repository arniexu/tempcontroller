# PID 整定引导 UI 与业务流程设计

适用对象：
- PID 算法专家
- UI 设计工程师
- 单片机固件工程师

适用原型：
- [docs/issue/lcd-ui-preview.html](/home/xuqj/temperature-controller/docs/issue/lcd-ui-preview.html)

关联现状：
- [firmware/Core/Inc/ui_service.h](/home/xuqj/temperature-controller/firmware/Core/Inc/ui_service.h)
- [firmware/Core/Inc/app_state.h](/home/xuqj/temperature-controller/firmware/Core/Inc/app_state.h)

## 1. 目标

把 PID 参数整定从“手工记忆公式和经验”改成“设备分步提示下一步操作”的业务流程。

最终用户在小屏设备上不直接面对多种整定方法名称，而是只面对三种使用方式：

1. 普通模式：只看目标温度和场景，不直接调 PID。
2. 引导整定：设备告诉用户当前步骤、当前现象、建议动作、下一步。
3. 专家模式：工程人员直接改 KP、KI、KD 与边界参数。

## 2. 5 键约束

实体按键固定为 5 个：

1. 模式/切换
2. 上
3. 下
4. 减
5. 加

约束原则：

1. 不新增专用分页键。
2. 专家模式分页复用“模式/切换”长按或双击行为。
3. 引导整定页面中，用户只能在单行焦点上执行一类动作，避免二义性。

## 3. 角色分工

### 3.1 PID 算法专家

负责定义：

1. 场景默认参数。
2. KP、KI、KD 取值边界。
3. 现象识别规则。
4. 对应建议动作。
5. 每一步验收标准。

输出物：

1. 场景参数表。
2. 建议规则表。
3. 验收阈值表。

### 3.2 UI 设计工程师

负责定义：

1. 三模式页面布局。
2. 引导整定页面显示骨架。
3. 告警、限幅、成功提示的视觉反馈。
4. 5 键在不同页面下的行为映射。

输出物：

1. 页面流。
2. 中文文案。
3. 状态提示规范。

### 3.3 单片机固件工程师

负责实现：

1. 调参状态机。
2. 参数服务与存储。
3. 建议引擎。
4. UI 与业务状态同步。
5. 日志与验收指标输出。

输出物：

1. 状态机代码。
2. 业务数据结构。
3. UI 服务接口。
4. 记录与导出接口。

## 4. 总体业务流

```text
上电
 -> 自检
 -> 首页
 -> 选择场景
 -> 选择模式（普通 / 引导 / 专家）
 -> 载入默认 PID 与边界
 -> 进入对应流程

引导整定流程：
预热检查
 -> 逼近目标
 -> 稳态观察
 -> 扰动恢复
 -> 结果验收
 -> 保存为场景参数或放弃
```

## 5. 引导整定步骤定义

### 5.1 预热检查

目标：确认系统具备基础升温能力。

显示内容：

1. 当前步骤序号。
2. 当前观察指标：升温斜率。
3. 用户观察到的现象。
4. 建议动作。

允许判断：

1. 正常。
2. 升温慢。

建议动作：

1. 升温慢时优先增大 KP。
2. 若输出已长时间打满，则提示检查加热能力而不是继续加参数。

### 5.2 逼近目标

目标：观察是否出现明显超调。

显示内容：

1. 峰值温度。
2. 当前目标温度。
3. 当前现象。
4. 建议动作。

允许判断：

1. 正常。
2. 升温慢。
3. 超调大。
4. 波动大。

建议动作：

1. 超调大：降低 KP 或提高 KD。
2. 升温慢：提高 KP，必要时轻微提高 KI。

### 5.3 稳态观察

目标：评估静差与持续波动。

允许判断：

1. 正常。
2. 超调大。
3. 波动大。
4. 静差大。

建议动作：

1. 静差大：提高 KI。
2. 波动大：降低 KI 或降低 KP。
3. 超调残留：提高 KD。

### 5.4 扰动恢复

目标：确认开盖、加水或负载扰动后的恢复能力。

显示内容：

1. 扰动后最低温度或偏差。
2. 恢复时间。
3. 建议动作。

建议动作：

1. 恢复慢：提高 KP。
2. 二次过冲：提高 KD。
3. 恢复中振荡：降低 KP 或 KI。

### 5.5 结果验收

显示内容：

1. 最大超调。
2. 稳态误差。
3. 达标时间。
4. 验收结论。

出口动作：

1. 保存到当前场景默认参数。
2. 保存为专家自定义参数。
3. 放弃本次修改。

## 6. 建议引擎

建议引擎不是公式展示器，而是症状到动作的规则层。

### 6.1 输入

1. 当前温度。
2. 目标温度。
3. 融合温度斜率。
4. 最大超调。
5. 稳态误差。
6. 波动幅度。
7. 输出占空比或继电器导通时间。
8. 当前 KP、KI、KD。
9. 当前整定步骤。

### 6.2 中间指标

1. 升温速度是否低于阈值。
2. 是否发生过冲。
3. 稳态误差是否持续存在。
4. 是否存在多次零点穿越。
5. 输出是否长时间饱和。

### 6.3 输出

1. 现象分类。
2. 建议动作。
3. 调节幅度。
4. 下一步是否允许继续。
5. 当前结论文案。

## 7. UI 页面流

建议在现有页面体系上新增调参相关页面族，而不是把所有内容硬塞进 `UI_PAGE_PID`。

建议的页面族：

1. 首页
2. 普通模式页
3. 引导整定页
4. 专家参数页 P1
5. 专家参数页 P2
6. 验收结果页
7. 报警/故障页

页面层级说明：

1. 首页是运行入口。
2. 引导整定页是主流程页。
3. 专家页只服务工程人员。
4. 验收结果页是保存前唯一出口。

## 8. 5 键行为映射

### 8.1 普通模式

1. 模式/切换：切到引导模式。
2. 上/下：移动字段焦点。
3. 加/减：修改目标温度或场景。

### 8.2 引导整定

1. 模式/切换：退出引导或进入下一页面族。
2. 上/下：切换“步骤/指标/现象/动作/结果”焦点。
3. 加/减：
   - 在“步骤”行：切换步骤。
   - 在“现象”行：切换现象判断。
   - 在“动作”行：应用建议。

### 8.3 专家模式

1. 模式/切换短按：在普通/引导/专家之间切换。
2. 模式/切换长按：专家页 P1/P2 之间切换。
3. 上/下：移动参数焦点。
4. 加/减：修改当前参数值。

## 9. 固件状态机建议

在 [firmware/Core/Inc/app_state.h](/home/xuqj/temperature-controller/firmware/Core/Inc/app_state.h) 现有 `app_mode_t` 之外，增加一层“调参流程状态机”。

建议新增：

```c
typedef enum
{
    TUNE_FLOW_IDLE = 0,
    TUNE_FLOW_READY,
    TUNE_FLOW_PREHEAT,
    TUNE_FLOW_APPROACH,
    TUNE_FLOW_STABLE,
    TUNE_FLOW_DISTURB,
    TUNE_FLOW_ACCEPT,
    TUNE_FLOW_SAVE,
    TUNE_FLOW_ABORT
} tune_flow_state_t;
```

建议新增“现象分类”：

```c
typedef enum
{
    TUNE_ISSUE_OK = 0,
    TUNE_ISSUE_SLOW,
    TUNE_ISSUE_OVERSHOOT,
    TUNE_ISSUE_WAVE,
    TUNE_ISSUE_STEADY_ERROR,
    TUNE_ISSUE_OUTPUT_SATURATED,
    TUNE_ISSUE_SENSOR_FAULT
} tune_issue_t;
```

建议新增“动作建议”：

```c
typedef enum
{
    TUNE_ACTION_NONE = 0,
    TUNE_ACTION_KP_UP,
    TUNE_ACTION_KP_DOWN,
    TUNE_ACTION_KI_UP,
    TUNE_ACTION_KI_DOWN,
    TUNE_ACTION_KD_UP,
    TUNE_ACTION_CHECK_HEATER,
    TUNE_ACTION_CHECK_SENSOR,
    TUNE_ACTION_ENTER_ACCEPT
} tune_action_t;
```

## 10. 数据结构建议

```c
typedef struct
{
    float kp;
    float ki;
    float kd;
    float kp_min;
    float kp_max;
    float ki_min;
    float ki_max;
    float kd_min;
    float kd_max;
} tune_pid_profile_t;

typedef struct
{
    tune_flow_state_t flow;
    tune_issue_t issue;
    tune_action_t action;
    uint8_t scene_id;
    uint8_t step_index;
    uint8_t accept_ready;
    float overshoot;
    float steady_error;
    float rise_rate;
    float recovery_time_s;
    char advice_text[24];
} tune_runtime_t;
```

## 11. 服务边界建议

建议把职责拆成三层：

1. `temp_manager`
   - 提供温度快照与统计窗口数据。
2. `pid_ctrl`
   - 提供当前 PID 参数应用接口。
3. 新增 `tune_service`
   - 维护调参流程状态。
   - 生成建议。
   - 输出 UI 所需的步骤、现象、建议文本、验收结果。

UI 层只消费 `tune_service` 的输出，不直接推导控制逻辑。

## 12. 建议的最小落地顺序

第一阶段：

1. 固定 3 模式 UI：普通 / 引导 / 专家。
2. 固定 5 个引导步骤。
3. 固定 5 类常见现象。
4. 固定 6 类建议动作。

第二阶段：

1. 让建议引擎从实时数据自动判定现象。
2. 记录验收指标。
3. 保存到场景参数表。

第三阶段：

1. 支持更细的扰动试验。
2. 支持多种场景模板。
3. 支持导出调参报告。

## 13. 当前原型对应关系

当前 HTML 原型已经体现：

1. 普通模式：目标温度与场景。
2. 引导模式：步骤、指标、现象、动作、结果。
3. 专家模式：参数与边界分页。

因此后续单片机实现建议按原型字段命名靠拢，减少 UI 与固件沟通成本。