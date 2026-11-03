import asyncio
import datetime
import json
import os
import sys
import uuid
import urllib.request
import urllib.error

# Optional websockets package for real-time mode
try:
    import websockets
    HAS_WEBSOCKETS = True
except ImportError:
    HAS_WEBSOCKETS = False

NAME = os.environ.get("AGENT_NAME", "pc-node")
HUB_URL = os.environ.get("HUB_URL", "https://agent-mesh-hub.slack-clone-arin.workers.dev").rstrip("/")
TOKEN = os.environ.get("AGENT_TOKEN") or os.environ.get("HUB_TOKEN", "cVc8xLDpHfMujg1GEkimVaDdSdhdR70k")
PROJECTS = [p.strip() for p in os.environ.get("PROJECTS", "aseprite2").split(",") if p.strip()]

def get_iso_timestamp():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()

def post_message(msg_dict):
    url = f"{HUB_URL}/send"
    data = json.dumps(msg_dict).encode("utf-8")
    req = urllib.request.Request(url, data=data, headers={
        "Authorization": f"Bearer {TOKEN}",
        "Content-Type": "application/json",
        "User-Agent": f"AgentMesh/{NAME}"
    })
    try:
        with urllib.request.urlopen(req) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except Exception as e:
        print(f"[HTTP] Failed to send message: {e}")
        return None

def fetch_inbox(since_id=""):
    projects_arg = ",".join(PROJECTS)
    url = f"{HUB_URL}/inbox?agent={NAME}&projects={projects_arg}&since={since_id}"
    req = urllib.request.Request(url, headers={
        "Authorization": f"Bearer {TOKEN}",
        "User-Agent": f"AgentMesh/{NAME}"
    })
    try:
        with urllib.request.urlopen(req) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            return data.get("messages", [])
    except Exception as e:
        print(f"[HTTP] Failed to poll inbox: {e}")
        return []

async def run_websocket():
    ws_url = HUB_URL.replace("https://", "wss://").replace("http://", "ws://")
    projects_arg = ",".join(PROJECTS)
    endpoint = f"{ws_url}/connect?agent={NAME}&projects={projects_arg}&token={TOKEN}"

    print(f"Connecting to Cloudflare Hub WebSocket: {ws_url}...")
    headers = {"User-Agent": f"AgentMesh/{NAME}"}
    connect_kwargs = {"ping_interval": None, "ping_timeout": None}
    try:
        import inspect
        if "additional_headers" in inspect.signature(websockets.connect).parameters:
            connect_kwargs["additional_headers"] = headers
        else:
            connect_kwargs["extra_headers"] = headers
    except Exception:
        connect_kwargs["additional_headers"] = headers

    async with websockets.connect(endpoint, **connect_kwargs) as ws:
        print(f"Connected to Cloudflare Hub as [{NAME}]")

        # Announce online
        announcement = {
            "id": str(uuid.uuid4()),
            "project": "global",
            "from": NAME,
            "to": "all",
            "type": "status",
            "body": f"online, projects: {PROJECTS}",
            "timestamp": get_iso_timestamp()
        }
        await ws.send(json.dumps(announcement))

        while True:
            raw = await ws.recv()
            try:
                data = json.loads(raw)
            except Exception:
                continue

            # Handle ping/pong/system silently
            m_type = (data.get("type") or "").strip().lower()
            m_proj = (data.get("project") or "").strip().lower()
            if m_type in ("ping", "pong") or m_proj == "system":
                if m_type == "ping":
                    pong = {"type": "pong", "project": "system", "timestamp": get_iso_timestamp()}
                    await ws.send(json.dumps(pong))
                continue

            project = data.get("project", "default")
            inbox_file = f"inbox_{project}.jsonl"
            with open(inbox_file, "a", encoding="utf-8") as f:
                f.write(json.dumps(data) + "\n")
            print(f"[{project}] Received message {data.get('id')} from {data.get('from')}")

async def run_polling():
    print(f"Running in HTTP Polling Mode (Interval: 15s) against {HUB_URL}...")
    last_id = ""

    # Announce online via POST /send
    post_message({
        "id": str(uuid.uuid4()),
        "project": "global",
        "from": NAME,
        "to": "all",
        "type": "status",
        "body": f"online (polling), projects: {PROJECTS}",
        "timestamp": get_iso_timestamp()
    })

    while True:
        msgs = fetch_inbox(since_id=last_id)
        for data in msgs:
            last_id = data.get("id", last_id)
            m_type = (data.get("type") or "").strip().lower()
            m_proj = (data.get("project") or "").strip().lower()
            if m_type in ("ping", "pong") or m_proj == "system":
                continue
            project = data.get("project", "default")
            inbox_file = f"inbox_{project}.jsonl"
            with open(inbox_file, "a", encoding="utf-8") as f:
                f.write(json.dumps(data) + "\n")
            print(f"[{project}] Received message {data.get('id')} from {data.get('from')}")
        await asyncio.sleep(15)

async def main():
    if HAS_WEBSOCKETS:
        try:
            await run_websocket()
        except Exception as e:
            print(f"WebSocket connection failed ({e}). Falling back to HTTP polling...")
            await run_polling()
    else:
        await run_polling()

if __name__ == "__main__":
    asyncio.run(main())
