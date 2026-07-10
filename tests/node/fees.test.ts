import assert from "node:assert/strict";
import test from "node:test";

import { balance, byId, runFixture } from "../helpers/runner.ts";

test("fee matrix combines default, route and asset components", () => {
  const report = runFixture("fee_matrix.json");
  const quote = byId(report.quotes, "ord-fee-1");
  const settlement = byId(report.settlements, "ord-fee-1");

  assert.equal(quote.fees.baseBps, 12);
  assert.equal(quote.fees.routeBps, 8);
  assert.equal(quote.fees.assetBps, 10);
  assert.equal(quote.fees.totalBps, 30);
  assert.equal(quote.fees.fee, "3000000000000");
  assert.equal(settlement.destinationFee, "3000000000000");
  assert.equal(balance(report, "treasury", "EUR18"), "3000000000000");
});
