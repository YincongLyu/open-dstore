# Spec-Kit + OpenCode 完整开发流程集成指南

## 概述

本文档将 Spec-Kit 的 SDD (Spec-Driven Development) 开发流程与 OpenCode 的 superpowers skills 整合，形成完整的开发工作流：从需求澄清 → 规格定义 → 技术计划 → 代码实现 → 验证测试 → Code Review → Git Commit → PR/Merge。

## 完整工作流程图

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                    Complete Development Workflow (SDD + Superpowers)                     │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                          │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐                  │
│  │ brainstorm  │──►│ constitution│──►│  git.feature│──►│  specify    │                  │
│  │ (superpowers)│   │ (speckit)   │   │ (speckit)   │   │ (speckit)   │                  │
│  └──────┬──────┘   └──────┬──────┘   └──────┬──────┘   └──────┬──────┘                  │
│         │                 │                  │                  │                        │
│         ▼                 ▼                  ▼                  ▼                        │
│    需求探索         项目宪法           创建分支          需求规格                      │
│    多轮对话         开发原则           001-xxx          spec.md                       │
│                                                                                          │
│         ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐          │
│         │  clarify    │──►│  checklist  │──►│    plan     │──►│   analyze   │          │
│         │ (speckit)   │   │ (speckit)   │   │ (speckit)   │   │ (speckit)   │          │
│         └──────┬──────┘   └──────┬──────┘   └──────┬──────┘   └──────┬──────┘          │
│                │                  │                  │                  │              │
│                ▼                  ▼                  ▼                  ▼              │
│           澄清歧义          验证清单          技术计划          审计计划              │
│           多轮对话          completeness       plan.md           consistency          │
│                                                                                          │
│         ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   ┌─────────────┐          │
│         │   tasks     │──►│  implement  │──►│ verification│──►│ git.commit  │          │
│         │ (speckit)   │   │ (speckit)   │   │ (superpowers)│   │ (speckit)   │          │
│         └──────┬──────┘   └──────┬──────┘   └──────┬──────┘   └──────┬──────┘          │
│                │                  │                  │                  │              │
│                ▼                  ▼                  ▼                  ▼              │
│           任务清单          代码实现          验证测试          Git 提交              │
│           tasks.md         代码变更          tests pass        commit changes        │
│                                                                                          │
│         ┌─────────────┐   ┌─────────────┐   ┌─────────────┐                              │
│         │code-review  │──►│ finishing   │──►│   merge/pr  │                              │
│         │(superpowers)│   │ (superpowers)│   │             │                              │
│         └──────┬──────┘   └──────┬──────┘   └──────┬──────┘                              │
│                │                  │                  │                                    │
│                ▼                  ▼                  ▼                                    │
│           代码审查          完成分支          合并/PR                                  │
│           review feedback     选择处理方式      integrate to main                      │
│                                                                                          │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

## 详细步骤说明

### Phase 0: 需求探索 (Brainstorming)

**Skill**: `brainstorming` (superpowers)

**时机**: 在任何创造性工作之前，理解用户真实意图和需求。

**流程**:
```
/speckit.specify → ❌ 先不要用！先用 brainstorming skill
```

**使用方式**:
```bash
# 在 OpenCode 中，先加载 brainstorming skill
skill name="brainstorming"
```

**Brainstorming Checklist**:
1. Explore project context — 检查文件、文档、最近提交
2. Ask clarifying questions — 一个问题一个问题地问
3. Propose 2-3 approaches — 提出方案和权衡
4. Present design — 分段展示设计，获取批准
5. Write design doc — 写入 `docs/superpowers/specs/YYYY-MM-DD-<topic>-design.md`
6. User reviews spec — 用户审查规格文件

**输出**: `docs/superpowers/specs/YYYY-MM-DD-<topic>-design.md`

---

### Phase 1: 项目宪法 (Constitution)

**Command**: `/speckit.constitution`

**时机**: 项目初始化或添加新的架构约束时。

**示例**:
```
/speckit.constitution This project follows "Library-First" approach. 
All features must be implemented as standalone libraries first. 
We use TDD strictly. 
We prefer functional programming patterns.
```

**输出**: `memory/constitution.md`

---

### Phase 2: 创建功能分支 (Git Feature)

**Command**: `/speckit.git.feature`

**时机**: 在开始编写规格之前，创建隔离的功能分支。

**示例**:
```
/speckit.git.feature Build user authentication module
```

**分支命名规则**:
- Sequential: `001-user-auth`, `002-payment-system`
- Timestamp: `20260414-143022-user-auth`

**输出**: Git 分支 + `specs/001-feature-name/` 目录

---

### Phase 3: 定义需求规格 (Specify)

**Command**: `/speckit.specify`

**时机**: Brainstorming 完成后，定义正式的需求规格。

**示例**:
```
/speckit.specify Build an application that can help me organize my photos 
in separate photo albums. Albums are grouped by date and can be re-organized 
by dragging and dropping on the main page.
```

