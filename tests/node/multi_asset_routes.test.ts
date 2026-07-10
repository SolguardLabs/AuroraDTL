import assert from "node:assert/strict";
import test from "node:test";

import { balance, byId, runFixture } from "../helpers/runner.ts";

test("multi-asset route records every leg and settles final asset", () => {
  const report = runFixture("multi_asset_route.json");
  const quote = byId(report.quotes, "ord-multi-1");
  const settlement = byId(report.settlements, "ord-multi-1");

  assert.equal(quote.accepted, true);
  assert.equal(quote.legs.length, 2);
  assert.equal(quote.legs[0].source, "USD18");
  assert.equal(quote.legs[0].destination, "MID18");
  assert.equal(quote.legs[0].output, "2000000000000000");
  assert.equal(quote.legs[1].source, "MID18");
  assert.equal(quote.legs[1].destination, "EUR18");
  assert.equal(quote.netOut, "998000000000000");
  assert.equal(settlement.status, "settled");
  assert.equal(balance(report, "alice", "EUR18"), "998000000000000");
});
