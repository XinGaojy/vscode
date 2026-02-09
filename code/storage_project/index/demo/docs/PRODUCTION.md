# Production Architecture (Reference)

This document describes a production-grade reference architecture for the demo TSDB in this repo.
It is a blueprint that extends the current single-node engine with distributed services,
operational controls, and scalability patterns needed for very large workloads.

## Goals
- Horizontal scalability (shards, replication, multi-tenant isolation)
- High write throughput with predictable query latency
- Strong operational safety: WAL, snapshotting, compaction, observability
- Efficient storage formats (ORC with column pruning and predicate pushdown)

## Data Model (from readme.txt)
- Series key: metric|tag=value|...
- Inverted index: token -> series ids
- Forward index: series id -> row range per time partition
- Points: time + 5 fields (min/max/avg/sum/count)

## Storage Layout
- Partitioned by time bucket: part_<bucket>/
- Files per partition:
  - serieskey.txt: series id -> series key
  - postings.txt + dict.txt: token postings
  - forward_index.txt: series id -> row ranges
  - points.orc (or points.bin/points.txt for debug)
  - deltas.txt + points_delta_*.orc (ORC ingest deltas, compacted by merge)

## Ingest Pipeline (Production)
1) Write path
   - Ingest API writes to a WAL (per shard) and an in-memory memtable.
   - Memtable flushes to immutable segment files in ORC format.
2) Compaction
   - Background compaction merges segments into larger ORC files.
   - Compact indexes (postings, series keys) and prune old data (TTL).
3) Index maintenance
   - Posting lists stored as sorted compressed int lists.
   - Forward index stored per partition for row ranges.

## Query Path (Production)
1) Filter by tags: intersect postings lists (smallest first).
2) Resolve series keys -> series ids.
3) Determine partitions by time range.
4) Read points from ORC with:
   - Column pruning (only requested fields)
   - Predicate pushdown for time bounds
5) Merge results from shards and tiers; apply final ordering/aggregation.

## Tiered Storage / Rollups
- Hot tier: recent partitions on NVMe
- Warm tier: older partitions on SSD
- Cold tier: object storage (S3/HDFS)
- Rollups: pre-aggregated tiers at coarser resolutions

## Sharding & Replication
- Shard key: hash(metric + tag set)
- Replication factor configurable; leader/follower writes
- Metadata service assigns shards to nodes

## Service Components
- Ingest Service: validates, batches, writes WAL/memtable
- Query Service: plans queries, routes to shard nodes
- Storage Nodes: own partitions, serve ORC reads
- Compaction Workers: background merging + TTL pruning
- Metadata/Coordinator: shard map, topology, placement

## Observability
- Metrics: query latency, rows scanned, cache hit rate
- Tracing: end-to-end query spans
- Logs: structured, per-shard ingest/query

## Security & Compliance
- TLS for all RPC
- AuthN/AuthZ per tenant
- Audit logs for admin operations

## What This Repo Implements Today
- Single-node build/query/ingest
- Inverted index + forward index
- Time partitioned storage
- Optional ORC points format
- ORC delta ingest + merge compaction

## What Remains for a Full Production System
- WAL + crash recovery
- Distributed sharding/replication
- Compaction scheduler
- Tiered storage & rollups
- Query router + metadata service
