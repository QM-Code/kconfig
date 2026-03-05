# Demo Compile

Compile/link/load baseline for `KConfigSDK` consumers.

## Build

```bash
# Run from the kconfig repo root
./kbuild.py --build-demos compile
```

## Run

```bash
./demo/compile/build/latest/test
```

## Build Specific Slot

```bash
./kbuild.py --version <slot>
./kbuild.py --version <slot> --build-demos compile
./demo/compile/build/<slot>/test
```

Expected output:

- `KConfig SDK compile/link/load check passed`
