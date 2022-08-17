# DPLL subsystem

The main purpose of DPLL subsystem is to provide general interface to configure devices that use any kind of Digital PLL and could use different sources of signal to synchronize to as well as different types of outputs. The inputs and outputs could be internal components of the device as well as external connections. The main interface is NETLINK_GENERIC based protocol with config and monitoring groups of commands defined.

## Configuration commands group

Configuration commands are used to get information about registered DPLL devices as well as get or set configuration of each used input or output. As DPLL device could not be abstract and reflects real hardware, there is no way to add new DPLL device via netlink from user space and each device should be registered by it's driver.

### List of command with possible attributes
* DPLL_CMD_DEVICE_GET
  ** DPLLA_DEVICE_ID - holds internal device index
  ** DPLLA_DEVICE_NAME - holds DPLL device name
  ** DPLLA_STATUS - holds DPLL device status information
  ** DPLLA_DEVICE_SRC_SELECT_MODE - holds DPLL source selection mode
  ** DPLLA_DEVICE_SRC_SELECT_MODE_SUPPORTED - holds DPLL supported source selection mode
  ** DPLLA_LOCK_STATUS - holds infromation about device's internal frequency-lock status
  ** DPLLA_TEMP - holds DPLL device temperature information
* DPLL_CMD_SET_SOURCE - used to set sources/inputs configuration
  ** DPLLA_DEVICE_ID - holds internal device index for source
  ** DPLLA_SOURCE_ID - holds index of source to configure
  ** DPLLA_SOURCE_NAME - holds name of source to configure
  ** DPLLA_SOURCE_TYPE - holds configuration value for selected source
* DPLL_CMD_SET_OUTPUT - used to set outputs configuration
  ** DPLLA_DEVICE_ID - holds internal device index for output
  ** DPLLA_OUTPUT_ID - holds index of output to configure
  ** DPLLA_OUTPUT_NAME - holds name of output to configure
  ** DPLLA_OUTPUT_TYPE - holds configuration value for selected output
* DPLL_CMD_SET_SRC_SELECT_MODE - used to set source selection mode
  ** DPLLA_DEVICE_ID - holds internal device index
  ** DPLLA_DEVICE_SRC_SELECT_MODE - holds DPLL source selection mode
* DPLL_CMD_SET_SOURCE_PRIO - used to set priority of a source for automatic source selection mode
  ** DPLLA_DEVICE_ID - holds internal device index for source
  ** DPLLA_SOURCE_ID - holds index of source to configure
  ** DPLLA_SOURCE_PRIO - holds priority of a source

The pre-defined enums are used to select type values for sources/inputs and outputs:

### Possible types of sources and outputs
* DPLL_TYPE_EXT_1PPS -  External 1PPS source
* DPLL_TYPE_EXT_10MHZ - External 10MHz source
* DPLL_TYPE_SYNCE_ETH_PORT - SyncE on Ethernet port
* DPLL_TYPE_INT_OSCILLATOR - Internal Oscillator (i.e. Holdover with Atomic Clock as a Source)
* DPLL_TYPE_GNSS - GNSS 1PPS source
* DPLL_TYPE_CUSTOM - Custom frequency

### Values for monitoring attributes STATUS
* DPLL_STATUS_NONE - No information provided
* DPLL_STATUS_CALIBRATING - DPLL device is not locked to the source frequency
* DPLL_STATUS_LOCKED - DPLL device is locked to the source frequency

### Values for monitoring attributes STATUS
* DPLL_LOCK_STATUS_UNLOCKED - DPLL device is not locked to source frequency
* DPLL_LOCK_STATUS_EXT_1PPS - DPLL device is locked to 1PPS source
* DPLL_LOCK_STATUS_EXT_10MHZ - DPLL device is locked to 10MHz source
* DPLL_LOCK_STATUS_SYNCE - DPLL device is locked to SyncE on Ethernet port
* DPLL_LOCK_STATUS_INT_OSCILLATOR - DPLL device is locked to internal oscillator
* DPLL_LOCK_STATUS_GNSS - DPLL device is locked to GNSS source

### Possible DPLL source selection mode values
* DPLL_SRC_SELECT_FORCED - source is forced by DPLL_CMD_SET_SOURCE_TYPE
* DPLL_SRC_SELECT_AUTOMATIC - source are auto selected according to configured priorities and source validity
* DPLL_SRC_SELECT_HOLDOVER - force holdover mode of DPLL
* DPLL_SRC_SELECT_FREERUN - DPLL is driven by supplied system clock without holdover capabilities
* DPLL_SRC_SELECT_NCO - similar to FREERUN, with possibility to numerically control frequency offset

## Notifications

DPLL device can provide notifications regarding status changes of the device, i.e. lock status changes, source/output type changes or alarms. There are 3 multicast groups that are used to notify user-space applications via netlink socket:
* DPLL_EVENT_DEVICE_CREATE - New DPLL device was created
* DPLL_EVENT_DEVICE_DELETE - DPLL device was deleted
* DPLL_EVENT_STATUS_LOCKED - DPLL device has locked to source
* DPLL_EVENT_STATUS_UNLOCKED - DPLL device is in freerun/calibration mode
* DPLL_EVENT_SOURCE_CHANGE - DPLL device source changed
* DPLL_EVENT_OUTPUT_CHANGE - DPLL device output changed
* DPLL_EVENT_SOURCE_PRIO - DPLL device source priority changed
* DPLL_EVENT_SELECT_MODE - DPLL device source selection mode changed

## Device driver implementation

For device to operate as DPLL subsystem device, it's should implement set of operations and register device via `dpll_device_alloc/dpll_device_register` providing desired device name and set of supported operations as well as the amount of sources/inputs and outputs. If there is no specific name supplied, dpll subsystem will use `dpll%d` template will be used to create device name within DPLL subsystem. Notifications of adding or removing DPLL devices are created within subsystem itself, but notifications about configurations changes or alarm should be implemented within driver as different ways of confirmation could be used. Alarms notifications should be implement in driver also. All the interfaces for notification messages could be found in `dpll.h`, constats and enums are placed in `uapi/linux/dpll.h` to be consistent with user-space.

There is no strict requeriment to implement all the operations for each device, every operation handler is checked for existance and ENOTSUPP is returned in case of absence of specific handler.
