import tkinter as tk
from tkinter import messagebox, scrolledtext
import zmq
import threading

class CarClientGUI:
    def __init__(self, master):
        self.master = master
        self.master.title("Car Guess & Shop Client")

        # ZMQ setup
        self.context = zmq.Context()
        self.pub_socket = self.context.socket(zmq.PUSH)
        self.sub_socket = self.context.socket(zmq.SUB)
        self.pub_socket.connect("tcp://benternet.pxl-ea-ict.be:24041")
        self.sub_socket.connect("tcp://benternet.pxl-ea-ict.be:24042")

        # Subscribe to all relevant topics once
        self.sub_socket.setsockopt_string(zmq.SUBSCRIBE, "Loic>CarGuess!>")
        self.sub_socket.setsockopt_string(zmq.SUBSCRIBE, "Loic>CarShop!>")

        self.points = 0  # Local point tracking

        # GUI layout
        self.name_label = tk.Label(master, text="Naam:")
        self.name_label.grid(row=0, column=0, sticky="e")
        self.name_entry = tk.Entry(master)
        self.name_entry.grid(row=0, column=1)

        self.guess_label = tk.Label(master, text="Gok:")
        self.guess_label.grid(row=1, column=0, sticky="e")
        self.guess_entry = tk.Entry(master)
        self.guess_entry.grid(row=1, column=1)

        self.send_guess_btn = tk.Button(master, text="Verzend Gok", command=self.send_guess)
        self.send_guess_btn.grid(row=2, column=0, pady=5)

        self.hint_btn = tk.Button(master, text="Vraag Hint", command=self.send_hint)
        self.hint_btn.grid(row=2, column=1, pady=5)

        self.shop_action_label = tk.Label(master, text="Shop Actie:")
        self.shop_action_label.grid(row=3, column=0, sticky="e")
        self.shop_action_entry = tk.Entry(master)
        self.shop_action_entry.grid(row=3, column=1)

        self.send_shop_btn = tk.Button(master, text="Verzend Shop Actie", command=self.send_shop_action)
        self.send_shop_btn.grid(row=4, column=0, columnspan=2, pady=5)

        self.log_area = scrolledtext.ScrolledText(master, width=60, height=20, state='disabled')
        self.log_area.grid(row=5, column=0, columnspan=2, padx=5, pady=5)

        self.running = True
        self.listener_thread = threading.Thread(target=self.listen, daemon=True)
        self.listener_thread.start()

    def log(self, msg):
        self.log_area.config(state='normal')
        self.log_area.insert(tk.END, msg + "\n")
        self.log_area.config(state='disabled')
        self.log_area.yview(tk.END)

    def get_name(self):
        return self.name_entry.get().strip()

    def send_guess(self):
        name = self.get_name()
        guess = self.guess_entry.get().strip()
        if not name or not guess:
            messagebox.showwarning("Invoer vereist", "Voer naam en gok in.")
            return
        msg = f"Loic>CarGuess?>{name}>{guess}>"
        self.pub_socket.send_string(msg)
        self.log(f"?? Gok verzonden: {guess}")

    def send_hint(self):
        name = self.get_name()
        if not name:
            messagebox.showwarning("Invoer vereist", "Voer je naam in.")
            return
        msg = f"Loic>CarGuess?>{name}>skip>"
        self.pub_socket.send_string(msg)
        self.log("? Hint gevraagd")

    def send_shop_action(self):
        name = self.get_name()
        action = self.shop_action_entry.get().strip()
        if not name or not action:
            messagebox.showwarning("Invoer vereist", "Voer naam en shop actie in.")
            return

        # Handle local point sync if 'setpoints_' is used
        if action.startswith("setpoints_"):
            try:
                self.points = int(action.split("_")[1])
                self.log(f"?? Punten lokaal ingesteld op {self.points}")
            except ValueError:
                self.log("?? Ongeldige puntenwaarde.")
                return

        msg = f"Loic>CarShop?>{name}>{action}>"
        self.pub_socket.send_string(msg)
        self.log(f"?? Shop actie verzonden: {action}")

    def listen(self):
        while self.running:
            try:
                msg = self.sub_socket.recv_string()
                if msg.startswith("Loic>CarGuess!>"):
                    content = msg.split(">", 3)[-1].rstrip(">")
                    self.log(f"?? Antwoord van server: {content}")
                    # Optional: track points if your server encodes them
                    if "punten" in content and "verdiend" in content:
                        try:
                            earned = int(''.join(filter(str.isdigit, content)))
                            self.points += earned
                            self.log(f"?? Punten bijgewerkt: totaal = {self.points}")
                        except:
                            pass
                elif msg.startswith("Loic>CarShop!>"):
                    content = msg.split(">", 3)[-1].rstrip(">")
                    self.log(f"?? Shop: {content}")
            except Exception as e:
                self.log(f"?? Fout bij ontvangen: {e}")

    def on_close(self):
        self.running = False
        self.context.term()
        self.master.destroy()


if __name__ == "__main__":
    root = tk.Tk()
    app = CarClientGUI(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()
