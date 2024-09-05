


## Email Sender Setup

### 1. Install Required Libraries

```bash
sudo apt-get install libcurl4-openssl-dev
sudo apt-get install libssl-dev
sudo apt-get install libcpp-base64-dev
```
### 2. Verify libcurl Installation
```bash
curl --version
```
You should see the default version including **smtp and smtps** protocols.
### 3. Reinstall libcurl if Needed
```bash
sudo apt-get install --reinstall libcurl4-openssl-dev curl
```
 Or, if you have a custom version:
 ```bash
sudo rm /usr/local/lib/libcurl.so*
```
then install it again
### 4. Compile the C++ Program
```bash
g++ -o read_email read_email.cpp -lcurl -lssl -lcrypto
```

### 5. Making a Custom App Password for Gmail

Gmail requires two-factor authentication (2FA) for enhanced security, which means that using your regular account password for SMTP connections will not work. You need to generate a custom app password for the program.

- Step 1: Enable 2FA for your Gmail account if you haven't already.

- Step 2: Go to your Google Account settings and navigate to the "Security" section.

- Step 3: Under "Signing in to Google," find "App passwords" and create a new app password.

- Step 4: Choose "Mail" as the app and "Other" as the device, and enter a custom name (e.g., "C++ Email Program").

- Step 5: Google will generate a 16-character app password. Use this password in the PASSWORD macro in your code instead of your regular Gmail password.








