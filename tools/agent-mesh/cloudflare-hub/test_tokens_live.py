import urllib.request
import urllib.error
import json
import sys

BASE_URL = "https://agent-mesh-hub.slack-clone-arin.workers.dev"
ADMIN_TOKEN = os.environ.get("HUB_TOKEN") or os.environ.get("ADMIN_TOKEN", "")

def req(path, method="GET", data=None, token=None):
    url = f"{BASE_URL}{path}"
    headers = {
        "Content-Type": "application/json",
        "User-Agent": "AgentMesh/test-suite"
    }
    if token:
        headers["Authorization"] = f"Bearer {token}"
    body = json.dumps(data).encode("utf-8") if data else None
    request = urllib.request.Request(url, data=body, headers=headers, method=method)
    try:
        with urllib.request.urlopen(request) as resp:
            return resp.status, json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        err_body = e.read().decode("utf-8")
        try:
            return e.code, json.loads(err_body)
        except:
            return e.code, {"raw": err_body}
    except Exception as e:
        return 0, {"exception": str(e)}

def run_tests():
    print("=== Testing Agent Mesh Hub Token Auth ===")

    # 1. Healthcheck
    st, res = req("/")
    print(f"1. Healthcheck: status={st}, version={res.get('version')}")
    assert st == 200, f"Expected 200, got {st}"

    # 2. Status with Admin Token
    st, res = req("/status", token=ADMIN_TOKEN)
    print(f"2. Status with admin token: status={st}, tokens={res.get('tokens')}")
    assert st == 200, f"Expected 200, got {st}"

    # 3. List tokens with Admin Token
    st, res = req("/tokens", token=ADMIN_TOKEN)
    print(f"3. List tokens: status={st}, count={res.get('count')}")
    assert st == 200, f"Expected 200, got {st}"

    # 4. Reserved agent name 'admin' rejected
    st, res = req("/tokens", method="POST", data={
        "agent": "admin",
        "projects": ["aseprite2"]
    }, token=ADMIN_TOKEN)
    print(f"4. Reserved agent name 'admin' rejection: status={st} (expected 400)")
    assert st == 400, f"Expected 400, got {st}: {res}"

    # 5. Create Token for test-agent-alpha
    st, res = req("/tokens", method="POST", data={
        "agent": "test-agent-alpha",
        "projects": ["aseprite2", "music-store"],
        "description": "Test Token Alpha"
    }, token=ADMIN_TOKEN)
    print(f"5. Create token: status={st}, ok={res.get('ok')}")
    assert st == 201, f"Expected 201, got {st}: {res}"
    alpha_token = res["token"]["token"]
    print(f"   Created token: {alpha_token}")

    # 6. Agent token CANNOT access admin /tokens endpoint
    st, res = req("/tokens", token=alpha_token)
    print(f"6. Agent token accessing /tokens: status={st} (expected 403)")
    assert st == 403, f"Expected 403, got {st}: {res}"

    # 7. Agent token sending valid message (from=test-agent-alpha, project=aseprite2)
    st, res = req("/send", method="POST", data={
        "project": "aseprite2",
        "from": "test-agent-alpha",
        "to": "all",
        "body": "Hello from Alpha"
    }, token=alpha_token)
    print(f"7. Valid send by agent: status={st}, ok={res.get('ok')}")
    assert st == 200, f"Expected 200, got {st}: {res}"

    # 8. Query param auth ?token= works for /send
    st, res = req(f"/send?token={alpha_token}", method="POST", data={
        "project": "aseprite2",
        "from": "test-agent-alpha",
        "to": "all",
        "body": "Hello via query token"
    })
    print(f"8. Send with ?token= query parameter: status={st}, ok={res.get('ok')}")
    assert st == 200, f"Expected 200, got {st}: {res}"

    # 9. Agent token sending to unauthorized project (project=secret-proj)
    st, res = req("/send", method="POST", data={
        "project": "secret-proj",
        "from": "test-agent-alpha",
        "to": "all",
        "body": "Should fail"
    }, token=alpha_token)
    print(f"9. Unauthorized project send: status={st} (expected 403): {res.get('error')}")
    assert st == 403, f"Expected 403, got {st}: {res}"

    # 10. Agent token trying to spoof 'from' (from=arin-pc)
    st, res = req("/send", method="POST", data={
        "project": "aseprite2",
        "from": "arin-pc",
        "to": "all",
        "body": "Impersonation attempt"
    }, token=alpha_token)
    print(f"10. Impersonation send: status={st} (expected 403): {res.get('error')}")
    assert st == 403, f"Expected 403, got {st}: {res}"

    # 11. Agent token polling inbox for allowed project
    st, res = req("/inbox?agent=test-agent-alpha&projects=aseprite2", token=alpha_token)
    print(f"11. Allowed inbox poll: status={st}, msg_count={len(res.get('messages', []))}")
    assert st == 200, f"Expected 200, got {st}: {res}"

    # 12. Agent token polling inbox for forbidden project
    st, res = req("/inbox?agent=test-agent-alpha&projects=secret-proj", token=alpha_token)
    print(f"12. Forbidden inbox poll: status={st} (expected 403): {res.get('error')}")
    assert st == 403, f"Expected 403, got {st}: {res}"

    # 13. Create a second token for compound revocation test
    st, res = req("/tokens", method="POST", data={
        "agent": "test-agent-alpha",
        "projects": ["aseprite2"],
        "description": "Alpha Token 2"
    }, token=ADMIN_TOKEN)
    assert st == 201
    alpha_token_2 = res["token"]["token"]
    print(f"13. Created second token for alpha: {alpha_token_2}")

    # 14. Revoke only alpha_token with both token and agent specified
    st, res = req("/tokens/revoke", method="POST", data={"token": alpha_token, "agent": "test-agent-alpha"}, token=ADMIN_TOKEN)
    print(f"14. Revoke token 1: status={st}, revoked={res.get('revoked')}")
    assert st == 200 and res.get("revoked") == 1, f"Expected revoked=1, got {res}"

    # 15. Attempt to use revoked token 1 (should fail 401)
    st, res = req("/send", method="POST", data={
        "project": "aseprite2",
        "from": "test-agent-alpha",
        "to": "all",
        "body": "Revoked token test"
    }, token=alpha_token)
    print(f"15. Use revoked token 1: status={st} (expected 401): {res.get('error')}")
    assert st == 401, f"Expected 401, got {st}: {res}"

    # 16. Verify token 2 is still valid (not affected by token 1 revocation)
    st, res = req("/send", method="POST", data={
        "project": "aseprite2",
        "from": "test-agent-alpha",
        "to": "all",
        "body": "Token 2 is still valid"
    }, token=alpha_token_2)
    print(f"16. Use token 2: status={st} (expected 200)")
    assert st == 200, f"Expected 200, got {st}: {res}"

    # 17. Test DELETE /admin/tokens/:token endpoint
    st, res = req(f"/admin/tokens/{alpha_token_2}?purge=true", method="DELETE", token=ADMIN_TOKEN)
    print(f"17. DELETE /admin/tokens/:token: status={st}, revoked={res.get('revoked')}")
    assert st == 200 and res.get("revoked") == 1, f"Expected 200 and revoked=1, got {st}: {res}"

    # 18. Purge revoked test token 1
    st, res = req(f"/tokens/{alpha_token}?purge=true", method="DELETE", token=ADMIN_TOKEN)
    print(f"18. Purge test token 1: status={st}, purged={res.get('revoked')}")
    assert st == 200 and res.get("revoked") == 1, f"Expected 200 and revoked=1, got {st}: {res}"

    print("\nALL VERIFICATION TESTS PASSED SUCCESSFULLY!")

if __name__ == "__main__":
    run_tests()
