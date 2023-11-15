# Xbox Bluetooth Tool

A simple diagnostic tool for Nintendo Switch for determining Bluetooth capabilities of Microsoft Xbox controllers for use with [Mission Control](https://github.com/ndeadly/MissionControl).

## Usage

Download and copy the .nro file to the `/switch` directory on your SD card and launch via hblauncher. Simply connect an Xbox controller to the console via USB to print information about the controller revision and its Bluetooth capabilities.

## Controller Compatibility

The following table shows how the program output can be interpreted in terms of compatibility with Mission Control.

|                            | **Yes**                                                                                                                                                                                                                                                                                                         | **No**                                                                                                                                                                                                           |
|----------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Bluetooth Support**      | It is currently possible to use this controller with Mission Control.                                                                                                                                                                                                                                                     | This controller cannot currently be used with Mission Control. This may change in the future with the addition of USB support. For now, check out [sys-con](https://github.com/cathery/sys-con) for USB controllers. |
| **Bluetooth LE Firmware**  | This controller uses a Bluetooth LE firmware. This standard is not currently supported by the official Mission Control releases. However, support is under active development and there are experimental releases available in the pinned messages of the `#testing` channel of my [Discord server](https://discord.gg/gegfNZ5Ucz). | This controller uses a legacy Bluetooth firmware and can be used normally through the Change Grip/Order screen.                                                                                                  |
| **Firmware Downgradeable** | This controller can be [downgraded](https://support.xbox.com/en-US/help/hardware-network/accessories/controller-firmware-reversion) from a Bluetooth LE firmware (≥ 5.0.0.0) to a legacy version that can be used normally through the Change Grip/Order screen.                                                | This controller cannot be downgraded to a legacy firmware. You will need to wait for official Bluetooth LE support to be released, or try one of the experimental builds.                                        |
