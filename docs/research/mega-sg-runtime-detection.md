# Mega Sg identification from a Genesis ROM

Investigated 2026-10-19. This is research, not a tested detector or a change to
Hardware Lab. No ROM was uploaded for these checks.

## Conclusion

I found historical ROM-side **behavioral fingerprinting**, but no documented,
stable Analogue identity register or cartridge-side identification API in the
official Mega Sg manual and firmware notes examined. This is an evidence limit,
not proof that an undocumented mechanism cannot exist.

Console identity and cartridge identity are independent: a Mega EverDrive can
run in an original Sega console or a Mega Sg. Detecting the cartridge does not
identify the console.

## Historical implementation

Novedicus's 2019 `md-platform-detection` assembly is preserved in a
[versioned source snapshot](https://gist.github.com/wyatt8740/da6484b0cb4ffd7f2d775efb619a8453/cf5189363dbc5965fea0666c99852b73b10c2cd5).
The original GitHub repository returned 404 during this investigation; this is
a backup of the original technical artifact, not an Analogue specification.

Its Mega Sg classification combines two observations: reading VDP debug port
`$C0001C` produces zero, and a DMA timing test observes that the Z80 halts during
the transfer. Several other platforms also return zero from that port, so the
port value alone is insufficient. These branches describe behavior observed
against the implementation landscape in 2019; the source does not identify a
tested Mega Sg firmware revision.

This is not a harmless query to insert into a running SGDK game. The complete
detector explicitly warns that it destroys VRAM and overwrites Z80 RAM, disables
interrupts, changes VDP state, installs Z80 code, and includes deliberate odd
address accesses that can cause address errors. It would require isolated
startup diagnostics and complete initialization afterward, plus revision and
platform testing. Do not treat a failure to recognize a known fingerprint as
proof of original Sega hardware.

## Official documentation and firmware caveats

The [official manual download](https://assets.analogue.co/pdf/cb0e551939f5a31cbe617382f3178d47/Analogue+Mega+Sg+User+Manual+1.3.pdf)
(URL labelled 1.3; internal pages labelled V1.2, February 2020) documents selectable
region behavior and automatic region detection. Region bits therefore describe
the exposed compatibility configuration, not a unique manufacturer identity.

[Official firmware 4.8](https://www.analogue.co/support/mega-sg/firmware/4.8)
adds an HDMI SPD packet identifying the output as `Mega sg`. This is an HDMI-side
identifier, not a documented 68000-accessible register. A video receiver that
exposes SPD could use it as additional host-side evidence; the existing HD60 X
frame capture has not been shown to expose that metadata.

[Official firmware 4.9](https://www.analogue.co/support/mega-sg/firmware/4.9)
(2023-05-04) records bus timing and RESET-output corrections, illustrating why
behavioral fingerprints must be versioned and revalidated after updates. The
official support page still listed 4.9 when checked; this investigation does not
validate the historical detector against it.

## Suitable integration policy

Record a user-configured console model and firmware as **declared** metadata.
Keep any future fingerprint result separate, with `unknown` and a heuristic
confidence label. Prefer capability probes for an actual feature requirement.
Use a dedicated diagnostic ROM for invasive accuracy tests rather than placing
the historical detector in every game's startup path.
