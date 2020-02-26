# trelliswifi
#### ESP32 based project for multi-purpose buttons and colors using NeoTrellis

## Goals

- Leverage the Adafruit NeoTrellis + ESP platform to generate events via Wifi
  - Publish button press events via MQTT (long and short combos using bitmask values)
  - Use MQTT subscriptions to provide complete control of lights and animations
- Keep config in non-volatile ram (one unchanged code image for multiple devices)

## Background

I needed a multi-purpose control panel of buttons to turn on/off lights in the
house. I also thought it would be nice if it had lights to provide
feedback on what is currently on. That idea evolved to using this cool gizmo as a
portable remote that interfaces with my home server via MQTT through Wifi.
Built-in animations are implemented to allow LEDs under buttons to blink, pulse or
change colors. At this point, it is generic enough to be used as a frontend for
controlling anything while providing 64 RGBs as a feedback display. :sparkles:

## Hardware

As of now, this project is picky about the hardware it uses.
More specifically, it is written for an 8x8 NeoTrellis setup with a Huzzah ESP feather.
The non-volatile storage part of this project is based on ESP32's
[preferences library](https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/src/Preferences.h).
So, a few tweaks to the code -- nothing earth-shattering :sweat_smile: -- would also be needed in order to make
it work on non-ESP32 micro-controllers.

