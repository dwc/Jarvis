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
* Always-on LCD display
* Custom number display (0-180)
* ESPHome configuration

Exploit that has been implemented so far:

* Always-on LCD display

Exploits that has not yet been implemented:

* Always-on leds with no display
* Always off leds and display but active handset. "Dark mode"


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

* [Home Assistant](https://www.home-assistant.io/) or [ESPHome](https://esphome.io/)
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

To compile and run on my D1 Mini, I used [Homebrew](https://brew.sh/) to install the [ESPHome CLI](https://formulae.brew.sh/formula/esphome#default):

```
esphome run jarvis.yaml
```

For the initial firmware programming on a new D1 Mini, I connected it via USB and select the USB serial device for programming. After that I reprogrammed when necessary over the air.

Finally, I added the device in Home Assistant by waiting a few minutes for Home Assistant to discover it, adding it via Integrations, and specifying my API encryption key when prompted.