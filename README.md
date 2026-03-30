# 🚀 TinyRender: 实时 C++ 软渲染引擎

**TinyRender** 是一个从零开始实现的轻量级 CPU 软渲染引擎。它不依赖现代图形 API（如 OpenGL 或 DirectX），通过纯 C++ 模拟了完整渲染管线的数学计算与逻辑实现。

---

## 🌟 核心特性

- **双趟阴影映射 (Two-Pass Shadow Mapping)**：支持实时动态阴影，通过光源视角深度图实现真实遮挡效果。
- **高性能多线程**：集成线程池技术，充分利用 CPU 多核性能，支持实时交互。
- **轨道相机 (Orbit Camera)**：支持鼠标旋转、平移和缩放，提供类似 3D 建模软件的交互体验。
- **高级着色模型**：
  - Blinn-Phong 反射模型
  - 纹理贴图 (Texture Mapping) & 法线贴图 (Normal Mapping)
  - 动态位移贴图 (Displacement Mapping)
- **视觉优化**：支持 SSAA 抗锯齿、背面剔除 (Back-face Culling) 和透视矫正插值。

---

## 🎮 交互控制 (快捷键)

| 按键 | 功能描述 |
| :--- | :--- |
| **`M`** | 切换 **多线程加速** (推荐开启) |
| **`S`** | 切换 **实时阴影 (Shadows)** |
| **`A`** | 切换 **SSAA 抗锯齿** |
| **`←` / `→`** | 切换渲染模型 (Cow, African Head, Monster等) |
| **`↑` / `↓`** | 切换片元着色器 (Phong, Texture, Normal等) |
| **鼠标左键** | 旋转相机视角 |
| **鼠标滚轮** | 缩放 (Zoom) |

---

## 🛠️ 构建与运行

1. **环境依赖**：OpenCV 4.x (用于显示窗口)、C++17/20 编译器。
2. **构建方式**：
   ```bash
   mkdir build && cd build
   cmake ..
   make -j8
