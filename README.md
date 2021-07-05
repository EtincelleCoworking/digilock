
# Install instructions for Espace-W

## Download the code and compile
```bash
cd
git clone https://github.com/EtincelleCoworking/digilock
cd digilock/digilock
git checkout espace-w
sudo apt-get install libcurl4-openssl-dev libsqlite3-dev
make
```

## Configure

You can run this command to check your Raspberry Pi's pinout:
(only use the pin numbers in the 'wPi' column)
```bash
gpio readall

```

Configure the pins using wiringPi numbering, the output of the command above should help:
```bash
# Intercom detector (attached to optocoupler board)
# Must match EPinIntercomButtonOUT in config.ini
gpio mode 23 out

# Door open command (attached to relay board)
# Must match EPinIntercomButtonOUT in config.ini 
gpio mode 28 in 

```

Edit the config.ini file, you must update these 5 values:
```bash
nano config.ini
```
```
# tell digilock to start scanning intercom when launched
SCAN_INTERCOM = 1;

# API stuff
SLUG = "<my_slug>";
KEY = "<my_key>";

# Intercom detector (attached to optocoupler board)
EPinIntercomBuzzerIN = 28;

# Door open command (attached to relay board)
EPinIntercomButtonOUT = 23;

```

# Run !
```
./digilock
```