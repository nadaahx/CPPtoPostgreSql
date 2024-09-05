## WhatsApp Sender Setup

### 1. Install Required Libraries

```bash
sudo apt install python3-dev python3
sudo apt-get install python3-venv
```

### 2. Create and Activate a Virtual Environment

```bash
python3 -m venv myenv
source myenv/bin/activate
```
### 3. Install Python Packages
```bash
pip install pywhatkit pyautogui
```
### 4. Use Xorg for GUI
```bash
sudo apt-get install xorg
ps aux | grep Xorg
xhost +local:
echo $DISPLAY
```
 It should display ‘:0’
### 5. Compile and Run the C++ Program
```bash
g++ -o myprogram main.cpp -I/usr/include/python3.12 -lpython3.12
```
Notes
Ensure all dependencies are correctly installed.
Follow the instructions carefully to set up the environment.
