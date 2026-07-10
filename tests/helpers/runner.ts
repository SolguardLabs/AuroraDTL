import { spawnSync } from "node:child_process";
import { existsSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

export type JsonObject = Record<string, any>;

export const root = resolve(dirname(fileURLToPath(import.meta.url)), "..", "..");
export const binary = join(root, "out", process.platform === "win32" ? "auroradtl.exe" : "auroradtl");

export function ensureBuilt(): void {
  if (existsSync(binary)) {
    return;
  }
  const result = spawnSync(process.execPath, ["scripts/build.mjs"], {
    cwd: root,
    encoding: "utf8",
  });
  if (result.status !== 0) {
    throw new Error(result.stderr || result.stdout || "build failed");
  }
}

export function runCli(args: string[]): string {
  ensureBuilt();
  const result = spawnSync(binary, args, {
    cwd: root,
    encoding: "utf8",
  });
  if (result.status !== 0) {
    throw new Error(`command failed: ${binary} ${args.join(" ")}\n${result.stderr}`);
  }
  return result.stdout;
}

export function runFixture(name: string, command = "run", options: string[] = []): JsonObject {
  const fixture = join("tests", "fixtures", name);
  const stdout = runCli([command, fixture, "--json", ...options]);
  return JSON.parse(stdout);
}

export function validateFixture(name: string): string {
  const fixture = join("tests", "fixtures", name);
  return runCli(["validate", fixture]).trim();
}

export function byId<T extends { id: string }>(collection: T[], id: string): T {
  const found = collection.find((item) => item.id === id);
  if (!found) {
    throw new Error(`missing id ${id}`);
  }
  return found;
}

export function balance(report: JsonObject, account: string, asset: string): string {
  const found = report.balances.find((row: JsonObject) => row.account === account && row.asset === asset);
  return found?.amount ?? "0";
}
