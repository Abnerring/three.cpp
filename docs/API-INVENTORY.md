# 移植范围与 API 清单

`utils/port-audit.mjs` 对照 `upstream.json` 锁定的 commit、package version，以及实际源码内容 SHA-256。上游 `src`、`examples/jsm` 或 `package.json` 有未提交修改时拒绝生成，防止把不同源码混成同一基准。

- `docs/port-status.json`：全部 `src/**/*.js` 与 `examples/jsm/**/*.js`、目录、非 JS 资源、目标文件候选和验证证据位置。
- `docs/api-inventory.json`：使用上游 checkout 中已有 Acorn 的 AST 提取导出、导入/再导出依赖、类、构造器/方法参数、属性写入点、对象字面量、传统函数构造器、动态接口待审查位置。保留源码行号。
- `examples/jsm/libs` 标为第三方依赖；其 JS/WASM、版本、许可、接口和原生替换仍需单独验收，不要求机械生成同名 C++ 文件。

已确定的移植范围是核心与 addons。editor、manual、文档站、开发工具、TSL 演示及 addons 以外的示例网页不作为原生应用重写。原生宿主行为、构建安装、示例与三平台验收仍需单独登记证据。

## 状态与证据

`missing` 表示没有同路径 C++ 实现文件；`partial-unverified` 只表示存在头文件或实现；`vulkan-port-pending` 表示上游渲染逻辑/后端映射尚待验收；`dependency-unverified` 表示第三方依赖或非 JS 资源未验收。当前生成器不会自动写入 complete。

模块的 `verification.apiInventory` 指向 API 清单中的 JSON Pointer。`upstreamUnitTests` 和 `cppUnitTests` 仅列出实际存在的同名测试候选；测试文件存在不代表已运行、通过或覆盖全部 API。实现审查、差分结果、渲染结果、依赖结果及最近通过记录为空时，表示尚无逐模块登记的证据。`docs/VALIDATION.md` 的历史整体结果不会自动证明每个模块完成。

AST 清单始终为 `apiCoverageComplete: false`。继承成员不展开；`this` 写入不推断为无条件默认值；动态键、Proxy、函数工厂、Object.assign/defineProperty、运行时生成的 TSL 接口、CommonJS/UMD 都可能扩大实际 API。具体盲区列于各模块 `reviewRequired`，通用限制列于 `limitations`。`static-only` 表示仅有静态语法候选，`partial-static` 表示另有明确待审查项，`parse-error` 保留诊断而不伪装成功。

## 生成与验证

无需安装或复制解析器。使用固定 three.js checkout 中已有的 Acorn；缺少解析器时明确失败。

```powershell
node utils/port-audit.mjs <three.js目录>
node utils/port-audit.mjs <three.js目录> --check
node utils/port-audit.tests.mjs <three.js目录>
```

`--check` 只重算并比较清单，不写文件。自验覆盖别名/再导出、字符串伪声明、参数默认表达式、getter/setter、private/static/computed 成员、词法 this 边界、传统函数构造器及解析失败；另外检查锁定版本全部模块的唯一性、资源与测试路径、JSON Pointer、真实 Vector3/WebGLRenderer/Addons 接口和生成结果一致性。
