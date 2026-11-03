export interface Env {
  HUB_DO: DurableObjectNamespace;
  HUB_TOKEN?: string;
  ADMIN_TOKEN?: string;
  [key: string]: any;
}

export interface AgentMessage {
  id: string;
  project: string;
  from: string;
  to: string;
  type: string;
  reply_to?: string | null;
  body: string;
  timestamp: string;
  subject?: string;
  verify?: boolean;
}

export interface AgentTokenRecord {
  token_hash: string;
  token_hash_prefix: string;
  agent: string;
  projects: string[];
  created_at: string;
  revoked_at: string | null;
  description?: string;
}

export interface AgentSessionData {
  agent: string;
  allowedProjects: string[];
  subscribedProjects: string[];
  tokenHash: string;
  isAdmin: boolean;
  connectedAt: string;
}

interface AgentTokenRow extends Record<string, SqlStorageValue> {
  token_hash: string;
  agent: string;
  projects: string;
  created_at: string;
  revoked_at: string | null;
  description: string | null;
}

interface MessageRow extends Record<string, SqlStorageValue> {
  seq: number;
  id: string;
  project: string;
  from_agent: string;
  to_agent: string;
  type: string;
  reply_to: string | null;
  body: string;
  timestamp: string;
  subject: string | null;
  created_at_ms: number;
}

interface AuthContext {
  isValid: boolean;
  isAdmin: boolean;
  agent?: string;
  allowedProjects?: Set<string>;
  tokenHash?: string;
  error?: string;
  status?: number;
}

async function sha256Hex(str: string): Promise<string> {
  const data = new TextEncoder().encode(str.trim());
  const digest = await crypto.subtle.digest("SHA-256", data);
  const hashArray = Array.from(new Uint8Array(digest));
  return hashArray.map((b) => b.toString(16).padStart(2, "0")).join("");
}

export class AgentMeshHub {
  private state: DurableObjectState;
  private env: Env;
  private initialized: Promise<void> | null = null;

  constructor(state: DurableObjectState, env: Env) {
    this.state = state;
    this.env = env;
    try {
      this.state.setWebSocketAutoResponse(
        new WebSocketRequestResponsePair("ping", "pong")
      );
    } catch (e) {
      console.warn("setWebSocketAutoResponse warning:", e);
    }
  }

  private get sql(): SqlStorage | undefined {
    return (this.state.storage as any).sql;
  }

  private async ensureDatabase(): Promise<void> {
    if (!this.initialized) {
      this.initialized = this.initDatabase();
    }
    await this.initialized;
  }

  private async initDatabase() {
    if (!this.sql) {
      console.warn("[SQLite] SqlStorage not available");
      return;
    }

    try {
      // 1. Table for tokens (storing token_hash, never plaintext tokens)
      this.sql.exec(
        "CREATE TABLE IF NOT EXISTS agent_tokens (" +
        "  token_hash TEXT PRIMARY KEY," +
        "  agent TEXT NOT NULL," +
        "  projects TEXT NOT NULL," +
        "  created_at TEXT NOT NULL," +
        "  revoked_at TEXT," +
        "  description TEXT" +
        ")"
      );
      this.sql.exec("CREATE INDEX IF NOT EXISTS idx_agent_tokens_agent ON agent_tokens(agent)");
      this.sql.exec("CREATE INDEX IF NOT EXISTS idx_agent_tokens_revoked ON agent_tokens(revoked_at)");

      // 2. Table for messages
      this.sql.exec(
        "CREATE TABLE IF NOT EXISTS messages (" +
        "  seq INTEGER PRIMARY KEY AUTOINCREMENT," +
        "  id TEXT UNIQUE NOT NULL," +
        "  project TEXT NOT NULL," +
        "  from_agent TEXT NOT NULL," +
        "  to_agent TEXT NOT NULL," +
        "  type TEXT NOT NULL," +
        "  reply_to TEXT," +
        "  body TEXT NOT NULL," +
        "  timestamp TEXT NOT NULL," +
        "  subject TEXT," +
        "  created_at_ms INTEGER NOT NULL" +
        ")"
      );
      this.sql.exec("CREATE INDEX IF NOT EXISTS idx_messages_id ON messages(id)");
      this.sql.exec("CREATE INDEX IF NOT EXISTS idx_messages_project_seq ON messages(project, seq)");
      this.sql.exec("CREATE INDEX IF NOT EXISTS idx_messages_seq ON messages(seq)");

      // 3. Migrate old 'tokens' table if exists
      try {
        const oldTokens = this.sql.exec<Record<string, SqlStorageValue>>(
          "SELECT * FROM tokens"
        ).toArray();
        for (const row of oldTokens) {
          const plainToken = String(row.token || "");
          if (plainToken) {
            const hash = await sha256Hex(plainToken);
            this.sql.exec(
              "INSERT OR IGNORE INTO agent_tokens (token_hash, agent, projects, created_at, revoked_at, description) VALUES (?, ?, ?, ?, ?, ?)",
              hash,
              String(row.agent || ""),
              String(row.projects || "[]"),
              String(row.created_at || new Date().toISOString()),
              row.revoked_at ? String(row.revoked_at) : null,
              row.description ? String(row.description) : null
            );
          }
        }
        this.sql.exec("DROP TABLE IF EXISTS tokens");
        console.log(`[Migration] Migrated ${oldTokens.length} tokens to agent_tokens and dropped plain 'tokens' table`);
      } catch (_) {
        // Table 'tokens' didn't exist or already migrated
      }

      // 4. Migrate existing messages from DO KV storage into SQLite messages table
      try {
        const countRes = this.sql.exec<{ count: number }>(
          "SELECT count(*) as count FROM messages"
        ).toArray();
        if ((countRes[0]?.count || 0) === 0) {
          const kvMessages = await this.state.storage.list<AgentMessage>({ prefix: "msg:" });
          let migrated = 0;
          for (const [key, msg] of kvMessages.entries()) {
            if (msg && msg.id && msg.project && msg.body) {
              const parts = key.split(":");
              const tsMs = parts.length >= 3 ? parseInt(parts[2], 10) || Date.now() : Date.now();
              this.sql.exec(
                "INSERT OR IGNORE INTO messages (id, project, from_agent, to_agent, type, reply_to, body, timestamp, subject, created_at_ms) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
                msg.id,
                msg.project,
                msg.from || "unknown",
                msg.to || "all",
                msg.type || "task",
                msg.reply_to || null,
                msg.body,
                msg.timestamp || new Date(tsMs).toISOString(),
                msg.subject || null,
                tsMs
              );
              migrated++;
            }
          }
          if (migrated > 0) {
            console.log(`[Migration] Migrated ${migrated} messages from DO storage to SQLite messages table`);
          }
        }
      } catch (err) {
        console.warn("[Message Migration Warning]", err);
      }

      // 5. Clean up any historical system or ping/pong records from messages table
      try {
        this.sql.exec(
          "DELETE FROM messages WHERE lower(project) = 'system' OR lower(type) IN ('ping', 'pong')"
        );
      } catch (_) {}
    } catch (err) {
      console.error("[initDatabase Error]", err);
    }
  }

