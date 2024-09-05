import pywhatkit as kit
import pyautogui
import time


def send_whatsapp_message(hour, minute):
    # Send a WhatsApp message
    print(f"Scheduling the message for {hour}:{minute:02d}...")
    kit.sendwhatmsg("+putnumberhere", "put message here", hour, minute)
    
    # Wait for the browser to open and the message to be prepared
    time.sleep(20)  # Adjust this sleep time as needed based on your network speed and system performance

    # Click on a specific position
    pyautogui.click(1050, 950)
    time.sleep(2)
    
    # Press Enter to send the message
    pyautogui.press('enter')
    print("Message sent!")

def main():
    # Start time
    start_hour = 17
    start_minute = 40
    # Number of times to send the message
    num_repeats = 50

    current_hour = start_hour
    current_minute = start_minute

    for _ in range(num_repeats):
        send_whatsapp_message(current_hour, current_minute)
        
        # Increase time by 2 minutes
        current_minute += 2
        if current_minute > 59:
            current_minute = 2
            current_hour += 1

        # Optional: Delay between each loop iteration (in seconds)
        delay_between_sends = 3
        print(f"Waiting {delay_between_sends} seconds before scheduling the next message...")
        time.sleep(delay_between_sends)

if __name__ == "__main__":
    main()