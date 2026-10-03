# sfrs-plsci-sim

Geant4 + ROOT simulation of a 300 mm (x, length) × 100 mm (y, height) × 1 mm (z, thickness) EJ-200 plastic scintillator centred at the origin. A pencil beam starts at (0,0,-5 mm) and crosses the centre along +z in vacuum. Carbon means fully stripped carbon-12; U means fully stripped uranium-238. Each primary has 1 GeV per nucleon kinetic energy (12 GeV for C, 238 GeV for U).

## Build and run

Run these commands from the project directory. Geant4 is activated by your `.zshrc`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DROOT_DIR="$(root-config --prefix)/share/root/cmake"
cmake --build build --parallel 8
./build/sfrs-plsci-sim --beam C --events 1000 --output output/C_1GeVu.root
./build/sfrs-plsci-sim --beam U --events 1000 --output output/U_1GeVu.root
root -l 'macros/view.C("output/C_1GeVu.root")'
```

Use `--seed N` for repeatable runs and `--birks VALUE` to change the quenching coefficient in mm/MeV. Output paths are overwritten when reused. The program supports Geant4 worker threads with separate event data for each worker and synchronized native ROOT histogram/tree writes. Without a macro it uses one worker. ROOT rows may appear in worker completion order; use the event ID to identify each event. ROOT and Geant4 do not need to be rebuilt to change event count or beam species.

## Source layout

Each Geant4 class has a declaration in `include/` and an implementation in `src/`:

| File/class | Responsibility |
|---|---|
| `DetectorConstruction` | Slab geometry, EJ-200 material, optical properties |
| `PrimaryGeneratorAction` | Ion gun and position commands |
| `ActionInitialization` | Create actions and event data for each worker |
| `EventAction` | Reset event totals and submit completed events |
| `SteppingAction` | Score deposited energy and optical path length |
| `StackingAction` | Count scintillation photons and select the transported sample |
| `RootOutput` | Own the ROOT file, histograms and tree; synchronize writes |
| `EventData.hh` | Plain per-event values owned by each worker's event action |
| `src/main.cc` | Command-line options, physics setup, UI startup and final output |

Existing commands and macros work with this layout. Rebuild after editing C++ files using `cmake --build build --parallel 8`.

## Batch runs with 10 threads

```sh
./build/sfrs-plsci-sim --beam C --macro macros/run.mac --output output/C_10threads.root
./build/sfrs-plsci-sim --beam U --macro macros/run.mac --output output/U_10threads.root
```

`macros/run.mac` selects 10 workers, initializes the simulation, and runs 10,000 events. Edit its thread count and event count as needed. Thread count must be set before `/run/initialize`. The operating system schedules workers onto the available cores; 10 workers does not guarantee 10 physical cores.

`macros/vis.mac` selects one worker for its ten-event interactive demonstration. It can also use 10 workers by changing its first `/run/numberOfThreads` command, but threading overhead usually outweighs the benefit for such short runs. User macros must now include `/run/initialize` before visualization or beam commands.

## Open the Geant4 GUI

Open Terminal and run:

```sh
./build/sfrs-plsci-sim --beam C --ui --macro macros/vis.mac --output output/interactive.root
```

This opens the Qt window showing the scintillator and ten carbon events at two positions. Replace `--beam C` with `--beam U` for uranium. The slab is only 1 mm thick; rotate the view to see its broad face.

In the GUI's command field, run more events with:

```text
/run/beamOn 10
```

A macro supplied with `--macro` controls the run, so `--events` is ignored in that mode. Exit the UI to write the ROOT file.

## ROOT objects

Gun position and direction can be changed in either macro after `/run/initialize` and before `/run/beamOn`, or in the GUI command field:

```text
/gun/position 50 0 -5 mm
/gun/direction 0 0 1
```

This moves the beam 5 cm along x while keeping it upstream of the slab. The slab spans x = ±150 mm, y = ±50 mm, z = ±0.5 mm. The executable still sets species and 1 GeV/u energy from `--beam`; macro position/direction persist across events.

- `hEdep`: energy deposited in the slab per incident ion, MeV, including all secondaries.
- `hScintPhotons`: number of scintillation photons generated in the slab per event.
- `hPhotonsVsEdep`: correlation between deposited energy and generated photons.
- `events`: event ID, `edep_MeV`, `primary_edep_MeV`, `scint_photons`, and `primary_exit_MeV`. Exit energy is the primary's kinetic energy at the slab boundary; -1 means no recorded exit. It is not a transmission flag, and may include backscattering.
- `configuration`: species, energy, seed, dimensions, physics, and Birks coefficient.

Histogram ranges include overflow bins; the event tree retains values beyond the plotted ranges. Inspect overflow for high-statistics runs and rare nuclear reactions.

## Material and physics assumptions

EJ-200 bulk density 1.023 g/cm³, H:C atomic ratio 1.104, nominal electron scintillation yield 10,000 photons/MeV, 2.1 ns decay time, and 425 nm emission peak follow Eljen's documentation:
https://eljentechnology.com/products/plastic-scintillators/ej-200-ej-204-ej-208-ej-212
https://eljentechnology.com/images/technical_library/Physical-Constants-Plastic.pdf

Composition approximates the doped polyvinyltoluene bulk by carbon/hydrogen mass fractions. The emission spectrum is an approximate shape around 425 nm, not a digitized manufacturer spectrum. By default scintillation photons are counted at creation and killed before transport. The optional photon macro transports a limited illustration sample; `sipm.mac` transports all generated photons for ideal sensor collection. Cherenkov production is disabled. `hScintPhotons` remains the generated yield; separate SiPM counters record ideal absorbed photons, not a calibrated photoelectron response.

**The default Birks coefficient 0.126 mm/MeV is a provisional modelling parameter, not a manufacturer-certified EJ-200 value or a calibration for carbon/uranium.** Heavy-ion quenching strongly affects photon yield. Vary it with `--birks`; `--birks 0` gives the unquenched baseline. Quantitative absolute light yield requires calibration and possibly a more detailed heavy-ion quenching model.

Physics: FTFP_BERT with electromagnetic option 4, Geant4 scintillation including Birks saturation, 10 µm production cuts and 10 µm maximum charged-particle step inside the slab. Nuclear interactions and fragments are included. Step-size/cut convergence and comparison with measured heavy-ion stopping/fragmentation/light-yield data remain necessary for research predictions. Ideal SiPM arrays are attached directly to both y edges. There is no wrapping, optical grease layer, electronics response, or beam spot/angular spread.

## Display two beam positions together

`vis.mac` alternates x = +20 mm and x = -20 mm within a single ten-event run:

```text
/gun/position 20 0 -5 mm
/beam/secondPosition -20 0 -5 mm
/beam/alternatePositions true
/run/beamOn 10
```

Even event IDs use `/gun/position`; odd IDs use `/beam/secondPosition`. Set `/beam/alternatePositions false` to return to one position. Both beams share the gun direction. Commands go after initialization. Keeping both positions in one run allows the viewer to redraw both when rotating or refreshing; Geant4 may redraw only the last run when separate `beamOn` runs are accumulated.

## One event with visible scintillation photons

```sh
./build/sfrs-plsci-sim --beam C --ui --macro macros/photons.mac --output output/C_photons.root
```

This runs one incident ion and transports the first 100 scintillation photons produced in the event. Optical photons are yellow, electrons red, gamma rays green, and other particles blue. Edit `/optics/maxPhotons 100` to change the limit. This is a visualization sample, not an unbiased detector-efficiency sample. The full generated yield remains in `hScintPhotons`. Optical transport is disabled by default in other runs; `/optics/transportPhotons true` enables it after initialization. The event tree additionally stores `transported_photons` and summed `optical_path_mm` for the transported sample.

The optical illustration uses constant refractive index 1.58 and absorption length 380 cm across the emission range, a vacuum exterior with index 1, and default smooth dielectric boundaries. There is no wrapping; ideal SiPM absorbers cover only nine separate 10 mm regions on each y edge. Escaping photons travel through the vacuum world until its boundary. Optical-photon absorption is excluded from deposited-energy scoring. Transport consumes random numbers, so samples with/without transport need not give identical subsequent event histories for the same seed.

## SiPM arrays on the top and bottom edges

Each array consists of ten adjacent 1×1 mm active faces, forming a 10×1 mm row along x. Nine arrays repeat at 3 cm pitch on each edge, centred at x=-12,-9,-6,-3,0,3,6,9,12 cm and z=0. The top active faces are at y=+50 mm and the bottom at y=-50 mm. This gives 18 arrays and 180 units. Keeping an array at x=0 gives nine complete arrays per edge; positions at ±15 cm would overhang the slab. Each unit extends 0.1 mm outward as a silicon volume. The arrays are attached without an air gap. Top units are magenta and bottom units cyan in the GUI.

The optical surfaces are ideal absorbers with zero reflection and 100% efficiency: every optical photon incident on a unit is absorbed and counted once. This is an ideal sensor model, not a real SiPM PDE, gain, saturation, or timing response. Only light reaching the small arrays is collected; the rest can escape or be absorbed in the slab.

For full collection, run all photons from one incident ion:

```sh
./build/sfrs-plsci-sim --beam C --macro macros/sipm.mac --output output/C_sipm_positions.root
root -l 'macros/viewSiPM.C("output/C_sipm_positions.root")'
```

Edit the final `beamOn` command in `sipm.mac` for more events. `/optics/maxPhotons -1` transports all generated scintillation photons. Full uranium photon transport is substantially more expensive. Other batch macros retain counting-only optical mode by default; they do not give sensor collection counts until optical transport is enabled.

For the geometry plus one event with 100 displayed photons:

```sh
./build/sfrs-plsci-sim --beam C --ui --macro macros/sipm-vis.mac --output output/C_sipm_vis.root
```

The limited optical sample is for visualization; its sensor counts are not full collection estimates.

Additional ROOT objects:

- `hSiPMTop`, `hSiPMBottom`: absorbed photons per event summed over all arrays on each edge.
- `hSiPMChannels`: summed absorbed photons by channel.
- Event branches `sipm_top_photons`, `sipm_bottom_photons`, and `sipm_photons[180]`.

Channels 0–89 are top and 90–179 are bottom, ordered by array position and then unit position from negative x to positive x. Each array contains ten adjacent units. Absorption is scored from the Geant4 optical-boundary `Detection` status and excluded from ion deposited-energy scoring.

### Array sums versus position

`viewSiPM.C` plots `hSiPMTopVsPosition`, `hSiPMBottomVsPosition`, and `hSiPMTotalVsPosition`. Each bin represents the centre of one array along x in cm and sums absorbed photons across its ten units and all simulated events. The combined histogram adds top and bottom counts at the same x. These are summed photon counts, not averages per event and not a reconstruction of the incident beam position.

The event tree additionally stores `sipm_array_photons[18]`: indices 0–8 are top arrays and 9–17 bottom arrays, both ordered by increasing x. The original individual-unit branch now has 180 entries. Files generated with the old two-array geometry retain their old schema; regenerate them to obtain these position histograms.

### Photon arrival times

`viewSiPM_onePad.C` opens two canvases: the overlaid array photon counts and a timing canvas with two pads. The top pad shows mean photon arrival time versus array position for top, bottom, and both edges combined. The bottom pad shows first-photon event timing resolution at each position, using the same colors.

```sh
./build/sfrs-plsci-sim --beam C --macro macros/sipm.mac --output output/C_sipm_timing.root
root -l 'macros/viewSiPM_onePad.C("output/C_sipm_timing.root")'
```

Time is the optical photon's global time at absorption, measured in ns from the primary event's time zero. It includes ion flight time, scintillation emission delay, and optical propagation/reflections. It is an ideal photon arrival time, not a SiPM electronics timestamp or the first-photon time. Each graph point averages all absorbed photons at that position across the file; the combined curve is photon-count weighted. Error bars are standard errors of the photon-time sample mean (zero for a single photon), not detector timing resolution or event-to-event jitter. Positions with no absorbed photons are omitted.

The event tree stores `sipm_array_time_sum_ns[18]` and `sipm_array_time_sum_sq_ns2[18]` alongside the array counts, allowing calculation of mean and spread without retaining every photon timestamp. Full optical transport is required for collection studies; a limited display sample gives only sampled timing. Existing ROOT files do not contain these new timing branches and must be regenerated. The macro still draws their count canvas and reports missing timing data.

The second canvas is exported as `<input>.sipm_arrival.png`; the count canvas remains `<input>.sipm_onePad.png`.

The top error bars are `sigma_t / sqrt(N)`, the statistical uncertainty of the pooled photon-time mean. The bottom instead measures an event timestamp resolution: for each array in each event, the timestamp is its earliest absorbed photon; the residual is `t_first - t_beam`. The graph shows the sample standard deviation of these residuals across events, using equal event weights. For the combined curve, the timestamp is the earlier of the first top and first bottom photon at that x, not their average. Missing detections are excluded; each point needs at least two detected events. A single-event file cannot give an event timing resolution.

This is an ideal first-photon timestamp estimator. It includes light production/transport fluctuations but no SiPM single-photon timing jitter, electronics noise, thresholds, or reference-detector resolution. No uncertainty bars are drawn for the resolution estimates. Twenty events demonstrate the calculation; substantially more events are needed for precise results. Keep beam position, beam energy, and geometry fixed within a resolution study unless the intended resolution includes their variations.

```sh
./build/sfrs-plsci-sim --beam C --macro macros/timing.mac --output output/C_sipm_first_photon.root
root -l 'macros/viewSiPM_onePad.C("output/C_sipm_first_photon.root")'
```

`timing.mac` copies the beam settings of `sipm.mac` when created and runs 20 full-transport events. Edit it for your desired beam location and statistics. The new event branches are `primary_time_ns`, `sipm_first_photon_ns` (earliest detection anywhere), and `sipm_array_first_photon_ns[18]`. First-photon values are true minima of global times, independent of Geant4 track processing order; -1 means no detection. Files without these branches must be regenerated for the bottom pad; their mean-arrival top pad still works.

## Scan multiple gun positions

```sh
./build/sfrs-plsci-sim --beam C --macro macros/scan.mac --output output/C_scan.root
```

`scan.mac` sets the `scanEvents` alias to 20 events per position, loads `scan_setup.mac` once, then executes `scan_points.mac`. The latter calls nine reusable files in `macros/positions/`, spanning x=-120,-90,-60,-30,0,30,60,90,120 mm at y=40 mm and z=-5 mm. Position files contain only `/gun/position`; initialization and `beamOn` belong to the parent macros. Edit `scanEvents` in `scan.mac`, the coordinates in the position files, or the call list in `scan_points.mac`. Full optical transport is enabled, and all nine runs accumulate into one ROOT file.

The event tree records `run_id`, `beam_x_mm`, `beam_y_mm`, and `beam_z_mm` from each actual generated primary vertex. Event IDs restart at each run; `(run_id,event)` identifies an event within this file. The run ID starts at 0 and follows the order of the position calls. Coordinates are also recorded correctly when alternating beam positions are enabled elsewhere.

Plot one scan position with the optional run-ID argument:

```sh
root -l 'macros/viewSiPM_onePad.C("output/C_scan.root",4)'
```

Run 4 is x=0 mm in the supplied scan. The photon counts, mean arrival times, and first-photon resolution all use only events from that run. Plot exports include `.run4` in their filename. Without a run ID, the macro pools the file and warns if it contains multiple runs; mixing positions changes the meaning of timing resolution. Use more events per position for precise estimates.