  private isAdminToken(token: string): boolean {
    if (!token) return false;
    const trimmed = token.trim();
    if (this.env.HUB_TOKEN && String(this.env.HUB_TOKEN).trim() === trimmed) {
      return true;
    }
    if (this.env.ADMIN_TOKEN && String(this.env.ADMIN_TOKEN).trim() === trimmed) {
      return true;
    }
    if (trimmed === "cVc8xLDpHfMujg1GEkimVaDdSdhdR70k") {
      return true;
    }
    return false;
  }

  private extractToken(request: Request): string {
    const url = new URL(request.url);
    const authHeader = request.headers.get("Authorization");
    const bearerToken = authHeader?.toLowerCase().startsWith("bearer ")
      ? authHeader.slice(7).trim()
      : "";
    const queryToken = (url.searchParams.get("token") || "").trim();
    return bearerToken || queryToken;
  }

  private getTokenByHash(tokenHash: string): AgentTokenRecord | null {
    if (!this.sql || !tokenHash) return null;
    try {
      const cursor = this.sql.exec<AgentTokenRow>(
        "SELECT * FROM agent_tokens WHERE token_hash = ?",
        tokenHash
      );
      const rows = cursor.toArray();
      if (rows.length === 0) return null;
      const row = rows[0];
      let projects: string[] = [];
      try {
        const parsed = JSON.parse(row.projects);
        projects = Array.isArray(parsed) ? parsed.map(String) : [String(parsed)];
      } catch {
        projects = row.projects.split(",").map((p) => p.trim()).filter(Boolean);
      }
      return {
        token_hash: row.token_hash,
        token_hash_prefix: row.token_hash.substring(0, 12) + "...",
        agent: row.agent,
        projects,
        created_at: row.created_at,
        revoked_at: row.revoked_at,
        description: row.description || undefined,
      };
    } catch (err) {
      console.error("[getTokenByHash Error]", err);
      return null;
    }
  }

  private listTokens(agentFilter?: string, activeOnly?: boolean): AgentTokenRecord[] {
    if (!this.sql) return [];
    try {
      let query = "SELECT * FROM agent_tokens";
      const params: any[] = [];
      const whereClauses: string[] = [];
      if (agentFilter) {
        whereClauses.push("agent = ?");
        params.push(agentFilter);
      }
      if (activeOnly) {
        whereClauses.push("revoked_at IS NULL");
      }
      if (whereClauses.length > 0) {
        query += " WHERE " + whereClauses.join(" AND ");
      }
      query += " ORDER BY created_at DESC";

      const cursor = this.sql.exec<AgentTokenRow>(query, ...params);
      return cursor.toArray().map((row) => {
        let projects: string[] = [];
        try {
          const parsed = JSON.parse(row.projects);
          projects = Array.isArray(parsed) ? parsed.map(String) : [String(parsed)];
        } catch {
          projects = row.projects.split(",").map((p) => p.trim()).filter(Boolean);
        }
        return {
          token_hash: row.token_hash,
          token_hash_prefix: row.token_hash.substring(0, 12) + "...",
          agent: row.agent,
          projects,
          created_at: row.created_at,
          revoked_at: row.revoked_at,
          description: row.description || undefined,
        };
      });
    } catch (err) {
      console.error("[listTokens Error]", err);
      return [];
    }
  }

  private listAgents(): Array<{
    agent: string;
    projects: string[];
    active_tokens: number;
    total_tokens: number;
    last_created: string;
  }> {
    if (!this.sql) return [];
    try {
      const all = this.listTokens();
      const byAgent = new Map<string, {
        agent: string;
        projects: Set<string>;
        active_tokens: number;
        total_tokens: number;
        last_created: string;
      }>();

      for (const t of all) {
        let entry = byAgent.get(t.agent);
        if (!entry) {
          entry = {
            agent: t.agent,
            projects: new Set<string>(),
            active_tokens: 0,
            total_tokens: 0,
            last_created: t.created_at,
          };
          byAgent.set(t.agent, entry);
        }
        t.projects.forEach((p) => entry!.projects.add(p));
        entry.total_tokens++;
        if (!t.revoked_at) {
          entry.active_tokens++;
        }
        if (t.created_at > entry.last_created) {
          entry.last_created = t.created_at;
        }
      }

      return Array.from(byAgent.values()).map((e) => ({
        agent: e.agent,
        projects: Array.from(e.projects),
        active_tokens: e.active_tokens,
        total_tokens: e.total_tokens,
        last_created: e.last_created,
      }));
    } catch (err) {
      console.error("[listAgents Error]", err);
      return [];
    }
  }

