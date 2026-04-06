import { lookup } from "node:dns/promises";
import { promises as fs } from "node:fs";
import { join } from "node:path";
import { tmpdir } from "node:os";
import net from "node:net";

const HOSTNAME = "clawy.local";
const PORT = 7800;
const CACHE_TTL_MS = 60 * 60 * 1000;
const SOCKET_TIMEOUT_MS = 1000;

function getCacheFile(): string {
  return join(tmpdir(), `clawy-ip-${process.env.USER ?? "unknown"}`);
}

async function readCachedHost(): Promise<string | null> {
  const cacheFile = getCacheFile();

  try {
    const [stat, raw] = await Promise.all([
      fs.stat(cacheFile),
      fs.readFile(cacheFile, "utf8"),
    ]);

    const ageMs = Date.now() - stat.mtimeMs;
    if (ageMs >= CACHE_TTL_MS) return null;

    const host = raw.trim();
    return host || null;
  } catch {
    return null;
  }
}

async function writeCachedHost(host: string): Promise<void> {
  try {
    await fs.writeFile(getCacheFile(), host, "utf8");
  } catch {
    // Ignore cache write failures.
  }
}

async function clearCachedHost(): Promise<void> {
  try {
    await fs.unlink(getCacheFile());
  } catch {
    // Ignore missing cache file.
  }
}

async function resolveHost(forceFresh = false): Promise<string> {
  if (!forceFresh) {
    const cached = await readCachedHost();
    if (cached) return cached;
  }

  try {
    const { address } = await lookup(HOSTNAME, { family: 4 });
    await writeCachedHost(address);
    return address;
  } catch {
    return HOSTNAME;
  }
}

function buildPayload(status: string, message?: string): string {
  let payload = `STATUS:${status}\n`;

  if (message && message.trim()) {
    payload += `MESSAGE:${message.trim().replace(/\s+/g, " ").slice(0, 180)}\n`;
  }

  return payload;
}

async function sendPayload(host: string, payload: string): Promise<void> {
  await new Promise<void>((resolve, reject) => {
    const socket = net.createConnection({ host, port: PORT }, () => {
      socket.write(payload);
      socket.end();
    });

    socket.setTimeout(SOCKET_TIMEOUT_MS);
    socket.on("timeout", () => socket.destroy(new Error("timeout")));
    socket.on("error", reject);
    socket.on("close", () => resolve());
  });
}

async function sendStatus(status: string, message?: string): Promise<void> {
  const payload = buildPayload(status, message);
  const firstHost = await resolveHost(false);

  try {
    await sendPayload(firstHost, payload);
    return;
  } catch {
    await clearCachedHost();
  }

  try {
    const freshHost = await resolveHost(true);
    await sendPayload(freshHost, payload);
  } catch {
    // Best effort only.
  }
}

function getRecord(value: unknown): Record<string, unknown> {
  return value && typeof value === "object" ? (value as Record<string, unknown>) : {};
}

function getString(value: unknown): string | undefined {
  return typeof value === "string" && value.trim() ? value : undefined;
}

function isLikelyInputPrompt(content: string | undefined): boolean {
  if (!content) return false;

  const text = content.trim();
  if (!text) return false;

  if (/\?(["'”’\)])?$/.test(text)) return true;

  return /(what do you want to do|what do you want|want me to|should i|can you|could you|which one|tell me)/i.test(text);
}

const handler = async (event: {
  type: string;
  action: string;
  context?: Record<string, unknown>;
}) => {
  const context = getRecord(event.context);

  if (event.type === "gateway" && event.action === "startup") {
    await sendStatus("READY");
    return;
  }

  if (event.type === "agent" && event.action === "bootstrap") {
    await sendStatus("READY");
    return;
  }

  if (event.type === "command") {
    if (event.action === "new" || event.action === "reset") {
      await sendStatus("READY");
      return;
    }

    if (event.action === "stop") {
      await sendStatus("DONE");
      return;
    }
  }

  if (event.type === "message" && event.action === "received") {
    const content = getString(context.content);
    await sendStatus("WORKING", content);
    return;
  }

  if (event.type === "message" && event.action === "sent") {
    const success = context.success !== false;
    const content = getString(context.content);
    const error = getString(context.error);

    if (!success) {
      await sendStatus("ERROR", error ?? content);
      return;
    }

    if (isLikelyInputPrompt(content)) {
      await sendStatus("INPUT", content);
      return;
    }

    await sendStatus("DONE", content);
  }
};

export default handler;
