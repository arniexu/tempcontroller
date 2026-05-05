# 固件调参与 UI 路由测试报告

## 1. 基本信息
- 报告时间: 2026-05-05 16:26:41 CST
- 工作目录: /home/xuqj/temperature-controller/firmware/tests
- 目标分支: hal
- 测试类型: Host 侧窄链路单元测试（单入口）

## 2. 测试范围
本次报告聚焦近期改动相关的两部分：
1. tune_service 业务逻辑闭环与边界行为
2. ui_service 三模式键路由（普通/引导/专家）与长按/重复按键组合

## 3. 执行命令
### 3.1 tune_service
gcc -std=c11 -Wall -Wextra -Werror -I../Core/Inc -I. ../Core/Src/tune_service.c test_tune_service.c test_single_main.c -DHOST_SINGLE_TEST=test_tune_service_run -o test_tune_service_bin && ./test_tune_service_bin; printf 'RC=%s\n' "$?"

结果: RC=0

### 3.2 ui_service
gcc -std=c11 -Wall -Wextra -Werror -Wno-error=unused-variable -Wno-error=unused-function -I../Core/Inc -I../Drivers/BSP -I. ../Core/Src/ui_service.c ../Core/Src/tune_service.c ../Core/Src/param_store.c ../Core/Src/log_service.c ../Drivers/BSP/bsp_key.c ../Drivers/BSP/bsp_lcd_8080.c ../Drivers/BSP/bsp_spiflash.c test_ui_service.c test_single_main.c -DHOST_SINGLE_TEST=test_ui_service_run -o test_ui_service_bin && ./test_ui_service_bin; printf 'RC=%s\n' "$?"

结果: RC=0

## 4. 覆盖摘要
### 4.1 tune_service 覆盖到的逻辑
- Profile 元数据与场景默认参数
- 常见问题分类与动作映射:
  - SLOW -> KP_UP
  - OVERSHOOT -> KP_DOWN
  - WAVE -> KI_DOWN
  - STEADY_ERROR -> KI_UP
  - OUTPUT_SATURATED -> CHECK_HEATER
  - SENSOR_FAULT -> CHECK_SENSOR
  - ACCEPT 阶段 OK -> ENTER_ACCEPT
- 建议应用后的参数变化正确性
- step/issue 上下边界夹紧行为
- 参数与温度目标上下限夹紧
- tune profile 反向边界输入修正（set_profile 修正 min/max 关系）

### 4.2 ui_service 覆盖到的逻辑
- 普通模式:
  - 目标温度调节（含重复键）
  - 场景切换与 profile 文案映射
  - SET_LONG 进入/退出编辑
- 引导模式:
  - SET 切换模式
  - BACK 行切换与 4 行回绕
  - 行 0 step 调整（含重复键）
  - 行 1 issue 切换
  - 行 2 SET_LONG 应用建议动作
- 专家模式:
  - SET 切换模式
  - SET_LONG 专家分页切换
  - 第 1 页 6 行回绕
  - 第 2 页 5 行回绕
  - UP_REPEAT/DOWN_REPEAT 对参数与边界字段的作用
  - KD 上限连续增量后的上限保护（30.0）

## 5. 结果判定
- 结论: 通过
- 关键指标: 两个目标测试入口均返回 RC=0

## 6. 注意事项
ui_service 窄链路测试编译过程中出现既有 warning（非本次新增逻辑引入），因此在该命令中放宽了两类 warning 的 error 处理:
- unused-variable（param_store.c）
- unused-function（bsp_lcd_8080.c）

这不影响本次改动逻辑的断言执行结果，但建议后续统一清理既有 warning，以便恢复全量 -Werror。

## 7. 关联文件
- firmware/tests/test_tune_service.c
- firmware/tests/test_ui_service.c
- firmware/tests/test_single_main.c
- firmware/Core/Src/tune_service.c
- firmware/Core/Src/ui_service.c
