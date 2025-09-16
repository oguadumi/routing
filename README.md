## OLSR-lite: Link-State Routing Twin

OLSR-lite is a C++20 desktop simulator that models neighbor discovery and shortest-path routing with weighted, undirected links. It includes a real-time ImGui overlay to visualize the topology, inspect nodes/links, interactively Jam/Unjam links, edit weights, and export routing tables as JSON. A hysteresis module (optional) can stabilize link status and route choices under jitter.

### What the program does
- Computes per-node shortest paths using Dijkstra on a link-state database built from the current topology.
- Visualizes nodes and links on a canvas; links are colored by status (UP green, DOWN red).
- Lets you Jam/Unjam a link (toggle UP/DOWN) and watch routes recompute live.
- Allows editing of link weights; recomputation happens immediately.
- Exports current per-node routing tables, along with nodes and links, to a JSON file.
- (Optional) Applies hysteresis to link weights and status to reduce route flapping.

---

## Build

Prereqs:
- CMake 3.22+
- C++20 compiler (gcc 12+/clang 15+/MSVC v19.3x+)
- Git (for fetching dependencies via CMake FetchContent)

Linux/macOS:
```bash
cd /home/ubuntu/project/routing-twin
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Windows (x64 Native Tools Developer Command Prompt):
```bat
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The binary will be at `build/olsr_lite` (or `build/olsr_lite.exe` on Windows).

---

## Run

### GUI mode (default)
Launch the interactive simulator with the ImGui overlay:
```bash
./build/olsr_lite --topo assets/topologies/sample_small.json
```
If `--topo` is omitted, a tiny default graph is used (2 nodes, 1 link).

### CLI / headless
Compute routes and export JSON without GUI:
```bash
./build/olsr_lite --no-gui --topo assets/topologies/sample_small.json --export build/routes.json
```
If `--export` is omitted in CLI, the routing table for the first node is printed to stdout.

Command-line flags:
- `--topo <file>`: Load topology JSON.
- `--export <file>`: Export routes JSON to file.
- `--no-gui`: Disable GUI (headless CLI).

---

## GUI controls

### Mouse
- Click a node to select it; drag to move it on the canvas.
- Click near a link (line) to select it.

### Keyboard
- `R`: Recompute routes.
- `E`: Export routes to `build/routes_gui.json`.
- `J`: Jam/Unjam the currently selected link.

### Menus and panels
- Main Menu → File:
  - Load topo: type a path and click Load.
  - Export routes: type a path and click Export.
- Main Menu → View: toggle visibility of Topology, Inspector, Routing Table, Actions, Event Log panels.
- Actions panel:
  - Recompute (shows time in ms in Event Log).
  - Export JSON (path field + button).
  - Topology Management: add node, add link, delete selected node/link.
  - Hysteresis: enable/disable and set parameters (alpha, theta_up, theta_down, hold_ms).
- Inspector panel:
  - Node selection: shows id, label, degree, routes count from that node.
  - Link selection: shows endpoints, editable weight, status; when hysteresis is on, also shows filtered weight.
- Routing Table panel:
  - Choose a source node and view [destination, next hop, total cost, hop count].
- Event Log panel: recent events such as recompute timings, jams, exports.

---

## End-to-end example (exercise all features)

Follow these steps to build the project, run the GUI, explore routing, jam/unjam links, edit weights, manage topology, enable hysteresis, and export routes. Finally, repeat export via CLI.

1) Build the project
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

2) Launch the GUI with a sample topology
```bash
./build/olsr_lite --topo assets/topologies/sample_small.json
```
You should see 3 nodes and 3 links in a triangle. Links are green (UP).

3) Inspect routing tables
- Open the “Routing Table” panel.
- Use the “Source” dropdown and browse routes for each node.
- Verify that the “Cost” values match the sum of weights along the chosen path.

4) Select and Jam a link
- Click near the link between two nodes (e.g., (1,2)) to select it (it becomes the current selection).
- In the “Actions” panel, click “Jam Link.”
- Observe: the link turns red (DOWN), the Event Log shows recompute time, and routing tables update to use alternative paths.

5) Unjam the link
- Click “Unjam Link.”
- Observe: link returns to green (UP) and routing recomputes.

6) Edit a link weight
- With the link selected, go to “Inspector.”
- Change “Weight” (e.g., from 1.2 → 3.0) and press Enter.
- Observe: recompute runs; routing tables reflect the changed cost.

