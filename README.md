# Loconet starter project
Starter project for Loconet projects, based on arm-samd20-starter.
The target compiler is GCC.

# Starting a project

## Include loconet

To start working with loconet, include its header file:

    #include "loconet/loconet.h"

To be able to respond on loconet messages, include the following header file:

    #include "loconet/loconet_rx.h"

To be able to send messages, include the following header file:

    #include "loconet/loconet_tx.h"

To simplify the sending of messages, a set of default send messages are provided in loconet/loconet_tx_messages.c. To use this module, add:

    #include "loconet/loconet_tx_messages.h"

## Initialize Loconet

### 1. Pins and timers

To send and receive messages, the loconet module needs to know which pins, ports and timers can be used. We pass this information via the macro LOCONET_BUILD, with the parameters how you want use the SERCOM interface.
The LOCONET_BUILD is structured as follows:

    LOCONET_BUILD(sercom, tx_port, tx_pin, rx_port, rx_pin, rx_pad)

with:

 - pmux:    the PMUX channel you'd like to use (e.g. C)
 - sercom:  the SERCOM interface number you'd like to use (e.g. 3)
 - tx_port: the PORT of the TX output (e.g. A)
 - tx_pin:  the PIN of the TX output (e.g. 14)
 - rx_port: the PORT of the RX input (e.g. A)
 - rx_pin:  the PIN of the RX input (e.g. 15)
 - rx_pad:  the PAD to use for RX input on the SERCOM interface (e.g. 1, see datasheet)
 - fl_port: the PORT of the FLANK detection (e.g. A)
 - fl_pin:  the PIN of the FLANK detection (e.g. 13)
 - fl_int:  the external interrupt associated to fl_pin (e.g. 1, see datasheet)
 - fl_tmr:  the TIMER used for Carrier and Break detection
 - tx_led_port: the PORT of the TX LED
 - tx_led_pin:  the PIN of the TX LED
 - rx_led_port: the PORT of the RX LED
 - rx_led_pin:  the PIN of the RX LED

For example, using SERCOM0 on PMUX D, to write using pin A04, read on pin A05, and to use pin A06 for flank detection, together with timer 0, we write:

    LOCONET_BUILD(
      D,          /* pmux */
      0,          /* sercom */
      A, 4,       /* tx: port, pin */
      A, 5, 1,    /* rx: port, pin, pad */
      A, 6, 6, 0  /* flank: port, pin, interrupt, timer */
      A, 27,      /* tx led */
      A, 28       /* rx led */
    );


### 2. Add required functions

For flank detection, we require an IRQ handler for EIC. As there is only one in the SAMD20, we do not want to claim it exclusively for loconet. Therefore, add the following function:

    void irq_handler_eic(void) {
      if (loconet_handle_eic()) {
        return;
      }
    }


### 3. Main function

In the main function of the project, ensure that you initialize loconet via `loconet_init()`. To be able to send and receive messages, use `loconet_loop()`;

    int main(void) {
      ...
      // Initialize loconet
      loconet_init();
      ...
      // Set the loconet address
      loconet_config.bit.ADDRESS = loconet_cv_get(0);
      // Set the priority of this device
      loconet_config.bit.PRIORITY = loconet_cv_get(2);
      ...
      // Initialize CVs for loconet
      loconet_cv_init();
      ...
      while(1)
      {
        loconet_loop();
        ...
      }
      return 0;
    }

# Loconet Configuration Values (LNCV)

Programming LNCVs using an Uhlenbrock Intellibox II is supported out of the box.

## Address and priority

It is important to realize that the loconet address (`loconet_config.bit.ADDRESS`) and LNCV 0 are not automatically coupled. The same goes for the loconet priority (`loconet_config.bit.PRIORITY`) which can be found in LNCV 2. This coupling needs to be done in the `main()` and before `loconet_init()` is called. This ensures proper listening to messages and proper wait time for sending.

## Known LNCVs

| LNCV | Name                                 | Possible values |
|------|--------------------------------------|-----------------|
|    0 | Address of the module                | 1 - 65535       |
|    1 | **Reserved**                         |                 |
|    2 | Priority                             | 1 -    10       |

## Read LNCV

To read a LNCV, call the function:

    loconet_cv_get(LNCVnumber);

 Each time the function is called, it will get the actual value from the Eeprom. There is no caching. If values need to be obtained frequently, manually cache them in the memory.

