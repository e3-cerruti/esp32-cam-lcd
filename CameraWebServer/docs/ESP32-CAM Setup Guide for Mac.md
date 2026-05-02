# **ESP32-CAM Setup Guide for Mac Users**

Welcome\! This guide will walk you through assembling your ESP32-CAM, setting up the software on your Mac, and streaming your first live video feed.

## **Part 1: Hardware Assembly**

### **1\. Install the Camera Module**

1. **Locate the Connector:** Find the small FPC connector on the ESP32-CAM board. It has a tiny dark locking flap.  
2. **Open the Flap:** Gently flip up the locking flap using your fingernail. **Be gentle—it breaks easily\!**  
3. **Insert the Ribbon:** Slide the camera's ribbon cable into the slot.  
   * *Important:* The exposed silver metal contacts on the ribbon must face **down** toward the circuit board.  
4. **Lock it:** Push the dark flap back down to clamp the cable.  
5. **Remove the Lens Cap:** Don't forget to take the tiny plastic cover off the camera lens\!

### **2\. Connect the Motherboard**

Align the pins of the ESP32-CAM with the ESP32-CAM-MB motherboard and press them together. The camera lens should face the same direction as the micro-USB port.

## **Part 2: Software Setup**

### **1\. Install Arduino IDE**

Open the **Manager (Self-Service)** app on your Mac. Search for **Arduino IDE** and click install.

### **2\. Configure the IDE for ESP32**

By default, the Arduino IDE doesn't know how to talk to ESP32 boards. We need to download the ESP32 "core" library first.

1. Open the Arduino IDE.  
2. In the top menu bar, go to **Arduino IDE** \> **Settings** (or **Preferences**).  
3. Find the box labeled **Additional Boards Manager URLs**.  
4. Paste this exact link into the box: https://espressif.github.io/arduino-esp32/package\_esp32\_index.json  
5. Click **OK**.  
6. Go to **Tools** \> **Board** \> **Boards Manager** in the top menu.  
7. Search for esp32. Find the package by **Espressif Systems** and click **Install**. *(Note: You must search for "esp32" here to download the master library that contains our specific board. This may take a few minutes).*

## **Part 3: Programming the Board**

### **1\. Open the Camera Code**

Go to **File** \> **Examples** \> **ESP32** \> **Camera** \> **CameraWebServer**.

### **2\. Configure the Code**

In the newest version of this example, the camera selection has been moved to a separate file tab.

1. Look near the top of the Arduino text editor and click on the tab named **board\_config.h**.  
2. Scroll down until you see the list of camera models.  
3. Add // in front of the default active camera (it's usually \#define CAMERA\_MODEL\_ESP\_EYE or WROVER\_KIT) to comment it out.  
4. Remove the // in front of \#define CAMERA\_MODEL\_AI\_THINKER to activate our specific board. *(This is crucial, or the camera will fail\!)*

Next, click back over to the main tab (usually called **CameraWebServer**) to set up your Wi-Fi network:

* Change the ssid to: "e3CivicHigh"  
* Change the password to: "*****"

### **3\. Connect to Your Mac**

Use the provided **USB-A to Micro-USB cable** along with the **USB-C adapter** to connect the ESP32-CAM-MB to your MacBook.

### **4\. Select the Board and Port**

1. In the top toolbar of the Arduino IDE, click the Board drop-down menu and choose **Select other board and port...** 2\. In the **BOARDS** search box, type **AI Thinker**. *(Do not just type "esp32" or you will have to scroll through hundreds of options\!)*  
2. Select **AI Thinker ESP32-CAM** from the list.  
3. On the right side of that same window under **PORTS**, look for your Mac port. It will usually look something like /dev/cu.usbserial-XXXX or /dev/cu.wchusbserialXXXX. Select it and click **OK**.  
   *(Note: It is completely normal if the Arduino IDE says "Unknown board". Just manually select the port and click OK).*

### **5\. Upload and Stream\!**

