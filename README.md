# ForgeKV + MLX KV Cache Hierarchy Lab

## Project summary

Build a small persistent C++ key-value store, **ForgeKV**, and use its realistic edit history as a benchmark for a local coding agent. The agent runs a Llama-style model locally on Apple Silicon through a C++ MLX inference runner.

The research component is an **intelligent LLM KV-cache memory-storage hierarchy**. An edit-aware cache manager uses coding-task locality, dependency information, recency, and reuse frequency to keep high-value KV blocks in unified memory, compress less critical blocks, or persist cold blocks through an application-level FTL-backed simulated NAND tier.

This is not primarily a Cursor clone. The user interface is deliberately out of scope at first. The project is an inference-systems experiment with a practical coding-agent workload.

## Core hypothesis

> For multi-turn coding-agent sessions over ForgeKV on an M5 MacBook Air, an edit-aware KV-cache memory-storage hierarchy can reduce peak unified-memory KV usage by at least 50% while retaining at least 90% of the full-precision baseline's task performance.

The exact figures are experimental targets, not assumed results.

## What the project demonstrates

- Modern C++: CMake, tests, RAII, binary I/O, filesystem safety, data structures, and storage-engine correctness.
- Storage systems: WAL recovery, memtables, immutable sorted files, metadata, compaction, tombstones, FTL mapping, garbage collection, and write-amplification analysis.
- Local LLM inference: model loading, tokenization, autoregressive decoding, attention, and KV-cache accounting on Apple Silicon.
- Systems research: tiering, quantization, admission, promotion, demotion, persistence, prefetching, eviction, reproducible evaluation, and quality/performance trade-offs.

## Build and test

Once CMake is installed, run the complete local build and test workflow with:

```bash
./scripts/build_and_test.sh
```

Pass `Release` when you want an optimized build:

```bash
./scripts/build_and_test.sh Release
```

## Terminology

| Term | Meaning in this project |
| --- | --- |
| KV store | Application data stored as `key -> value`, such as `user:42 -> Alice`. |
| WAL | Write-ahead log. A durable append-only record of writes used for crash recovery. |
| MemTable | Sorted in-memory storage for recent database writes. |
| SSTable | Sorted String Table: an immutable, sorted on-disk file of database entries. |
| LSM tree | The storage design that moves writes from a log and memtable into SSTables, then merges them. |
| Tombstone | A deletion marker that prevents an older value from reappearing during compaction. |
| LLM KV cache | Per-layer attention keys and values from previous tokens, reused during generation. This is unrelated to a KV store. |
| FTL | Flash Translation Layer. In this project, an application-level simulator that maps logical KV pages to simulated NAND physical pages. It does not control the Mac's actual SSD firmware. |
| NAND page / block | A simulated persistent-storage page and its erase-unit block, used by the FTL cold tier. |
| Garbage collection (GC) | Reclaims a simulated NAND block by relocating valid pages and invalidating old physical pages. |
| Write amplification | Simulated physical bytes written divided by logical bytes written; GC can make this greater than one. |
| TTFT | Time to first token: latency before the model begins responding. |

## System architecture

```text
Coding task
    |
    v
Context builder -----------------------------------+
  - source files                                   |
  - current diff                                   |
  - test output                                    |
  - conversation history                           |
  - token provenance                               |
    |                                               |
    v                                               |
C++ MLX inference runner                           |
  - tokenizer                                      |
  - Llama-style transformer                         |
  - generation loop                                |
  - KVCacheManager <--------------------------------+
       | hot / warm / cold
       v
  FTL-backed simulated NAND
  - logical-to-physical page mapping
  - out-of-place writes and invalidation
  - garbage collection and modeled I/O cost
    |
    v
Patch or diagnosis --> build ForgeKV --> run tests --> evaluation record
```

The context builder labels each prompt segment with provenance. For example:

```text
System prompt                         -> hot
Current task and latest diff          -> hot
Edited files and direct dependencies  -> hot / warm
Earlier relevant test output          -> warm
Unrelated old code and discussions    -> cold
```

The cache manager turns these labels into placement and precision choices:

