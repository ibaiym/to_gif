# to_gif — 智能 GIF 优化器

<div align="center">

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![CMake](https://img.shields.io/badge/CMake-≥3.16-green.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
![Ubuntu](https://img.shields.io/badge/Ubuntu-22.04-orange.svg)
![Fedora](https://img.shields.io/badge/Fedora-latest-blue.svg)
![Arch](https://img.shields.io/badge/Arch-linux-1793d1.svg)

**自动在目标文件大小限制下寻找最佳质量的 GIF 转换工具**

to_gif 是一个智能的 GIF 优化工具，它能够根据用户设定的目标文件大小，自动搜索最佳的分辨率、帧率和颜色参数组合，以生成质量最优的 GIF 动画。该工具采用预测性搜索算法，基于体积-分辨率反平方模型快速收敛到最优参数，支持视频裁剪、全局调色板优化等功能。

</div>

---

## 效果展示

### 不同文件大小限制下的质量对比

以下是使用 to_gif 将同一段视频（12-17秒，共5秒）转换为不同大小限制的 GIF 效果对比：

**视频来源**: [Pexels - Splashing Waves in Slow Motion](https://www.pexels.com/video/splashing-waves-in-slow-motion-14979266/)

#### 10 MB 限制
![10MB GIF](data/waves_10m.gif)
- **实际大小**: 9.95 MB
- **分辨率**: 454 × 256
- **帧率**: 20 fps
- **命令**: `./build/to_gif -i data/waves.mp4 -o data/waves_10m.gif --limit 10 --quality 100 --ss 12 --to 17`

#### 5 MB 限制
![5MB GIF](data/waves_5m.gif)
- **实际大小**: 4.99 MB
- **分辨率**: 320 × 180
- **帧率**: 19 fps
- **命令**: `./build/to_gif -i data/waves.mp4 -o data/waves_5m.gif --limit 5 --quality 100 --ss 12 --to 17`

#### 3 MB 限制
![3MB GIF](data/waves_3m.gif)
- **实际大小**: 2.89 MB
- **分辨率**: 320 × 180
- **帧率**: 11 fps
- **命令**: `./build/to_gif -i data/waves.mp4 -o data/waves_3m.gif --limit 3 --quality 100 --ss 12 --to 17`

#### 1 MB 限制（测试失败）
```
[ERROR] Even minimum parameters (320px, 5fps) cannot compress below 1.000000MB.
Suggestion: trim video duration or relax the limit.
```
> **说明**: 由于视频内容复杂度较高（海浪慢动作），即使使用最小参数也无法压缩到 1MB 以下。如需更小的文件，建议缩短时长或降低颜色数。

### 智能优化效果

to_gif 的预测性搜索算法会自动在目标大小限制下寻找最佳质量参数：

```
原始视频: 1920x1080, 24fps, 5秒片段
优化策略: 基于体积-分辨率反平方模型 (size ∝ width² × fps)
搜索结果: 自动调整分辨率、帧率、调色板以达到目标大小
```

**实际运行示例**:

```bash
# 10MB 限制下的优化结果
[INFO] Probe (max: 720px, 20fps): 22.5661 MB
[INFO] Locked best resolution: 454px @ 20fps
[INFO] Size: 9.95353 MB (target: 10 MB)
[INFO] Final params: width=454, height=256, fps=20, colors=256, quality=100

# 5MB 限制下的优化结果  
[INFO] Resolution already reduced to minimum (320px) still exceeds limit, reducing fps...
[INFO] Size: 4.98849 MB (target: 5 MB)
[INFO] Final params: width=320, height=180, fps=19, colors=256, quality=100

# 3MB 限制下的优化结果
[INFO] Resolution already reduced to minimum (320px) still exceeds limit, reducing fps...
[INFO] Size: 2.89153 MB (target: 3 MB)
[INFO] Final params: width=320, height=180, fps=11, colors=256, quality=100
```

### 快速体验

#### 基础用法

```bash
# 限制 5MB，自动优化参数
./build/to_gif -i input.mp4 -o output.gif --limit 5

# 截取视频片段（12-17秒），限制 3MB
./build/to_gif -i input.mp4 -o output.gif --limit 3 --ss 12 --to 17
```

#### 完整测试示例

使用项目内置视频进行不同大小限制的测试：

```bash
# 项目已包含示例视频 data/waves.mp4（来自 Pexels）
# 视频来源：https://www.pexels.com/video/splashing-waves-in-slow-motion-14979266/

# 测试不同文件大小限制（截取 12-17 秒片段）
./build/to_gif -i data/waves.mp4 -o waves_10m.gif --limit 10 --quality 100 --ss 12 --to 17
./build/to_gif -i data/waves.mp4 -o waves_5m.gif --limit 5 --quality 100 --ss 12 --to 17
./build/to_gif -i data/waves.mp4 -o waves_3m.gif --limit 3 --quality 100 --ss 12 --to 17

# 自定义参数：限制帧率和颜色数
./build/to_gif -i data/waves.mp4 -o waves_custom.gif --limit 3 --max-fps 15 --colors 128 --quality 90
```

#### 其他测试方法

```bash
# 使用 FFmpeg 生成测试视频（完全无版权问题）
ffmpeg -f lavfi -i testsrc=duration=5:size=640x480:rate=25 test.mp4
./build/to_gif -i test.mp4 -o test.gif --limit 2

# 使用项目内置示例视频
./build/to_gif -i data/oceans.mp4 -o output.gif --limit 3
```

> **提示**: 
> - 示例视频来自 [Pexels](https://www.pexels.com/)，采用 [Pexels License](https://www.pexels.com/license/)，可免费用于商业和非商业用途
> - GIF 文件是通过 to_gif 工具从原始视频转换而来，属于二次创作内容
> - 建议使用自己的视频或从免费素材网站获取
> - 如需生成完全无版权的测试视频，可使用 FFmpeg 的 `testsrc` 滤镜

### 示例素材说明

本 README 中使用的示例视频和 GIF：

- **视频标题**: Splashing Waves in Slow Motion
- **来源平台**: [Pexels](https://www.pexels.com/)
- **视频链接**: https://www.pexels.com/video/splashing-waves-in-slow-motion-14979266/
- **许可协议**: [Pexels License](https://www.pexels.com/license/)
- **使用说明**: GIF 文件是通过 to_gif 工具对原始视频进行格式转换和参数优化生成的二次创作内容

---

## 特性

- **智能参数搜索**：基于体积-分辨率反平方模型 (`size ∝ width² × fps`) 的预测性搜索策略，快速收敛到最优参数组合
- **全局调色板优化**：使用 `libimagequant` 进行跨帧全局调色板生成与抖动量化
- **视频裁剪支持**：支持指定起止时间截取视频片段
- **可配置质量边界**：自定义分辨率、帧率、调色板颜色数、量化质量等参数范围
- **模块化架构**：基于接口的流水线设计，各组件可独立替换

## 技术栈

| 组件 | 库 | 职责 |
|------|------|------|
| 视频解码 | FFmpeg (libavcodec/format/swscale/avutil) | 解码视频、提取帧、缩放 |
| 颜色量化 | libimagequant | 全局调色板优化、抖动量化 |
| GIF 编码 | giflib | LZW 压缩、GIF 文件写入 |
| 构建系统 | CMake ≥ 3.16 | C++17 |

## 架构设计

```
┌─────────────────┐     ┌──────────────────┐     ┌─────────────────┐
│   IDecoder      │────▶│   IQuantizer     │────▶│   IEncoder      │
│  FFmpegDecoder  │     │ ImageQuantQuant  │     │ GiflibEncoder   │
│  (解码/缩放)     │     │ (调色板/量化)     │     │ (LZW/GIF写入)   │
└─────────────────┘     └──────────────────┘     └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 ▼
                    ┌──────────────────────┐
                    │    GifPipeline       │  ← Builder 模式组装
                    │   (流水线编排)        │
                    └──────────────────────┘
                                 ▲
                                 │
                    ┌──────────────────────┐
                    │ ParameterOptimizer   │  ← 策略模式注入搜索策略
                    │  (参数优化器)         │
                    └──────────────────────┘
                                 │
                    ┌────────────┴────────────┐
                    ▼                         ▼
           ┌─────────────────┐      ┌─────────────────┐
           │ ISearchStrategy │      │  PredictiveSearch│
           │    (接口)        │      │   (预测搜索实现)  │
           └─────────────────┘      └─────────────────┘
```

**设计模式**

- **Strategy**：`ISearchStrategy` 定义参数搜索策略，支持替换不同的搜索算法
- **Builder**：`GifPipeline` 链式组装解码器、量化器、编码器
- **Dependency Injection**：流水线依赖 `IDecoder` / `IQuantizer` / `IEncoder` 接口，而非具体实现
- **Factory**：`IPipelineFactory` 负责创建流水线实例，解耦对象创建与使用

## 安装与使用

### 安装依赖

```bash
./scripts/install_dependencies.sh
```

支持 Ubuntu/Debian、Fedora、Arch 及其衍生发行版。

### 编译

```bash
# Release 模式（推荐）
./scripts/compile.sh

# Debug 模式
./scripts/compile.sh --debug

# 清理并重新编译
./scripts/compile.sh --clean

# 指定并行编译任务数
./scripts/compile.sh -j 8
```

编译完成后，可执行文件位于 `build/to_gif`。

### 运行测试

```bash
# 编译并运行所有测试
cd build && make to_gif_tests && ./to_gif_tests

# 或使用 CTest（CMake 内置测试工具）
cd build && ctest --output-on-failure

# 运行特定测试用例
./to_gif_tests --gtest_filter="PredictiveSearch.*"
```

项目使用 **Google Test** 框架，包含以下测试模块：
- `test_predictive_search` - 预测搜索算法测试
- `test_parameter_optimizer` - 参数优化器测试
- `test_gif_pipeline` - GIF 流水线测试
- `test_file_utils` - 文件工具测试
- `test_logger` - 日志系统测试
- `test_math_utils` - 数学工具测试
- `test_config_validate` - 配置验证测试

### 使用

```bash
./build/to_gif -i <input_video> -o <output.gif> --limit <MB> [options]
```

**必填参数**

| 参数 | 说明 |
|------|------|
| `-i, --input <video>` | 输入视频文件路径 |
| `-o, --output <gif>` | 输出 GIF 文件路径 |
| `--limit, -l <MB>` | 目标文件大小限制（MB）|

**可选参数**

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--max-width` | 720 | 最大输出宽度（像素） |
| `--min-width` | 320 | 最小输出宽度（像素） |
| `--max-fps` | 20 | 最大输出帧率 |
| `--min-fps` | 5 | 最小输出帧率 |
| `--colors` | 256 | 调色板颜色数（**有效值：64 / 128 / 192 / 256**，其他值将自动校正到最近的有效值） |
| `--quality` | 100 | libimagequant 量化质量（1-100） |
| `--ss <秒>` | - | 裁剪起始时间 |
| `--to <秒>` | - | 裁剪结束时间 |
| `--verbose, -v` | - | 启用调试日志输出 |
| `-h, --help` | - | 显示帮助信息 |

**示例**

```bash
# 基础用法：限制 5MB
./build/to_gif -i input.mp4 -o output.gif --limit 5

# 限制 3MB，同时约束帧率和颜色数
./build/to_gif -i input.mp4 -o output.gif --limit 3 --max-fps 15 --colors 128

# 截取视频 2-8 秒片段，限制 2MB，质量 90
./build/to_gif -i input.mp4 -o output.gif --limit 2 --ss 2 --to 8 --quality 90
```

## 工作原理

### 数学直觉

GIF 文件大小主要受以下因素影响：

```
size ∝ width × height × fps × colors × duration
```

由于保持宽高比，`height ∝ width`，因此简化为：

```
size ∝ width² × fps × C
```

其中 `C` 是与内容复杂度、颜色数、压缩率相关的常数。

**核心思想**：
- GIF 的 LZW 压缩对连续帧有较好的压缩效果
- 分辨率对大小的影响是**平方级**的（width²）
- 帧率对大小的影响是**线性**的（fps）
- 因此，降低分辨率比降低帧率能更有效地控制文件大小

### 搜索策略

1. **参数预测**：工具首先以最大允许分辨率和帧率生成一个试转换 GIF，根据其实际大小与目标大小的比例，利用反平方模型预测下一组更优参数。
2. **迭代收敛**：在 `[min_width, max_width]` 和 `[min_fps, max_fps]` 范围内迭代搜索，每次根据前一次结果调整参数，直到生成的大小最接近且不超出目标限制。
3. **流水线执行**：每次迭代中，`GifPipeline` 按顺序执行解码 → 量化 → 编码三步，生成候选 GIF。

## 贡献

欢迎提交 Issue 和 Pull Request！

- **报告 Bug**：请创建 Issue 并附上复现步骤
- **功能建议**：欢迎提出新功能想法
- **代码贡献**：Fork 项目并提交 PR

## 许可证

本项目采用 [MIT License](LICENSE) 开源协议。

## 技术依赖

- CMake ≥ 3.16
- C++17 编译器
- FFmpeg 开发库（libavcodec, libavformat, libswscale, libavutil）
- giflib
- libimagequant

## 免责声明

本项目是作者学习视频处理与 GIF 优化技术的个人练习作品。

### 使用条款

- **自由使用**：欢迎任何人自由使用、修改、分享本项目的代码
- **无担保声明**：代码按"现状"提供，不提供任何形式的明示或暗示担保
- **责任限制**：作者不对因使用本项目代码而产生的任何直接或间接损失承担责任

### 滥用举报

如果您发现有人利用本项目代码从事违法活动（如诈骗、侵权等），欢迎向作者举报。但请注意，作者不对第三方的滥用行为承担任何责任。

### 商业使用

- 允许在商业产品中使用本项目代码
- 不强制要求署名，但如果您愿意标注来源，作者将非常感激
- 建议遵守项目采用的 MIT 许可证要求

详细许可证信息请查看 [LICENSE](LICENSE) 文件。

---

<div align="center">

Made with ❤️ by to_gif contributor

</div>
