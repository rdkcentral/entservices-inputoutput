# HDMI-CEC Testcase RAFT Coverage Report (Updated After Rename)

Date: 2026-06-09
Prepared for: Manager review
Scope:
- Old suite: OLD_TESTCASE_RDKE/rdkservices/Tests/L2HALMockTests/TestCases/HdmiCecSource
- New suite: MW_TESTCASE/entservices-inputoutput/Tests/vDeviceTests

## 1) RAFT definition used
RAFT means testcases executed through:
- `python3 suiteManager.py hdmicecsource`
- Test list from `SUITES["hdmicecsource"]["tests"]` in `suiteManager.py`

## 2) Current inventory (after rename)
- Old baseline testcase files: 32 (TCID001 to TCID032)
- New testcase files present: 23 (TCID001 to TCID023)

## 3) RAFT configuration status (as-is)
Configured in `suiteManager.py` for hdmicecsource:
- TCID001, TCID002, TCID003, TCID004, TCID005, TCID006, TCID007, TCID008, TCID009, TCID010, TCID011, TCID012, TCID013, TCID014, TCID015, TCID016, TCID022, TCID023, TCID024, TCID021, TCID018, TCID020, TCID019

Important mismatch found:
- `TCID024` is configured in suiteManager but file `TCID024.py` is not present.
- `TCID017.py` is present in new folder but not configured in suiteManager.

## 4) Coverage summary based on current state
- Old baseline cases: 32
- New files present: 23
- RAFT configured entries: 23
- RAFT effectively runnable entries (configured and file exists): 22

High-level gap view:
- Missing as files in new suite vs old baseline: 9
  - TCID024, TCID025, TCID026, TCID027, TCID028, TCID029, TCID030, TCID031, TCID032
- Present but not RAFT-enabled: 1
  - TCID017
- Configured but file missing (suite break risk): 1
  - TCID024
- Old cases not effectively RAFT-covered today: 10
  - TCID017, TCID024, TCID025, TCID026, TCID027, TCID028, TCID029, TCID030, TCID031, TCID032

## 5) Immediate fixes required
1. Fix suite mismatch first:
   - Remove `TCID024` from suite list, or restore `TCID024.py`.
2. Include `TCID017` in suite list (file exists but not configured).
3. Re-run RAFT after above correction to avoid import-time suite failure.

## 6) Updated recommendation on server support
Ask server/vComponent support for:
1. Event trigger support equivalent to legacy sendEvents flow (TCID025 parity).
2. Deterministic active-source transition/status support for routing-change validation (TCID027 parity).

No server dependency required for:
- TCID017, TCID028, TCID029, TCID030, TCID031, TCID032
(these are mainly suite wiring and testcase migration/assertion work)

## 7) Evidence used
- Current new testcase files in `vDeviceTests` directory
- Current hdmicecsource list in `suiteManager.py`
- Old baseline files in old HdmiCecSource directory
