#!/usr/bin/env python3
"""
Agent Mesh Client (Cross-Platform: Windows & Linux)
Background listener (WebSocket with keep-alive + HTTP polling fallback) & CLI message sender.

Environment Variables:
  HUB_URL      : Base URL of Cloudflare Agent Mesh Hub (default: https://agent-mesh-hub.slack-clone-arin.workers.dev)
  AGENT_NAME   : Name of this agent (e.g. arin-pc, muse)
  AGENT_TOKEN  : Authentication token for this agent
  PROJECTS     : Comma-separated list of allowed projects (default: agent-hub)

Usage:
  # Background listener (default):
  python mesh_client.py listen
  python mesh_client.py

  # Send a message:
  python mesh_client.py send --to muse --project agent-hub --body "Hello Muse"
  python mesh_client.py send --to arin-pc --project agent-hub --type chat --body "Ready to sync"

  # Check status:
  python mesh_client.py status
"""

import argparse
import asyncio
import datetime
import json
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid
from pathlib import Path

# Check for websockets library
try:
    import websockets
    HAS_WEBSOCKETS = True
except ImportError:
    HAS_WEBSOCKETS = False

DEFAULT_HUB_URL = os.environ.get(
    "HUB_URL", "https://agent-mesh-hub.slack-clone-arin.workers.dev"
).rstrip("/")


def get_config(args=None):
    hub_url = (getattr(args, "url", None) or os.environ.get("HUB_URL") or DEFAULT_HUB_URL).rstrip("/")
    agent = getattr(args, "agent", None) or os.environ.get("AGENT_NAME")
    token = getattr(args, "token", None) or os.environ.get("AGENT_TOKEN")
    projects_raw = getattr(args, "projects", None) or os.environ.get("PROJECTS") or "agent-hub"
    projects = [p.strip() for p in projects_raw.split(",") if p.strip()]

    if not agent:
        print("[Error] AGENT_NAME is required (set env var AGENT_NAME or pass --agent)", file=sys.stderr)
        sys.exit(1)
    if not token:
        print("[Error] AGENT_TOKEN is required (set env var AGENT_TOKEN or pass --token)", file=sys.stderr)
        sys.exit(1)

    return {
        "hub_url": hub_url,
        "agent": agent,
        "token": token,
        "projects": projects,
        "state_file": Path(f".mesh_last_id_{agent}.txt"),
    }


