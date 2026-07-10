import assert from "node:assert/strict";
import test from "node:test";

import { byId, runFixture } from "../helpers/runner.ts";

test("single-hop quote applies tolerance and emits net output", () => {
  const report = runFixture("balanced_quote.json", "quote");
  const quote = byId(report.quotes, "ord-balanced-1");

  assert.equal(quote.accepted, true);
  assert.equal(quote.source, "USD18");
  assert.equal(quote.destination, "EUR18");
  assert.equal(quote.grossOut, "1000000000000000");
  assert.equal(quote.netOut, "998000000000000");
  assert.equal(quote.toleranceFloor, "993010000000000");
  assert.equal(quote.fees.totalBps, 20);
});

test("quote output respects destination asset precision", () => {
  const report = runFixture("precision_quote.json", "quote");
  const quote = byId(report.quotes, "ord-precision-1");

  assert.equal(quote.accepted, true);
  assert.equal(quote.source, "WETH18");
  assert.equal(quote.destination, "USDC6");
  assert.equal(quote.grossOut, "3000");
  assert.equal(quote.netOut, "3000");
  assert.equal(quote.legs.length, 1);
  assert.equal(quote.legs[0].output, "3000");
});
