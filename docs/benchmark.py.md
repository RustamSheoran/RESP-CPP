# Documentation: `benchmark.py`

The [`benchmark.py`](file:///home/rustam/Projects/resp-cpp/benchmark.py) file is an asynchronous, high-concurrency performance benchmarking suite written in Python 3 using `asyncio`. It simulates hundreds to thousands of concurrent client connections issuing pipelined Redis requests, measuring throughput (QPS) and tail latency percentiles (p50, p95, p99).

---

## Complete Source Code

```python
1: #!/usr/bin/env python3
2: import asyncio
3: import time
4: import argparse
5: import statistics
6: 
7: async def run_client(host, port, total_reqs, pipeline_depth, latencies):
8:     try:
9:         reader, writer = await asyncio.open_connection(host, port)
10:     except Exception:
11:         return
12: 
13:     sent = 0
14:     while sent < total_reqs:
15:         batch_size = min(pipeline_depth, total_reqs - sent)
16:         batch = bytearray()
17:         
18:         for j in range(batch_size):
19:             idx = sent + j
20:             key = f"k_{idx}"
21:             val = f"v_{idx}"
22:             if idx % 2 == 0:
23:                 batch.extend(f"SET {key} {val}\n".encode())
24:             else:
25:                 batch.extend(f"GET {key}\n".encode())
26: 
27:         t0 = time.perf_counter()
28:         writer.write(batch)
29:         await writer.drain()
30: 
31:         # Read back all responses in this pipeline batch
32:         for _ in range(batch_size):
33:             line = await reader.readline()
34:             if not line:
35:                 break
36:             # If bulk string response ($<len>\r\n), read the payload line too
37:             if line.startswith(b"$") and not line.startswith(b"$-1"):
38:                 await reader.readline()
39: 
40:         t1 = time.perf_counter()
41:         batch_latency = (t1 - t0) * 1000.0 / batch_size
42:         for _ in range(batch_size):
43:             latencies.append(batch_latency)
44: 
45:         sent += batch_size
46: 
47:     writer.close()
48:     await writer.wait_closed()
49: 
50: async def main():
51:     parser = argparse.ArgumentParser(description="RESP-CPP High-Performance Benchmark")
52:     parser.add_argument("--host", default="127.0.0.1", help="Server host")
53:     parser.add_argument("--port", type=int, default=6380, help="Server port")
54:     parser.add_argument("--clients", type=int, default=1000, help="Number of concurrent client connections")
55:     parser.add_argument("--requests", type=int, default=50000, help="Total number of requests to execute")
56:     parser.add_argument("--pipeline", type=int, default=1, help="Pipeline batch depth (e.g. 1 for ping-pong, 16 for pipelined)")
57:     args = parser.parse_args()
58: 
59:     reqs_per_client = max(1, args.requests // args.clients)
60:     actual_requests = reqs_per_client * args.clients
61: 
62:     print(f"============================================================")
63:     print(f"  RESP-CPP Engine Benchmark Suite")
64:     print(f"============================================================")
65:     print(f"Target:               {args.host}:{args.port}")
66:     print(f"Concurrent Clients:   {args.clients}")
67:     print(f"Pipeline Depth:       {args.pipeline}")
68:     print(f"Total Requests:       {actual_requests:,} ({reqs_per_client} per client)")
69:     print(f"Commands:             Alternating SET & GET")
70:     print(f"============================================================")
71:     print("Benchmarking in progress...")
72: 
73:     latencies = []
74:     start_time = time.perf_counter()
75: 
76:     tasks = [
77:         run_client(args.host, args.port, reqs_per_client, args.pipeline, latencies)
78:         for _ in range(args.clients)
79:     ]
80:     await asyncio.gather(*tasks)
81: 
82:     elapsed = time.perf_counter() - start_time
83:     qps = len(latencies) / elapsed if elapsed > 0 else 0
84: 
85:     latencies.sort()
86:     p50 = statistics.median(latencies) if latencies else 0
87:     p95 = latencies[int(len(latencies) * 0.95)] if latencies else 0
88:     p99 = latencies[int(len(latencies) * 0.99)] if latencies else 0
89:     avg = statistics.mean(latencies) if latencies else 0
90: 
91:     print("\n------------------------- Results -------------------------")
92:     print(f"[SUCCESS] Completed {len(latencies):,} requests across {args.clients:,} clients in {elapsed:.2f} seconds.")
93:     print(f"[RESULT]  Throughput:  {qps:,.0f} QPS (Requests/sec)")
94:     print(f"[LATENCY] Avg: {avg:.2f} ms | p50: {p50:.2f} ms | p95: {p95:.2f} ms | p99: {p99:.2f} ms")
95:     print(f"============================================================\n")
96: 
97: if __name__ == "__main__":
98:     asyncio.run(main())
```

---

## Line-by-Line Breakdown & Explanation

### Shebang & Imports (Lines 1–5)
- **Line 1: `#!/usr/bin/env python3`**  
  POSIX shebang specifying execution using the Python 3 interpreter.
- **Line 2: `import asyncio`**  
  Asynchronous I/O framework enabling cooperative multitasking and non-blocking TCP socket streams.
- **Line 3: `import time`**  
  High-resolution performance counter (`time.perf_counter()`) for microsecond-accurate timing.
- **Line 4: `import argparse`**  
  Command-line option parsing library.
- **Line 5: `import statistics`**  
  Mathematical statistics module for computing mean and median latencies.

---

### Asynchronous Client Worker: `run_client` (Lines 7–48)
- **Line 7: `async def run_client(host, port, total_reqs, pipeline_depth, latencies):`**  
  Coroutine representing an independent concurrent client issuing `total_reqs` requests in pipelined batches of `pipeline_depth`.
- **Line 8–11:**  
  Attempts connection via `asyncio.open_connection(host, port)`. Silently returns if connection fails.
- **Line 13: `    sent = 0`**  
  Tracks total requests sent by this client instance.
- **Line 14: `    while sent < total_reqs:`**  
  Loops until the client completes its assigned request quota.
- **Line 15: `        batch_size = min(pipeline_depth, total_reqs - sent)`**  
  Calculates the request batch size for this iteration without exceeding remaining quota.
- **Line 16: `        batch = bytearray()`**  
  Initializes a mutable byte buffer to pack pipelined commands into a single TCP write.
- **Lines 18–25:**  
  Generates alternating `SET` and `GET` commands:
  - If `idx % 2 == 0`: Appends `SET k_<idx> v_<idx>\n`.
  - Otherwise: Appends `GET k_<idx>\n`.
- **Line 27: `        t0 = time.perf_counter()`**  
  Captures high-precision timestamp before socket dispatch.
- **Line 28: `        writer.write(batch)`**  
  Buffers the entire pipelined batch for transmission.
- **Line 29: `        await writer.drain()`**  
  Flushes the batch over the TCP socket.
- **Lines 32–38:**  
  Reads back the `batch_size` responses:
  - Line 33: Reads the response header line (`readline()`).
  - Line 34–35: Handles premature EOF disconnects.
  - Line 37–38: If the response is a bulk string (starts with `$` and is not `$-1`), reads the subsequent payload line (`<val>\r\n`).
- **Line 40: `        t1 = time.perf_counter()`**  
  Captures completion timestamp.
- **Line 41: `        batch_latency = (t1 - t0) * 1000.0 / batch_size`**  
  Computes average per-request latency for the batch in milliseconds.
- **Lines 42–43: `        for _ in range(batch_size): latencies.append(batch_latency)`**  
  Appends calculated latency to the shared latency distribution array.
- **Line 45: `        sent += batch_size`**  
  Advances sent counter.
- **Line 47–48: `    writer.close(); await writer.wait_closed()`**  
  Closes socket connection and waits for OS stream cleanup.

---

### Benchmark Orchestrator: `main` (Lines 50–95)
- **Line 50: `async def main():`**  
  Main benchmark runner.
- **Lines 51–57:**  
  Defines CLI arguments (`--host`, `--port`, `--clients`, `--requests`, `--pipeline`).
- **Line 59–60:**  
  Distributes total requests evenly across client tasks (`reqs_per_client`).
- **Lines 62–71:**  
  Prints formatted benchmark configuration header.
- **Line 73: `    latencies = []`**  
  Initializes array collecting all individual request latencies.
- **Line 74: `    start_time = time.perf_counter()`**  
  Records benchmark start timestamp.
- **Lines 76–80:**  
  Instantiates concurrent client tasks and launches them simultaneously via `asyncio.gather(*tasks)`.
- **Line 82: `    elapsed = time.perf_counter() - start_time`**  
  Measures total wall-clock benchmark elapsed time in seconds.
- **Line 83: `    qps = len(latencies) / elapsed if elapsed > 0 else 0`**  
  Calculates overall queries per second (QPS).
- **Line 85: `    latencies.sort()`**  
  Sorts latencies in ascending order for percentile extraction.
- **Lines 86–89:**  
  Calculates median (`p50`), 95th percentile (`p95`), 99th percentile (`p99`), and arithmetic average (`avg`).
- **Lines 91–95:**  
  Prints final benchmark report with throughput and latency percentiles.

---

### Script Entry Point (Lines 97–98)
- **Lines 97–98: `if __name__ == "__main__": asyncio.run(main())`**  
  Standard Python idiom running `main()` within the asyncio event loop.
