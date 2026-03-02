# Demo Compile

Compile/link/load baseline for `KConfigSDK` consumers.

## Build

```bash
cd m-kconfig/demo/compile
./abuild.py -a <agent> -d build/<slot>/ \
  --kconfig-sdk ../../build/<slot>/sdk/ \
  --ktrace-sdk ../../../m-ktrace/build/<slot>/sdk/
```

## Run

```bash
./build/<slot>/test
```

## Full Test Script

From `m-kconfig` root:

```bash
./tests/full-test.sh --version <slot> --agent <agent>
```

Expected output:

- `KConfig SDK compile/link/load check passed`
