// Relay: sits on your home network between the critter and Claude.
//   critter  --POST /event-->  relay  --> Claude
//   phone    --POST /say---->  relay  --> Claude, reply queued for the critter
//   critter  --GET  /poll--->  relay  (picks up queued replies)
import { createServer, type IncomingMessage, type ServerResponse } from "node:http";
import { readFileSync } from "node:fs";
import { networkInterfaces } from "node:os";
import { react, facts, type Reply } from "./brain.ts";

const PORT = Number(process.env.PORT ?? 8787);
const TOKEN = process.env.CRITTER_TOKEN ?? "";
const PAGE = readFileSync(new URL("./page.html", import.meta.url));

const outbox: Reply[] = [];
let last: Reply = { say: "", mood: "sleepy", remember: [] };
let lastSeen = 0;

const EVENTS: Record<string, string> = {
  boot: "You just woke up (powered on).",
  poke: "Someone pressed your button (a poke).",
  puff: "Someone blew on your airflow sensor (a puff of air).",
};

async function body(req: IncomingMessage): Promise<Record<string, unknown>> {
  let raw = "";
  for await (const chunk of req) {
    raw += chunk;
    if (raw.length > 10_000) throw new Error("body too large");
  }
  const parsed: unknown = JSON.parse(raw || "{}");
  if (typeof parsed !== "object" || parsed === null) throw new Error("expected an object");
  return parsed as Record<string, unknown>;
}

function send(res: ServerResponse, status: number, data?: unknown) {
  res.writeHead(status, { "content-type": "application/json" });
  res.end(data === undefined ? "" : JSON.stringify(data));
}

const server = createServer(async (req, res) => {
  const url = new URL(req.url ?? "/", "http://critter");
  try {
    if (req.method === "GET" && url.pathname === "/") {
      res.writeHead(200, { "content-type": "text/html; charset=utf-8" });
      return res.end(PAGE);
    }
    if (TOKEN && req.headers["x-critter-token"] !== TOKEN) {
      return send(res, 401, { error: "wrong or missing token" });
    }
    if (req.method === "GET" && url.pathname === "/poll") {
      lastSeen = Date.now();
      const next = outbox.shift();
      return next ? send(res, 200, next) : send(res, 204);
    }
    if (req.method === "POST" && url.pathname === "/event") {
      lastSeen = Date.now();
      const { type } = await body(req);
      const event = EVENTS[String(type)];
      if (!event) return send(res, 400, { error: `unknown event type: ${String(type)}` });
      console.log(`[critter] ${String(type)}`);
      last = await react(event);
      console.log(`  -> (${last.mood}) ${last.say}`);
      return send(res, 200, last);
    }
    if (req.method === "POST" && url.pathname === "/say") {
      const { text } = await body(req);
      if (typeof text !== "string" || !text.trim()) return send(res, 400, { error: "text required" });
      console.log(`[you] ${text}`);
      last = await react(`Your person typed to you: "${text.trim().slice(0, 500)}"`);
      console.log(`  -> (${last.mood}) ${last.say}`);
      outbox.push(last);
      outbox.splice(0, Math.max(0, outbox.length - 5));
      return send(res, 200, last);
    }
    if (req.method === "GET" && url.pathname === "/status") {
      return send(res, 200, { last, facts: facts(), online: Date.now() - lastSeen < 10_000 });
    }
    send(res, 404, { error: "not found" });
  } catch (error) {
    send(res, 400, { error: error instanceof Error ? error.message : "bad request" });
  }
});

server.listen(PORT, () => {
  const addresses = Object.values(networkInterfaces())
    .flat()
    .filter((n) => n && n.family === "IPv4" && !n.internal)
    .map((n) => n!.address);
  console.log(`Critter relay listening on port ${PORT}`);
  for (const a of addresses) console.log(`  open http://${a}:${PORT} on your phone; use ${a} as RELAY_HOST`);
  if (!TOKEN) console.log("  (no CRITTER_TOKEN set: anyone on your network can talk to it)");
});