7) Use keyboard shortcuts
- Press `R` to recompute; check Event Log for duration.
- Press `E` to export to `build/routes_gui.json` (default GUI path).
- With a link selected, press `J` to Jam/Unjam it.

8) Manage topology (add/delete)
- Open “Actions” → “Topology Management.”
- Click “Add Node” (a new node appears; check Event Log for the new id).
- Enter “Link u”, “Link v” (node ids) and “Link weight,” then click “Add Link.”
- Select a node and click “Delete Selected Node,” or select a link and click “Delete Selected Link.”
- After each change, routes recompute automatically.

9) Enable hysteresis to stabilize routes
- In “Actions,” expand “Hysteresis.”
- Check “Enable Hysteresis.”
- Adjust parameters, for example: `alpha=0.3`, `theta_up=1.6`, `theta_down=1.3`, `hold_ms=1000`.
- Click “Apply Hysteresis Params.”
- With a link selected, note the “Filtered” value in Inspector. Jam/weight edits will now interact with filtered values and status transitions with hold-down.

10) Export routes from the GUI
- In “Actions,” type an export path (e.g., `build/routes.json`) and click “Export JSON.”
- Confirm success in the Event Log.
- Inspect the file:
```bash
head -n 80 build/routes.json
```

11) Export routes via CLI (headless)
```bash
./build/olsr_lite --no-gui --topo assets/topologies/sample_small.json --export build/routes_cli.json
head -n 80 build/routes_cli.json
```

Tips:
- If you move nodes, only positions change (visual); routing depends on link weights/status.
- If export fails, ensure the directory (e.g., `build/`) exists and is writable.

---

## Topology JSON format (input)
Minimal schema:
```json
{
  "nodes": [{ "id": 1, "label": "R1", "x": 200, "y": 100 }],
  "links": [{ "u": 1, "v": 2, "weight": 1.0 }]
}
```
- Links are undirected and share the same weight in both directions.
- Node `id` values in the file are mapped to internal IDs and used in link references.

A sample file is included at `assets/topologies/sample_small.json`.

---

## Routes JSON export (output)
The export includes metadata, nodes, links, and per-source routing tables. Example excerpt:
```json
{
  "meta": { "version": "1.0.0", "timestamp_ms": 1757632800000 },
  "nodes": [{ "id": 1, "label": "R1" }],
  "links": [{ "u": 1, "v": 2, "weight": 1.2, "status": "UP" }],
  "routes": {
    "1": [
      { "destination": 2, "next_hop": 2, "total_cost": 1.2, "hop_count": 1 }
    ]
  }
}
```

Schema highlights (conceptual):
- `meta.version` and `meta.timestamp_ms` (integer ms since epoch).
- `links[].status` is `"UP"` or `"DOWN"` at the time of export.
- `routes` is an object keyed by source node id (as string); each entry is an array of route objects.

---

## Hysteresis (optional)
When enabled (Actions → Hysteresis), the simulator applies an exponential moving average and threshold-based status transitions to each link:
- `alpha`: EMA factor in (0,1]; lower values smooth more.
- `theta_down` and `theta_up`: lower/upper thresholds for transitioning status.
- `hold_ms`: minimum time before allowing another status flip (reduces flapping).

The filtered weight and derived status are used by Dijkstra during that frame. The Inspector displays the filtered value for the selected link when hysteresis is active.

---

## Project layout
```
olsr-lite/
  CMakeLists.txt
  assets/topologies/
    sample_small.json
  src/
    app/Main.cpp            # CLI entry point (+ GUI wiring)
    app/MainGui.cpp         # ImGui/GLFW setup and frame loop
    core/Graph.{h,cpp}      # Nodes, links, invariants
    route/Dijkstra.{h,cpp}  # Weighted single-source shortest paths
    route/Router.{h,cpp}    # All-sources aggregation
    hyst/Hysteresis.{h,cpp} # EMA + thresholds + hold-down (optional)
    io/JsonImporter.{h,cpp} # Topology loader
    io/JsonExporter.{h,cpp} # Routes export
    ui/UiOverlay.{h,cpp}    # ImGui panels and interactions
```

---

## Troubleshooting
- Build fails fetching dependencies: ensure outbound network access and that Git is installed.
- Startup crash on GL context: update GPU drivers; ensure OpenGL 3.3 core profile is available.
- No nodes shown: verify your topology JSON path, or try the default sample provided.
- Export fails: check write permissions and that the target directory exists.

---

## License
MIT (or project default).