```text
Hot:     unified memory, original precision
Warm:    unified memory, 8-bit or 4-bit KV
Cold:    simulated NAND through the FTL, compressed KV blocks
Evicted: no retained KV; recompute through prefill if needed
```

The FTL is intentionally a simulator. It never attempts to control the actual FTL inside the Mac's SSD.

## ForgeKV scope

ForgeKV is a single-process, single-threaded embedded C++ key-value store.

Public API:

```cpp
Open(path)
Put(key, value)
Get(key)
Delete(key)
Close()
```

Initial source layout:

```text
forgekv/
  include/forgekv/
    status.h       // errors: NotFound, IOError, Corruption, ...
    db.h           // public database API
    wal.h          // write-ahead log
    memtable.h     // sorted in-memory write buffer
    sstable.h      // immutable sorted-file writer and reader
    manifest.h     // active SSTable metadata
    compaction.h   // merging policy
  src/
    db.cc
    wal.cc
    memtable.cc
    sstable.cc
    manifest.cc
    compaction.cc
  tests/
  tools/kv_cli.cc
```

### Write path

```text
Put/Delete -> WAL append -> MemTable update -> flush threshold -> SSTable
```

### Read path

```text
Get -> MemTable -> newest relevant SSTable -> older SSTables -> NotFound
```

### Compaction

```text
Old SSTables -> sorted merge -> newest entry wins -> new SSTable -> manifest update
```

Do not add concurrency, replication, transactions, SQL, or a graphical UI in version one. Target roughly 1,500-3,000 lines of C++ for ForgeKV.

## Benchmark task design

Each coding task has a known repository state, a natural-language request, relevant context, and a test-based success criterion.

Examples:

1. Ignore an incomplete final WAL record during crash recovery.
2. Add tombstone support across the API, memtable, and SSTable reader.
3. Fix compaction so a deleted key does not reappear.
4. Repair an SSTable index boundary bug.
5. Add snapshot sequence-number behavior.
6. Add a block cache without breaking reads after compaction.

Store tasks in JSONL with fields such as:

```json
{
  "task_id": "compact-tombstone-001",
  "base_revision": "<git commit>",
  "prompt": "Deleted keys reappear after compaction. Diagnose and fix it.",
  "changed_files": ["src/compaction.cc", "src/sstable.cc"],
  "dependency_files": ["src/db.cc", "include/forgekv/compaction.h"],
  "test_command": "ctest --test-dir build --output-on-failure",
  "success": "compaction_test passes"
}
```

## KV-cache experiment

The experiment compares policies under the same unified-memory budget:

| Policy | Purpose |
| --- | --- |
| Full precision | Quality and latency reference. |
| Uniform 8-bit / 4-bit | Simple compression baselines. |
| Recency-only tiers | Standard cache heuristic. |
| Edit-aware memory tiers | Uses changed files, dependencies, and reuse information without persistent cold storage. |
| Edit-aware FTL-backed hierarchy | Project contribution: selects memory, simulated NAND persistence, or eviction using relevance and cost. |

Each cache entry records:

```text
layer ID, token range, source file or conversation turn, tier, precision,
byte size, last-used step, retrieval count, logical page ID, physical page ID
```

Metrics:

- Peak KV-cache memory.
- Simulated NAND capacity, read/write bytes, and modeled page-in latency.
- Simulated garbage-collection work and write amplification.
- Prefill latency and time to first token.
- Decode throughput in tokens per second.
- Cache hit/reuse behavior, promotion/demotion rate, and recomputation rate.
- Task success: compiled patch and passing tests.
- Quality loss compared with the full-precision baseline.

## Step-by-step roadmap

### Phase 0 — Project contract

- [x] Create the repository and this README.
- [x] Record the hypothesis, scope, metrics, and non-goals.
- [ ] Choose one initial Llama-style model family for the C++ runner.

### Phase 1 — C++ foundation

- [x] Configure CMake and automated tests.
- [ ] Implement `Status` and the basic ForgeKV CLI.
- [x] Build and test a clean checkout.

### Phase 2 — Durable ForgeKV vertical slice