## Write a CV value

To write a LNCV, call the function:

    loconet_cv_set(LNCVnumber, value);

Internally, this function will first validate (via the function loconet_cv_write_allowed) whether writing is allowed. If so, the value is stored in the Eeprom.

## Validating a LNCV before writing

As LNCVs may have different restrictions on the values one can write to them, you can implement the 'loconet_cv_write_allowed' function that is called before a LNCV is being written. Depending on the return code, the LNCV will be written, or an error is sent to the programming station.

    uint8_t loconet_cv_write_allowed(uint16_t lncv_number, uint16_t value);
    uint8_t loconet_cv_write_allowed(uint16_t lncv_number, uint16_t value) {
      (void)lncv_number;
      (void)value;
      return LOCONET_CV_ACK_OK;
    }

The return value should be either one of:

| Code                                 | Reason                                |
|--------------------------------------|---------------------------------------|
| `LOCONET_CV_ACK_ERROR_GENERIC`       | Generic error code                    |
| `LOCONET_CV_ACK_ERROR_OUTOFRANGE`    | Value out of range                    |
| `LOCONET_CV_ACK_ERROR_READONLY`      | LNCV is read only                     |
| `LOCONET_CV_ACK_ERROR_INVALID_VALUE` | Unsupported/non-existing LNCV         |
| `LOCONET_CV_ACK_OK`                  | Everything is ok, LNCV can be written |

## Responding after a LNCV changed

After a LNCV has been successfully written to the Eeprom, the system fires the loconet_cv_written_event, so that the program can update cached cv values. This should be implemented as follows:

    void loconet_cv_written_event(uint16_t lncv_number, uint16_t value);
    void loconet_cv_written_event(uint16_t lncv_number, uint16_t value) {
      ...
    }

## Responding after a programming session

After a programming session to set some LNCVs, the system automatically calls the loconet_cv_prog_off_event, which can be overriden by the program via:

    void loconet_cv_prog_off_event(void);
    void loconet_cv_prog_off_event(void){
      ...
    }


# Responding on received messages

Each message has its opcode. Each opcode has its own function that can be implemented to respond on that message. For example, to respond on the B0 opcode (opc_sw_req), implement the function with 2 arguments (sw1, sw2) e.g.:

    void loconet_rx_sw_req(uint8_t sw1, uint8_t sw2) {
      ...
    }

# Sending loconet messages

Loconet messages can be sent using the 'loconet_tx_queue_X' functions (with X = 2, 4, 6 or n). Messages are added in a fair priority queue. The queue is fair in the sense that a message always will be sent (eventually).

For example, to send a sensor input message:

    loconet_tx_queue_4(0xB2, 5, byte1, byte2);

As the address and state need to be encoded in `byte1` and `byte2`, `loconet_tx_messages.h` has some helper functions. Calling:

    loconet_tx_input_rep(address, true);

encodes the address and state in the two bytes, and sends the message.


# Eeprom usage
Example code to use the eeprom emulator with 4 rows. One row is used for master row, one is used as
spare row and the other two can be used for pages.

    static void hard_reset(void)
    {
      __DSB();
      asm volatile ("cpsid i");
      WDT->CONFIG.reg = 0;
      WDT->CTRL.reg |= WDT_CTRL_ENABLE;
      while(1);
    }

    static void eeprom_init(void)
    {
      enum status_code error_code = eeprom_emulator_init();

      // Fusebits for memory are not set, or too low.
      // We need at least 3 pages, so set to 1024
      if (error_code == STATUS_ERR_NO_MEMORY) {
        struct nvm_fusebits fusebits;
        nvm_get_fuses(&fusebits);
        fusebits.eeprom_size = NVM_EEPROM_EMULATOR_SIZE_1024;
        nvm_set_fuses(&fusebits);
        hard_reset();
      } else if (error_code != STATUS_OK) {
        // Erase eeprom, assume unformated or corrupt
        eeprom_emulator_erase_memory();
        hard_reset();
      }
    }

Then you can call `eeprom_init();` in your `main` to initialize the eeprom. The allowed values for
`eeprom_size` can be found in `utils/nvm.h`.

# Linting

If you want to lint the project make sure you have [OCLint](http://oclint.org/) installed and available in your `PATH` environment variable.
