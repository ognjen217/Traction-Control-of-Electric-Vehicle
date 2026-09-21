# Traction Control of an Electric Vehicle

Master's thesis project at the Faculty of Technical Sciences, University of Novi Sad.
Comparison of PID and fractional-order PID (FOPID) traction control for two PMSM
drives, with TI C2000 firmware, Typhoon HIL experiments, and a Python white-box
model with particle swarm optimization (PSO).

The repository contains source code, model/panel configuration, notebooks,
compact numerical results, and only the standalone plots used in the thesis.
Recordings, thesis/paper documents, literature, development reports and build
products are deliberately excluded.

## Layout

| Path | Contents |
| --- | --- |
| `kontroler/` | TI CCS project, controller, hardware interface, device support and C tests |
| `kontroler/reference/` | Earlier laboratory passthrough implementation |
| `SMSM_bigP_2inv_controlled.tse` | Current Typhoon HIL schematic |
| `SMSM_panel*.cus` | Manual and automated HIL panels, including earlier versions |
| `SMSM_settings_controller.runx` | HIL runtime settings |
| `model sistema/` | Physical parameter JSON files and numerical tools |
| `model sistema/PSO_FOPID/` | GL FOPID simulator, PSO notebook, tests and result summaries |
| `model sistema/PSO_PID/` | PID simulator, PSO notebook, tests and result summaries |
| `model sistema/Whitebox_validacija_H5/` | Model-to-HIL validation code and metrics |
| `finalni_testovi/analiza_pid_fopid/` | Final comparison notebooks, analysis utilities and metrics |
| `numerical_simulations/` | Earlier HIL tuning analysis notebook |
| `reference/baseline_hil/` | Original reference HIL schematic, panel and runtime settings |
| `figures/thesis/` | Nine plots matched by exact hashes to images embedded in the thesis |

Directory names and relative paths are preserved because notebooks use them to
locate firmware headers and physical parameters.

## Python setup

Use Python 3.12 (the local development environment). From the repository root:

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
python -m jupyterlab
```

On Linux/macOS, activate with `source .venv/bin/activate` instead.
Saved notebook outputs are cleared to keep generated plots and local logs out of Git.

### PSO simulations

Open and run either notebook in order:

- `model sistema/PSO_PID/PSO_PID_whitebox.ipynb`
- `model sistema/PSO_FOPID/PSO_FOPID_whitebox_GL.ipynb`

These do not need HIL recordings. Physical parameters come from
`model sistema/Parametri_whitebox_TC_v1.json`; controller constants come from
`kontroler/app/controller_config.h` and, for PID, `traction_controller.h`.
Full optimization and fine-step validation can take substantial time; start with
the notebook's quick profile.

Timestamped `rezultati/` folders contain historical result summaries, which may
predate changes to the current firmware/model. They do not demonstrate that the
current implementation has been reoptimized. Saved JSON files retain run
configuration and provenance. Dense response CSVs and NumPy traces are excluded.

The notebook builder scripts regenerate notebooks and overwrite outputs. Their
optional local model-description Markdown file is not required for generation.

### Analyses requiring local recordings

Restore your own recordings, with original filenames, to:

- `finalni_testovi/<scenario>_<PID|FOPID>.h5` for the final comparison;
- `model sistema/simulation_1.h5` for white-box validation;
- `numerical_simulations/*.h5` for earlier HIL tuning analysis.

Final scenarios are `AW01_SAT_RELEASE`, `NOISE_0p005`, `NOISE_0p01`,
`RAPID_SWITCH_HIGH_TORQUE`, `STEP_RESPONSE_STEADY_STATE` and
`TE01_TRACTION_EFFICIENCY`, each with PID and FOPID recordings.
Companion `.svc` files may be placed beside them for HIL tools; both formats are
ignored by Git. Git LFS is not required.

Run final-analysis notebooks in order: `00_provera_podataka.ipynb`,
`01_PID_FOPID_analiza.ipynb`, then `02_zbirni_rezultati.ipynb`.
Without recordings, inspect committed CSV summaries instead of running
data-dependent cells.

## Embedded controller and HIL

Import `kontroler/` as an existing Code Composer Studio project for TMS320F28377D
CPU1. Its metadata references TI C2000 compiler 22.6, C2000Ware 26.1.0.00 and
SysConfig 1.28.0. Install these tools and let CCS resolve
`COM_TI_C2000WARE_INSTALL_DIR`. The linked `driverlib.lib` uses the SDK variable;
compiled libraries and `CPU1_RAM`/`CPU1_FLASH` output are not versioned.

Open `SMSM_bigP_2inv_controlled.tse` in Typhoon HIL, compile for the available
target, and load the appropriate `.cus` panel and `.runx` settings.
`* Target files` directories are generated locally. `reference/baseline_hil/`
contains the historical baseline, separate from the current root-level model.
Hardware execution requires the corresponding TI/Typhoon tools and target setup.

## Checks

With the Python environment activated, run from the root:

```powershell
$env:PYTEST_DISABLE_PLUGIN_AUTOLOAD = '1'
python -m pytest -q "model sistema/PSO_FOPID/test_whitebox_pso.py" "model sistema/PSO_PID/test_whitebox_pid.py" "model sistema/PSO_PID/test_convergence_reporting.py"
```

Disabling automatic pytest plugin loading avoids unrelated Typhoon plugins.
Packaging check (2026-09-21): 54 tests passed; the PID and FOPID
`test_controller_aligned_scenario_numerical_regression` tests failed with the same
values in both the original workspace and this clone. These pre-existing
reference-value mismatches are retained for investigation, not silently updated.
No full PSO optimization or target-hardware build was performed during packaging.

Host C checks can be built with GCC without target hardware:

```sh
mkdir -p build
gcc -std=c99 -Ikontroler/app kontroler/tests/test_controller.c kontroler/app/controller.c kontroler/app/traction_controller.c kontroler/app/safety_allocator.c -lm -o build/test_controller
./build/test_controller
gcc -std=c99 -Ikontroler kontroler/tests/test_signal_scaling.c kontroler/hardware/signal_scaling.c -lm -o build/test_signal_scaling
./build/test_signal_scaling
```

Before committing, clear notebook outputs after local execution and inspect
`git diff --cached --stat`. `.gitignore` excludes recordings, documents, temporary
environments, build products and images other than the nine thesis plots.
The original [Apache 2.0 license](LICENSE) is retained. Bundled TI sources retain
the notices and license terms in their headers.