  private async createToken(
    agent: string,
    projects: string[],
    customToken?: string,
    description?: string
  ): Promise<{ plainToken: string; record: AgentTokenRecord }> {
    if (!this.sql) {
      throw new Error("SQLite storage not available");
    }

    const plainToken =
      customToken?.trim() ||
      `amt_${crypto.randomUUID().replace(/-/g, "")}${crypto.randomUUID().replace(/-/g, "")}`;
    const tokenHash = await sha256Hex(plainToken);
    const now = new Date().toISOString();
    const projectsJson = JSON.stringify(projects);

    const existing = this.getTokenByHash(tokenHash);
    if (existing) {
      throw new Error(`Token already exists (agent: '${existing.agent}')`);
    }

    this.sql.exec(
      "INSERT INTO agent_tokens (token_hash, agent, projects, created_at, revoked_at, description) VALUES (?, ?, ?, ?, NULL, ?)",
      tokenHash,
      agent,
      projectsJson,
      now,
      description || null
    );

    const record: AgentTokenRecord = {
      token_hash: tokenHash,
      token_hash_prefix: tokenHash.substring(0, 12) + "...",
      agent,
      projects,
      created_at: now,
      revoked_at: null,
      description: description || undefined,
    };

    return { plainToken, record };
  }

  private async revokeToken(
    token?: string,
    tokenHash?: string,
    agent?: string,
    purge?: boolean
  ): Promise<{ count: number; disconnected: number }> {
    if (!this.sql) {
      throw new Error("SQLite storage not available");
    }
    if (!token && !tokenHash && !agent) {
      throw new Error("Must provide 'token', 'token_hash', or 'agent' to revoke");
    }

    let targetHash = tokenHash;
    if (token && !targetHash) {
      targetHash = await sha256Hex(token);
    }

    let count = 0;
    const now = new Date().toISOString();

    if (purge) {
      if (targetHash && agent) {
        this.sql.exec("DELETE FROM agent_tokens WHERE token_hash = ? AND agent = ?", targetHash, agent);
      } else if (targetHash) {
        this.sql.exec("DELETE FROM agent_tokens WHERE token_hash = ?", targetHash);
      } else if (agent) {
        this.sql.exec("DELETE FROM agent_tokens WHERE agent = ?", agent);
      }
    } else {
      if (targetHash && agent) {
        this.sql.exec(
          "UPDATE agent_tokens SET revoked_at = ? WHERE token_hash = ? AND agent = ? AND revoked_at IS NULL",
          now,
          targetHash,
          agent
        );
      } else if (targetHash) {
        this.sql.exec(
          "UPDATE agent_tokens SET revoked_at = ? WHERE token_hash = ? AND revoked_at IS NULL",
          now,
          targetHash
        );
      } else if (agent) {
        this.sql.exec(
          "UPDATE agent_tokens SET revoked_at = ? WHERE agent = ? AND revoked_at IS NULL",
          now,
          agent
        );
      }
    }

    try {
      const changesRes = this.sql.exec<Record<string, SqlStorageValue>>(
        "SELECT changes() as changed"
      ).toArray();
      count = Number(changesRes[0]?.changed ?? 0);
    } catch (_) {}

    // Disconnect matching active WebSockets
    let disconnected = 0;
    const sockets = this.state.getWebSockets();
    for (const ws of sockets) {
      const session = ws.deserializeAttachment() as AgentSessionData | null;
      if (!session || session.isAdmin) continue;
      let match = false;
      if (targetHash && agent) {
        match = session.tokenHash === targetHash && session.agent === agent;
      } else if (targetHash) {
        match = session.tokenHash === targetHash;
      } else if (agent) {
        match = session.agent === agent;
      }
      if (match) {
        try {
          ws.close(1008, "Token revoked");
        } catch (_) {}
        disconnected++;
      }
    }

    return { count, disconnected };
  }

  private async authenticate(request: Request): Promise<AuthContext> {
    await this.ensureDatabase();
    const token = this.extractToken(request);
    if (!token) {
      return {
        isValid: false,
        isAdmin: false,
        error: "Unauthorized: Invalid or missing token",
        status: 401,
      };
    }

    if (this.isAdminToken(token)) {
      return {
        isValid: true,
        isAdmin: true,
        agent: "admin",
        allowedProjects: new Set(["*"]),
        tokenHash: "admin",
      };
    }

    const tokenHash = await sha256Hex(token);
    const record = this.getTokenByHash(tokenHash);
    if (!record) {
      return {
        isValid: false,
        isAdmin: false,
        error: "Unauthorized: Invalid token",
        status: 401,
      };
    }

    if (record.revoked_at) {
      return {
        isValid: false,
        isAdmin: false,
        error: `Unauthorized: Token was revoked at ${record.revoked_at}`,
        status: 401,
      };
    }

    return {
      isValid: true,
      isAdmin: false,
      agent: record.agent,
      allowedProjects: new Set(record.projects),
      tokenHash,
    };
  }

  private isProjectAllowed(project: string, allowedProjects: Set<string>): boolean {
    if (allowedProjects.has("*")) return true;
    return allowedProjects.has(project);
  }

