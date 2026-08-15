import assert from "node:assert/strict";
import test from "node:test";

import {
  AuroraClient,
  applyFee,
  denormalizeFrom18,
  deviationBps,
  formatUnits,
  normalizeTo18,
  parseUnits,
  planWindow,
  quoteByPrice,
  stressPortfolio,
  toleranceFloor,
  type AuroraTransport,
} from "./AuroraClient.ts";

test("native units round-trip without floating point", () => {
  const raw = parseUnits("1234.56789", 6);
  assert.equal(raw, 1_234_567_890n);
  assert.equal(formatUnits(raw, 6), "1234.56789");
});

test("normalization converts native units to and from the 18-decimal index", () => {
  const raw = 3_000n;
  const normalized = normalizeTo18(raw, 6);
  assert.equal(normalized, 3_000_000_000_000_000n);
  assert.equal(denormalizeFrom18(normalized, 6), raw);
});

test("price quote returns destination-native precision", () => {
  const output = quoteByPrice(1_000_000_000_000n, 18, 6, 3000_00000000n, 1_00000000n);
  assert.equal(output, 3_000n);
});

test("fees and tolerance use deterministic bigint rounding", () => {
  const fees = applyFee(1_000_001n, 25);
  assert.deepEqual(fees, { fee: 2_500n, net: 997_501n });
  assert.equal(toleranceFloor(fees.net, 50), 992_514n);
});

test("deviation is symmetric and bounded", () => {
  assert.equal(deviationBps(100n, 90n), 1000);
  assert.equal(deviationBps(90n, 100n), 1000);
  assert.equal(deviationBps(0n, 0n), 0);
});

test("portfolio stress reports loss, coverage and concentration", () => {
  const report = stressPortfolio(
    [
      {
        asset: "WETH18",
        amountRaw: 2n * 10n ** 18n,
        decimals: 18,
        priceE8: 3000n * 10n ** 8n,
        shockBps: 2000,
        accessibilityHaircutBps: 1000,
      },
      {
        asset: "USDC6",
        amountRaw: 7000n * 10n ** 6n,
        decimals: 6,
        priceE8: 10n ** 8n,
        shockBps: 500,
        accessibilityHaircutBps: 500,
      },
    ],
    1500n * 10n ** 8n,
  );
  assert.equal(report.baselineE8, 13_000n * 10n ** 8n);
  assert.ok(report.lossE8 > 1500n * 10n ** 8n);
  assert.ok(report.uncoveredLossE8 > 0n);
  assert.ok(report.largestAssetShareBps > 5000);
  assert.ok(report.reserveCoverageBps < 10_000);
});

test("window planner separates stale windows from exhausted capacity", () => {
  assert.equal(planWindow(10n, 9n, 50n, 100n, 10n).reason, "window-mismatch");
  assert.equal(planWindow(10n, 10n, 95n, 100n, 10n).reason, "capacity-exceeded");
  assert.deepEqual(planWindow(10n, 10n, 40n, 100n, 25n), {
    accepted: true,
    window: 10n,
    requestedRaw: 25n,
    remainingRaw: 35n,
    projectedUtilizationBps: 6500,
    reason: "accepted",
  });
});

test("client delegates commands and parses operational reports", async () => {
  const calls: unknown[][] = [];
  const transport: AuroraTransport = {
    async execute(command, fixture, options = []) {
      calls.push([command, fixture, options]);
      if (command === "validate") return "ok\n";
      return JSON.stringify({
        scenario: "sample",
        status: "ok",
        quotes: [],
        settlements: [],
        balances: [],
        stress: {},
        checkpoint: {},
      });
    },
  };
  const client = new AuroraClient(transport);
  assert.equal(await client.validate("scenario.json"), true);
  assert.equal((await client.quote("scenario.json")).scenario, "sample");
  assert.equal((await client.settle("scenario.json", true)).status, "ok");
  assert.deepEqual(calls[2], ["settle", "scenario.json", ["--json", "--events"]]);
});
