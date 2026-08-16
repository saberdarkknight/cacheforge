# ForgeKV + MLX KV Cache Lab — Conversation Notes

This note preserves the key decisions made while planning the project.

## Background

- Hardware: MacBook Air with an M5 chip.
- Current experience: storage-system engineering.
- Goal: move toward LLM and hardware-aware inference work.
- Initial idea: a mini Cursor-like local coding assistant using MLX, with a KV-cache optimization research component.

## Decisions

1. The project is not primarily a Cursor clone. A command-line coding agent is sufficient for the first version.
2. The benchmark workload is multi-turn coding-agent sessions and long-context codebase Q&A with repeated edits.
3. The cache work is KV compression/tiering, not a claim that KV caching directly reduces generated token count.
4. The research contribution is an edit-aware hot/warm/cold policy.
5. Edit logs are one signal, not the only signal. Policy inputs also include recency, direct dependencies, prompt provenance, and reuse frequency.
6. The realistic codebase will be ForgeKV, a compact C++ LSM-tree key-value store.
7. The project starts with an offline/replay KV-cache simulator, then progresses to live inference.
8. The target end state is a C++ MLX inference runner, rather than a Python-only application.

## Why ForgeKV

ForgeKV naturally creates difficult but bounded code-edit tasks: crash recovery, binary formats, delete tombstones, compaction, manifests, and cross-file invariants. These tasks create meaningful code locality and dependency signals for the KV-cache policy.

## Important distinction

There are two different meanings of “KV”:

- **ForgeKV:** a key-value database storing application data.
- **LLM KV cache:** attention keys and values from model layers, stored to avoid recomputing prior-token attention.

ForgeKV supplies the workload and benchmark; the LLM KV cache is the optimized system.

## Initial experimental policy

```text
Hot:  active task, latest diff, changed files, direct dependencies; retain high precision.
Warm: earlier relevant files/test output; retain at lower precision, initially 8-bit.
Cold: unrelated or old context; use 4-bit precision or evict under pressure.
```

## Baselines

1. Full precision.
2. Uniform 8-bit and 4-bit cache compression.
3. Recency-only tiering.
4. Edit-aware tiering.

The experiment must compare policies using identical prompts, memory budgets, task revisions, and test criteria.

## First implementation order

1. C++ ForgeKV with WAL recovery and tests.
2. SSTables, manifest, compaction, and task history.
3. Synthetic/replay cache-policy simulator.
4. MLX C++ runtime smoke test.
5. Small C++ Llama-style inference baseline.
6. Live KV instrumentation, quantization, and edit-aware tiering.