1. Click the right-pointing arrow (**Upload**) at the top left of the IDE. Wait for it to say "Done uploading."  
2. Open the **Serial Monitor** by clicking the magnifying glass icon in the very top right corner of the Arduino window (or go to **Tools** \> **Serial Monitor** in the menu bar).  
3. Look at the top right of the new Serial Monitor panel that opens at the bottom of your screen. Set the baud rate dropdown menu to **115200 baud**.  
4. Press the small **RST** button on the ESP32 motherboard.  
5. Watch the Serial Monitor. It will print an IP address (e.g., http://192.168.x.x).  
6. Open Safari or Chrome and type that IP address in. Click **Start Stream** at the bottom of the page\!

## **Advanced Modification: Removing the IR Filter (Night Vision)**

**⚠️ EXTREME WARNING: PROCEED AT YOUR OWN RISK\! ⚠️** Removing the IR filter requires physically shattering a tiny piece of glass inside the lens. It is highly likely you will permanently scratch the main lens or destroy the camera entirely. If you break it, your camera will not work for tomorrow's demo\!

If your team absolutely needs night vision immediately and accepts the risk of destroying the module, follow these instructions carefully:

1. **Watch the tutorial first:** See exactly how delicate this process is here: [https://www.youtube.com/watch?v=mRSLSeX3omA](https://www.youtube.com/watch?v=mRSLSeX3omA)  
2. **Unscrew the Lens:** Carefully twist and unscrew the threaded lens housing completely out of its square black base.  
3. **Locate the Filter:** Look at the bottom of the lens piece you just removed. You will see a tiny square of reddish-tinted glass. This is the IR filter.  
4. **Shatter the Filter:** Using a very sharp pin, needle, or precision craft knife, carefully push into the reddish glass to crack it. *Do not push too far, or you will gouge the actual curved camera lens underneath\!*  
5. **Clear the Glass:** Carefully pick out all the broken glass shards.  
6. **Clean:** Blow out the lens housing thoroughly to ensure zero glass dust is left inside.  
7. **Reassemble & Focus:** Screw the lens back into the base. Because you removed a layer of glass, the camera will now be out of focus. You will need to turn on the live stream and slowly twist the lens left or right until the image is sharp again.

## **Troubleshooting Common Issues**

**Issue: I get one single image, and then the stream freezes or crashes.**

*(This means your board doesn't have quite enough continuous power to keep the Wi-Fi radio and camera running at the same time, or your Wi-Fi signal is too weak).*

* **Change the Resolution and Quality:** Refresh the page. Before clicking "Start Stream", change the **Resolution** to **VGA (640x480)**. *(Counter-intuitively, lower resolutions like QVGA sometimes try to send too many frames per second and crash the board, while VGA is more stable\!)* Also, find the **Quality** slider and *increase* the number to 15 or 20 (a higher number means lower image quality, making it easier to transmit).  
* **Move closer to the Router:** The ESP32-CAM has a very tiny, relatively weak built-in Wi-Fi antenna. Try moving closer to your classroom's Wi-Fi access point.  
* **Try a different USB Port/Cable:** Sometimes one specific USB port on a Mac provides slightly more stable current than another. Ensure your adapter is plugged in very tightly.

**Issue: I click "Start Stream" and get a "Broken Image" icon.**

*(This means the camera tried to send an image, but it failed halfway through).*

* **Re-seat the Camera Cable:** This is the most common fix. Unplug the board from your Mac, open the dark flap on the camera connector, push the golden ribbon cable in as far as it will possibly go, and snap the flap down hard.  
* **Power Issues:** Try plugging directly into a different port on the Mac, avoiding external USB hubs if possible. Lower the resolution to VGA before hitting start.

**Issue: I click "Start Stream" but just see a black screen\!**

* **The Lens Cap:** Double-check that the tiny plastic lens cap (and any tiny protective film) is actually removed from the camera lens.

**Issue: The Serial Monitor just prints random "garbage" symbols.**

* Double-check that the baud rate drop-down in the top right corner of the Serial Monitor panel is set to exactly **115200**.