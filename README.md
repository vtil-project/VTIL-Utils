# VTIL-Utils

VTIL command line utility.

## Building

```sh
mkdir build && cd build
cmake ..
```

## Usage

Use `vtil --help` to display the documentation. Use `vtil <command> --help` to display the documentation for a specific command.

```
  vtil COMMAND {OPTIONS}

    VTIL command line utility

  OPTIONS:

      Commands
        dump                              Dump a .vtil file
        lift                              Lift a .vtil file
        opt                               Optimize a .vtil file
      Arguments
        -h, --help                        Display this help menu
```

### Examples

You can find the files used for these examples in the `examples/` folder.

Dumping a VTIL file:

```
vtil dump test.vtil
```

Optimizing a VTIL file with the default optimization passes:

```
vtil opt hello_world.vtil hello_world.opt.vtil
```

Lifting a single function from an executable:

```
vtil lift hello.exe __security_init_cookie.vtil 140001694
```