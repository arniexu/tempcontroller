WaterTempControl v1.0.2 二进制发布包

一、版本说明
本版本在 v1.0.1 基础上，重点修复以下现场问题：
1) OLED 字符集补全：新增 Z 与 + 字形，修复 ACT 页面 BUZ 与 BUZ+RLY 显示异常。
2) 按键映射固定：SET 固定为 PA13，DOWN 固定为 PA15（按当前硬件要求保持不变）。

二、发布文件
- WaterTempControl.axf
- WaterTempControl.map
- WaterTempControl.bin（如本次构建生成）
- WaterTempControl.hex（如本次构建生成）
- SHA256SUMS.txt

三、完整性校验
对本目录文件执行 SHA-256 校验，并与 SHA256SUMS.txt 对比。
