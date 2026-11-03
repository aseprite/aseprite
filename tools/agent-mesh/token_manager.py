#!/usr/bin/env python3
"""
Agent Mesh Hub Token Manager CLI
Manage agent-specific tokens (create, list, revoke) for Cloudflare Agent Mesh Hub.
"""

import argparse
import json
import os
import sys
import urllib.request
import urllib.error
import urllib.parse

DEFAULT_HUB_URL = os.environ.get("HUB_URL", "https://agent-mesh-hub.slack-clone-arin.workers.dev").rstrip("/")
DEFAULT_ADMIN_TOKEN = os.environ.get("ADMIN_TOKEN")
if not DEFAULT_ADMIN_TOKEN:
    hub_tok = os.environ.get("HUB_TOKEN", "")
    if hub_tok and not hub_tok.startswith("amt_"):
        DEFAULT_ADMIN_TOKEN = hub_tok
    else:
        DEFAULT_ADMIN_TOKEN = ""

def api_request(base_url, admin_token, path, method="GET", body=None):
    url = f"{base_url}{path}"
    headers = {
        "Authorization": f"Bearer {admin_token}",
        "Content-Type": "application/json",
        "User-Agent": "AgentMesh/TokenManager"
    }
    data = json.dumps(body).encode("utf-8") if body else None
    req = urllib.request.Request(url, data=data, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req) as resp:
            content = resp.read().decode("utf-8")
            return resp.status, json.loads(content) if content else {}
    except urllib.error.HTTPError as e:
        err_content = e.read().decode("utf-8")
        try:
            err_json = json.loads(err_content)
        except Exception:
            err_json = {"raw": err_content}
        return e.code, err_json
    except Exception as e:
        print(f"[Error] Request failed: {e}", file=sys.stderr)
        sys.exit(1)

def cmd_list(args):
    params = []
    if args.agent:
        params.append(f"agent={urllib.parse.quote(args.agent)}")
    if args.active:
        params.append("active=true")
    qs = ("?" + "&".join(params)) if params else ""
    status, res = api_request(args.url, args.admin_token, f"/tokens{qs}", method="GET")

    if status != 200:
        print(f"Error ({status}): {res.get('error', res)}", file=sys.stderr)
        sys.exit(1)

    tokens = res.get("tokens", [])
    if not tokens:
        print("No tokens found.")
        return

    print(f"\nTotal tokens: {len(tokens)}\n")
    fmt = "{:<36} {:<18} {:<24} {:<10} {:<24}"
    print(fmt.format("TOKEN HASH", "AGENT", "PROJECTS", "STATUS", "CREATED AT"))
    print("-" * 115)
    for t in tokens:
        status_str = "REVOKED" if t.get("revoked_at") else "ACTIVE"
        projs = ",".join(t.get("projects", []))
        tok_display = t.get("token_hash_prefix") or t.get("token_hash") or t.get("token", "")
        print(fmt.format(
            tok_display,
            t.get("agent", "")[:17],
            projs[:23],
            status_str,
            t.get("created_at", "")[:19]
        ))
    print()

def cmd_agents(args):
    status, res = api_request(args.url, args.admin_token, "/admin/agents", method="GET")
    if status != 200:
        print(f"Error ({status}): {res.get('error', res)}", file=sys.stderr)
        sys.exit(1)

    agents = res.get("agents", [])
    if not agents:
        print("No agents registered.")
        return

    print(f"\nTotal agents: {len(agents)}\n")
    fmt = "{:<18} {:<30} {:<15} {:<15} {:<24}"
    print(fmt.format("AGENT", "PROJECTS", "ACTIVE TOKENS", "TOTAL TOKENS", "LAST CREATED"))
    print("-" * 105)
    for a in agents:
        projs = ",".join(a.get("projects", []))
        print(fmt.format(
            a.get("agent", "")[:17],
            projs[:29],
            str(a.get("active_tokens", 0)),
            str(a.get("total_tokens", 0)),
            a.get("last_created", "")[:19]
        ))
    print()

