# Benchmark Logs & Tail Latency Profiles

This directory contains the condensed progression logs and structured benchmark execution logs comparing **Valkey 9.1.2** against **RESP-CPP** across multiple durations (60s, 120s, 500s) and high-precision tail latency distributions (p50, p75, p87.5, p95, p99, p99.9, p99.99).

---

## 🖥️ Test Hardware & Platform Specifications

| Specification | Hardware Configuration |
| :--- | :--- |
| **CPU Architecture** | x86_64 (Intel Haswell microarchitecture) |
| **CPU Model** | Intel(R) Core(TM) i5-4430 CPU @ 3.00GHz |
| **Cores / Threads** | 4 physical cores, 4 threads (1 thread per core, SMT/hyperthreading disabled) |
| **CPU Cache Hierarchy**| L1d: 128 KiB (4x32K) \| L1i: 128 KiB (4x32K) \| L2: 1 MiB (4x256K) \| L3: 6 MiB shared |
| **Total RAM** | 16 GB DDR3 (15 GiB physical, ~12 GiB free/available) |
| **Operating System** | Linux 7.2.5-3-omarchy x86_64 |
| **Compiler / Build Flags**| `g++ 15.2.1` with `-std=c++17 -Wall -Wextra -O3 -march=native -DNDEBUG` |
| **Core Isolation** | **Core 0:** Server process (`taskset -c 0`)<br>**Cores 1–3:** Benchmark client (`taskset -c 1-3`) |
| **Network & Transport** | Local loopback (`127.0.0.1`), TCP unpipelined, 50 concurrent persistent connections |

---

## 📁 Directory Structure & Log Index

| File | Description | Target | Duration |
| :--- | :--- | :--- | :--- |
| [`valkey_60s.log`](./valkey_60s.log) | 60-second microbenchmark progression & throughput | Valkey 9.1.2 | 60 seconds |
| [`resp_60s.log`](./resp_60s.log) | 60-second microbenchmark progression & throughput | RESP-CPP | 60 seconds |
| [`valkey_120s.log`](./valkey_120s.log) | 120-second soak test progression & throughput | Valkey 9.1.2 | 120 seconds |
| [`resp_120s.log`](./resp_120s.log) | 120-second soak test progression & throughput | RESP-CPP | 120 seconds |
| [`valkey_500s.log`](./valkey_500s.log) | 500-second sustained endurance benchmark | Valkey 9.1.2 | 500 seconds |
| [`resp_500s.log`](./resp_500s.log) | 500-second sustained endurance benchmark | RESP-CPP | 500 seconds |
| [`valkey_percentiles.log`](./valkey_percentiles.log) | Complete cumulative percentile latency distribution | Valkey 9.1.2 | High Precision |
| [`resp_percentiles.log`](./resp_percentiles.log) | Complete cumulative percentile latency distribution | RESP-CPP | High Precision |

---

## ⏱️ Detailed Tail Latency Percentiles (Valkey vs RESP-CPP)

All latency distributions measured with 50 parallel clients at `--precision 3`:

### 1. `PING_INLINE` (Inline Protocol)

| Percentile | Valkey 9.1.2 Latency | RESP-CPP Latency | Difference |
| :--- | :--- | :--- | :--- |
| **p50 (Median)** | 0.215 ms | 0.215 ms | Identical (0.000 ms) |
| **p75** | 0.231 ms | 0.239 ms | +0.008 ms |
| **p87.5** | 0.247 ms | 0.255 ms | +0.008 ms |
| **p95** | 0.271 ms | 0.279 ms | +0.008 ms |
| **p99** | 0.319 ms | 0.327 ms | +0.008 ms |
| **p99.9** | 0.479 ms | 0.767 ms | +0.288 ms |
| **p99.99** | 0.911 ms | 1.927 ms | +1.016 ms |
| **Max** | 1.895 ms | 1.983 ms | +0.088 ms |
| **Throughput** | 115,051.60 req/s | 115,831.10 req/s | +0.68% |

