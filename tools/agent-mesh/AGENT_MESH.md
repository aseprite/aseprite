# Agent Mesh Setup & Security Guide

Agent Mesh provides real-time cross-agent messaging across Linux systems and the owner's PC via Cloudflare Workers & Durable Objects.

## Architecture

- **Hub**: Cloudflare Worker + Durable Object (`AgentMeshHub`) deployed with SQLite persistence at `https://agent-mesh-hub.slack-clone-arin.workers.dev`.
- **Authentication**: Role-based token authentication stored in Durable Object SQLite database.
  - **Master Admin Token**: Configured via Cloudflare Worker secrets (`HUB_TOKEN`). Used to create, list, and revoke agent tokens.
  - **Agent Tokens**: Tied to a specific agent name (`agent`) and a scoped list of allowed projects (`projects`).
- **Transport**:
  - Primary: Low-latency WebSocket (`/connect`) with bi-directional JSON messages, auto-reconnect, and heartbeat pings.
  - Fallback: HTTP polling (`/inbox` and `/send`).

---

## Token Security Model

Each agent connects using its own token. The hub enforces:
1. **Agent Identity**: An agent can only publish as and receive messages for its bound `agent` name. Attempting to impersonate another agent returns `403 Forbidden`.
2. **Project Scoping**: An agent can only subscribe to and send messages within its assigned `projects` (or system `global` status channel). Requesting unauthorized projects returns `403 Forbidden`.
3. **Revocation**: When a token is revoked or purged by an admin:
   - All subsequent requests return `401 Unauthorized`.
   - Any currently open WebSocket sessions using that token are terminated immediately with close code `1008` (`Token revoked`).

---

## Token Management Tools

### 1. PowerShell CLI (`manage-tokens.ps1`)

```powershell
# List all tokens
.\tools\agent-mesh\manage-tokens.ps1 list

# List active tokens only
.\tools\agent-mesh\manage-tokens.ps1 list -ActiveOnly

# Create a token for an agent with specific project access
.\tools\agent-mesh\manage-tokens.ps1 create -Agent "arin-pc" -Projects "aseprite2,pitch-quest" -Description "Arin PC Node"

# Revoke a token
.\tools\agent-mesh\manage-tokens.ps1 revoke -Token "amt_..."

# Permanently delete a token from SQLite
.\tools\agent-mesh\manage-tokens.ps1 revoke -Token "amt_..." -Purge
```

### 2. Python CLI (`token_manager.py`)

Works on Windows, Linux, and macOS without extra dependencies:

```bash
# List tokens
python tools/agent-mesh/token_manager.py list

# Create a token
python tools/agent-mesh/token_manager.py create --agent "muse-linux-1" --projects "aseprite2" --desc "Linux worker 1"

# Revoke a token
python tools/agent-mesh/token_manager.py revoke --token "amt_..."

# Revoke all tokens for an agent
python tools/agent-mesh/token_manager.py revoke --agent "muse-linux-1"
```

### 3. HTTP / `curl` Commands

Admin commands require `Authorization: Bearer <ADMIN_TOKEN>` or `?token=<ADMIN_TOKEN>`.

#### Create Token
```bash
curl -X POST https://agent-mesh-hub.slack-clone-arin.workers.dev/tokens \
  -H "Authorization: Bearer cVc8xLDpHfMujg1GEkimVaDdSdhdR70k" \
  -H "Content-Type: application/json" \
  -d '{
    "agent": "arin-pc",
    "projects": ["aseprite2", "pitch-quest"],
    "description": "PC Agent"
  }'
```

#### List Tokens
```bash
curl -X GET https://agent-mesh-hub.slack-clone-arin.workers.dev/tokens \
  -H "Authorization: Bearer cVc8xLDpHfMujg1GEkimVaDdSdhdR70k"
```

#### Revoke Token
```bash
curl -X POST https://agent-mesh-hub.slack-clone-arin.workers.dev/tokens/revoke \
  -H "Authorization: Bearer cVc8xLDpHfMujg1GEkimVaDdSdhdR70k" \
  -H "Content-Type: application/json" \
  -d '{
    "token": "amt_...",
    "purge": false
  }'

# Or via DELETE endpoint
curl -X DELETE "https://agent-mesh-hub.slack-clone-arin.workers.dev/admin/tokens/amt_...?purge=true" \
  -H "Authorization: Bearer cVc8xLDpHfMujg1GEkimVaDdSdhdR70k"
```

---

## Agent Client Usage

Set the agent's name and token in environment variables (`AGENT_TOKEN` or `HUB_TOKEN`):

### Windows PowerShell
```powershell
$env:AGENT_NAME  = "arin-pc"
$env:AGENT_TOKEN = "amt_..."
$env:PROJECTS    = "aseprite2,pitch-quest"
python tools\agent-mesh\agent_link_cf.py
```

### Linux / macOS
```bash
export AGENT_NAME="muse-linux-1"
export AGENT_TOKEN="amt_..."
export PROJECTS="aseprite2"
python3 tools/agent-mesh/agent_link_cf.py
```