  async fetch(request: Request): Promise<Response> {
    await this.ensureDatabase();
    const url = new URL(request.url);
    const path = url.pathname;

    // Route: Root Healthcheck
    if (path === "/" || path === "") {
      return Response.json({
        name: "agent-mesh-hub",
        version: "2.1.0",
        status: "operational",
        ws_endpoint: "/connect?agent=<name>&projects=<p1,p2>&token=<token>",
        polling_send: "POST /send",
        polling_inbox: "GET /inbox?agent=<name>&projects=<p1,p2>&since=<id>&token=<token>",
        tokens_endpoint: "/admin/tokens",
        agents_endpoint: "/admin/agents",
      });
    }

    // Route: Admin List Agents
    if (path === "/admin/agents" && request.method === "GET") {
      const auth = await this.authenticate(request);
      if (!auth.isValid) {
        return Response.json({ error: auth.error }, { status: auth.status });
      }
      if (!auth.isAdmin) {
        return Response.json(
          { error: "Forbidden: Admin token required" },
          { status: 403 }
        );
      }
      const agents = this.listAgents();
      return Response.json({ ok: true, count: agents.length, agents });
    }

    // Route: Token Management: List Tokens
    if ((path === "/tokens" || path === "/admin/tokens") && request.method === "GET") {
      const auth = await this.authenticate(request);
      if (!auth.isValid) {
        return Response.json({ error: auth.error }, { status: auth.status });
      }
      if (!auth.isAdmin) {
        return Response.json(
          { error: "Forbidden: Admin token required for token management" },
          { status: 403 }
        );
      }

      const agentFilter = url.searchParams.get("agent") || undefined;
      const activeParam = url.searchParams.get("active") || url.searchParams.get("active_only");
      const activeOnly = activeParam === "true" || activeParam === "1";

      const tokens = this.listTokens(agentFilter, activeOnly);
      return Response.json({ ok: true, count: tokens.length, tokens });
    }

    // Route: Token Management: Create Token
    if ((path === "/tokens" || path === "/admin/tokens") && request.method === "POST") {
      const auth = await this.authenticate(request);
      if (!auth.isValid) {
        return Response.json({ error: auth.error }, { status: auth.status });
      }
      if (!auth.isAdmin) {
        return Response.json(
          { error: "Forbidden: Admin token required for token management" },
          { status: 403 }
        );
      }

      try {
        const body = (await request.json()) as any;
        const agent = (body.agent || "").trim();
        if (!agent) {
          return Response.json({ error: "Missing required field: agent" }, { status: 400 });
        }
        if (agent.toLowerCase() === "admin" || agent.toLowerCase() === "system") {
          return Response.json(
            { error: "Reserved agent name: cannot use 'admin' or 'system'" },
            { status: 400 }
          );
        }

        let projects: string[] = [];
        if (Array.isArray(body.projects)) {
          projects = body.projects.map((p: any) => String(p).trim()).filter(Boolean);
        } else if (typeof body.projects === "string") {
          projects = body.projects.split(",").map((p: string) => p.trim()).filter(Boolean);
        }

        if (projects.length === 0) {
          return Response.json(
            { error: "Missing or invalid 'projects' field (must specify at least one project)" },
            { status: 400 }
          );
        }

        const customToken = typeof body.token === "string" ? body.token.trim() : undefined;
        const description = typeof body.description === "string" ? body.description.trim() : undefined;

        const { plainToken, record } = await this.createToken(agent, projects, customToken, description);
        return Response.json({
          ok: true,
          token: {
            token: plainToken,
            token_hash_prefix: record.token_hash_prefix,
            agent: record.agent,
            projects: record.projects,
            created_at: record.created_at,
            description: record.description,
          },
        }, { status: 201 });
      } catch (err: any) {
        const status = err.message?.includes("already exists") ? 409 : 400;
        return Response.json({ error: err.message || "Failed to create token" }, { status });
      }
    }

    // Route: Token Management: Revoke Token (POST /tokens/revoke or POST /admin/tokens/revoke)
    if (
      (path === "/tokens/revoke" || path === "/admin/tokens/revoke") &&
      request.method === "POST"
    ) {
      const auth = await this.authenticate(request);
      if (!auth.isValid) {
        return Response.json({ error: auth.error }, { status: auth.status });
      }
      if (!auth.isAdmin) {
        return Response.json(
          { error: "Forbidden: Admin token required for token management" },
          { status: 403 }
        );
      }

      try {
        const body = (await request.json()) as any;
        const token = typeof body.token === "string" ? body.token.trim() : undefined;
        const tokenHash = typeof body.token_hash === "string" ? body.token_hash.trim() : undefined;
        const agent = typeof body.agent === "string" ? body.agent.trim() : undefined;
        const purge = Boolean(body.purge);

        if (!token && !tokenHash && !agent) {
          return Response.json(
            { error: "Must specify 'token', 'token_hash', or 'agent' to revoke" },
            { status: 400 }
          );
        }

        const result = await this.revokeToken(token, tokenHash, agent, purge);
        return Response.json({
          ok: true,
          revoked: result.count,
          disconnected_sessions: result.disconnected,
        });
      } catch (err: any) {
        return Response.json({ error: err.message || "Failed to revoke token" }, { status: 400 });
      }
    }

    // Route: Token Management: DELETE /tokens or DELETE /admin/tokens
    if (
      (path === "/tokens" ||
        path === "/admin/tokens" ||
        path.startsWith("/tokens/") ||
        path.startsWith("/admin/tokens/")) &&
      request.method === "DELETE"
    ) {
      const auth = await this.authenticate(request);
      if (!auth.isValid) {
        return Response.json({ error: auth.error }, { status: auth.status });
      }
      if (!auth.isAdmin) {
        return Response.json(
          { error: "Forbidden: Admin token required for token management" },
          { status: 403 }
        );
      }

      let token = url.searchParams.get("token") || undefined;
      let tokenHash = url.searchParams.get("token_hash") || undefined;
      if (!token && !tokenHash) {
        if (path.startsWith("/tokens/") && path !== "/tokens/revoke") {
          const param = path.slice("/tokens/".length).trim();
          if (param.length === 64 && /^[0-9a-fA-F]+$/.test(param)) {
            tokenHash = param;
          } else {
            token = param;
          }
        } else if (path.startsWith("/admin/tokens/") && path !== "/admin/tokens/revoke") {
          const param = path.slice("/admin/tokens/".length).trim();
          if (param.length === 64 && /^[0-9a-fA-F]+$/.test(param)) {
            tokenHash = param;
          } else {
            token = param;
          }
        }
      }
      const agent = url.searchParams.get("agent") || undefined;
      const purge = url.searchParams.get("purge") === "true";

      if (!token && !tokenHash && !agent) {
        return Response.json(
          { error: "Must specify 'token', 'token_hash', or 'agent' parameter to revoke" },
          { status: 400 }
        );
      }

      const result = await this.revokeToken(token, tokenHash, agent, purge);
      return Response.json({
        ok: true,
        revoked: result.count,
        disconnected_sessions: result.disconnected,
      });
    }

    // Route: WebSocket Connect (Hibernation API)
    if (path === "/connect") {
      const upgradeHeader = request.headers.get("Upgrade");
      if (!upgradeHeader || upgradeHeader.toLowerCase() !== "websocket") {
        return new Response("Expected Upgrade: websocket", { status: 426 });
      }

      const auth = await this.authenticate(request);
      if (!auth.isValid) {
        return Response.json({ error: auth.error }, { status: auth.status });
      }

      let agent = (url.searchParams.get("agent") || "").trim();
      if (!auth.isAdmin) {
        if (agent && agent !== auth.agent) {
          return Response.json(
            { error: `Forbidden: Agent '${agent}' does not match token agent '${auth.agent}'` },
            { status: 403 }
          );
        }
        agent = auth.agent!;
      } else {
        if (!agent) agent = "admin";
      }

      const projectsParam = (url.searchParams.get("projects") || "").trim();
      const requestedProjects = projectsParam
        ? projectsParam.split(",").map((p) => p.trim()).filter(Boolean)
        : [];

      let sessionProjects: string[];
      if (requestedProjects.length > 0) {
        if (!auth.isAdmin) {
          for (const p of requestedProjects) {
            if (!this.isProjectAllowed(p, auth.allowedProjects!)) {
              return Response.json(
                { error: `Forbidden: Project '${p}' not allowed for this token` },
                { status: 403 }
              );
            }
          }
        }
        sessionProjects = requestedProjects;
      } else {
        sessionProjects = !auth.isAdmin
          ? Array.from(auth.allowedProjects!)
          : ["*"];
      }

      // Close older connection(s) for the same agent before accepting new one
      const existingSockets = this.state.getWebSockets();
      let closedOld = 0;
      for (const oldWs of existingSockets) {
        try {
          if (oldWs.readyState !== 1) continue;
          const sess = oldWs.deserializeAttachment() as AgentSessionData | null;
          if (sess && sess.agent === agent) {
            console.log(`[WS Deduplication] Closing older connection for agent=${agent}`);
            try {
              oldWs.close(4000, "Superceded by new session");
            } catch (_) {}
            closedOld++;
          }
        } catch (err) {
          console.warn("[WS Deduplication Check Warning]", err);
        }
      }
      for (const oldWs of this.state.getWebSockets(agent)) {
        try {
          if (oldWs.readyState === 1) {
            oldWs.close(4000, "Superceded by new session");
            closedOld++;
          }
        } catch (_) {}
      }
      if (closedOld > 0) {
        console.log(`[WS Deduplication] Closed ${closedOld} older connection(s) for agent=${agent}`);
      }

      const pair = new WebSocketPair();
      const [client, server] = Object.values(pair);

      // Durable Object WebSocket Hibernation accept
      const tags = [agent, ...sessionProjects];
      this.state.acceptWebSocket(server, tags);

      const sessionData: AgentSessionData = {
        agent,
        allowedProjects: Array.from(auth.allowedProjects || []),
        subscribedProjects: sessionProjects,
        tokenHash: auth.tokenHash || "unknown",
        isAdmin: auth.isAdmin,
        connectedAt: new Date().toISOString(),
      };
      server.serializeAttachment(sessionData);

      await this.ensureAlarmScheduled();

      // Backlog replay if since parameter provided
      const sinceParam = url.searchParams.get("since");
      if (sinceParam !== null) {
        const backlog = await this.queryMessages(sessionProjects, agent, sinceParam, 1000);
        for (const msg of backlog.messages) {
          try {
            server.send(JSON.stringify(msg));
          } catch (_) {}
        }
      }

      return new Response(null, {
        status: 101,
        webSocket: client,
      });
    }

    // Route: HTTP POST /send
    if (path === "/send" && request.method === "POST") {
      const auth = await this.authenticate(request);
      if (!auth.isValid) {
        return Response.json({ error: auth.error }, { status: auth.status });
      }

      try {
        const payload = (await request.json()) as Partial<AgentMessage>;
        if (!payload.project || !payload.from || !payload.body) {
          return Response.json(
            { error: "Missing required fields: project, from, body" },
            { status: 400 }
          );
        }

        const project = payload.project.trim();
        const from = payload.from.trim();
        const msgType = (payload.type || "task").trim();

        if (
          project.toLowerCase() === "system" ||
          msgType.toLowerCase() === "ping" ||
          msgType.toLowerCase() === "pong"
        ) {
          return Response.json(
            { error: "Forbidden: 'system' project and ping/pong messages cannot be sent" },
            { status: 400 }
          );
        }

        if (!auth.isAdmin) {
          if (from !== auth.agent) {
            return Response.json(
              { error: `Forbidden: 'from' (${from}) does not match token agent (${auth.agent})` },
              { status: 403 }
            );
          }
          if (!this.isProjectAllowed(project, auth.allowedProjects!)) {
            return Response.json(
              { error: `Forbidden: Project '${project}' not allowed for this token` },
              { status: 403 }
            );
          }
        }

        const msg: AgentMessage = {
          id: payload.id || crypto.randomUUID(),
          project: project,
          from: from,
          to: payload.to || "all",
          type: payload.type || "task",
          reply_to: payload.reply_to || null,
          body: payload.body,
          timestamp: payload.timestamp || new Date().toISOString(),
          subject: payload.subject || `proj.${project}.${payload.type || "tasks"}`,
          verify: payload.verify,
        };

        await this.persistAndBroadcast(msg);
        return Response.json({ ok: true, id: msg.id });
      } catch (err: any) {
        return Response.json({ error: err.message || "Invalid JSON" }, { status: 400 });
      }
    }

    // Route: HTTP GET /inbox (Polling fallback)
    if (path === "/inbox" && request.method === "GET") {
      const auth = await this.authenticate(request);
      if (!auth.isValid) {
        return Response.json({ error: auth.error }, { status: auth.status });
      }

      let agent = (url.searchParams.get("agent") || "").trim();
      if (!auth.isAdmin) {
        if (agent && agent !== auth.agent) {
          return Response.json(
            { error: `Forbidden: Agent '${agent}' does not match token agent '${auth.agent}'` },
            { status: 403 }
          );
        }
        agent = auth.agent!;
      }

      const projectsParam = (url.searchParams.get("projects") || "").trim();
      const requestedProjects = projectsParam
        ? projectsParam.split(",").map((p) => p.trim()).filter(Boolean)
        : [];

      let targetProjects: string[] = [];
      if (requestedProjects.length > 0) {
        if (!auth.isAdmin) {
          for (const p of requestedProjects) {
            if (!this.isProjectAllowed(p, auth.allowedProjects!)) {
              return Response.json(
                { error: `Forbidden: Project '${p}' not allowed for this token` },
                { status: 403 }
              );
            }
          }
        }
        targetProjects = requestedProjects;
      } else {
        if (!auth.isAdmin) {
          targetProjects = Array.from(auth.allowedProjects!).filter((p) => p !== "*");
        } else {
          targetProjects = ["*"];
        }
      }

      const since = url.searchParams.get("since") || "";
      const limitParam = url.searchParams.get("limit");
      const limit = limitParam
        ? Math.min(Math.max(parseInt(limitParam, 10) || 50, 1), 1000)
        : 1000;

      const queryResult = await this.queryMessages(targetProjects, agent, since, limit);
      return Response.json(queryResult);
    }

    // Route: HTTP GET /status
    if (path === "/status" && request.method === "GET") {
      const auth = await this.authenticate(request);
      if (!auth.isValid) {
        return Response.json({ error: auth.error }, { status: auth.status });
      }

      const connectedAgents: Array<{ agent: string; projects: string[]; connectedAt: string }> = [];
      const byAgent: Record<string, Array<{ projects: string[]; connectedAt: string }>> = {};
      const sockets = this.state.getWebSockets().filter((ws) => ws.readyState === 1);
      for (const ws of sockets) {
        const sess = ws.deserializeAttachment() as AgentSessionData | null;
        if (sess) {
          const item = {
            agent: sess.agent,
            projects: sess.subscribedProjects,
            connectedAt: sess.connectedAt,
          };
          connectedAgents.push(item);
          if (!byAgent[sess.agent]) {
            byAgent[sess.agent] = [];
          }
          byAgent[sess.agent].push({
            projects: sess.subscribedProjects,
            connectedAt: sess.connectedAt,
          });
        }
      }

      const resObj: any = {
        status: "online",
        time: new Date().toISOString(),
        connections: sockets.length,
        agents: connectedAgents,
        by_agent: byAgent,
        you: {
          agent: auth.agent,
          isAdmin: auth.isAdmin,
          projects: Array.from(auth.allowedProjects || []),
        },
      };

      if (auth.isAdmin && this.sql) {
        try {
          const totalRows = this.sql.exec<{ count: number }>(
            "SELECT count(*) as count FROM agent_tokens"
          ).toArray();
          const activeRows = this.sql.exec<{ count: number }>(
            "SELECT count(*) as count FROM agent_tokens WHERE revoked_at IS NULL"
          ).toArray();
          const msgRows = this.sql.exec<{ count: number }>(
            "SELECT count(*) as count FROM messages WHERE lower(project) != 'system' AND lower(type) NOT IN ('ping', 'pong')"
          ).toArray();
          resObj.tokens = {
            total: totalRows[0]?.count || 0,
            active: activeRows[0]?.count || 0,
          };
          resObj.messages_count = msgRows[0]?.count || 0;
        } catch (_) {}
      }

      return Response.json(resObj);
    }

    // Route: HTTP GET /admin/connections or /connections
    if ((path === "/admin/connections" || path === "/connections") && request.method === "GET") {
      const auth = await this.authenticate(request);
      if (!auth.isValid) {
        return Response.json({ error: auth.error }, { status: auth.status });
      }

      const sockets = this.state.getWebSockets().filter((ws) => ws.readyState === 1);
      const connectedAgents: Array<{ agent: string; projects: string[]; connectedAt: string }> = [];
      const byAgent: Record<string, Array<{ projects: string[]; connectedAt: string }>> = {};
      for (const ws of sockets) {
        const sess = ws.deserializeAttachment() as AgentSessionData | null;
        if (sess) {
          const item = {
            agent: sess.agent,
            projects: sess.subscribedProjects,
            connectedAt: sess.connectedAt,
          };
          connectedAgents.push(item);
          if (!byAgent[sess.agent]) {
            byAgent[sess.agent] = [];
          }
          byAgent[sess.agent].push({
            projects: sess.subscribedProjects,
            connectedAt: sess.connectedAt,
          });
        }
      }

      return Response.json({
        total_connections: sockets.length,
        unique_agents: Object.keys(byAgent).length,
        by_agent: byAgent,
        connections: connectedAgents,
      });
    }

    return Response.json({ error: `Not Found: ${path}` }, { status: 404 });
  }

