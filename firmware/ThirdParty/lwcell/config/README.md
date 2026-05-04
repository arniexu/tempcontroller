LwCELL Project Configuration

This folder stores project-local configuration files for LwCELL.

Current status:
- LwCELL upstream source is vendored under firmware/ThirdParty/lwcell.
- This project provides lwcell_opts.h in this folder.
- PlatformIO include path is configured to find this options file.
- LwCELL source files are intentionally NOT added to build_src_filter yet.

Next integration steps (when needed):
1. Add required LwCELL core/system source files to platformio.ini.
2. Provide OS/system port implementation matching project runtime model.
3. Provide low-level UART/LL adaptation for ML307R transport.
