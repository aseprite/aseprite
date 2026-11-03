const BASE_URL = "https://agent-mesh-hub.slack-clone-arin.workers.dev";
const WS_URL = "wss://agent-mesh-hub.slack-clone-arin.workers.dev";
const ADMIN_TOKEN = "cVc8xLDpHfMujg1GEkimVaDdSdhdR70k";

async function post(path, body) {
  const res = await fetch(`${BASE_URL}${path}`, {
    method: "POST",
    headers: {
      "Authorization": `Bearer ${ADMIN_TOKEN}`,
      "Content-Type": "application/json",
      "User-Agent": "AgentMesh/test"
    },
    body: JSON.stringify(body)
  });
  return res.json();
}

async function run() {
  console.log("=== Node WebSocket Verification ===");

  // 1. Create agent token with multiple allowed projects
  const createRes = await post("/tokens", {
    agent: "node-agent",
    projects: ["aseprite2", "music-store"],
    description: "Node WS Test Agent"
  });
  const token = createRes.token.token;
  console.log("1. Created token:", token);

  // 2. Connect valid WebSocket listening to aseprite2
  const ws1 = new WebSocket(`${WS_URL}/connect?agent=node-agent&projects=aseprite2&token=${token}`, {
    headers: { "User-Agent": "AgentMesh/test" }
  });

  await new Promise((resolve, reject) => {
    ws1.onopen = () => {
      console.log("2. WebSocket connected successfully");
      resolve();
    };
    ws1.onerror = (e) => reject(new Error("WS open failed: " + e.message));
  });

  // 3. Send message and receive echo/broadcast
  const msgPromise = new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error("Timeout waiting for echo")), 6000);
    const handler = (event) => {
      const data = JSON.parse(event.data);
      if (data.type === "task" && data.body === "Hello from node test") {
        clearTimeout(timer);
        ws1.removeEventListener("message", handler);
        console.log("3. Received broadcast over WebSocket:", data.body);
        resolve(data);
      }
    };
    ws1.addEventListener("message", handler);
  });

  ws1.send(JSON.stringify({
    type: "task",
    project: "aseprite2",
    from: "node-agent",
    body: "Hello from node test"
  }));

  await msgPromise;

  // 4. Send message to another project allowed by token (music-store), even if socket only subscribed to aseprite2
  const tokenAllowedPromise = new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error("Timeout waiting for music-store message")), 6000);
    // Connect an admin listener to receive all broadcasts
    const wsAdmin = new WebSocket(`${WS_URL}/connect?agent=admin&projects=*&token=${ADMIN_TOKEN}`);
    wsAdmin.onmessage = (event) => {
      const data = JSON.parse(event.data);
      if (data.project === "music-store" && data.body === "Allowed multi-project test") {
        clearTimeout(timer);
        wsAdmin.close();
        console.log("4. Successfully sent to second allowed project (music-store)");
        resolve(data);
      }
    };
    wsAdmin.onopen = () => {
      ws1.send(JSON.stringify({
        type: "task",
        project: "music-store",
        from: "node-agent",
        body: "Allowed multi-project test"
      }));
    };
  });

  await tokenAllowedPromise;

  // 5. Send message to forbidden project over WS
  const errPromise = new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error("Timeout waiting for forbidden project error")), 6000);
    const handler = (event) => {
      const data = JSON.parse(event.data);
      if (data.type === "error" && data.error.includes("Forbidden: Project")) {
        clearTimeout(timer);
        ws1.removeEventListener("message", handler);
        console.log("5. Received forbidden project error over WS:", data.error);
        resolve(data);
      }
    };
    ws1.addEventListener("message", handler);
  });

  ws1.send(JSON.stringify({
    project: "forbidden-proj",
    from: "node-agent",
    body: "Should fail"
  }));

  await errPromise;

  // 6. Send message with spoofed 'from' over WS
  const spoofPromise = new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error("Timeout waiting for spoof error")), 6000);
    const handler = (event) => {
      const data = JSON.parse(event.data);
      if (data.type === "error" && data.error.includes("Forbidden: 'from'")) {
        clearTimeout(timer);
        ws1.removeEventListener("message", handler);
        console.log("6. Received spoofing rejection over WS:", data.error);
        resolve(data);
      }
    };
    ws1.addEventListener("message", handler);
  });

  ws1.send(JSON.stringify({
    project: "aseprite2",
    from: "impersonated-agent",
    body: "Should fail spoof"
  }));

  await spoofPromise;

  // 7. Connect with mismatched agent name fails (closes with error or fails to connect)
  const badAgentPromise = new Promise((resolve) => {
    const wsBad = new WebSocket(`${WS_URL}/connect?agent=other-agent&projects=aseprite2&token=${token}`);
    wsBad.onerror = (e) => {
      console.log("7. Rejected connect with mismatched agent name as expected");
      resolve();
    };
    wsBad.onopen = () => {
      throw new Error("Should not have connected with mismatched agent name");
    };
  });
  await badAgentPromise;

  // 8. Revocation causes active socket disconnect with code 1008
  const closePromise = new Promise((resolve, reject) => {
    const timer = setTimeout(() => reject(new Error("Timeout waiting for close on revocation")), 6000);
    ws1.onclose = (event) => {
      clearTimeout(timer);
      console.log(`8. WebSocket closed on revocation: code=${event.code}, reason=${event.reason}`);
      resolve(event);
    };
  });

  const revRes = await post("/tokens/revoke", { token });
  console.log("   Revoke response:", revRes);
  const closeEvent = await closePromise;
  if (closeEvent.code !== 1008) {
    throw new Error("Expected close code 1008, got " + closeEvent.code);
  }

  // 9. Connect with revoked token fails
  const revokedPromise = new Promise((resolve) => {
    const wsRevoked = new WebSocket(`${WS_URL}/connect?agent=node-agent&projects=aseprite2&token=${token}`);
    wsRevoked.onerror = (e) => {
      console.log("9. Rejected connect with revoked token as expected");
      resolve();
    };
    wsRevoked.onopen = () => {
      throw new Error("Should not have connected with revoked token");
    };
  });
  await revokedPromise;

  // 10. Purge token from database
  await post("/tokens/revoke", { token, purge: true });
  console.log("10. Purged test token successfully");

  console.log("\nALL NODE WEBSOCKET VERIFICATION TESTS PASSED!");
  process.exit(0);
}

run().catch((err) => {
  console.error("Test failed:", err);
  process.exit(1);
});
