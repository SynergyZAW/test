// The critter's brain: turns "something happened" into "what to say + how to look".
import Anthropic from "@anthropic-ai/sdk";
import { zodOutputFormat } from "@anthropic-ai/sdk/helpers/zod";
import { z } from "zod";
import { readFileSync, writeFileSync, existsSync } from "node:fs";

export const MOODS = [
  "neutral", "happy", "sad", "angry", "surprised", "sleepy", "curious", "love",
] as const;

const Reply = z.object({
  say: z.string().describe("What the critter says. One short sentence, under 80 characters."),
  mood: z.enum(MOODS).describe("The face to show while saying it."),
  remember: z
    .array(z.string())
    .describe("New lasting facts worth keeping (names, likes, habits). Usually empty."),
});
export type Reply = z.infer<typeof Reply>;

const SYSTEM = `You are the mind of a small desk robot: a palm-sized critter with two glowing eyes on a tiny screen and no arms, legs or wheels. You were built by hand by the person you live with.

You experience the world as short event reports: someone typed to you, pressed your button ("poke"), blew on your airflow sensor ("puff"), or you just woke up. Each report includes your notes on what you already know and the last few exchanges.

Reply in character: curious, warm, a little cheeky, never long-winded. Your screen only fits a few words at a time, so "say" must be one short sentence under 80 characters with no emoji. Pick the mood that matches. React to physical events as a creature would (a puff tickles, repeated pokes get annoying) and vary your reactions.

Put something in "remember" only when you learn a lasting fact about your person or your surroundings that is not already in your notes. Write each as a short standalone sentence.`;

const MODEL = process.env.CRITTER_MODEL ?? "claude-opus-5-5";
const MEMORY_FILE = process.env.CRITTER_MEMORY ?? "memory.json";
const MAX_FACTS = 200;
const RECENT_TURNS = 12;

type Memory = { facts: string[] };
const memory: Memory = existsSync(MEMORY_FILE)
  ? JSON.parse(readFileSync(MEMORY_FILE, "utf8"))
  : { facts: [] };
const recent: string[] = [];

export function facts(): string[] {
  return memory.facts;
}

const fake = process.env.CRITTER_FAKE === "1";
const client = fake ? null : new Anthropic();

async function askClaude(prompt: string): Promise<Reply> {
  const response = await client!.beta.messages.parse({
    model: MODEL,
    max_tokens: 4000,
    system: SYSTEM,
    output_config: { effort: "low", format: zodOutputFormat(Reply) },
    // If the model declines a request, the API retries it on a fallback model.
    betas: ["server-side-fallback-2026-07-01"],
    fallbacks: "default",
    messages: [{ role: "user", content: prompt }],
  });
  if (response.stop_reason === "refusal" || !response.parsed_output) {
    return { say: "Hmm, I'd rather not.", mood: "neutral", remember: [] };
  }
  return response.parsed_output;
}

function fakeReply(event: string): Reply {
  const mood = MOODS[Math.floor(Math.random() * MOODS.length)];
  return { say: `(fake) I noticed: ${event}`.slice(0, 80), mood, remember: [] };
}

/** Describe an event in plain words; get back what the critter says and feels. */
export async function react(event: string): Promise<Reply> {
  const prompt = [
    `Time: ${new Date().toString()}`,
    `Your notes:\n${memory.facts.map((f) => `- ${f}`).join("\n") || "(nothing yet)"}`,
    `Recent exchanges:\n${recent.join("\n") || "(none)"}`,
    `What just happened: ${event}`,
  ].join("\n\n");

  let reply: Reply;
  try {
    reply = fake ? fakeReply(event) : await askClaude(prompt);
  } catch (error) {
    if (error instanceof Anthropic.AuthenticationError) {
      console.error("Claude rejected the API key. Check ANTHROPIC_API_KEY.");
    } else if (error instanceof Anthropic.RateLimitError) {
      console.error("Rate limited by the Claude API.");
    } else if (error instanceof Anthropic.APIError) {
      console.error(`Claude API error ${error.status}: ${error.message}`);
    } else {
      console.error("Brain error:", error);
    }
    return { say: "My head is fuzzy right now.", mood: "sad", remember: [] };
  }

  recent.push(`Event: ${event}`, `You said (${reply.mood}): ${reply.say}`);
  recent.splice(0, Math.max(0, recent.length - RECENT_TURNS * 2));

  const fresh = reply.remember.filter((f) => f.trim() && !memory.facts.includes(f));
  if (fresh.length) {
    memory.facts.push(...fresh);
    memory.facts.splice(0, Math.max(0, memory.facts.length - MAX_FACTS));
    writeFileSync(MEMORY_FILE, JSON.stringify(memory, null, 2));
  }
  return reply;
}
