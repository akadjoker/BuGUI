# 10 - Serialization

Goal: validate widget tree save/load roundtrip.

Covered systems:
- `WidgetSerializer::saveToFile`
- `WidgetSerializer::loadFromFile`

Run:

```bash
./bin/tutorials/widgets/bugui_tutorial_widgets_10_serialization
```

Manual checks:
1. Click `Save JSON` and confirm file status.
2. Click `Load JSON` and inspect cloned widget tree.
3. Click `Clear Loaded` and verify host panel resets.
