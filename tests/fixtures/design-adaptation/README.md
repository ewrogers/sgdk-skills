# Frozen forward-test case

`brief.md` and `rubric.md` were written before the independent run. The agent
received the brief and the instructions in `prompt.md` (line-wrapped here), but
not the rubric or parent conversation. `inputs.json` records the input commit,
entry-point hashes, installer command and start time. It is historical evidence,
not configuration that CI must update whenever a skill changes.

`submission/` retains the submitted game source, test, header, notes, licenses and
small packaging tools unchanged. Generated ROMs/logs, editor configuration and
the SDK's stock boot assembly are omitted. The host/Docker test reconstructs a
fresh project using the installed project helper, which supplies that assembly.
The submitted README's standalone build commands apply to a reconstructed project.

Run the portable regression from the repository root:

```sh
python -m unittest discover -s tests -p test_design_adaptation.py -v
```

For Docker/target checks, install `tests/requirements-math.txt` in a local virtual
environment and set `SGDK_SKILLS_INTEGRATION=1`, with `SGDK_PLATFORM=linux/arm64`
or `linux/amd64` in your shell. ROMs and logs go under `build/`.

The submitted C test and the separate reviewer-owned `review_checks.h` exercise
observable behavior. The reviewer target includes the unmodified submitted
`main.c`, tests its real SDK button mapping and deliberately breaks C's mapping
in a temporary copy to verify rejection. It does not run the renderer under
Unicorn. `provenance.json` records exact-source and longest-token comparisons;
it is a review aid, not a detector of semantic design copying.

The [assessment](../../../docs/validation-design-adaptation.md) records the manual
design review and evidence limits. Passing these regression tests does not
automatically re-evaluate a new agent's originality. Do not silently edit the
frozen submission to improve the historical result; label corrections separately.