**All parts can be purchased at [my favorite DIY store (aka Adafruit)](https://adafruit.com)**:

- Adafruit [HUZZAH ESP32](https://www.adafruit.com/product/3405)
- Adafruit [8x8 Trellis Feather M4 Acrylic Enclosure + Hardware Kit](https://www.adafruit.com/product/4372)
- 4 x [Adafruit NeoTrellis RGB Driver PCB for 4x4 Keypad](https://www.adafruit.com/product/3954)
- 4 x [Silicone Elastomer 4x4 Button Keypad - for 3mm LEDs](https://www.adafruit.com/product/1611)
- [Lithium-Ion Polymer Battery](https://www.adafruit.com/product/1578)
- [Toggle Switch](https://www.adafruit.com/product/3220)

:paperclip:
These pages from [learn.adafruit.com](https://learn.adafruit.com) tell you how to put it all together:
- [NeoTrellis Feather Case Assembly](https://learn.adafruit.com/neotrellis-feather-case-assembly)
- [Adafruit NeoTrellis](https://learn.adafruit.com/adafruit-neotrellis/) 

:warning:
The 8x8 setup is comprised of four 4x4 [keypads](https://www.adafruit.com/product/3954) and each
of them must have a unique I2C address. Make sure to address them properly by
leaving one untouched, bridging "A0" on the second, "A1" on the third, and both "A0+A1" on the fourth. If
you use a different configuration, tweak
[the code here](https://github.com/flavio-fernandes/trelliswifi/blob/stable/lights.ino#L16-L19) before
[uploading the project](https://github.com/flavio-fernandes/trelliswifi/blob/stable/README.md#compile-and-upload)
to your ESP32 board.

## Software

### Board Manager

First, follow the
[steps described on this page](https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md)
to make the
[Arduino IDE](https://www.arduino.cc/en/Main/Software)
work with the [Espressif ESP32 board](https://www.espressif.com/en/products/hardware/esp32/overview).
I found that [this page](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/) offers good details
about this but uses a different value for the board manager URL. And since I use both ESP32 and the ESP8266 boards with the Arduino IDE,
the **Additional Board Manager URLs** field in my system looks like this:

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json, http://arduino.esp8266.com/stable/package_esp8266com_index.json
```

After that, you will be able to search and find ESP32 from the **Tools ==> Boards ==> Boards Manager...** menu. The image
below is what it looks like in my system. Feel free to use a version newer than 1.0.4.

![Trellis Wifi setup -- Arduino Boards Manager ESP32](https://i.imgur.com/J6BvLBb.png)

Once that is done, select **Tools ==> Board ==> Adafruit ESP32 Feather**. For my setup, this is how that looks like:

![ESP32 Arduino IDE Settings](https://i.imgur.com/VeFZDy7.png)

### Library manager

:bangbang:
You need additional libraries to build trelliswifi. Install them in the libraries folder, which is located under
your sketchbook folder. If you are not sure where that is in your system, open the **Arduino ==> Preferences...** menu. It should look like this:

![Arduino IDE project and libraries folder](https://i.imgur.com/S2IPe8Q.png)

#### Download the trellis-wifi project

```bash
$ cd ${sketchbook_folder} ; # use the folder you see in the preferences menu mentioned above
$ [ ! -e stable.zip ] && curl -L -O https://github.com/flavio-fernandes/trelliswifi/archive/stable.zip ; \
  [ ! -d trelliswifi-stable ] && unzip stable.zip && rm -f stable.zip ; \
  [ ! -d trelliswifi ] && mv -v trelliswifi-stable trelliswifi
$ open ./trelliswifi/trelliswifi.ino
```

These are the required libraries:

- ArduinoJson
- Adafruit_seesaw_Library
- Adafruit_MQTT_Library
- TickerScheduler
- WiFiManager

#### Download ArduinoJson

Open **Sketch ==> Include Library ==> Manager Libraries...**
In the search field, type **ArduinoJson**.
I installed version 6.14.1, but anything newer should be okay, too.

#### Download Adafruit_seesaw_Library

Just like you install _ArduinoJson_, open **Sketch ==> Include Library ==> Manager Libraries...**
In the search field, type **Adafruit_seesaw_Library**.
I installed version 1.3.0, but anything newer should work just fine.

:warning:
This library depends on other libraries. It is likely that you will be prompted to install them.
If so, select **Install all** to satisfy all these dependencies.

#### Download Adafruit_MQTT_Library

This project uses version 1.0.3 of this library with a
[bugfix](https://github.com/adafruit/Adafruit_MQTT_Library/pull/166) that has not been merged yet.
This is how to install it:

```bash
$ cd ${sketchbook_folder}/libraries
$ [ ! -e trelliswifi.zip ] && curl -L -O https://github.com/flavio-fernandes/Adafruit_MQTT_Library/archive/trelliswifi.zip ; \
  [ ! -d Adafruit_MQTT_Library-trelliswifi ] && unzip trelliswifi.zip && rm -f trelliswifi.zip ; \
  [ ! -d Adafruit_MQTT_Library ] && mv -v Adafruit_MQTT_Library-trelliswifi Adafruit_MQTT_Library
```

#### Download TickerScheduler

This project leverages the awesome [TickerScheduler](https://github.com/Toshik/TickerScheduler)
library written by [Toshik](https://github.com/Toshik)
in order to perform periodic tasks.
Small changes, now [merged](https://github.com/Toshik/TickerScheduler/pull/22),
were needed to make it support ESP32.
This is a way of adding it to libraries:

```bash
$ cd ${sketchbook_folder}/libraries
$ [ ! -e trelliswifi.zip ] && curl -L -O https://github.com/flavio-fernandes/TickerScheduler/archive/trelliswifi.zip ; \
  [ ! -d TickerScheduler-trelliswifi ] && unzip trelliswifi.zip && rm -f trelliswifi.zip ; \
  [ ! -d TickerScheduler ] && mv -v TickerScheduler-trelliswifi TickerScheduler
```

#### Download WiFiManager

Last but far from least, trelliswifi rides on the shoulders of [tzapu](https://tzapu.com/) giant to leverage
[WiFiManager](https://github.com/tzapu/WiFiManager). This gives us a server portal for storing non-volatile
info such as SSID and MQTT server. More info on that later on. For now, follow these commands
to make it part of your Arduino library:

```bash
$ cd ${sketchbook_folder}/libraries
$ [ ! -e trelliswifi.zip ] && curl -L -O https://github.com/flavio-fernandes/WiFiManager/archive/trelliswifi.zip ; \
  [ ! -d WiFiManager-trelliswifi ] && unzip trelliswifi.zip && rm -f trelliswifi.zip ; \
  [ ! -d WiFiManager ] && mv -v WiFiManager-trelliswifi WiFiManager
```

#### :checkered_flag: Compile and Upload

If you made it here, you should be able to compile and
[upload](https://www.dummies.com/computers/arduino/how-to-upload-a-sketch-to-an-arduino/)
the project to your ESP32. Nice going! :clap:

### Initial configuration of MQTT and topic

It is time to jump into the temporary webserver started by your ESP, so you can provide details on the
Wifi and MQTT server it should connect to every time it boots. This is likely something you will just do
once per device, since the config is kept in non-volatile memory.

#### Connect to ESP's temporary AP

When booted for the first time -- or when you force the device to do so
([section below](https://github.com/flavio-fernandes/trelliswifi/blob/stable/README.md#recycle-resettingchanging-values-in-non-volatile-memory))
-- the ESP will boot as an
access point and start to broadcast its own SSID. I can't tell you what that SSID will be because each ESP
comes with a unique MAC address. Simply look for SSIDs that start with the name **ESP32_**.

Once Wifi connection is established, you should be presented with the WiFiManager window. If that is not the
case, open your browser and connect to the [portal url](http://192.168.4.1/):

**http://192.168.4.1/**


A couple of caveats you should keep in mind while configuring your device:

- The current version of WifiManager has a [bug on getting the wifi password](https://github.com/tzapu/WiFiManager/issues/1004#issuecomment-583666829). To work around this issue, make sure to provide the Wifi password on both fields of the form.  :dizzy_face:
- Use a unique topic prefix and do not forget the value you used. It should start and end with the **/** character, as
shown below. The default value used for this field is **/trelliswifi/**
- The MQTT server is mandatory. It can be an IP address or a [FQDN](https://kb.iu.edu/d/aiuv) (like io.adafruit.com).
- MQTT username and password fields are optional, depending on the server you use.

![WiFi Manager Example](https://i.imgur.com/KSHvyWz.png)

### :recycle: Resetting/changing values in non-volatile memory

Let's say you need to change the non-volatile setting or would like to wipe the existing values. No problem! :relaxed:

In [less than 2 minutes](https://github.com/flavio-fernandes/trelliswifi/blob/stable/myMqttClient.ino#L376-L382)
after turning the device on, do a "long press" on the 4 corner buttons. Long press means pressing
down the button until it turns solid purple.

![Factory Reset Mode](https://i.imgur.com/vG5BZrs.jpg)

This will make the trelliswifi clear all settings and restart its internal web server, as explained
[above](https://github.com/flavio-fernandes/trelliswifi/blob/stable/README.md#initial-configuration-of-mqtt-and-topic).
Clean, rinse and repeat. :bowtie:

## Animate via built-in button pushes

Before interacting with the device via network messages, try holding down button 8 until it turns purple and release it.
As a point of reference, that is the corner button on the edge farthest away from the feather micro-controller.
If it all goes well, all LEDs should turn on white. Short pressing the same button (a second or two)
should undo that behavior.

Next, try long-pressing button 8 and button 16. Short pressing them should make the animation stop.
For all the built-in animations, look at the [localButtonProcess function](https://github.com/flavio-fernandes/trelliswifi/blob/stable/animations.ino#L16-L56).
You will see pairs of 64 bitmasks for each animation. The first pair value represents the
buttons that need to be short pressed and the second bitmask is for the buttons that have to be held down until they
turn purple (aka long press).

## Let's talk MQTT

### Receiving events

Once trelliswifi is able to establish a connection with the configured MQTT server, there
are all sorts of fun you can have with it. The device will publish into 5 different MQTT topics
to provide updates on its current state. First, it may be better to explain how to get them and then
we can dive into each one of these topics.

#### Install MQTT client so you can interact with trelliswifi

Any MQTT client will work. The one I use quite often and really like is called Mosquitto.
More info at [mosquitto.org/download/](https://mosquitto.org/download/).
Here is a cheat sheet set of commands for easily getting it in your computer:

```bash
$ # If you are using Debian / Ubuntu / Raspberry-Pi:
$ sudo apt-add-repository ppa:mosquitto-dev/mosquitto-ppa
$ sudo apt-get update
$ sudo apt-get install -y mosquitto-clients

$ # If you are using a mac:
$ brew install mosquitto
```

#### :star2: Doing MQTT via a browser

If you have a chrome browser and would like to use that instead of Mosquitto, check out
[MQTTLens](https://docs.litmusautomation.com/display/DOC/MQTTLens). I found it super easy
to install and use. Kudos to [Sandro](https://github.com/sandro-k) for building such an
awesome [javascript-based app](https://github.com/sandro-k/MQTTLensChromeApp).

#### Start (or use an existing) MQTT server

Start an MQTT server or connect to an existing one. For simple testing,
you can just use a known public server, like [io.adafruit.com](https://io.adafruit.com/),
[test.mosquitto.org](https://test.mosquitto.org/) or anything listed
in [mqtt.github.io](https://github.com/mqtt/mqtt.github.io/wiki/public_brokers).

:exclamation: It is important that you have the same **MQTT server** value
[configured](https://github.com/flavio-fernandes/trelliswifi/blob/stable/README.md#connect-to-esps-temporary-ap)
for the trelliswifi device you use. The same goes for the **MQTT topic prefix** attribute.

#### Subscribing to events generated by trelliswifi device

```bash
MQTT=test.mosquitto.org  ; # replace this with whatever you decide to use as MQTT server
PREFIX_CONFIGURED=trelliswifi ; # replace this value with whatever you configured in web portal

mosquitto_sub -F '@Y-@m-@dT@H:@M:@S@z : %q : %t : %p' -h $MQTT \
-t /${PREFIX_CONFIGURED}/buttons \
-t /${PREFIX_CONFIGURED}/battery \
-t /${PREFIX_CONFIGURED}/memory \
-t /${PREFIX_CONFIGURED}/uptime \
-t /${PREFIX_CONFIGURED}/etc
```

At this point, try pressing and releasing a button. That will trigger the device to publish a "_buttons_" event.
Do you see it? Without closing the _mosquitto_sub_ command, open a new session and let's poke the device to
generate the other 4 topics:

```bash
MQTT=test.mosquitto.org  ; # replace this with whatever you decide to use as MQTT server
PREFIX_CONFIGURED=trelliswifi ; # replace this value with whatever you configured in web portal

mosquitto_pub -h $MQTT -t "/${PREFIX_CONFIGURED}/ping" -n
```

Here is a sample output:
```text
2020-02-22T20:06:03-0500 : 0 : /trelliswifi/buttons : {"p":"0x0000000000000001","l":"0x0000000000000000","x":"0x0000000000000000"}

2020-02-22T20:06:06-0500 : 0 : /trelliswifi/battery : {"volts":"3.70","isLow":"no"}
2020-02-22T20:06:06-0500 : 0 : /trelliswifi/uptime : {"up":"14","mqttUp":"14","dog":"14"}
2020-02-22T20:06:06-0500 : 0 : /trelliswifi/memory : {"maxMsg":"105","freeKb":"246","minFreeKb":"244","maxAllocKb":"111"}
2020-02-22T20:06:06-0500 : 0 : /trelliswifi/etc : {"pixelsOn":"0x0000000000000000","lightUnitsSize":"0"}
```

More info on each one of the topics you are seeing:

- /${PREFIX_CONFIGURED}/**buttons**
  - Gives you 3 hexadecimal values [that represent](https://github.com/flavio-fernandes/trelliswifi/blob/stable/buttons.ino#L5-L7):
    - (p) buttons that were pressed and released between 200 milliseconds and 2.4 seconds
    - (l) buttons that were pressed and held for longer than 2.4 seconds
    - (x) buttons held down for longer than 15 seconds
- /${PREFIX_CONFIGURED}/**battery**
  - Tells you the current battery voltage
  - Gives an "isLow" boolean, which gets set as _true_ when [battery output is less than 3.45 volts](https://github.com/flavio-fernandes/trelliswifi/blob/stable/trelliswifi.ino#L94-L101)
- /${PREFIX_CONFIGURED}/**memory**
  - Basic runtime info on [memory usage](https://github.com/flavio-fernandes/trelliswifi/blob/stable/myMqttClient.ino#L337-L340) of ESP
- /${PREFIX_CONFIGURED}/**uptime**
  - Gives you info on how long trelliswifi has been operational
    - **up**: minutes since it was turned on
    - **mqttUp**: minutes since last time it connected to MQTT server
    - **dog**: increases every minute, and gets reset when a specific MQTT request is received:
      - mosquitto_pub -h $MQTT -t "/${PREFIX_CONFIGURED}/ping" -m periodic
      - For more info on how that can be used, see [this commit](https://github.com/flavio-fernandes/trelliswifi/commit/a67e05800810056a37f86ed2c42648838083c641)
- /${PREFIX_CONFIGURED}/**etc**
  - Gives a bitmask in hexadecimal, representing which LEDs are currently on
  - It also tells how many _light unit entries_ are in use. More on that [later on](https://github.com/flavio-fernandes/trelliswifi/blob/stable/README.md#light-unit-entries), but these are created/deleted via the set/rm commands. Keep reading to learn more.

### Publishing events

The trelliswifi device subscribes to **ping** and **cmd** topics in order to handle external requests.

At this point, there is not much more to say about the _ping_ topic. It is simply a way of prompting
for status as well as resetting the watchdog. These status topics are
[generated every 10 minutes](https://github.com/flavio-fernandes/trelliswifi/blob/stable/myMqttClient.ino#L265-L272),
regardless of the _ping_ message.

#### The /${PREFIX_CONFIGURED}/**cmd** topic

- cmd
  - op
    - $ANIMATION_NAME
    - rm
    - set

#### Animations

These are actually built-in entries that use [id](https://github.com/flavio-fernandes/trelliswifi/blob/stable/animations.ino#L8) [511](https://github.com/flavio-fernandes/trelliswifi/blob/stable/lightUnit.h#L11). There is nothing special about that id; it's just a number.

The name simply maps to a pre-built function. See [initCmdOpHandlers](https://github.com/flavio-fernandes/trelliswifi/blob/stable/msgHandler.ino#L133-L156)
if you are interested in learning how that was coded.

Give them a spin, using these example commands:

```bash
TOPIC="/${PREFIX_CONFIGURED}/cmd"

mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "flash"}'
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "flashlight"}'
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "!flashlight"}'
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "counter6"}'
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "!counter"}'
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "crazy"}'
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "scan"}'

# stop animation using the well known id
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "rm", "id": "511"}'
```

#### Light Unit Entries

At the heart of the display implementation, the trelliswifi code handles
[entries](https://github.com/flavio-fernandes/trelliswifi/blob/stable/lightUnit.ino#L6-L7)
called [LightUnit](https://github.com/flavio-fernandes/trelliswifi/blob/stable/lightUnit.h#L38-L47).
That is implemented using the [C++ STL map](http://www.cplusplus.com/reference/map/map/).

You can get a good feel for all the attributes you can associate with an entry by looking at
the [file lightUnit.h](https://github.com/flavio-fernandes/trelliswifi/blob/stable/lightUnit.h).

```c++
typedef struct LightUnit_t {
  LightUnitId id;
  uint64_t pixelMask;  // overriden by randomPixels
  uint32_t color;  // overriden by sameRandomColor
  int8_t brightness;  // overriden by pulse. Adjusts color (0->ignored, 1->dark full->255)
  LightUnitAnimation animation;
} LightUnit;

typedef struct LightUnitAnimation_t {
  uint32_t frames;      // total number of frames this is part of
  uint32_t step;        // the frame index for this
  uint32_t speed;       // overriden by pulse. how many refreshes between each frame (in 100 ms units)
  uint64_t expiration;  // how many steps until animation stops (0 => never)
  LightUnitId dependsOn;   // will expire if entry it depends on is gone (0 => no-dep)
  bool randomPixels;       // pick a random pixelMask
  bool sameRandomColor;    // overriden by randomColor. Pick a random color for pixelMask
  bool randomColor;        // overriden by rainbowColor. Pick a random color for each pixel in mask
  bool rainbowColor;       // pick rainbow color for pixelMask (or pixel, when used with randomColor)
  bool keepPixelWhenDone;  // upon expiration, leave pixelMask alone?
  bool blink;              // set color to 0 on every other frame
  bool pulse;              // overriden by rainbowColor, randomColor, sameRandomColor.
                           // if true, will apply modify brightness to color
} LightUnitAnimation;
```

In order to be super flexible on what and how to display things, the **cmd** uses JSON strings.
The [MQTT library limits](https://github.com/flavio-fernandes/Adafruit_MQTT_Library/blob/trelliswifi/Adafruit_MQTT.h#L98)
how long these strings can be, but nothing stops you from tweaking that! The ESP32 can totally handle more
than 150 bytes, but many original Arduino boards don't have much memory.
**set cmd** takes an optional **id**, which allows us to break the attributes of a given
entry into multiple messages. If **id** is not provided in a **set** command, the implementation
creates a new entry. Let's clarify that with examples.


Creating entries that use multiple pixels.
```bash
# pixels: all 64
# id: not provided, so a new entry will be created
# color: lightest green
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "pixelMask": [4294967295, 4294967295], "color": 4096}'

# remove all entries
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "rm"}'


# pixels: higher 32
# id: 10
# color: brightest blue
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": 10, "pixelMask": [0, 4294967295], "color": 255}'

# pixels: lower 32
# id: 11
# color: brightest green
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": 11, "pixelMask": 4294967295, "color": 65280}'

# pixels: 32 in the middle, covering 16 pixels from ids 10 and 11 because its id is lower (lowest gets drawn last)
# id: 9
# color: brightest red
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": 9, "pixelMask": {"low": 4294901760, "high": 65535}}'
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": 9, "color": 16711680}'

# remove ids 9,10,11
for i in {9..11}; do mosquitto_pub -h $MQTT -t $TOPIC -m "{'op' : 'rm', 'id': $i}" ; done

# Add random blinking pixels of random colors that self delete in 10 seconds
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "animation": {"expiration":100, "randomPixels":1, "randomColor":1, "blink":1}}'
```

#### A word on **pixelMask** attribute

:boom:
As you may have noticed, **pixelMask** is a bitmask that
[can represent all 64 LEDs](https://github.com/flavio-fernandes/trelliswifi/blob/stable/msgHandler.ino#L50-L61)
in the device. Unfortunately, a single integer value can only hold up to 40 bits in ArduinoJson library (0x8000000000).
There are 3 workarounds for dealing with this limitation:
1) shift: If you need to reach beyond bit 40, use "pixelMask" with a single integer and then "pixelShiftUp". Examples below use that
approach.
2) array: As you can see in the example above, "pixelMask" can be an array of 2 elements that represent low and high 32 bits.
3) object: "pixelMask" can also be a JSON object that explicitly dictates low and high attributes.



Time for more examples! Making 3 entries that run on the same number of frames and speed to produce a simple color animation:
```bash
for i in {10..12}; do \
  mosquitto_pub -h $MQTT -t $TOPIC -m "{'op' : 'set', 'id': $i, 'pixelMask': 1, 'animation': {'speed': '10'}}"
  mosquitto_pub -h $MQTT -t $TOPIC -m "{'op' : 'set', 'id': $i, 'animation': {'frames': '3'}}"
done
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": 10, "color": 16711680, "animation": {"step": "0"}}'
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": 11, "color": 65280, "animation":    {"step": "1"}}'
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": 12, "color": 255, "animation":      {"step": "2"}}'

# Introducing entries dependence
for i in {10..11}; do \
  mosquitto_pub -h $MQTT -t $TOPIC -m "{'op' : 'set', 'id': $i, 'animation': {'dependsOn': $((i+1))}}"
done

# Removing entry 12 causes its 'dependents' to also get deleted
mosquitto_pub -h $MQTT -t $TOPIC -m "{'op' : 'rm', 'id': 12}"
```

For the last batch of examples, let's create some blinking and pulsing LEDs:
```bash
# Create entry with 4 green pixels
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": "1", "pixelMask": "15", "color": "65280"}'

# Shift the pixels up 3 positions
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": "1", "pixelShiftUp": 3}'

# Shift the pixels down 1 position
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": "1", "pixelShiftDown": 1}'

# Make it blink
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": "1", "animation": {"blink": 1}}'

# Make it pulse
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": "1", "animation": {"blink": 0, "pulse": 1}}'

# Make it rainbow colors
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": "1", "animation": {"rainbowColor": 1}}'

# Add a blinking red button 1
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": "123", "pixelMask": 1, "color": 16711680}'
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": "123", "animation": {"blink": 1}}'
# Reduce its brightness
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": "123", "brightness": 9}'
# Move it to last button
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "set", "id": "123", "pixelShiftUp": 63}'

# remove all entries
mosquitto_pub -h $MQTT -t $TOPIC -m '{"op" : "rm"}'
```

### Closing thoughts

I hope you have as much fun with trelliswifi as I do. If you hit a snag on anything mentioned here, please do not
hesitate to open an issue or make a PR. More importantly, I'm super curious to learn more about how this platform
can be leveraged to do the amazing IoT things you do. Enjoy!  :revolving_hearts:

### TO DO :construction:

- Make I2C address used for Trellis keypad configurable and stored in non-volatile memory (via WIFI Manager)
- Add buzzer for sounds :musical_score:
- Make it work on a non-ESP32 platform
- Adapt for Trellis of different sizes
- Use it to control [house sprinklers on Opensprinkler](https://opensprinkler.com/product/opensprinkler-pi/)
- Make an app that uses Trellis as a gamepad/scratchpad
- Enable [watchdog](https://github.com/flavio-fernandes/trelliswifi/commit/a67e05800810056a37f86ed2c42648838083c641) via WifiManager and keep that as part of non-volatile attributes

