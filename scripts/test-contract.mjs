import { spawnSync } from "node:child_process";
import { fileURLToPath } from "node:url";
const result = spawnSync("python3", [fileURLToPath(new URL("./check.py", import.meta.url))], { stdio: "inherit" });
if (result.error) throw result.error;
process.exitCode = result.status ?? 1;
