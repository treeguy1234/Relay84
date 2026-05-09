
# Assembly Instructions
 
## Parts Needed
 
| Part Name | Link |
|---|---|
| ESP32-S3 Dev Board | [https://amzn.to/4tnXTXk](https://amzn.to/4tnXTXk) |
| USB-C Male to USB-Mini-B Male Cable | [https://amzn.to/4eC95fA](https://amzn.to/4eC95fA) |
| USB-A Male to USB-C Male Cable | [https://amzn.to/4eCnA2W](https://amzn.to/4eCnA2W) |
| TI-84 Plus CE Python | [https://amzn.to/4dxuDbX](https://amzn.to/4dxuDbX) |
| Power Bank (any USB-A output) | |
 
*Disclaimer: These are Amazon affiliate links. Purchasing through them supports the project at no extra cost to you.*
 
---
 
## Before You Start
 
Some ESP32-S3 boards require a small soldering step to enable USB OTG (On-The-Go) mode, which is what allows the board to act as a USB host and communicate with the calculator. Check your board for a pair of pads labeled **USB OTG** or **OTG BRIDGE**. They are usually located on the underside of the board near the USB port.
 
If your board has these pads, you must bridge them with solder before proceeding. Without this, the calculator will not be recognized.
 
> **Not sure if your board needs this?** Check the product page or schematic for your specific ESP32-S3 variant. The DevKitM-1 does require this step.
 
---
 
## Assembly Steps
 
### Step 1: Solder the USB OTG Pads (if required)
 
1. Locate the USB OTG solder pads on the underside of your ESP32-S3 board.
2. Apply a small amount of solder to bridge the two pads together.
3. Inspect the joint to confirm the pads are fully connected with no cold solder.
### Step 2: Launch the Messaging Program
 
1. Turn on your TI-84 Plus CE.
2. Open the messaging client on the calculator before connecting the ESP32-S3.
### Step 3: Connect the Calculator
 
1. Take the **USB-C Male to USB-Mini-B Male** cable.
2. Plug the **USB-Mini-B** end into the top of the TI-84 Plus CE.
3. Plug the **USB-C** end into the port labeled **USB** on the ESP32-S3 board.
> **Important:** Make sure to use the port labeled **USB**, not **COM**. The USB port handles data and USB host functionality. The COM port is for power and flashing only.
 
### Step 4: Power the ESP32-S3
 
1. Take the **USB-A Male to USB-C Male** cable.
2. Plug the **USB-C** end into the port labeled **COM** on the ESP32-S3 board.
3. Plug the **USB-A** end into your power bank.
4. Turn the power bank on.
### Step 5: Verify
 
Once powered, the ESP32-S3 should boot and the calculator should be recognized within a few seconds. If the connection is successful, the messaging client will display a confirmation message.
 
---
 
## Port Reference
 
| Port Label | Purpose |
|---|---|
| `USB` | Calculator data connection (USB-Mini-B cable goes here) |
| `COM` | Power input - 5V - from power bank (USB-A to USB-C cable goes here) |
 
---
 
## Troubleshooting
 
**Calculator not recognized**
- Confirm the USB OTG pads are properly bridged. A cold or incomplete solder joint is the most common cause of this issue.
- Make sure the cable is plugged into the port labeled `USB`, not `COM`.
- Try a different USB-C to USB-Mini-B cable. Some cables are charge-only and carry no data lines.
- Make sure the messaging program is open on the calculator before connecting the ESP32-S3.

**ESP32-S3 not powering on**
- Check that the power bank is turned on and has charge.
- Confirm the USB-A to USB-C cable is plugged into the `COM` port on the board.

**Messages not reaching other calculators**
- Confirm all ESP32-S3 boards in the group have been flashed with firmware containing each other's MAC addresses.
- Keep boards within a reasonable range. ESP-NOW has a practical range of roughly 200-400m in open air, significantly less indoors.