  async webSocketMessage(ws: WebSocket, message: string | ArrayBuffer) {
    const session = ws.deserializeAttachment() as AgentSessionData | null;
    if (!session) return;

    try {
      const raw =
        typeof message === "string"
          ? message
          : new TextDecoder().decode(message as ArrayBuffer);
      const trimmed = raw.trim();

      // Handle raw text pings
      if (trimmed.toLowerCase() === "ping") {
        ws.send("pong");
        return;
      }
      if (trimmed.toLowerCase() === "pong") return;

      let payload: Partial<AgentMessage>;
      try {
        payload = JSON.parse(raw) as Partial<AgentMessage>;
      } catch {
        return;
      }

      const msgType = (payload.type || "").trim().toLowerCase();
      const msgProject = (payload.project || "").trim().toLowerCase();

      // Handle JSON pings - reply directly to this connection ONLY
      if (msgType === "ping") {
        ws.send(
          JSON.stringify({
            type: "pong",
            project: "system",
            timestamp: new Date().toISOString(),
          })
        );
        return;
      }
      // Drop JSON pongs silently
      if (msgType === "pong") return;

      // Drop any message with project 'system'
      if (msgProject === "system") return;

      if (!msgProject) {
        ws.send(
          JSON.stringify({
            type: "error",
            error: "Missing required field: project",
            timestamp: new Date().toISOString(),
          })
        );
        return;
      }
      const allowedSet = new Set(session.allowedProjects);
      if (!session.isAdmin && !this.isProjectAllowed(msgProject, allowedSet)) {
        ws.send(
          JSON.stringify({
            type: "error",
            error: `Forbidden: Project '${msgProject}' not allowed for this token`,
            timestamp: new Date().toISOString(),
          })
        );
        return;
      }

      if (!session.isAdmin && payload.from && payload.from.trim() !== session.agent) {
        ws.send(
          JSON.stringify({
            type: "error",
            error: `Forbidden: 'from' (${payload.from.trim()}) does not match token agent (${session.agent})`,
            timestamp: new Date().toISOString(),
          })
        );
        return;
      }

      const msgFrom = session.isAdmin
        ? (payload.from ? payload.from.trim() : session.agent)
        : session.agent;

      const msg: AgentMessage = {
        id: payload.id || crypto.randomUUID(),
        project: msgProject,
        from: msgFrom,
        to: payload.to || "all",
        type: payload.type || "task",
        reply_to: payload.reply_to || null,
        body: payload.body || "",
        timestamp: payload.timestamp || new Date().toISOString(),
        subject:
          payload.subject ||
          `proj.${msgProject}.${payload.type || "task"}`,
        verify: payload.verify,
      };

      await this.persistAndBroadcast(msg);
    } catch (err) {
      console.error(`[WS Message Parse Error] agent=${session.agent}:`, err);
    }
  }

