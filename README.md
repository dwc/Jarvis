# ESPHome Jarvis component

This project is a ESPHome component for [fully's Jarvis standing desk](https://www.fully.com/standing-desks/jarvis.html). 

It enables Home Assistant to control the desk and get data out of it. It has all the features from the official handset and more. It's a man-in-the-middle device to control all UART messages between the desk's controller and the handset.

### Backstory
Originally I wanted to be able to control the desk from a Home Assistant automation so I started looking if there are any projects that has done this already. Luckily, I came across [Phil Hord's IoT project](https://github.com/phord/Jarvis), which I strongly recommend to check out. He has done the reverse engineering of the communication and the wiring that I extensively used to create this project. In the beginning I tried to make that work for Home Assistant but during that I realized that I could probably use the screen for other things (temperature, humidity, stock market changes?).

So I started reverse engineer it even further. I discovered, through fuzzing about 2 dozen of message types that were not used by neither the controller nor the handset in the stock configuration and managed to exploit some bugs as well that I found during the journey. With all that knowledge I decided to implement a man-in-the-middle device that captures all traffic and is able to inject fake messages into both of the data streams.

There were 3 things I wanted to have: (1) The ability to turn the screen on anytime for notification (this I think is impossible). (2) Always-on display to show numbers without activating the screen. I really didn't like touching it twice to move it. (3) Custom numbers on the display to have something more useful than the current height, which is almost completely useless for me.


## Features

* All controls that the handset provides
* Capture all and inject fake UART messages
* ESPHome configuration

### Display exploits

The reverse engineering behind the handset display exploits is written up under
[Technical notes](#technical-notes) below, but **none of them are implemented in
this component**.

Earlier revisions carried a `HandsetMode` enum and an always-on display timer
for them. That code was never reachable: the mode was fixed at `Factory` and
nothing ever changed it, so the always-on display, the custom number display,
and the "leds off" exploit had no effect. The exploit path also blocked the main
loop for 22 seconds, which would have stalled the handset bridge and the API
along with it. It has been removed rather than left as dead weight.

Re-implementing any of them means driving the handset link on a timer instead of
purely forwarding it -- worth doing as a real, configurable feature:

* Always-on LCD display
* Custom number display (0-180)
* Always-on leds with no display
* Always off leds and display but active handset. "Dark mode"


## Repository layout

The desk logic lives in `components/jarvis_desk/`, a standard [ESPHome external
component](https://esphome.io/components/external_components.html). `jarvis.yaml`
pulls it in with:

```yaml
external_components:
  - source:
      type: local
      path: components
```

The component is split the way ESPHome expects: Python files declare the
configuration schema and generate the C++ wiring, and the `.cpp`/`.h` files hold
the runtime behaviour.

| File | Purpose |
| --- | --- |
| `__init__.py` | The `jarvis_desk:` hub — validates config and binds the two UART buses |
| `sensor.py`, `text_sensor.py`, `number.py`, `select.py`, `button.py` | One platform per entity domain |
| `jarvis_desk.*` | Protocol orchestration and Home Assistant state publishing |
| `serial_device.*`, `serial_message.*` | Message framing, checksums, and the receive state machine |
| `handset_handler.*` | The handset end of the link; records the last reported height |
| `desk_settings.*` | The desk's setting vocabulary, and the string ⇄ protocol byte tables |

The component never blocks the main loop. Re-reading the desk's settings needs
roughly 70 ms between each of its three requests, so that sequence is a small
state machine driven from `loop()` rather than a busy-wait; the handset bridge
keeps running throughout.

Both serial links use ESPHome's own `uart` component rather than
`SoftwareSerial`, so there is no external library dependency. The handset sits on
hardware UART0 (GPIO1/GPIO3), which is why `logger:` is configured with
`baud_rate: 0` — the logger must not take that port. The control box uses a
software UART on GPIO4/GPIO5.

Earlier versions of this project used ESPHome's `custom_component:` with a list
of `includes:`. That mechanism has since been removed from ESPHome, so the
configuration above replaces it.

## Technical notes

Check out [Phil's repository](https://github.com/phord/Jarvis) for details on the wiring and the communication protocol.

[Google Sheet containing all the notes I took](https://docs.google.com/spreadsheets/d/1GKZfDFljVX4eQBMawq0-Rc8t0x8V6gjQ5BgAYngPYTo/edit?usp=sharing) 

Important findings:
* The "handset control lines" are unnecessary. Everything can be done via UART messages.
* __Display behavior__:
	* When 0x01 (height report) is received by the HS (Handset), normally it gets stored in memory (0x1B) and then displayed.
	* If 0x01's payload is outside the display's range (1-1800) it won't be written to memory, but the content of 0x1B will be shown. (i.e. the last value sent)
	* 0x1B can be set by hand without updating the display. __Can be set to 0__, so the next out-of-range 0x01 will show 0. It cannot be set above 1800.
	* The screen turns off if the received 0x01 is the same as the number in 0x1B for ~9 seconds. This means that if an out-of-range number is being sent periodically (0 or anything above 1800) the screen won't turn off, since 0x1B won't be updated.
	* Some errors can turn it on in off mode without touching, but I found no way to keep it active, it always goes to sleep mode after the error disappears.
	* I found no way of changing brightness or locking the handset since these are self contained.
* __Controlbox__:
	* Kill mode: disable anti-collision. It's not possible through the handset for some weird reason.
	* You can get the current settings with 0x07. Check the sheet for details.
	* No motor frequency change or anything too crazy unfortunately.
	* I managed to crash it a couple of times, rendering it unresponsive, but I cannot reproduce it. A simple power cycle fixes it though.
* (exploitable) __Bugs__:
    * Empty screen, active buttons: On powerup the hanset turns on and starts spamming 0x29 until answered. Goes to sleep in about 10 seconds after showing "fully" logo. Next time it's pressed it wakes up the controller with signallines. However, if no answer is received then the buttons stay lit and active and the screen is blank.
    * Dark mode: Sending 0x23 to the sleeping handset will put it in an errornous state if 1 or 2 send in P0. The lights will not turn on until reboot. Everything else behaves the same way. No way found to reset it without power cycle.
(Same applies to error satetes 0x01 - 0x0D and 0x10)

## Installation

### Requirements

* [Home Assistant](https://www.home-assistant.io/) or [ESPHome](https://esphome.io/) 2025.7.0 or newer
* Wemos D1 Mini (or another ESP8266 board)
* A bit of soldering

### Notes

I followed a few related guides to get this all working:

* [Mahko_Mahko's Home Assistant forum post about Desky (and other Jiecang controller-based) desks](https://community.home-assistant.io/t/desky-standing-desk-esphome-works-with-desky-uplift-jiecang-assmann-others/383790/18)
   * This includes a good overview of the hardware I ended up with to wire everything together, along with a 3D printed enclosure. It also has some good information on the RJ45 passthrough and signal cables.
* [phord's Fully Jarvis IoT interface project](https://github.com/phord/Jarvis)
   * This includes a [more detailed breakdown of the pinouts](https://github.com/phord/Jarvis#physical-interface-rj-45) which helped me confirm my wiring setup through the RJ45 female breakout box.
* [maraid's Jarvis ESPHome component](https://github.com/maraid/Jarvis)
   * This is where this repository forks from. It worked with a few minor changes.

I ended up with the following pin arrangement:

* RJ45 pin 2 (yellow wire) → D1 Mini GPIO5 (RX)
* RJ45 pin 3 (black wire) → D1 Mini GND
* RJ45 pin 4 (green wire) → D1 Mini GPIO4 (TX)
* RJ45 pin 5 (red wire) → D1 Mini 5V

You can see photos of the wiring setup and the setup in its enclosure in the `images` directory.

To configure my device, I made some minor changes to the existing `jarvis.yaml` ESPHome configuration file which you can see in this repository's commit history. Mostly this involved renaming or relabeling to my preferences. I also created a `secrets.yaml` file to contain my WiFi details, Home Assistant API encryption key, and over-the-air update password.

Every entity is optional. Drop a key from a platform block and that entity simply is not created, so you can trim the entity list back to whatever you actually use in Home Assistant.

To compile and run on my D1 Mini, I used [Homebrew](https://brew.sh/) to install the [ESPHome CLI](https://formulae.brew.sh/formula/esphome#default):

```
esphome run jarvis.yaml
```

For the initial firmware programming on a new D1 Mini, I connected it via USB and select the USB serial device for programming. After that I reprogrammed when necessary over the air.

Finally, I added the device in Home Assistant by waiting a few minutes for Home Assistant to discover it, adding it via Integrations, and specifying my API encryption key when prompted.