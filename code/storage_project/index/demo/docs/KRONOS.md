# Kronos Demo Architecture

This document describes the "Kronos" demo binary added to this repo. The implementation is a
minimal, single-host simulation of the lambda architecture described in `sourcepaper.md`.

## Components
- **Message Queue (MQ)**: File-backed queues (`mq/raw`, `mq/rollup_<sec>`). Each topic is an
  append-only log (`queue.log`) with per-consumer offsets.
- **Online Indexer**: Consumes raw MQ data and writes shard-local indexes using the TSDB ingest path.
- **Offline Builder**: Consumes MQ data and produces versioned indexes in a Pangu-like directory.
- **Pangu Storage**: Local directory (`pangu/index_<version>/shard_<id>`) used as cold storage.
- **Router / Query**: A simple query scatter/gather across shards with optional exact routing.

## Storage Layout
```
cluster_root/
  cluster.meta
  online/
    shard_0/
    shard_1/
  mq/
    raw/queue.log
    rollup_20/queue.log
  pangu/
    index_1700000000/shard_0/
```

## CLI Summary
```
./build/kronos init <cluster_dir> shards=2
./build/kronos publish <cluster_dir> <input_file> rollup=20,60,600,3600
./build/kronos online_ingest <cluster_dir> max=10000
./build/kronos offline_build <cluster_dir> format=orc rollup=20,60,600,3600
./build/kronos query <cluster_dir> metric=cpu tag=host=10.0.0.1 start=... end=... exact=1
```

## Notes
- The MQ, Pangu, and metadata services are simplified local implementations for demo purposes.
- Offline builds are incremental in this demo (built from the consumed batch), not full replays.
- Sharding uses a hash of `metric|tag=value|...` (canonical tag order).
- Query routing:
  - `exact=1` routes to a single shard by series key hash.
  - Otherwise queries all shards.
