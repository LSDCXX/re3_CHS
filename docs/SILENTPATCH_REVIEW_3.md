# SilentPatch III 第三轮移植 / Third native adaptation

来源 / Source: [SilentPatch III dev](https://github.com/CookiePLMonster/SilentPatch/blob/74976544adc196fa664831ddc59778e684cf6af0/SilentPatchIII/SilentPatchIII.cpp), revision `74976544adc196fa664831ddc59778e684cf6af0`.

本轮五项均置于 `FIX_BUGS` 下，无需额外游戏资源，不改变存档格式。
All five changes are guarded by `FIX_BUGS`, require no additional assets and preserve the save format.

| 修复 / Fix | 实现 / Implementation |
|---|---|
| 拾取物槽位复用泄漏 / Pickup slot reuse leak | 替换临时拾取物前移除并释放旧的世界对象。Remove and delete the previous world object before reusing its pickup slot. |
| 清除任务音频后仍播放 / Mission audio continues after clearing | 同时停止流式任务音频及 re3 专用任务音频通道，再清理状态。Stop the mission stream and re3's dedicated mission channel before resetting state. |
| 音效反射污染后续声源 / Reflections alter subsequent sound sources | 生成反射后恢复原始声源位置与距离。Restore source position and distance after reflection generation. |
| Decoy 路障车辆优先级 / Decoy roadblock priority | 优先选择 SWAT，再 FBI、Army，保留模型未加载时的警车回退。Prefer SWAT, then FBI, then Army; retain the unloaded-model fallback. |
| 车辆掩体位置 / Vehicle cover position | 以前轮中点为基准，向远离威胁方向偏移 1.5；仅选择具有两个前轮节点的汽车。Use the front-wheel midpoint, offset 1.5 away from the threat; require an automobile with both front-wheel nodes. |

按用户要求直接上传，仅进行源码审阅与差异格式检查；本轮未编译、未运行自动测试或游戏测试。
Uploaded as requested after source review and diff-format checks only; this batch has not been compiled or tested automatically/in game.

SilentPatch attribution and MIT license: [SilentPatch-LICENSE.txt](SilentPatch-LICENSE.txt).