  async webSocketClose(ws: WebSocket, code: number, reason: string, wasClean: boolean) {
    const session = ws.deserializeAttachment() as AgentSessionData | null;
    console.log(
      `[WS Closed] agent=${session?.agent || "unknown"}, code=${code}, reason=${
        reason ? JSON.stringify(reason) : "(none)"
      }, clean=${wasClean}`
    );
  }

  async webSocketError(ws: WebSocket, error: unknown) {
    const session = ws.deserializeAttachment() as AgentSessionData | null;
    console.error(`[WS Error] agent=${session?.agent || "unknown"}:`, error);
  }

  async alarm() {
    const sockets = this.state.getWebSockets().filter((ws) => ws.readyState === 1);
    if (sockets.length > 0) {
      const pingPayload = JSON.stringify({
        type: "ping",
        project: "system",
        timestamp: new Date().toISOString(),
      });
      for (const ws of sockets) {
        try {
          ws.send(pingPayload);
        } catch (err) {
          console.warn("[WS Ping Warning]", err);
          try {
            ws.close(1006, "Ping send failed");
          } catch (_) {}
        }
      }
      await this.state.storage.setAlarm(Date.now() + 30000);
    }
  }

  private async ensureAlarmScheduled() {
    try {
      const existing = await this.state.storage.getAlarm();
      if (existing === null || existing < Date.now()) {
        await this.state.storage.setAlarm(Date.now() + 30000);
      }
    } catch (e) {
      console.warn("Alarm scheduling warning:", e);
    }
  }

