#!/usr/bin/env python3
import asyncio
import time
import argparse
import statistics

async def run_client(host, port, total_reqs, pipeline_depth, latencies):
    try:
        reader, writer = await asyncio.open_connection(host, port)
    except Exception:
        return

    sent = 0
    while sent < total_reqs:
        batch_size = min(pipeline_depth, total_reqs - sent)
        batch = bytearray()
        
        for j in range(batch_size):
            idx = sent + j
            key = f"k_{idx}"
            val = f"v_{idx}"
            if idx % 2 == 0:
                batch.extend(f"SET {key} {val}\n".encode())
            else:
                batch.extend(f"GET {key}\n".encode())

        t0 = time.perf_counter()
        writer.write(batch)
        await writer.drain()

        # Read back all responses in this pipeline batch
        for _ in range(batch_size):
            line = await reader.readline()
            if not line:
                break
            # If bulk string response ($<len>\r\n), read the payload line too
            if line.startswith(b"$") and not line.startswith(b"$-1"):
                await reader.readline()

        t1 = time.perf_counter()
        batch_latency = (t1 - t0) * 1000.0 / batch_size
        for _ in range(batch_size):
            latencies.append(batch_latency)

        sent += batch_size

    writer.close()
    await writer.wait_closed()

async def main():
    parser = argparse.ArgumentParser(description="RESP-CPP High-Performance Benchmark")
    parser.add_argument("--host", default="127.0.0.1", help="Server host")
    parser.add_argument("--port", type=int, default=6380, help="Server port")
    parser.add_argument("--clients", type=int, default=1000, help="Number of concurrent client connections")
    parser.add_argument("--requests", type=int, default=50000, help="Total number of requests to execute")
    parser.add_argument("--pipeline", type=int, default=1, help="Pipeline batch depth (e.g. 1 for ping-pong, 16 for pipelined)")
    args = parser.parse_args()

    reqs_per_client = max(1, args.requests // args.clients)
    actual_requests = reqs_per_client * args.clients

    print(f"============================================================")
    print(f"  RESP-CPP Engine Benchmark Suite")
    print(f"============================================================")
    print(f"Target:               {args.host}:{args.port}")
    print(f"Concurrent Clients:   {args.clients}")
    print(f"Pipeline Depth:       {args.pipeline}")
    print(f"Total Requests:       {actual_requests:,} ({reqs_per_client} per client)")
    print(f"Commands:             Alternating SET & GET")
    print(f"============================================================")
    print("Benchmarking in progress...")

    latencies = []
    start_time = time.perf_counter()

    tasks = [
        run_client(args.host, args.port, reqs_per_client, args.pipeline, latencies)
        for _ in range(args.clients)
    ]
    await asyncio.gather(*tasks)

    elapsed = time.perf_counter() - start_time
    qps = len(latencies) / elapsed if elapsed > 0 else 0

    latencies.sort()
    p50 = statistics.median(latencies) if latencies else 0
    p95 = latencies[int(len(latencies) * 0.95)] if latencies else 0
    p99 = latencies[int(len(latencies) * 0.99)] if latencies else 0
    avg = statistics.mean(latencies) if latencies else 0

    print("\n------------------------- Results -------------------------")
    print(f"[SUCCESS] Completed {len(latencies):,} requests across {args.clients:,} clients in {elapsed:.2f} seconds.")
    print(f"[RESULT]  Throughput:  {qps:,.0f} QPS (Requests/sec)")
    print(f"[LATENCY] Avg: {avg:.2f} ms | p50: {p50:.2f} ms | p95: {p95:.2f} ms | p99: {p99:.2f} ms")
    print(f"============================================================\n")

if __name__ == "__main__":
    asyncio.run(main())
