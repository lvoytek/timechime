# Timechime configurator

Serve this webpage locally, such as with Python's http server

```bash
python3 -m http.server
```

Open the page on a Chromium-based browser, plug in the device, then connect with "Connect via WebUSB".

## Linux
Copy the timechime udev rules to allow WebUSB connection:

```bash
sudo cp site/99-timechime.rules /etc/udev/rules.d/
sudo udevadm control --reload
```