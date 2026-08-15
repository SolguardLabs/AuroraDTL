import { existsSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { spawnSync } from "node:child_process";

const root = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const executable = join(
  root,
  "out",
  process.platform === "win32" ? "aurora_native_tests.exe" : "aurora_native_tests",
);

if (!existsSync(executable)) {
  console.error(`Native test executable not found: ${executable}`);
  process.exit(1);
}

const result = spawnSync(executable, [], {
  cwd: root,
  encoding: "utf8",
  stdio: "inherit",
  shell: false,
});
process.exit(result.status ?? 1);