def cmd_create(args):
    projects = [p.strip() for p in args.projects.split(",") if p.strip()]
    if not projects:
        print("Error: Must specify at least one project", file=sys.stderr)
        sys.exit(1)

    payload = {
        "agent": args.agent.strip(),
        "projects": projects
    }
    if args.token:
        payload["token"] = args.token.strip()
    if args.desc:
        payload["description"] = args.desc.strip()

    status, res = api_request(args.url, args.admin_token, "/tokens", method="POST", body=payload)
    if status != 201:
        print(f"Error ({status}): {res.get('error', res)}", file=sys.stderr)
        sys.exit(1)

    tok = res.get("token", {})
    token_str = tok.get("token")
    agent_str = tok.get("agent")
    projs_str = ", ".join(tok.get("projects", []))

    print("\n" + "=" * 60)
    print(" Agent Token Created Successfully")
    print("=" * 60)
    print(f" Agent:       {agent_str}")
    print(f" Token:       {token_str}")
    print(f" Projects:    {projs_str}")
    print(f" Created At:  {tok.get('created_at')}")
    if tok.get("description"):
        print(f" Description: {tok.get('description')}")
    print("-" * 60)
    print(" NOTE: This token is NOT stored in plain text anywhere on the server.")
    print(" Save it now — it will not be shown again.")
    print("-" * 60)
    print(" Usage Environment Variables:")
    print(f"   PowerShell: $env:AGENT_NAME=\"{agent_str}\"; $env:AGENT_TOKEN=\"{token_str}\"")
    print(f"   Bash / zsh: export AGENT_NAME=\"{agent_str}\" AGENT_TOKEN=\"{token_str}\"")
    print("=" * 60 + "\n")

def cmd_revoke(args):
    if not args.token and not args.token_hash and not args.agent:
        print("Error: Must specify either --token <token>, --token-hash <hash>, or --agent <agent>", file=sys.stderr)
        sys.exit(1)

    payload = {
        "purge": bool(args.purge)
    }
    if args.token:
        payload["token"] = args.token.strip()
    if args.token_hash:
        payload["token_hash"] = args.token_hash.strip()
    if args.agent:
        payload["agent"] = args.agent.strip()

    status, res = api_request(args.url, args.admin_token, "/tokens/revoke", method="POST", body=payload)
    if status != 200:
        print(f"Error ({status}): {res.get('error', res)}", file=sys.stderr)
        sys.exit(1)

    action = "purged" if args.purge else "revoked"
    print(f"\nSuccessfully {action} {res.get('revoked', 0)} token(s).")
    if res.get("disconnected_sessions"):
        print(f"Disconnected {res['disconnected_sessions']} active WebSocket session(s).")
    print()

def main():
    parser = argparse.ArgumentParser(description="Agent Mesh Hub Token Manager")
    parser.add_argument("--url", default=DEFAULT_HUB_URL, help=f"Hub URL (default: {DEFAULT_HUB_URL})")
    parser.add_argument("--admin-token", default=DEFAULT_ADMIN_TOKEN, help="Master admin token")

    sub = parser.add_subparsers(dest="command", required=True)

    # list
    p_list = sub.add_parser("list", help="List tokens")
    p_list.add_argument("--agent", help="Filter by agent name")
    p_list.add_argument("--active", action="store_true", help="Show active tokens only")
    p_list.set_defaults(func=cmd_list)

    # agents
    p_agents = sub.add_parser("agents", help="List agents and their permissions")
    p_agents.set_defaults(func=cmd_agents)

    # create
    p_create = sub.add_parser("create", help="Create new agent token")
    p_create.add_argument("--agent", required=True, help="Agent name (e.g. arin-pc)")
    p_create.add_argument("--projects", required=True, help="Comma-separated projects (e.g. aseprite2,pitch-quest)")
    p_create.add_argument("--token", help="Optional custom token string")
    p_create.add_argument("--desc", help="Optional description")
    p_create.set_defaults(func=cmd_create)

    # revoke
    p_revoke = sub.add_parser("revoke", help="Revoke agent token(s)")
    p_revoke.add_argument("--token", help="Token string to revoke")
    p_revoke.add_argument("--token-hash", help="Token SHA-256 hash to revoke")
    p_revoke.add_argument("--agent", help="Agent name to revoke all tokens for")
    p_revoke.add_argument("--purge", action="store_true", help="Permanently delete from database instead of soft-revoking")
    p_revoke.set_defaults(func=cmd_revoke)

    args = parser.parse_args()
    if not args.admin_token:
        print("[Error] Admin token required. Set HUB_TOKEN/ADMIN_TOKEN env var or pass --admin-token.", file=sys.stderr)
        sys.exit(1)
    args.func(args)

if __name__ == "__main__":
    main()
