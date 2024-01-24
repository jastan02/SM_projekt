import tkinter as tk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
import serial
import threading
import time
import csv
from datetime import datetime

# Inicjalizacja połączenia szeregowego
ser = serial.Serial('COM3', 115200)  # Zastąp 'COM4' odpowiednim portem



# Prosta funkcja PID
def update_pid(kp, ki, kd, set_point, current_value):
    error = set_point - current_value
    return error * kp  # Przykładowa implementacja, tylko część proporcjonalna


# Funkcja do odczytu danych z kontrolera STM
def read_from_stm():
    while True:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8').rstrip()
            print(f"Otrzymano dane: {line}")  # Dodano logowanie otrzymanych danych

            try:
                parts = line.split(', ')
                if len(parts) > 0 and ": " in parts[0]:
                    sensor_val_str = parts[0].split(': ')[1]
                    sensor_val = float(sensor_val_str)
                    y_values.append(sensor_val)
                    x_values.append(time.time())
                else:
                    print("Nieprawidłowy format danych")
            except ValueError as e:
                print(f"Błąd podczas parsowania danych: {line}. Szczegóły błędu: {e}")


# Funkcja do wysyłania danych do kontrolera STM
def write_to_stm():
    while True:
        command = f"Set Kp:{kp.get()} Ki:{ki.get()} Kd:{kd.get()} Setpoint:{set_point.get()}\n"
        ser.write(command.encode('utf-8'))
        time.sleep(1)  # Opóźnienie

# Funkcja do zapisywania danych do pliku CSV
def save_to_csv():
    filename = f"balancing_car_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    with open(filename, 'w', newline='') as csvfile:
        csvwriter = csv.writer(csvfile)
        csvwriter.writerow(['Time', 'Sensor Value'])
        for x, y in zip(x_values, y_values):
            csvwriter.writerow([x, y])
    print(f"Dane zostały zapisane do pliku: {filename}")


# Funkcja resetująca wykres
def reset_plot():
    x_values.clear()
    y_values.clear()
    ax.clear()
    ax.axhline(y=set_point.get(), color='r', linestyle='-', label="Wartość zadana")
    ax.legend()
    canvas.draw()


# Funkcja aktualizująca wykres
def update_plot():
    while True:
        time.sleep(0.1)  # Opóźnienie dla zmniejszenia obciążenia procesora

        if len(x_values) == len(y_values) and len(x_values) > 0:
            ax.clear()
            ax.plot(x_values, y_values, label="Czujnik")
            ax.axhline(y=set_point.get(), color='r', linestyle='-', label="Wartość zadana")
            ax.axhline(y=set_point.get() * 1.05, color='g', linestyle='--', label="5% Odchył")
            ax.axhline(y=set_point.get() * 0.95, color='g', linestyle='--')
            ax.legend()
            canvas.draw()


# Tworzenie okna głównego
root = tk.Tk()
root.title("Balancing Car")  # Tytuł okna

# Inicjalizacja zmiennych
kp = tk.DoubleVar(value=10.0)
ki = tk.DoubleVar(value=0.1)
kd = tk.DoubleVar(value=0.0)
set_point = tk.DoubleVar(value=4)

# Tworzenie wykresu
fig, ax = plt.subplots()
ax.set_title("Balancing Car")  # Tytuł wykresu
canvas = FigureCanvasTkAgg(fig, master=root)
widget = canvas.get_tk_widget()
widget.grid(row=0, column=0, columnspan=4)

# Dodanie pól do zmiany nastaw PID
tk.Label(root, text="Kp:").grid(row=1, column=0)
tk.Scale(root, variable=kp, from_=10, to=100, resolution=1, orient=tk.HORIZONTAL).grid(row=1, column=1)
tk.Label(root, text="Ki:").grid(row=2, column=0)
tk.Scale(root, variable=ki, from_=0.001, to=0.1, resolution=0.005, orient=tk.HORIZONTAL).grid(row=2, column=1)
tk.Label(root, text="Kd:").grid(row=3, column=0)
tk.Scale(root, variable=kd, from_=0, to=50, resolution=1, orient=tk.HORIZONTAL).grid(row=3, column=1)

# Pole do ustawienia wartości zadanej
tk.Label(root, text="Wartość zadana:").grid(row=4, column=0)
tk.Scale(root, variable=set_point, from_=4, to=45, resolution=1, orient=tk.HORIZONTAL).grid(row=4, column=1)

# Dodanie przycisku resetu wykresu
reset_button = tk.Button(root, text="Resetuj wykres", command=reset_plot)
reset_button.grid(row=5, column=0, columnspan=2)

save_button = tk.Button(root, text="Zapisz dane do CSV", command=save_to_csv)
save_button.grid(row=6, column=0, columnspan=2)

# Listy do przechowywania danych wykresu
x_values = []
y_values = []

# Uruchomienie wątków
threading.Thread(target=read_from_stm, daemon=True).start()
threading.Thread(target=write_to_stm, daemon=True).start()
threading.Thread(target=update_plot, daemon=True).start()

root.mainloop()