- [x] Implement `DB`, WAL, and a `std::map` MemTable.
- [x] Implement WAL replay on startup.
- [x] Test recovery, including a truncated final WAL record.

### Phase 3 — Persistent sorted storage

- [ ] Implement SSTable writer and reader.
- [ ] Flush the MemTable to SSTables.
- [ ] Add the Manifest.
- [ ] Implement one-level compaction and tombstones.
- [ ] Add recovery and compaction test coverage.

### Phase 4 — Coding-agent benchmark

- [ ] Create 15-20 deterministic edit tasks.
- [ ] Record task metadata, edits, dependency files, and expected tests.
- [ ] Validate that tasks are repeatable from their base Git revisions.

### Phase 5 — C++ KV-cache simulator

- [ ] Define cache-entry metadata and token provenance.
- [ ] Implement FP16, INT8, and INT4 accounting.
- [ ] Implement quantize/dequantize simulation.
- [ ] Implement recency-only and edit-aware policies.
- [ ] Export reproducible policy and memory metrics.

### Phase 6 — FTL-backed simulated NAND tier

- [ ] Define fixed-size logical KV pages and simulated NAND blocks.
- [ ] Implement logical-to-physical page mapping and out-of-place writes.
- [ ] Track valid and invalid pages for overwritten or evicted cold KV blocks.
- [ ] Implement a simple greedy garbage collector.
- [ ] Record simulated read, program, and erase latency plus write amplification.
- [ ] Connect cold-tier promotion, demotion, and eviction to the KV-cache simulator.

### Phase 7 — MLX C++ environment

- [ ] Build the MLX C++ library locally.
- [ ] Run a minimal C++ tensor program through MLX.
- [ ] Link MLX to a CMake target and verify Metal runtime availability.

### Phase 8 — C++ inference baseline

- [ ] Load one model family and tokenizer.
- [ ] Implement a decoder-only forward pass, RoPE, and greedy decoding.
- [ ] Implement a normal full-precision per-layer KV cache.
- [ ] Validate generated outputs against a reference runner.

### Phase 9 — Live cache instrumentation

- [ ] Measure KV-cache memory by layer and token range.
- [ ] Measure prefill, TTFT, and decoding throughput.
- [ ] Add uniform 8-bit and 4-bit baseline modes.

### Phase 10 — Edit-aware live hierarchy

- [ ] Connect prompt provenance to live cache entries.
- [ ] Implement hot/warm/cold placement and precision tiers.
- [ ] Implement promotion on repeated file/context use.
- [ ] Persist selected cold KV blocks through the FTL simulator.
- [ ] Apply a fixed unified-memory budget and cost-aware eviction policy.
- [ ] Compare against full precision, recency-only, and memory-only tiering.

### Phase 11 — Results and portfolio handoff

- [ ] Run all benchmark tasks under all policies.
- [ ] Produce charts and a concise findings report.
- [ ] Record a demo: task, context labels, generated patch, tests, cache metrics.
- [ ] Write the final technical report and resume bullet.

## Constraints that protect the project

- Build ForgeKV and the standalone KV-cache simulator before building the full model runner.
- Keep the FTL simulator application-level; do not attempt to access or alter the Mac SSD's real FTL.
- Start FTL simulation with one channel, one plane, fixed-size pages, and greedy GC; defer ECC, wear leveling, and NAND command protocols.
- Support one model architecture before attempting model-general infrastructure.
- Use a command-line agent first; a Cursor-like interface is a later enhancement.
- Report quality regressions honestly. A faster, smaller cache that fails coding tasks is not a win.
- Keep every benchmark task tied to a fixed Git revision and automated test.

## Portfolio statement draft

> Built a C++ local LLM inference prototype on Apple Silicon using MLX and an edit-aware KV-cache memory-storage hierarchy for coding-agent workloads. Implemented an FTL-backed simulated NAND cold tier with logical-to-physical mapping, garbage collection, and write-amplification tracking; evaluated memory, modeled storage I/O, latency, throughput, and patch-test quality against full-precision and memory-only baselines using a custom C++ LSM storage-engine benchmark.
