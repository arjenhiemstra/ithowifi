## Version 3.3.0-beta1

First beta of the 3.3 line. Adds a periodic RF poll + keep-alive feature for setups that talk to the Itho through an RFT CO2 send remote.

### Features

- **Periodic RF CO2 status request + fan-demand / CO2-level keep-alive sends.** When RF CO2 control is on (`itho_control_interface == 1`) and a joined RFT CO2 send remote is selected on the system settings page, the add-on can actively:
  - request **31DA + 31D9** status from the Itho on the existing **Itho status update frequency** interval — independent on/off setting, separate from the fallback auto-poll standalone mode does on its own,
  - send a periodic **31E0 fan-demand keep-alive** using the last actually-sent demand value, falling back to a user-configured default when nothing has been sent yet this boot,
  - send a periodic **1298 CO2-level keep-alive** with the same last-sent / configured-default fallback.

  All three are independent on/off settings under the existing **RF CO2 control** section. Keep-alives run on their own configurable interval (default 5 min) so 31E0 / 1298 don't share the much shorter status-poll cadence. The dropdown is populated from the active remotes list filtered to non-empty SEND / RFTCO2 slots; if no eligible remote is configured the rest of the section stays inert.

  In RF standalone mode the new poll takes over from the existing `findFirstBidirectionalSendRemote`-based auto-poll when RF CO2 mode is active — so turning the status-request setting off actually stops the polls, instead of leaving the standalone auto-poll running underneath.
