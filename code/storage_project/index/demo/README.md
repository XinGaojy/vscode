# TSDB Demo

This is a self-contained C++ demo that implements the storage and query flow described in `readme.txt`:
- Parse timeline points (metric + tags + timestamp + 5 fields).
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

## Run

Build the demo storage:
```
./build/tsdb build data/input.txt data/out partition=3600 format=binary
```

Query:
```
./build/tsdb query data/out metric=cpu tag=host=10.0.0.1 tag=domain=beijing start=1769866020 end=1769866024 fields=min,max,avg,sum,count
```

Incremental ingest (append new points):
```
./build/tsdb ingest data/new_points.txt data/out
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

## Notes (Production-like Features)
- Partitioned storage by time bucket (default 1h) in `part_*` directories.
- Binary points format (`format=binary`) uses varint-encoded ids/timestamps for smaller storage.
- Posting list cache inside query path.
- Multi-threaded query via `threads=<n>`.
- Incremental ingest appends points and refreshes indices.

## Output files (data/out)
- `serieskey.txt`: `series_id|metric|tag=value|...`
- `meta.txt`: partition settings
- `partitions.txt`: partition list
- `part_*/points.bin` or `part_*/points.txt`: points (binary uses varint ids/timestamps + doubles)
- `part_*/forward_index.txt`: `series_id start_row end_row`
- `postings.txt`: `token_hash series_id1,series_id2,...`
- `dict.txt`: `token_hash offset length`
Use text points instead (debug-friendly):
```
./build/tsdb build data/input.txt data/out partition=3600 format=text
```
