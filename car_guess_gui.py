import tkinter as tk
from tkinter import messagebox, scrolledtext
import zmq
import threading

class CarGuessClient:
    def __init__(self, master):
        self.master = master
        self.master.title("Car Guess Client")
        
        self.context = zmq.Context()
        self.pub_socket = self.context.socket(zmq.PUSH)
        self.sub_socket = self.context.socket(zmq.SUB)

        # Connect to server
        self.pub_socket.connect("tcp://benternet.pxl-ea-ict.be:24041")
        self.sub_socket.connect("tcp://benternet.pxl-ea-ict.be:24042")

        # GUI layout
        self.name_label = tk.Label(master, text="Naam:")
        self.name_label.grid(row=0, column=0, sticky="e")
        self.name_entry = tk.Entry(master)
        self.name_entry.grid(row=0, column=1, padx=5)

        self.guess_label = tk.Label(master, text="Gok:")
        self.guess_label.grid(row=1, column=0, sticky="e")
        self.guess_entry = tk.Entry(master)
        self.guess_entry.grid(row=1, column=1, padx=5)

        self.send_button = tk.Button(master, text="Verzend Gok", command=self.send_guess)
        self.send_button.grid(row=2, column=0, pady=5)

        self.hint_button = tk.Button(master, text="Vraag Hint", command=self.send_hint)
        self.hint_button.grid(row=2, column=1, pady=5)

        self.log_area = scrolledtext.ScrolledText(master, width=50, height=15, state='disabled')
        self.log_area.grid(row=3, column=0, columnspan=2, padx=5, pady=5)

        self.running = True
        self.listener_thread = threading.Thread(target=self.listen_for_response, daemon=True)
        self.listener_thread.start()

    def log(self, message):
        self.log_area.config(state='normal')
        self.log_area.insert(tk.END, message + '\n')
        self.log_area.config(state='disabled')
        self.log_area.yview(tk.END)

    def send_guess(self):
        name = self.name_entry.get().strip()
        guess = self.guess_entry.get().strip()
        if not name or not guess:
            messagebox.showwarning("Input vereist", "Voer zowel naam als gok in.")
            return
        message = f"Loic>CarGuess?>{name}>{guess}>"
        self.pub_socket.send_string(message)
        self.log(f"? Verzonden gok: {guess}")

    def send_hint(self):
        name = self.name_entry.get().strip()
        if not name:
            messagebox.showwarning("Input vereist", "Voer je naam in om een hint te vragen.")
            return
        message = f"Loic>CarGuess?>{name}>skip>"
        self.pub_socket.send_string(message)
        self.log("? Hint gevraagd")

    def listen_for_response(self):
        topic_prefix = "Loic>CarGuess!>"
        self.sub_socket.setsockopt_string(zmq.SUBSCRIBE, topic_prefix)  # Subscribe once!

        while self.running:
            try:
                msg = self.sub_socket.recv_string()
                # Example: Loic>CarGuess!>Loic>Hint: Duits>
                # Strip the topic prefix and ending >
                if msg.startswith(topic_prefix):
                    content = msg[len(topic_prefix):]
                    if content.endswith(">"):
                        content = content[:-1]
                    self.log(f"? Server: {content}")
            except Exception as e:
                self.log(f"?? Fout bij ontvangen: {e}")

    def on_close(self):
        self.running = False
        self.context.term()
        self.master.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    client = CarGuessClient(root)
    root.protocol("WM_DELETE_WINDOW", client.on_close)
    root.mainloop()