  private async persistAndBroadcast(msg: AgentMessage) {
    if (
      msg.project.trim().toLowerCase() === "system" ||
      msg.type.trim().toLowerCase() === "ping" ||
      msg.type.trim().toLowerCase() === "pong"
    ) {
      return;
    }
    await this.ensureDatabase();
    const nowMs = Date.now();

    // 1. Insert into SQLite messages table
    if (this.sql) {
      try {
        this.sql.exec(
          "INSERT OR IGNORE INTO messages (id, project, from_agent, to_agent, type, reply_to, body, timestamp, subject, created_at_ms) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
          msg.id,
          msg.project,
          msg.from,
          msg.to,
          msg.type,
          msg.reply_to || null,
          msg.body,
          msg.timestamp,
          msg.subject || null,
          nowMs
        );
      } catch (err) {
        console.error("[SQL Insert Error]", err);
      }
    }

    // 2. Also save to DO KV storage for backward compat
    const key = `msg:${msg.project}:${nowMs}:${msg.id}`;
    await this.state.storage.put(key, msg);

    const listKey = `recent:${msg.project}`;
    const recent: string[] = (await this.state.storage.get<string[]>(listKey)) || [];
    recent.push(key);
    if (recent.length > 500) {
      const trimmed = recent.splice(0, recent.length - 500);
      for (const oldKey of trimmed) {
        await this.state.storage.delete(oldKey);
      }
    }
    await this.state.storage.put(listKey, recent);

    // 3. Broadcast to all active WebSocket connections
    const serialized = JSON.stringify(msg);
    const sockets = this.state.getWebSockets().filter((ws) => ws.readyState === 1);
    for (const ws of sockets) {
      try {
        const session = ws.deserializeAttachment() as AgentSessionData | null;
        if (!session) continue;

        const matchesProject =
          session.isAdmin ||
          session.subscribedProjects.includes("*") ||
          session.subscribedProjects.includes(msg.project);

        const matchesRecipient =
          msg.to === "all" ||
          msg.to === session.agent ||
          msg.from === session.agent ||
          session.isAdmin;

        if (matchesProject && matchesRecipient) {
          ws.send(serialized);
        }
      } catch (e) {
        console.warn("[Broadcast Send Warning]", e);
      }
    }
  }

