# Visual Studio 配置
## 基础配置(启用modules)
- 项目 -> 属性  -> C,C++ -> 语言 -> C++语言标准 -> 最新
- 项目 -> 属性  -> C,C++ -> 语言 -> 启用实验性的C++标准库模块
- 项目 -> 属性  -> C,C++ -> 语言 -> 生成ISO C++23 标准库模块
- 项目 -> 属性  -> C,C++ -> 常规 -> 扫描源以查找模块依赖关系

## 添加第三方库方法
- 项目 -> 属性  -> C,C++ -> 常规 -> 添加包含目录 (头文件目录)
- 项目 -> 属性  -> 链接器 -> 常规 -> 添加库目录 (静态库目录)
- 项目 -> 属性 -> 链接器 -> 输入 -> 附加依赖项 (具体链接的静态库文件名称)
# 第三方库
## glfw3

[glfw/glfw: A multi-platform library for OpenGL, OpenGL ES, Vulkan, window and input (github.com)](https://github.com/glfw/glfw)

从github中下载release（我这里下载了win64）

> GLFW是一个用C语言编写的库，专门针对OpenGL。GLFW为我们提供了将好东西呈现到屏幕上所需的基本必需品。它允许我们创建OpenGL上下文、定义窗口参数和处理用户输入，这对于我们的目的来说已经足够了。

我们需要的是：

- include中的头文件， 将其移到项目目录的include目录下
- 对应编译环境的库文件（vs2022）
  - glfw3.lib

## Vulkan
在这个网站中下载vulkan的sdk并安装：
https://vulkan.lunarg.com/sdk/home 

- include目录：头文件
- Lib目录：静态库（需要使用其中的vulkan-1.lib）