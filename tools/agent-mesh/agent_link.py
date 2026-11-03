import asyncio
import datetime
import json
import os
import uuid
import nats

NAME = os.environ.get("AGENT_NAME", "pc-node")
URL = os.environ.get("NATS_URL", "nats://127.0.0.1:4222")
PROJECTS = [p.strip() for p in os.environ.get("PROJECTS", "aseprite2").split(",") if p.strip()]

def get_iso_timestamp():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()

async def main():
    print(f"Connecting to NATS at {URL} as {NAME}...")
    nc = await nats.connect(URL, max_reconnect_attempts=-1)
    print("Connected to NATS successfully.")

    async def handle(msg):
        try:
            data = json.loads(msg.data.decode("utf-8"))
            to = data.get("to")
            if to in (NAME, "all"):
                project = data.get("project", "default")
                inbox_filename = f"inbox_{project}.jsonl"
                with open(inbox_filename, "a", encoding="utf-8") as f:
                    f.write(json.dumps(data) + "\n")
                print(f"[{project}] Received message {data.get('id')} from {data.get('from')}")
        except Exception as e:
            print(f"Error handling message: {e}")

    for p in PROJECTS:
        subject = f"proj.{p}.>"
        await nc.subscribe(subject, cb=handle)
        print(f"Subscribed to {subject}")

    # Global announcement
    announcement = {
        "id": str(uuid.uuid4()),
        "from": NAME,
        "type": "status",
        "body": f"online, projects: {PROJECTS}",
        "timestamp": get_iso_timestamp()
    }
    await nc.publish("global.announce", json.dumps(announcement).encode("utf-8"))
    print("Published global.announce")

    try:
        while True:
            await asyncio.sleep(30)
    except asyncio.CancelledError:
        print("Agent link shutting down...")
    finally:
        await nc.close()

if __name__ == "__main__":
    asyncio.run(main())