class SingleInstanceLock:
    """Ensures only one listener process runs per agent on this machine."""
    def __init__(self, agent: str):
        self.agent = agent
        self.lock_file = Path(f".mesh_listener_{agent}.lock")
        self.file_handle = None

    def acquire(self) -> bool:
        try:
            self.lock_file.parent.mkdir(parents=True, exist_ok=True)
            self.file_handle = open(self.lock_file, "a+", encoding="utf-8")
            if sys.platform == "win32":
                import msvcrt
                self.file_handle.seek(0)
                msvcrt.locking(self.file_handle.fileno(), msvcrt.LK_NBLCK, 1)
            else:
                import fcntl
                fcntl.flock(self.file_handle.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            self.file_handle.seek(0)
            self.file_handle.truncate()
            self.file_handle.write(f"{os.getpid()}\n")
            self.file_handle.flush()
            return True
        except (IOError, OSError):
            if self.file_handle:
                try:
                    self.file_handle.close()
                except Exception:
                    pass
                self.file_handle = None
            return False

    def release(self):
        try:
            if self.file_handle:
                if sys.platform == "win32":
                    import msvcrt
                    self.file_handle.seek(0)
                    msvcrt.locking(self.file_handle.fileno(), msvcrt.LK_UNLCK, 1)
                else:
                    import fcntl
                    fcntl.flock(self.file_handle.fileno(), fcntl.LOCK_UN)
                self.file_handle.close()
                self.file_handle = None
                self.lock_file.unlink(missing_ok=True)
        except Exception:
            pass


def get_last_seen_id(state_file: Path) -> str:
    try:
        if state_file.exists():
            content = state_file.read_text(encoding="utf-8").strip()
            if content and content != "0" and content != "null":
                return content
    except Exception as e:
        print(f"[Warning] Could not read state file: {e}", file=sys.stderr)
    return "0"


def save_last_seen_id(state_file: Path, message_id: str):
    if not message_id or message_id in ("0", "null", "undefined"):
        return
    try:
        tmp_file = state_file.with_suffix(".tmp")
        tmp_file.write_text(message_id.strip(), encoding="utf-8")
        try:
            tmp_file.replace(state_file)
        except Exception:
            state_file.write_text(message_id.strip(), encoding="utf-8")
            if tmp_file.exists():
                tmp_file.unlink(missing_ok=True)
    except Exception as e:
        print(f"[Warning] Could not save state file: {e}", file=sys.stderr)


def log_message(msg: dict, state_file: Path, seen_ids: set):
    if not isinstance(msg, dict):
        return

    # Drop ping/pong and system messages silently (never log, never save to inbox, never wake AI)
    msg_type = (msg.get("type") or "").strip().lower()
    msg_proj = (msg.get("project") or "").strip().lower()
    if msg_type in ("ping", "pong") or msg_proj == "system":
        return

    msg_id = msg.get("id")
    if not msg_id or msg_id in seen_ids:
        return
    seen_ids.add(msg_id)
    if len(seen_ids) > 2000:
        seen_ids.pop()

    project = msg_proj or "general"
    sender = msg.get("from", "unknown")
    recipient = msg.get("to", "all")
    body = msg.get("body", "").replace("\n", " ")
    snippet = (body[:75] + "...") if len(body) > 75 else body
    timestamp = msg.get("timestamp", datetime.datetime.now(datetime.timezone.utc).isoformat())
    time_str = timestamp[:19].replace("T", " ")

    # Append to JSONL inbox file
    inbox_file = Path(f"inbox_{project}.jsonl")
    try:
        with open(inbox_file, "a", encoding="utf-8") as f:
            f.write(json.dumps(msg, ensure_ascii=False) + "\n")
    except Exception as e:
        print(f"[Warning] Failed writing to {inbox_file}: {e}", file=sys.stderr)

    # Print one-line notice
    print(f"[{time_str}] [{project}] {sender} -> {recipient}: {snippet}")

    # Persist last seen ID
    save_last_seen_id(state_file, msg_id)


def http_request(url: str, token: str, agent: str, method="GET", body=None):
    headers = {
        "Authorization": f"Bearer {token}",
        "User-Agent": f"AgentMesh/{agent}",
        "Content-Type": "application/json",
    }
    data = json.dumps(body).encode("utf-8") if body else None
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=15) as resp:
            content = resp.read().decode("utf-8")
            return resp.status, json.loads(content) if content else {}
    except urllib.error.HTTPError as e:
        err_content = e.read().decode("utf-8")
        try:
            return e.code, json.loads(err_content)
        except Exception:
            return e.code, {"error": err_content}
    except Exception as e:
        return 0, {"error": str(e)}


