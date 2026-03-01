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
./build/<slot>/client
./build/<slot>/server
```

## Full Test Script

From `m-kconfig` root:

```bash
./demo/compile/full-test.sh --version <slot> --agent <agent>
```

Expected output from each executable:

- `KConfig SDK compile/link/load check passed`
