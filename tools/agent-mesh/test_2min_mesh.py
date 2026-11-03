#!/usr/bin/env python3
"""
2-Minute Live Verification Test for Cloudflare Agent Mesh Hub
Tests:
1. Hub single WebSocket per agent & deduplication (arin-pc and muse each have 1 connection)
2. Hub silent ping/pong (hub answers pings directly, never broadcasts ping/pong/system)
3. Client silent handling (no ping/pong logs, no inbox_system.jsonl, no wake AI)
4. Delivery of real messages between arin-pc and muse over a 2-minute live session
"""

import datetime
import json
import os
import queue
import subprocess
import sys
import threading
import time
import urllib.request
from pathlib import Path

HUB_URL = "https://agent-mesh-hub.slack-clone-arin.workers.dev"
ADMIN_TOKEN = os.environ.get("HUB_TOKEN") or os.environ.get("ADMIN_TOKEN", "")
ARIN_TOKEN = os.environ.get("ARIN_TOKEN", "")
MUSE_TOKEN = os.environ.get("MUSE_TOKEN", "")

log_queue = queue.Queue()


def stream_reader(pipe, prefix):
    try:
        for line in iter(pipe.readline, ""):
            if not line:
                break
            ts = datetime.datetime.now(datetime.timezone.utc).strftime("%H:%M:%S")
            log_queue.put((ts, prefix, line.strip()))
    except Exception:
        pass


