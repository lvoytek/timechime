## Linux
Copy the timechime udev rules to allow WebUSB connection:

```bash
sudo cp site/99-timechime.rules /etc/udev/rules.d/
sudo udevadm control --reload
```