# 1 => Make executable files compatible with AmigaOS. Created ADFs will
#      contain a special bootblock that maximizes amount of chip memory
#      and automatically starts the binary using `startup-sequence`.
# 0 => Executable files must be started from ROM or ADF since they require
#      custom environment created by bootstrap code.
AMIGAOS := 0

# 1 => Use fs-uae dependant features i.e. call-traps (see `include/uae.h`)
#      to aid debugging and profiling. This may render executable files
#      unusable (most likely crash) on real hardware.
# 0 => Disable use of aforementioned features. Required for a release!
UAE := 1

# Pass "VERBOSE=1" at command line to display command being invoked by GNU Make
VERBOSE ?= 0