---

### 2. `PING_MBULK` (RESP Multibulk Protocol)

| Percentile | Valkey 9.1.2 Latency | RESP-CPP Latency | Difference |
| :--- | :--- | :--- | :--- |
| **p50 (Median)** | 0.215 ms | 0.215 ms | Identical (0.000 ms) |
| **p75** | 0.231 ms | 0.231 ms | Identical (0.000 ms) |
| **p87.5** | 0.247 ms | 0.247 ms | Identical (0.000 ms) |
| **p95** | 0.279 ms | 0.271 ms | -0.008 ms |
| **p99** | 0.335 ms | 0.311 ms | -0.024 ms |
| **p99.9** | 1.855 ms | 0.767 ms | -1.088 ms |
| **p99.99** | 3.471 ms | 2.487 ms | -0.984 ms |
| **Max** | 5.615 ms | 4.151 ms | -1.464 ms |
| **Throughput** | 113,622.80 req/s | 115,592.60 req/s | +1.73% |

---

### 3. `SET` (Write Mutation)

| Percentile | Valkey 9.1.2 Latency | RESP-CPP Latency | Difference |
| :--- | :--- | :--- | :--- |
| **p50 (Median)** | 0.223 ms | 0.215 ms | -0.008 ms |
| **p75** | 0.239 ms | 0.239 ms | Identical (0.000 ms) |
| **p87.5** | 0.255 ms | 0.255 ms | Identical (0.000 ms) |
| **p95** | 0.279 ms | 0.279 ms | Identical (0.000 ms) |
| **p99** | 0.335 ms | 0.327 ms | -0.008 ms |
| **p99.9** | 1.895 ms | 1.407 ms | -0.488 ms |
| **p99.99** | 2.503 ms | 2.487 ms | -0.016 ms |
| **Max** | 4.095 ms | 2.727 ms | -1.368 ms |
| **Throughput** | 113,209.20 req/s | 117,190.70 req/s | +3.52% |

---

### 4. `GET` (Read Query)

| Percentile | Valkey 9.1.2 Latency | RESP-CPP Latency | Difference |
| :--- | :--- | :--- | :--- |
| **p50 (Median)** | 0.223 ms | 0.223 ms | Identical (0.000 ms) |
| **p75** | 0.231 ms | 0.255 ms | +0.024 ms |
| **p87.5** | 0.247 ms | 0.303 ms | +0.056 ms |
| **p95** | 0.271 ms | 0.551 ms | +0.280 ms |
| **p99** | 0.303 ms | 1.903 ms | +1.600 ms |
| **p99.9** | 0.479 ms | 3.703 ms | +3.224 ms |
| **p99.99** | 2.343 ms | 5.743 ms | +3.400 ms |
| **Max** | 3.175 ms | 6.799 ms | +3.624 ms |
| **Throughput** | 116,295.30 req/s | 95,191.04 req/s | -18.1% |

---

## 📈 Sustained Throughput Summary Across Runs

| Test Duration | Engine | `PING_INLINE` | `PING_MBULK` | `SET` | `GET` |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **60s (Standard)** | **Valkey 9.1.2** | 115,408 req/s | 115,589 req/s | 116,163 req/s | 116,483 req/s |
| | **RESP-CPP** | 114,251 req/s | 114,380 req/s | 115,258 req/s | 116,059 req/s |
| **120s (Soak)** | **Valkey 9.1.2** | 115,812 req/s | 116,121 req/s | 116,120 req/s | 116,360 req/s |
| | **RESP-CPP** | 114,388 req/s | 114,115 req/s | 114,356 req/s | 115,088 req/s |
| **500s (Endurance)** | **Valkey 9.1.2** | 115,648 req/s | 115,845 req/s | 116,218 req/s | 116,024 req/s |
| | **RESP-CPP** | 113,118 req/s | 112,992 req/s | 113,094 req/s | 113,122 req/s |
