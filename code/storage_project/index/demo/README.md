# TSDB Demo

This is a self-contained C++ demo that implements the storage and query flow described in
`readme.txt` and the Khronos paper summary in `sourcepaper.md`:
- Parse schemaless timeline points (metric + tags + timestamp + dynamic fields).
- Build series keys, forward index, inverted index (token -> series ids).
- Dump to disk and query by metric, tags, and time range.

## Build

Install CMake on WSL/Ubuntu:
```
sudo apt update
sudo apt install -y cmake
```

```
cmake -S . -B build
cmake --build build
```

Run tests:
```
ctest --test-dir build
```

Optional ORC support (requires Apache ORC C++ headers/libs):
```
cmake -S . -B build -DTSDB_ENABLE_ORC=ON -DORC_INCLUDE_DIR=/path/to/orc/include -DORC_LIBRARY=/path/to/liborc.so
cmake --build build
```

Bundled ORC support (uses Apache ORC source under `third_party/orc`):
```
cmake -S . -B build -DTSDB_ENABLE_ORC=ON -DTSDB_USE_BUNDLED_ORC=ON
cmake --build build
```

Notes:
- The bundled ORC build disables Java/tools/tests by default. See Apache ORC build docs for system deps.
- The bundled tree is expected to match Apache ORC release 2.2.2 (tag rel/release-2.2.2).

## Run

Build the demo storage:
```
./build/tsdb build data/input.txt data/out partition=3600 format=binary
```

Use ORC format (if enabled at build time):
```
./build/tsdb build data/input.txt data/out partition=3600 format=orc
```

Note: ORC format supports build/query/ingest. Ingest writes per-partition delta ORC files; run
`tsdb merge` to compact deltas into the base `points.orc`.

Build with rollups (down-sampling, e.g. 20s/1m/10m/60m) and tier policy:
```
./build/tsdb build data/input.txt data/out partition=3600 format=orc \
  rollup=20,60,600,3600 tier_hot=43200 tier_warm=259200
```

Query:
```
./build/tsdb query data/out metric=cpu tag=host=10.0.0.1 tag=domain=beijing start=1769866020 end=1769866024 fields=min,max,avg,sum,count
```

Query only hot/warm tiers (if tiers.txt exists):
```
./build/tsdb query data/out metric=cpu tag=domain=beijing start=1769866020 end=1769866024 tier=hot,warm
```

Query rollup data (resolution in seconds uses `rollup_<sec>` directory):
```
./build/tsdb query data/out metric=cpu tag=domain=beijing start=1769866020 end=1769866024 resolution=60
```

Incremental ingest (append new points):
```
./build/tsdb ingest data/new_points.txt data/out
```

Merge (compact ORC deltas into base files and postings delta):
```
./build/tsdb merge data/out
```

## Kronos (Lambda Architecture Demo)

This repo includes a small "Kronos" demo binary that simulates the online/offline split,
message queue ingestion, and Pangu-like cold storage on local disk.

Initialize a local cluster (2 shards):
```
./build/kronos init data/cluster shards=2
```

Publish data into the MQ (optional rollups at 20s/1m/10m/60m):
```
./build/kronos publish data/cluster data/input.txt rollup=20,60,600,3600
```

Online ingest (consumes MQ and writes shard-local indexes):
```
./build/kronos online_ingest data/cluster max=10000
```

Offline build (consumes MQ and writes versioned indexes to "pangu"):
```
./build/kronos offline_build data/cluster format=orc rollup=20,60,600,3600
```

Query across shards (optionally exact series routing):
```
./build/kronos query data/cluster metric=cpu tag=host=10.0.0.1 tag=domain=beijing \
  start=1769866020 end=1769866024 tier=hot,warm,cold exact=1
```

## Stress Test

Generate a larger dataset and run parallel queries:
```
./scripts/stress.sh
```

Override defaults (example):
```
SERIES_COUNT=500 POINTS_PER_SERIES=2000 PARALLEL=8 QUERY_THREADS=4 ITERATIONS=200 ./scripts/stress.sh
```

Run stress test with ORC (requires ORC enabled build):
```
FORMAT=orc ORC_BUNDLED=1 ./scripts/stress.sh
```

## Notes (Production-like Features)
- Partitioned storage by time bucket (default 1h) in `part_*` directories.
- Binary points format (`format=binary`) uses varint-encoded ids/timestamps for smaller storage.
- Posting list cache inside query path.
- Multi-threaded query via `threads=<n>`.
- Incremental ingest appends points and refreshes indices.
- WAL replay is triggered on open/ingest to recover from crashes.
- Posting list deltas (`postings_delta.txt`) provide incremental indexing; `merge` compacts them.

## Production Reference
See `docs/PRODUCTION.md` for a production-grade architecture blueprint that builds on this demo.
See `docs/KRONOS.md` for the local lambda-architecture demo walkthrough.

## Output files (data/out)
- `serieskey.txt`: `series_id|metric|tag=value|...`
- `meta.txt`: partition settings
- `partitions.txt`: partition list
- `part_*/points.orc` or `part_*/points.bin` or `part_*/points.txt`: points (binary uses varint ids/timestamps + doubles)
- `part_*/deltas.txt` + `part_*/points_delta_*.orc`: ORC delta files (created by ingest, compacted by merge)
- `part_*/forward_index.txt`: `series_id start_row end_row`
- `postings.txt`: `token_hash series_id1,series_id2,...`
- `dict.txt`: `token_hash offset length is_complement`
- `postings.meta`: `series_count <n>` used by complement index
- `postings_delta.txt` + `dict_delta.txt`: postings delta for incremental indexing
- `wal.log`: write-ahead log for crash recovery
- `tiers.txt`: `bucket tier` (hot/warm/cold)
- `rollup_<sec>/...`: rollup storage directories
Use text points instead (debug-friendly):
```
./build/tsdb build data/input.txt data/out partition=3600 format=text
```
