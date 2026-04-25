# Widget Tutorials

This folder is split by topic so each tutorial can stay small and focused.

## Build

```bash
cmake -S . -B build -G Ninja -DBUGUI_BUILD_TUTORIALS=ON
cmake --build build -j4
```

## Run

```bash
./bin/tutorials/widgets/bugui_tutorial_widgets_01_basic
./bin/tutorials/widgets/bugui_tutorial_widgets_02_inputs
./bin/tutorials/widgets/bugui_tutorial_widgets_03_layouts
./bin/tutorials/widgets/bugui_tutorial_widgets_04_scroll_list
./bin/tutorials/widgets/bugui_tutorial_widgets_05_tree_datagrid
./bin/tutorials/widgets/bugui_tutorial_widgets_06_menus_dialogs
./bin/tutorials/widgets/bugui_tutorial_widgets_07_tabs_dock
./bin/tutorials/widgets/bugui_tutorial_widgets_08_node_curve_timeline
./bin/tutorials/widgets/bugui_tutorial_widgets_09_audio_widgets
./bin/tutorials/widgets/bugui_tutorial_widgets_10_serialization
./bin/tutorials/widgets/bugui_tutorial_widgets_11_gizmo2d_property_panel
./bin/tutorials/widgets/bugui_tutorial_widgets_12_gizmo3d_property_panel
```

## Tutorial Map

- `01-basic` - Label, Button, CheckBox, ProgressBar
- `02-inputs` - TextInput, Slider, Switch, ComboBox
- `03-layouts` - BoxLayout, GridLayout, FormLayout
- `04-scroll-list` - ScrollView, ListBox, ListWidget
- `05-tree-datagrid` - TreeView and DataGrid selection/filtering
- `06-menus-dialogs` - MenuBar, ContextMenu, Dialog and InputBox
- `07-tabs-dock` - TabLayout and DockPanel workspace
- `08-node-curve-timeline` - NodeEditor, CurveEditor, Timeline
- `09-audio-widgets` - ADSR, VUMeter, Waveform, Spectrum, PianoRoll
- `10-serialization` - WidgetSerializer save/load roundtrip
- `11-gizmo2d-property-panel` - 2D gizmo editing with PropertyGrid sync
- `12-gizmo3d-property-panel` - 3D gizmo + camera/property workflow

The `app` demo remains the main smoke test for integration/regressions.