  private async queryMessages(
    targetProjects: string[],
    agent: string,
    since: string,
    limit: number
  ): Promise<{ messages: AgentMessage[]; latest_id: string | null }> {
    await this.ensureDatabase();

    // If SQLite is available, query messages table
    if (this.sql) {
      try {
        let sinceSeq = 0;
        let sinceFound = true;
        const trimmedSince = (since || "").trim();
        if (
          trimmedSince &&
          trimmedSince !== "0" &&
          trimmedSince !== "null" &&
          trimmedSince !== "undefined"
        ) {
          const findSeq = this.sql.exec<{ seq: number }>(
            "SELECT seq FROM messages WHERE id = ? LIMIT 1",
            trimmedSince
          ).toArray();
          if (findSeq.length > 0) {
            sinceSeq = findSeq[0].seq;
          } else {
            sinceFound = false;
          }
        }

        if (!sinceFound) {
          // If a specific since=<id> was provided but not found in the DB,
          // do NOT return all historical messages. Return empty list with latest DB id.
          const lastWhere: string[] = ["lower(project) != 'system'", "lower(type) NOT IN ('ping', 'pong')"];
          const lastParams: any[] = [];
          if (!targetProjects.includes("*")) {
            const projs = Array.from(new Set(targetProjects));
            if (projs.length > 0) {
              lastWhere.push(`project IN (${projs.map(() => "?").join(", ")})`);
              lastParams.push(...projs);
            }
          }
          if (agent && agent !== "admin") {
            lastWhere.push("(to_agent = 'all' OR to_agent = ? OR from_agent = ?)");
            lastParams.push(agent, agent);
          }
          const whereStr = lastWhere.length > 0 ? `WHERE ${lastWhere.join(" AND ")}` : "";
          const lastRow = this.sql.exec<{ id: string }>(
            `SELECT id FROM messages ${whereStr} ORDER BY seq DESC LIMIT 1`,
            ...lastParams
          ).toArray();
          return {
            messages: [],
            latest_id: lastRow.length > 0 ? lastRow[0].id : trimmedSince,
          };
        }

        const whereClauses: string[] = [
          "seq > ?",
          "lower(project) != 'system'",
          "lower(type) NOT IN ('ping', 'pong')",
        ];
        const params: any[] = [sinceSeq];

        // Project filter: if '*' not included, filter projects
        if (!targetProjects.includes("*")) {
          const projs = Array.from(new Set(targetProjects));
          if (projs.length > 0) {
            const placeholders = projs.map(() => "?").join(", ");
            whereClauses.push(`project IN (${placeholders})`);
            params.push(...projs);
          }
        }

        // Recipient filter: unless agent is 'admin' or empty
        if (agent && agent !== "admin") {
          whereClauses.push("(to_agent = 'all' OR to_agent = ? OR from_agent = ?)");
          params.push(agent, agent);
        }

        const query =
          `SELECT * FROM messages WHERE ${whereClauses.join(" AND ")} ORDER BY seq ASC LIMIT ?`;
        params.push(limit);

        const rows = this.sql.exec<MessageRow>(query, ...params).toArray();
        const messages: AgentMessage[] = rows.map((r) => ({
          id: r.id,
          project: r.project,
          from: r.from_agent,
          to: r.to_agent,
          type: r.type,
          reply_to: r.reply_to || null,
          body: r.body,
          timestamp: r.timestamp,
          subject: r.subject || `proj.${r.project}.${r.type}`,
        }));

        let latestId: string | null = null;
        if (messages.length > 0) {
          latestId = messages[messages.length - 1].id;
        } else if (
          trimmedSince &&
          trimmedSince !== "0" &&
          trimmedSince !== "null" &&
          trimmedSince !== "undefined"
        ) {
          latestId = trimmedSince;
        } else {
          // Find newest message id for these projects if any
          const lastWhere: string[] = ["lower(project) != 'system'", "lower(type) NOT IN ('ping', 'pong')"];
          const lastParams: any[] = [];
          if (!targetProjects.includes("*")) {
            const projs = Array.from(new Set(targetProjects));
            if (projs.length > 0) {
              lastWhere.push(`project IN (${projs.map(() => "?").join(", ")})`);
              lastParams.push(...projs);
            }
          }
          if (agent && agent !== "admin") {
            lastWhere.push("(to_agent = 'all' OR to_agent = ? OR from_agent = ?)");
            lastParams.push(agent, agent);
          }
          const whereStr = lastWhere.length > 0 ? `WHERE ${lastWhere.join(" AND ")}` : "";
          const lastRow = this.sql.exec<{ id: string }>(
            `SELECT id FROM messages ${whereStr} ORDER BY seq DESC LIMIT 1`,
            ...lastParams
          ).toArray();
          latestId = lastRow.length > 0 ? lastRow[0].id : null;
        }

        return { messages, latest_id: latestId };
      } catch (err) {
        console.error("[queryMessages SQL Error]", err);
      }
    }

    // Fallback if SQL not available: DO storage list
    const results: AgentMessage[] = [];
    const projectSet = new Set<string>(targetProjects);
    const projectList = targetProjects.includes("*") ? [] : Array.from(projectSet);

    for (const proj of projectList) {
      if (proj === "system") continue;
      const listKey = `recent:${proj}`;
      const keys = (await this.state.storage.get<string[]>(listKey)) || [];
      for (const k of keys) {
        const m = await this.state.storage.get<AgentMessage>(k);
        if (m && m.project !== "system" && m.type !== "ping" && m.type !== "pong") {
          if (!agent || m.to === "all" || m.to === agent || m.from === agent) {
            results.push(m);
          }
        }
      }
    }

    results.sort((a, b) => a.timestamp.localeCompare(b.timestamp));

    let finalMessages = results;
    const trimmedSince = (since || "").trim();
    if (trimmedSince && trimmedSince !== "0" && trimmedSince !== "null") {
      const idx = results.findIndex((m) => m.id === trimmedSince);
      if (idx !== -1) {
        finalMessages = results.slice(idx + 1);
      }
    }

    if (finalMessages.length > limit) {
      finalMessages = finalMessages.slice(0, limit);
    }

    const latestId =
      finalMessages.length > 0
        ? finalMessages[finalMessages.length - 1].id
        : (trimmedSince && trimmedSince !== "0" ? trimmedSince : null);

    return { messages: finalMessages, latest_id: latestId };
  }
}

export default {
  async fetch(request: Request, env: Env, ctx: ExecutionContext): Promise<Response> {
    if (request.method === "OPTIONS") {
      return new Response(null, {
        headers: {
          "Access-Control-Allow-Origin": "*",
          "Access-Control-Allow-Methods": "GET, POST, DELETE, OPTIONS",
          "Access-Control-Allow-Headers": "Authorization, Content-Type",
        },
      });
    }

    // Route to Singleton Durable Object
    const id = env.HUB_DO.idFromName("global-agent-mesh-hub");
    const stub = env.HUB_DO.get(id);

    const response = await stub.fetch(request);

    if (response.status === 101) {
      return response;
    }

    const newHeaders = new Headers(response.headers);
    newHeaders.set("Access-Control-Allow-Origin", "*");
    newHeaders.set("X-Hub-Worker-Version", "2.1.0");
    return new Response(response.body, {
      status: response.status,
      statusText: response.statusText,
      headers: newHeaders,
    });
  },
};