**要点**:
- ✅ 聚焦于 WHAT (用户需要什么) 和 WHY (为什么需要)
- ❌ 避免 HOW (技术栈、API、代码结构)

**输出**: `specs/001-feature-name/spec.md`

---

### Phase 4: 澄清需求歧义 (Clarify)

**Command**: `/speckit.clarify`

**时机**: specify 之后，plan 之前。可选但强烈推荐。

**示例**:
```
/speckit.clarify Focus on security and performance requirements.

/speckit.clarify I want to clarify the task card details. 
For each task in the UI, you should be able to change the current status 
between different columns in the Kanban board.
```

**澄清重点**:
- 安全性要求
- 性能约束
- 用户交互细节
- 边界条件处理

---

### Phase 5: 验证规格清单 (Checklist)

**Command**: `/speckit.checklist`

**时机**: clarify 之后，plan 之前。可选。

**示例**:
```
/speckit.checklist
```

**检查项**:
- [ ] 用户故事是否完整
- [ ] 验收标准是否明确
- [ ] 边界条件是否考虑
- [ ] 安全要求是否定义

---

### Phase 6: 技术实现计划 (Plan)

**Command**: `/speckit.plan`

**时机**: 规格验证后，提供技术栈和架构选择。

**示例**:
```
/speckit.plan The application uses Vite with minimal libraries. 
Use vanilla HTML, CSS, and JavaScript. 
Metadata stored in local SQLite database.
```

**输出**: `specs/001-feature-name/plan.md`

---

### Phase 7: 审计实现计划 (Analyze)

**Command**: `/speckit.analyze`

**时机**: plan 之后，tasks 之前。可选但推荐。

**示例**:
```
/speckit.analyze
```

**审计维度**:
- 规格与计划的一致性
- 架构约束遵循情况
- 依赖关系合理性
- 风险识别

---

### Phase 8: 任务分解 (Tasks)

**Command**: `/speckit.tasks`

**时机**: 计划审计后，生成依赖排序的任务列表。

**示例**:
```
/speckit.tasks
```

**输出**: `specs/001-feature-name/tasks.md`

---

### Phase 9: 代码实现 (Implement)

**Command**: `/speckit.implement`

**时机**: 任务分解后，执行代码实现。

**示例**:
```
/speckit.implement
```

**建议**: 复杂项目分阶段实现，避免上下文过载。

---

### Phase 10: 完成前验证 (Verification)

**Skill**: `verification-before-completion` (superpowers)

**时机**: 实现完成后，在声称完成之前，必须验证。

**Iron Law**:
```
NO COMPLETION CLAIMS WITHOUT FRESH VERIFICATION EVIDENCE
```

**流程**:
```bash
# 1. 运行测试命令
npm test / cargo test / pytest / make run_dstore_ut_all

# 2. 检查输出
# 3. 确认 0 failures
# 4. THEN 才能声称 "All tests pass"
```

**使用方式**:
```bash
skill name="verification-before-completion"
```

---

### Phase 11: Git 提交 (Git Commit)

**Command**: `/speckit.git.commit`

**时机**: 验证通过后，自动提交变更。

**配置** (`.specify/extensions/git/git-config.yml`):
```yaml
auto_commit:
  default: false
  after_implement:
    enabled: true
    message: "[Spec Kit] Implement feature"
```

**手动提交**:
```
/speckit.git.commit
```

---

### Phase 12: 代码审查 (Code Review)

**Skill**: `requesting-code-review` (superpowers)

**时机**: 实现完成后，在合并之前。

**流程**:
```bash
# 1. 获取 git SHAs
BASE_SHA=$(git rev-parse HEAD~1)  # 或 origin/main
HEAD_SHA=$(git rev-parse HEAD)

# 2. 加载 skill
skill name="requesting-code-review"

# 3. 根据 skill 指导，使用 call_omo_agent 调用 oracle agent 进行审查
```

**审查重点**:
- Critical issues — 立即修复
- Important issues — 继续前修复
- Minor issues — 稍后处理

**接收审查反馈**:
```bash
skill name="receiving-code-review"
```

---

### Phase 13: 完成开发分支 (Finishing)

**Skill**: `finishing-a-development-branch` (superpowers)

**时机**: 代码审查完成后，决定如何集成工作。

**流程**:
```bash
skill name="finishing-a-development-branch"
```

**四个选项**:
1. **Merge locally** — 本地合并到 base branch
2. **Push and create PR** — 推送并创建 Pull Request
3. **Keep as-is** — 保持分支状态
4. **Discard** — 丢弃工作

---

## 完整命令序列速查表

### 标准开发流程