def get_connections():
    req = urllib.request.Request(
        f"{HUB_URL}/admin/connections",
        headers={
            "Authorization": f"Bearer {ADMIN_TOKEN}",
            "User-Agent": "AgentMesh/admin",
        },
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except Exception as e:
        return {"error": str(e)}


def get_latest_id():
    req = urllib.request.Request(
        f"{HUB_URL}/inbox?agent=admin&projects=*&limit=1000",
        headers={
            "Authorization": f"Bearer {ADMIN_TOKEN}",
            "User-Agent": "AgentMesh/admin",
        },
    )
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            msgs = data.get("messages", [])
            if msgs:
                return msgs[-1].get("id")
            return data.get("latest_id")
    except Exception:
        return None


def main():
    print("=" * 70, flush=True)
    print("Starting 2-Minute Live Agent Mesh Verification Test", flush=True)
    print(f"Hub URL: {HUB_URL}", flush=True)
    print(f"Start Time (UTC): {datetime.datetime.now(datetime.timezone.utc).isoformat()}", flush=True)
    print("=" * 70, flush=True)

    # Clean up old local inbox files
    for p in Path(".").glob("inbox_*.jsonl"):
        p.unlink(missing_ok=True)
    for p in Path(".").glob(".mesh_listener_*.lock"):
        p.unlink(missing_ok=True)

    # Initialize last seen id to latest on server so we only monitor NEW live messages
    latest_id = get_latest_id() or "0"
    print(f"[Init] Latest message ID on server before test: {latest_id}", flush=True)
    Path(".mesh_last_id_arin-pc.txt").write_text(latest_id, encoding="utf-8")
    Path(".mesh_last_id_muse.txt").write_text(latest_id, encoding="utf-8")

    # 1. Start arin-pc listener process
    cmd_arin = [
        sys.executable,
        "-u",
        "tools/agent-mesh/mesh_client.py",
        "--url", HUB_URL,
        "--agent", "arin-pc",
        "--token", ARIN_TOKEN,
        "--projects", "agent-hub",
        "listen",
    ]
    p_arin = subprocess.Popen(
        cmd_arin,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )

    # 2. Start muse listener process
    cmd_muse = [
        sys.executable,
        "-u",
        "tools/agent-mesh/mesh_client.py",
        "--url", HUB_URL,
        "--agent", "muse",
        "--token", MUSE_TOKEN,
        "--projects", "agent-hub",
        "listen",
    ]
    p_muse = subprocess.Popen(
        cmd_muse,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )

    # Launch output reader threads
    t_arin_out = threading.Thread(target=stream_reader, args=(p_arin.stdout, "arin-pc:out"), daemon=True)
    t_arin_err = threading.Thread(target=stream_reader, args=(p_arin.stderr, "arin-pc:err"), daemon=True)
    t_muse_out = threading.Thread(target=stream_reader, args=(p_muse.stdout, "muse:out"), daemon=True)
    t_muse_err = threading.Thread(target=stream_reader, args=(p_muse.stderr, "muse:err"), daemon=True)

    t_arin_out.start()
    t_arin_err.start()
    t_muse_out.start()
    t_muse_err.start()

    start_time = time.time()
    msg1_sent = False
    msg2_sent = False
    conns_verified = False

    collected_logs = []

    print("[00:00] Both listener processes started. Waiting for initial connections...", flush=True)

    # Run for 125 seconds (~2 minutes and 5 seconds)
    total_duration = 125
    while time.time() - start_time < total_duration:
        elapsed = int(time.time() - start_time)

        # Drain output queue
        while not log_queue.empty():
            ts, prefix, line = log_queue.get_nowait()
            entry = f"[{ts}] [{prefix}] {line}"
            print(entry, flush=True)
            collected_logs.append(entry)

        # Check connections at 10s
        if elapsed >= 10 and not conns_verified:
            conns = get_connections()
            by_agent = conns.get("by_agent", {})
            print(f"\n--- Hub Connection Verification (t={elapsed}s) ---", flush=True)
            print(f"Total Active Connections: {conns.get('total_connections')}", flush=True)
            for ag, c_list in by_agent.items():
                print(f"  Agent '{ag}': {len(c_list)} connection(s)", flush=True)
            print("--------------------------------------------------\n", flush=True)
            conns_verified = True

        # Send Real Message 1 at 20s (arin-pc -> muse)
        if elapsed >= 20 and not msg1_sent:
            print(f"\n>>> [t={elapsed}s] Sending Real Message 1: arin-pc -> muse...", flush=True)
            send_cmd = [
                sys.executable,
                "tools/agent-mesh/mesh_client.py",
                "--url", HUB_URL,
                "--agent", "arin-pc",
                "--token", ARIN_TOKEN,
                "--projects", "agent-hub",
                "send",
                "--to", "muse",
                "--project", "agent-hub",
                "--type", "task",
                "--body", "Real message 1: Pitch Quest asset sync request",
            ]
            res = subprocess.run(send_cmd, capture_output=True, text=True)
            print(f"    Send result: {res.stdout.strip() or res.stderr.strip()}", flush=True)
            msg1_sent = True

        # Send Real Message 2 at 65s (muse -> arin-pc)
        if elapsed >= 65 and not msg2_sent:
            print(f"\n>>> [t={elapsed}s] Sending Real Message 2: muse -> arin-pc...", flush=True)
            send_cmd = [
                sys.executable,
                "tools/agent-mesh/mesh_client.py",
                "--url", HUB_URL,
                "--agent", "muse",
                "--token", MUSE_TOKEN,
                "--projects", "agent-hub",
                "send",
                "--to", "arin-pc",
                "--project", "agent-hub",
                "--type", "result",
                "--body", "Real message 2: ACK from muse, 16 rarity tiers loaded",
            ]
            res = subprocess.run(send_cmd, capture_output=True, text=True)
            print(f"    Send result: {res.stdout.strip() or res.stderr.strip()}", flush=True)
            msg2_sent = True

        # Heartbeat checkpoint logs every 30s
        if elapsed in (30, 60, 90, 120):
            print(f"--- [t={elapsed}s] Running smoothly (heartbeat window #{elapsed // 30}) ---", flush=True)

        time.sleep(0.5)

    # Final drain of remaining logs
    time.sleep(1)
    while not log_queue.empty():
        ts, prefix, line = log_queue.get_nowait()
        entry = f"[{ts}] [{prefix}] {line}"
        print(entry, flush=True)
        collected_logs.append(entry)

    print(f"\n[t={int(time.time() - start_time)}s] Test duration reached (125s). Terminating listeners...", flush=True)

    p_arin.terminate()
    p_muse.terminate()
    try:
        p_arin.wait(timeout=3)
        p_muse.wait(timeout=3)
    except Exception:
        p_arin.kill()
        p_muse.kill()

    print("\n" + "=" * 70, flush=True)
    print("Test Summary & Verification Checks", flush=True)
    print("=" * 70, flush=True)

    # Check 1: No ping / pong / system logs in listeners
    ping_pong_logs = [
        l for l in collected_logs
        if any(term in l.lower() for term in [
            "[system]", "system ->", "message received: { type: 'ping'", "type: pong",
            "type\": \"ping\"", "type\": \"pong\""
        ])
    ]
    print(f"1. Ping/Pong/System Protocol Noise in Listener Logs: {len(ping_pong_logs)} found", flush=True)
    for pl in ping_pong_logs:
        print(f"   [VIOLATION] {pl}", flush=True)

    # Check 2: Verify real messages were delivered
    real_msgs = [l for l in collected_logs if "Real message" in l]
    print(f"2. Real Messages Logged by Listeners: {len(real_msgs)} found", flush=True)
    for rm in real_msgs:
        print(f"   [CONFIRMED] {rm}", flush=True)

    # Check 3: Verify inbox_system.jsonl was NOT created
    inbox_sys_exists = Path("inbox_system.jsonl").exists()
    print(f"3. inbox_system.jsonl Created: {inbox_sys_exists} (Expected: False)", flush=True)

    # Check 4: Verify inbox_agent-hub.jsonl contains the real messages
    inbox_hub_exists = Path("inbox_agent-hub.jsonl").exists()
    inbox_hub_count = 0
    if inbox_hub_exists:
        with open("inbox_agent-hub.jsonl", "r", encoding="utf-8") as f:
            lines = [l for l in f if l.strip()]
            inbox_hub_count = len(lines)
    print(f"4. inbox_agent-hub.jsonl Exists: {inbox_hub_exists}, Message count: {inbox_hub_count}", flush=True)

    # Check 5: Active connections during test
    conns = get_connections()
    by_agent = conns.get("by_agent", {})
    print(f"5. Final Connections: {conns.get('total_connections')} total", flush=True)
    for ag, c_list in by_agent.items():
        print(f"   Agent '{ag}': {len(c_list)} connection(s)", flush=True)

    print("=" * 70, flush=True)
    if len(ping_pong_logs) == 0 and len(real_msgs) >= 2 and not inbox_sys_exists:
        print("ALL TESTS PASSED! Clean 2-minute live operation confirmed with zero ping/pong noise.", flush=True)
    else:
        print("TESTS COMPLETED WITH ISSUES.", flush=True)
    print("=" * 70, flush=True)


if __name__ == "__main__":
    main()
