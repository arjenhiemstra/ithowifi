# Version 3.1.4-beta2

Hotfix on top of [3.1.4-beta1](https://github.com/arjenhiemstra/ithowifi/releases/tag/Version-3.1.4-beta1). All beta1 fixes still apply — see those release notes for the full list.

## Bug fix

- **RF CO2 mode: demand slider on the index page now works in both directions.** When dragging the index-page slider down (to a lower demand), the unit previously kept running at the higher previous value. Same bug affected the REST `percentage` / `fandemand` endpoints (used by the Home Assistant integration) and the MQTT command path.
  - Root cause: an "auto" RF command was sent immediately before the 31E0 demand frame; the RFT CO2 unit then treated the demand as an upward-only boost relative to its current state and ignored values lower than the previous one.
  - Fix: send only the 31E0 demand command, matching the RF devices page (which never had this bug).

If you installed beta1 in RF CO2 mode and noticed "fan stuck at higher speed", this is the fix.