```bash
# 1. 需求探索 (必须在 specify 前使用)
skill name="brainstorming"

# 2. 项目宪法 (可选，项目初始化时)
/speckit.constitution

# 3. 创建功能分支
/speckit.git.feature <feature description>

# 4. 需求规格
/speckit.specify <what and why>

# 5. 澄清歧义 (推荐)
/speckit.clarify <focus areas>

# 6. 验证清单 (可选)
/speckit.checklist

# 7. 技术计划
/speckit.plan <tech stack and architecture>

# 8. 审计计划 (推荐)
/speckit.analyze

# 9. 任务分解
/speckit.tasks

# 10. 代码实现
/speckit.implement

# 11. 完成前验证 (必须)
skill name="verification-before-completion"

# 12. Git 提交
/speckit.git.commit

# 13. 代码审查 (推荐)
skill name="requesting-code-review"

# 14. 完成分支
skill name="finishing-a-development-branch"
```

### 快速原型流程

```bash
/speckit.git.feature <feature>
/speckit.specify <requirements>
/speckit.plan <tech stack>
/speckit.tasks
/speckit.implement
skill name="verification-before-completion"
/speckit.git.commit
skill name="finishing-a-development-branch"
```

### 严格质量流程 (生产环境)

```bash
skill name="brainstorming"
/speckit.constitution
/speckit.git.feature <feature>
/speckit.specify <requirements>
/speckit.clarify
/speckit.checklist
/speckit.plan <tech stack>
/speckit.analyze
/speckit.tasks
/speckit.implement
skill name="verification-before-completion"
skill name="requesting-code-review"
skill name="receiving-code-review"
/speckit.git.commit
skill name="finishing-a-development-branch"
```

---

## 文件产出物

| 阶段 | 命令/Skill | 输出文件 |
|------|-----------|---------|
| Brainstorming | `brainstorming` | `docs/superpowers/specs/YYYY-MM-DD-<topic>-design.md` |
| Constitution | `/speckit.constitution` | `memory/constitution.md` |
| Feature Branch | `/speckit.git.feature` | Git branch `001-xxx` |
| Specify | `/speckit.specify` | `specs/001-xxx/spec.md` |
| Plan | `/speckit.plan` | `specs/001-xxx/plan.md` |
| Tasks | `/speckit.tasks` | `specs/001-xxx/tasks.md` |
| Implement | `/speckit.implement` | 源代码变更 |
| Git Commit | `/speckit.git.commit` | Git commits |

---

## 项目配置文件结构

```
project/
├── .agents/
│   └── commands/               # OpenCode 斜杠命令
│       ├── speckit.constitution.md
│       ├── speckit.specify.md
│       ├── speckit.clarify.md
│       ├── speckit.checklist.md
│       ├── speckit.plan.md
│       ├── speckit.analyze.md
│       ├── speckit.tasks.md
│       ├── speckit.implement.md
│       ├── speckit.git.feature.md
│       ├── speckit.git.commit.md
│       ├── speckit.git.validate.md
│       └── speckit.git.remote.md
├── memory/
│   └── constitution.md         # 项目宪法
├── specs/
│   └── 001-feature-name/
│       ├── spec.md             # 需求规格
│       ├── plan.md             # 技术计划
│       └── tasks.md            # 任务列表
├── docs/
│   └── superpowers/
│       └── specs/
│           └── YYYY-MM-DD-<topic>-design.md  # Brainstorming 设计文档
├── .specify/
│   ├── scripts/                # 自动化脚本
│   ├── init-options.json       # 初始化选项
│   └── extensions/
│       └── git/
│           ├── git-config.yml  # Git 自动提交配置
│           └── scripts/
│               ├── bash/
│               └── powershell/
└── AGENTS.md                   # OpenCode 项目指南
```

---

## Git 自动提交配置

在 `.specify/extensions/git/git-config.yml` 中配置自动提交：

```yaml
auto_commit:
  default: false
  after_specify:
    enabled: true
    message: "[Spec Kit] Add specification"
  after_plan:
    enabled: true
    message: "[Spec Kit] Add implementation plan"
  after_tasks:
    enabled: true
    message: "[Spec Kit] Add task list"
  after_implement:
    enabled: true
    message: "[Spec Kit] Implement feature"
```

---

## 关键原则总结

### Spec-Kit SDD 原则
1. **规格先行** — 先定义 WHAT，再考虑 HOW
2. **迭代澄清** — 使用 clarify 发现隐藏问题
3. **审计计划** — analyze 验证一致性
4. **分阶段实现** — 避免上下文过载

### Superpowers 原则
1. **Brainstorming First** — 任何创造性工作前先探索需求
2. **Evidence Before Claims** — 验证测试结果后再声称完成
3. **Review Early, Review Often** — 每个任务完成后审查
4. **Fix Before Proceeding** — 修复问题再继续

---

## 参考资源

- [Spec-Kit GitHub](https://github.com/github/spec-kit)
- [Spec-Kit Docs](https://github.github.io/spec-kit/)
- [Quick Start Guide](https://github.github.io/spec-kit/quickstart.html)
- [Superpowers Skills](https://github.com/obra/superpowers)