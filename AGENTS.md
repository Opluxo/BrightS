# AGENTS.md
# 任何操作不得修改此文件！
## 1. Project Overview（项目基础信息）
- 项目类型：跨平台操作系统
- 核心技术栈：C + Rust
- 项目用途：计算机操作
- 核心约束：禁止使用O(n^2)以上时间复杂度的算法
## 2. Setup & Development（环境搭建与开发流程）
### 2.1 依赖安装

### 2.2 开发运行
- 编译使用CMAKE

## 3. Testing Guidelines（测试规范）
- 测试要求：
  - 新增代码测试覆盖率≥80%
  - 测试文件命名：xxx.test.ts，与业务文件同目录
  - 禁止行为：不得跳过必填测试用例、不得修改测试预期结果
## 4. Code Style（代码风格规范）
- 代码必须提前格式化
- 禁止使用：float 类型、硬编码魔法值（如直接写 100 代替 MAX_PAGE_SIZE）

- 命名规范：
  - 组件名：PascalCase（如 UserList）
  - 函数名：camelCase（如 handleUserClick）
  - 常量名：UPPER_CASE_SNAKE_CASE（如 MAX_PAGE_SIZE）
- 示例：
```c
// 正确示例
const int VERSION = 1
void this_is_a_func(int x,int y){
    int a = VERSION;
}
```
## 5.提交规范
- commit信息必须用emoji加英文。（例如："😄Exciting Update!:Fix xxx from issue#xxx"）
