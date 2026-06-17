# LTP Configuration Tool — migrating off the deprecated `maxber`

`doc/ION-LTP-configuration_tool.xlsm` currently emits the LTP retransmission
configuration as a **maxBER** (bit-error-rate) value, intended for the
deprecated `m maxber` command. ION 4.2 replaces `maxber` with the segment loss
rate and retry count set **directly** — `m maxseglossrate` and `m maxretries`
(unified-mode defaults `maxSegmentLossRate = 0.01`, `maxRetries = 5`; split mode
adds per-direction variants). This note records the precise edits, **now applied**
to `doc/ION-LTP-configuration_tool.xlsm`: the cell/formula/label changes were made
directly in the OOXML parts; the VBA (`vbaProject.bin`), conditional formatting,
embedded drawing/image, and all untouched cells were preserved byte-for-byte, and
the stale `calcChain` cache was dropped with `fullCalcOnLoad="1"` so Excel
recomputes on open. The maxBER cell was kept (relabeled "legacy") for reference;
no VBA was changed. **Verification limit:** the result was validated structurally
(well-formed XML, preserved parts, formulas read back correctly) but not opened in
Excel here — open it once to confirm it recalculates without a repair prompt and
to let it rebuild the calc cache.

## Why this is small, not a rewrite

The tool already computes the physically meaningful **segment error rate**
directly on the *Link* sheet; `maxBER` is only a derived encoding that the
*Main* sheet then converts *back* into a segment loss rate. Removing that
round-trip and emitting the segment loss rate directly is most of the job.

Current data flow:

    Link!C8  ser_ccsds(segSize, frameBytes, frameErrRate)  -> segment error rate (e.g. 2.0e-5)
    Link!C9  ser2ber($C$6, $C$8)                           -> maxBER (e.g. 1.39e-9)   [conversion]
    Main!C3  =Link!C9                                       -> "Maximum bit error rate" input
    Main!C34 MIN(0.99, 1-POWER((1-Main!$C$3), 8*(C31+C11))) -> segment loss rate (≈ Link!C8) [round-trip]
    Main!C40 ...*twoSigmaNs(C34, C32)                       -> 95th-pct retransmission load
    Main!F40 "a span ..."                                   -> generated span command (no maxBER)

## Engine reference (`ltp/library/libltpP.c`)

- **Legacy:** `segLossRate = 1 − (1 − maxBER)^(segSize·8)`; then
  `maxTimeouts = max(3, ceil( log(1e-6) / log(segLossRate) )) × SIGNAL_REDUNDANCY`.
- **New (unified):** `xmitSegLossRate = maxSegmentLossRate` (set directly);
  `maxTimeouts = maxRetries × SIGNAL_REDUNDANCY`. Defaults `0.01` / `5`.

So the legacy retry count is just the residual-block-failure formula evaluated
at the segment loss rate, with a `1e-6` target and a floor of 3. The tool should
reproduce that to emit a `maxRetries` consistent with prior behavior.

## Cell edits

### *Link* sheet

| Cell | Now | Change |
|------|-----|--------|
| `C8`/`D8` | `ser_ccsds(...)` — segment error rate | **Keep.** This becomes the primary deliverable (= `maxseglossrate`). Relabel `B8` "Segment loss rate (use for `m maxseglossrate`)". |
| `B9` | "maxBER Computation" | Relabel "maxBER (legacy `m maxber` only — deprecated)". |
| `C9`/`D9` | `ser2ber($C$6,$C$8)` | **Keep for legacy reference or delete.** No longer the primary output. |
| `C10`/`D10` | `ser2efer(...)` — Ethernet error rate | **Keep unchanged** (lab loss simulation derives from segment error rate). |

### *Main* sheet

| Cell | Now | Change |
|------|-----|--------|
| `B3` | "Expected (mean) bit error rate (assumed symmetrical)" | Relabel "Expected (mean) segment loss rate (assumed symmetrical)". |
| `C3`/`D3` | `=Link!C9` (maxBER) | `=Link!C8` (segment loss rate). |
| `C34`/`D34` | `MIN(0.99, 1-POWER((1-Main!$C$3),(8*(Main!C31+Main!C11))))` | `=MIN(0.99, Main!$C$3)` — drop the round-trip; `C3` is already the loss rate. |
| `E10` | `IF(C3 > 10^-6, "Loss is high - …")` | Threshold no longer valid for a loss rate; change to a loss-rate threshold (e.g. `IF(C3 > 0.05, …)`) or remove. |
| `C40`/`D40` | `...*twoSigmaNs(D34,D32)` | **Unchanged** — already a function of the loss rate (`C34`/`D34`), not maxBER. |
| `F40` | `"a span …"` | **Unchanged** (does not reference maxBER). |
| *(new output)* | — | `="m maxseglossrate "&TEXT(Main!C3,"0.000000000")` (and a Y→X cell from `D3`). |
| *(new output)* | — | `="m maxretries "&MAX(3, CEILING(LOG($target)/LOG(Main!C34),1))`, with a new yellow input cell `$target` = residual block-failure target (default `1e-6`, matching the engine). |

### VBA (`xl/vbaProject.bin`)

- `ser_ccsds`, `ser2efer`: **unchanged.**
- `ser2ber`: no longer used for the primary output — keep for legacy reference or remove.
- Optional helper (if you prefer VBA to the inline `CEILING`/`LOG`):
  `Function RetriesForLoss(loss As Double, target As Double) As Long` returning
  `WorksheetFunction.Max(3, WorksheetFunction.Ceiling(Log(target)/Log(loss), 1))`.

## Behavioral caveat to surface in the tool and the doc

Under `maxber` the segment loss rate **auto-tracked** the segment size
(`(1−ber)^(segSize·8)`). Under `m maxseglossrate` it is set **absolutely**. The
spreadsheet stays internally consistent (`Link!C8` recomputes when the
Link-sheet segment size `C6` changes), but a *deployed* configuration does not:
if an operator later changes `maxSegmentSize` in the `a span` line without
re-running the tool, the configured `maxseglossrate` becomes stale. Add a note
beside the `maxseglossrate` output cell.

## Optional: split mode for asymmetric links

The *Link* sheet already carries X→Y and Y→X columns. To target ION split mode,
emit `m maxseglossratexmit`/`recv` and `m maxretriesxmit`/`recv` from the `C`/`D`
columns instead of a single symmetric value. The engine already computes
separate xmit/recv loss rates from the xmit/recv segment sizes.