def cmd_send(args):
    cfg = get_config(args)
    url = f"{cfg['hub_url']}/send"
    project = getattr(args, "project", None) or cfg["projects"][0]

    payload = {
        "id": str(uuid.uuid4()),
        "project": project,
        "from": cfg["agent"],
        "to": args.to.strip(),
        "type": args.type or "task",
        "body": args.body,
        "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    }
    if getattr(args, "reply_to", None):
        payload["reply_to"] = args.reply_to.strip()

    status, res = http_request(url, cfg["token"], cfg["agent"], method="POST", body=payload)
    if status == 200 and res.get("ok"):
        print(f"[Sent] id={res.get('id')} to={payload['to']} project={payload['project']}")
    else:
        print(f"[Error {status}] Failed to send: {res.get('error', res)}", file=sys.stderr)
        sys.exit(1)


def cmd_status(args):
    cfg = get_config(args)
    url = f"{cfg['hub_url']}/status"
    status, res = http_request(url, cfg["token"], cfg["agent"], method="GET")

    print("\n" + "=" * 55)
    print(f" Agent Mesh Client Status — [{cfg['agent']}]")
    print("=" * 55)
    print(f" Hub URL:        {cfg['hub_url']}")
    print(f" Agent Name:     {cfg['agent']}")
    print(f" Projects:       {', '.join(cfg['projects'])}")
    print(f" Token:          [CONFIGURED] (length: {len(cfg['token'])})")
    last_id = get_last_seen_id(cfg["state_file"])
    print(f" Last Seen ID:   {last_id if last_id != '0' else '(none)'}")
    print(f" WebSocket Lib:  {'Available (websockets)' if HAS_WEBSOCKETS else 'Not Installed (polling mode)'}")
    print("-" * 55)

    if status == 200:
        print(f" Hub Status:     {res.get('status', 'online')}")
        print(f" Hub Time:       {res.get('time')}")
        print(f" Active Conns:   {res.get('connections', 0)}")
        by_agent = res.get("by_agent", {})
        if by_agent:
            print(f" Online Agents:  {len(by_agent)}")
            for a_name, conns in by_agent.items():
                dup_str = f" [WARNING: {len(conns)} active connections!]" if len(conns) > 1 else ""
                projs = ", ".join(conns[0].get("projects", []))
                print(f"   • {a_name} ({len(conns)} conn): {projs}{dup_str}")
        else:
            agents = res.get("agents", [])
            if agents:
                print(f" Online Agents:  {len(agents)}")
                for a in agents:
                    print(f"   • {a.get('agent')} ({', '.join(a.get('projects', []))})")
    else:
        print(f" Hub Error:      HTTP {status} — {res.get('error', res)}")
    print("=" * 55 + "\n")


def cmd_connections(args):
    cfg = get_config(args)
    url = f"{cfg['hub_url']}/admin/connections"
    status, res = http_request(url, cfg["token"], cfg["agent"], method="GET")
    if status != 200:
        url = f"{cfg['hub_url']}/connections"
        status, res = http_request(url, cfg["token"], cfg["agent"], method="GET")
    if status != 200:
        url = f"{cfg['hub_url']}/status"
        status, res = http_request(url, cfg["token"], cfg["agent"], method="GET")

    print("\n" + "=" * 55)
    print(" Active WebSocket Connections per Agent")
    print("=" * 55)
    if status == 200:
        by_agent = res.get("by_agent", {})
        total = res.get("total_connections", res.get("connections", 0))
        print(f" Total Connections: {total}")
        print(f" Unique Agents:     {len(by_agent) if by_agent else len(res.get('agents', []))}")
        print("-" * 55)
        if by_agent:
            for ag, conns in by_agent.items():
                dup_alert = " *** DUPLICATE DETECTED ***" if len(conns) > 1 else ""
                print(f" Agent: {ag} ({len(conns)} active connection{'s' if len(conns) != 1 else ''}){dup_alert}")
                for idx, c in enumerate(conns, 1):
                    print(f"   [{idx}] Connected: {c.get('connectedAt')}, Projects: {', '.join(c.get('projects', []))}")
        else:
            for a in res.get("agents", []):
                print(f" Agent: {a.get('agent')}, Connected: {a.get('connectedAt')}, Projects: {', '.join(a.get('projects', []))}")
    else:
        print(f" Failed to retrieve connections: HTTP {status} — {res.get('error', res)}")
    print("=" * 55 + "\n")


async def run_polling_loop(cfg: dict, seen_ids: set):
    print(f"[{cfg['agent']}] Running in HTTP Polling Mode (interval: 30s)...")
    projs_str = ",".join(cfg["projects"])

    while True:
        last_id = get_last_seen_id(cfg["state_file"])
        qs = urllib.parse.urlencode({
            "agent": cfg["agent"],
            "projects": projs_str,
            "since": last_id,
        })
        url = f"{cfg['hub_url']}/inbox?{qs}"

        status, res = http_request(url, cfg["token"], cfg["agent"], method="GET")
        if status == 200:
            messages = res.get("messages", [])
            for msg in messages:
                m_type = (msg.get("type") or "").strip().lower()
                m_proj = (msg.get("project") or "").strip().lower()
                if m_type not in ("ping", "pong") and m_proj != "system":
                    log_message(msg, cfg["state_file"], seen_ids)
            latest_id = res.get("latest_id")
            if latest_id and latest_id not in ("0", "null"):
                save_last_seen_id(cfg["state_file"], latest_id)
        else:
            print(f"[Poll Error {status}] {res.get('error', 'Request failed')}", file=sys.stderr)

        await asyncio.sleep(30)


async def run_websocket_loop(cfg: dict, seen_ids: set):
    ws_url = cfg["hub_url"].replace("https://", "wss://").replace("http://", "ws://")
    projs_str = ",".join(cfg["projects"])
    backoff = 2
    max_backoff = 30

    while True:
        last_id = get_last_seen_id(cfg["state_file"])
        qs_params = {
            "agent": cfg["agent"],
            "projects": projs_str,
            "token": cfg["token"],
            "since": last_id,
        }

        endpoint = f"{ws_url}/connect?{urllib.parse.urlencode(qs_params)}"
        connect_time = time.time()

        try:
            print(f"[{cfg['agent']}] Connecting WebSocket to {ws_url} (projects: {projs_str})...")
            headers_dict = {"User-Agent": f"AgentMesh/{cfg['agent']}"}
            connect_kwargs = {
                "ping_interval": None,  # Hub alarm heartbeat handles ping/pong; disable client 20s auto-ping
                "ping_timeout": None,
                "close_timeout": 10,
            }
            try:
                import inspect
                sig = inspect.signature(websockets.connect)
                if "additional_headers" in sig.parameters:
                    connect_kwargs["additional_headers"] = headers_dict
                else:
                    connect_kwargs["extra_headers"] = headers_dict
            except Exception:
                connect_kwargs["additional_headers"] = headers_dict

            async with websockets.connect(endpoint, **connect_kwargs) as ws:
                print(f"[{cfg['agent']}] WebSocket connected and active. Listening for messages...")

                async for raw_message in ws:
                    # Reset backoff if connected stably
                    if time.time() - connect_time > 15:
                        backoff = 2

                    if isinstance(raw_message, bytes):
                        raw_message = raw_message.decode("utf-8")
                    trimmed = raw_message.strip()

                    # Handle string pings/pongs silently
                    if trimmed.lower() == "ping":
                        try:
                            await ws.send("pong")
                        except Exception:
                            pass
                        continue
                    if trimmed.lower() == "pong":
                        continue

                    try:
                        data = json.loads(raw_message)
                    except json.JSONDecodeError:
                        continue

                    msg_type = (data.get("type") or "").strip().lower()
                    msg_proj = (data.get("project") or "").strip().lower()

                    # Handle JSON pings silently (reply directly with pong, never log, never save, never wake AI)
                    if msg_type == "ping":
                        pong_payload = json.dumps({
                            "type": "pong",
                            "project": "system",
                            "timestamp": datetime.datetime.now(datetime.timezone.utc).isoformat(),
                        })
                        try:
                            await ws.send(pong_payload)
                        except Exception:
                            pass
                        continue

                    # Ignore JSON pongs and system project messages silently
                    if msg_type == "pong" or msg_proj == "system":
                        continue

                    if msg_type == "error":
                        print(f"[Hub Notice] {data.get('error')}", file=sys.stderr)
                        continue

                    log_message(data, cfg["state_file"], seen_ids)

                if getattr(ws, "close_code", None) in (1008, 4000) or (getattr(ws, "close_reason", None) and "Superceded" in str(ws.close_reason)):
                    if ws.close_code == 1008:
                        print("[Fatal] Token was revoked by hub. Exiting.", file=sys.stderr)
                        sys.exit(1)
                    print(f"[{cfg['agent']}] Connection closed: superseded by newer session. Exiting listener.", file=sys.stderr)
                    sys.exit(0)

        except websockets.exceptions.ConnectionClosed as e:
            print(f"[WebSocket Closed] Code {e.code}: {e.reason or 'Connection closed by server'}")
            if e.code == 1008:
                print("[Fatal] Token was revoked by hub. Exiting.", file=sys.stderr)
                sys.exit(1)
            if e.code == 4000 or (e.reason and "Superceded" in str(e.reason)):
                print(f"[{cfg['agent']}] Connection closed: superseded by newer session. Exiting listener.", file=sys.stderr)
                sys.exit(0)
        except Exception as e:
            print(f"[WebSocket Error] {e}")

        # Fallback HTTP poll while WebSocket is down
        try:
            current_id = get_last_seen_id(cfg["state_file"])
            qs = urllib.parse.urlencode({"agent": cfg["agent"], "projects": projs_str, "since": current_id})
            status, res = http_request(f"{cfg['hub_url']}/inbox?{qs}", cfg["token"], cfg["agent"])
            if status == 200:
                for msg in res.get("messages", []):
                    m_type = (msg.get("type") or "").strip().lower()
                    m_proj = (msg.get("project") or "").strip().lower()
                    if m_type not in ("ping", "pong") and m_proj != "system":
                        log_message(msg, cfg["state_file"], seen_ids)
                if res.get("latest_id") and res["latest_id"] not in ("0", "null"):
                    save_last_seen_id(cfg["state_file"], res["latest_id"])
            elif status == 401:
                print(f"[Fatal] HTTP 401: {res.get('error', 'Unauthorized')}. Exiting.", file=sys.stderr)
                sys.exit(1)
        except Exception as e:
            print(f"[Warning] Polling fallback error: {e}", file=sys.stderr)

        print(f"[{cfg['agent']}] Reconnecting in {backoff}s...")
        await asyncio.sleep(backoff)
        backoff = min(backoff * 2, max_backoff)


def cmd_listen(args):
    cfg = get_config(args)
    lock = SingleInstanceLock(cfg["agent"])
    if not lock.acquire():
        print(f"[Error] Another listener for agent '{cfg['agent']}' is already running on this machine.", file=sys.stderr)
        print("Only one listener per agent name is permitted.", file=sys.stderr)
        sys.exit(1)

    try:
        seen_ids = set()

        # Pre-populate seen_ids from existing inbox files to prevent duplicate logs/prints
        for p in cfg["projects"]:
            inbox_f = Path(f"inbox_{p}.jsonl")
            if inbox_f.exists():
                try:
                    for line in inbox_f.read_text(encoding="utf-8").splitlines():
                        if line.strip():
                            try:
                                m = json.loads(line)
                                if m.get("id"):
                                    seen_ids.add(m["id"])
                            except Exception:
                                pass
                except Exception:
                    pass

        last_id = get_last_seen_id(cfg["state_file"])
        print(f"\n=======================================================")
        print(f" Agent Mesh Client — [{cfg['agent']}]")
        print(f" Projects:     {', '.join(cfg['projects'])}")
        print(f" Hub:          {cfg['hub_url']}")
        print(f" Last Seen ID: {last_id if last_id != '0' else '(none - will sync all)'}")
        print(f" Transport:    {'WebSocket (Keep-Alive)' if HAS_WEBSOCKETS else 'HTTP Polling (30s)'}")
        print(f"=======================================================\n")

        if HAS_WEBSOCKETS:
            try:
                asyncio.run(run_websocket_loop(cfg, seen_ids))
            except KeyboardInterrupt:
                print(f"\n[{cfg['agent']}] Listener stopped by user.")
        else:
            try:
                asyncio.run(run_polling_loop(cfg, seen_ids))
            except KeyboardInterrupt:
                print(f"\n[{cfg['agent']}] Listener stopped by user.")
    finally:
        lock.release()


def main():
    parser = argparse.ArgumentParser(
        description="Agent Mesh Client: Background Listener & CLI Sender",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--url", help=f"Hub URL (default: {DEFAULT_HUB_URL})")
    parser.add_argument("--agent", help="Agent name (e.g. arin-pc, muse)")
    parser.add_argument("--token", help="Agent authentication token")
    parser.add_argument("--projects", help="Comma-separated project names (e.g. agent-hub)")

    sub = parser.add_subparsers(dest="command")

    # listen (default)
    p_listen = sub.add_parser("listen", help="Start background message listener (WebSocket/Polling)")
    p_listen.set_defaults(func=cmd_listen)

    # send
    p_send = sub.add_parser("send", help="Send a message to another agent")
    p_send.add_argument("--to", required=True, help="Recipient agent name (e.g. muse, arin-pc, all)")
    p_send.add_argument("--body", required=True, help="Message body text")
    p_send.add_argument("--project", help="Target project (default: first configured project)")
    p_send.add_argument("--type", default="task", choices=["task", "chat", "result", "status"], help="Message type")
    p_send.add_argument("--reply-to", help="Optional message ID being replied to")
    p_send.set_defaults(func=cmd_send)

    # status
    p_status = sub.add_parser("status", help="Show client and hub connection status")
    p_status.set_defaults(func=cmd_status)

    # connections
    p_conn = sub.add_parser("connections", help="List active WebSocket connections grouped per agent")
    p_conn.set_defaults(func=cmd_connections)

    args = parser.parse_args()

    if not args.command:
        # Default to listen command
        cmd_listen(args)
    else:
        args.func(args)


if __name__ == "__main__":
    main()
