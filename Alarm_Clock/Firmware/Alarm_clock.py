import os
import time
import subprocess
import threading
from flask import Flask, request, render_template

app = Flask(__name__)

# Dictionary to store alarms
alarms = {}

UPLOAD_FOLDER = '/home/Edac145/Projects/alarmclock/alarm_set_v0.1/upload'
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER

# Function to check if alarm condition is met
def alarm_condition_met(alarm_hour, alarm_minute, alarm_date):
    current_time = time.localtime()
    current_hour = current_time.tm_hour
    current_minute = current_time.tm_min
    current_date = time.strftime("%Y-%m-%d", current_time)
    return current_date == alarm_date and current_time.tm_hour == alarm_hour and current_time.tm_min == alarm_minute

# Function to start streaming audio from phone to Pi via Bluetooth
def start_audio_stream():
    subprocess.run(["pactl", "set-default-sink", "bluez_sink.<90:80:0D:AC:B2:10>"], capture_output=True)

# Function to stop audio stream
def stop_audio_stream(mac_address):
    subprocess.run(['bluetoothctl', 'disconnect', mac_address], check=True)

# Function to connect to Bluetooth device
def connect_device(mac_address):
    subprocess.run(['bluetoothctl', 'connect', mac_address], check=True)

# Function to play alarm ringtone from memory
def play_alarm_ringtone(ringtone_path, num_times, interval):
    for _ in range(num_times):
        subprocess.run(["mpg123", ringtone_path])
        time.sleep(interval)

# Flask route to handle the date input
# Flask route to handle the date input
# Flask route to handle the date input and toggle alarms
@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        # Check if the form was submitted for setting new alarms
        if 'name' in request.form:
            alarm_name = request.form['name']
            alarm_hour = int(request.form['hour'])
            alarm_minute = int(request.form['minute'])
            alarm_date = request.form['date']
            num_times = int(request.form['num_times'])
            interval = int(request.form['interval'])
            ringtone_file = request.files['ringtone']

            if ringtone_file.filename != '':
                ringtone_path = os.path.join(app.config['UPLOAD_FOLDER'], ringtone_file.filename)
                ringtone_file.save(ringtone_path)
            else:
                ringtone_path = None

            # Set the alarm active by default
            alarms[alarm_name] = {'hour': alarm_hour, 'minute': alarm_minute, 'date': alarm_date,
                                  'ringtone_path': ringtone_path, 'num_times': num_times,
                                  'interval': interval, 'active': True}
        else:  # Handle toggling alarms
            for name, details in alarms.items():
                alarm_status = request.form.get(name)
                if alarm_status == 'on':
                    alarms[name]['active'] = True
                else:
                    alarms[name]['active'] = False

    current_time = time.localtime()
    current_hour = current_time.tm_hour
    current_minute = current_time.tm_min
    current_date = time.strftime("%Y-%m-%d", current_time)
    
    future_alarms = {name: details for name, details in alarms.items() if (details['date'] > current_date) or 
                     (details['date'] == current_date and ((details['hour'] > current_hour) or 
                     (details['hour'] == current_hour and details['minute'] > current_minute)))}

    return render_template('index.html', alarms=future_alarms)



# Function to handle alarm logic
def handle_alarms():
    mac_address = '18:AB:1D:27:80:1E'
    alarm_triggered = {}
    device_connected = False
    audio_stopped = False
    alarm_start_time = None  # Variable to store the start time of the alarm

    while True:
        try:
            current_time = time.localtime()
            print("Current time:", time.strftime("%H:%M:%S", current_time))

            # Check if the alarm has been triggered and start time is not set
            if any(alarm_triggered.values()) and alarm_start_time is None:
                alarm_start_time = time.time()  # Record the start time of the alarm

            for alarm_name, alarm_details in alarms.items():
                if alarm_details.get('active', True) and alarm_condition_met(alarm_details['hour'], alarm_details['minute'], alarm_details['date']):
                    if alarm_name not in alarm_triggered or not alarm_triggered[alarm_name]:
                        if not audio_stopped:
                            stop_audio_stream(mac_address)
                            audio_stopped = True

                        play_alarm_ringtone(alarm_details['ringtone_path'], alarm_details['num_times'], alarm_details['interval'])
                        alarm_triggered[alarm_name] = True

                        if not device_connected:
                            connect_device(mac_address)
                            device_connected = True
                else:
                    alarm_triggered[alarm_name] = False
                    device_connected = False
                    audio_stopped = False

            # Check if the alarm has been playing for a specific duration (e.g., 5 minutes)
            if alarm_start_time is not None and time.time() - alarm_start_time > 300:  # 300 seconds = 5 minutes
                stop_audio_stream(mac_address)
                audio_stopped = True
                alarm_triggered = {alarm_name: False for alarm_name in alarm_triggered}  # Reset alarm_triggered
                alarm_start_time = None  # Reset alarm start time

                # Reconnect to Bluetooth
                if not device_connected:
                    connect_device(mac_address)
                    device_connected = True
                time.sleep(1)  # Wait for 1 second before checking again
                continue  # Skip to the next iteration of the loop

            start_audio_stream()
            time.sleep(1)

        except KeyboardInterrupt:
            print("Stopping alarm clock...")
            break

def main():
    server_thread = threading.Thread(target=app.run, kwargs={'host': '0.0.0.0', 'port': 5000})
    server_thread.start()

    alarm_thread = threading.Thread(target=handle_alarms)
    alarm_thread.start()

if __name__ == "__main__":
    main()
