"""
Environmental Monitoring Dashboard for Raspberry Pi
====================================================
Multi-sensor BLE data collection and visualization system.

This application connects to multiple BLE sensor nodes simultaneously:
- SPL Meter (XIAO MG24): A-weighted sound pressure level monitoring
- AI Vision Node (ESP32-C3): Person detection and occupancy tracking

Features:
- Real-time data visualization with live updating graphs
- Connection management with auto-reconnect
- Data logging to CSV files
- Statistics display and alerts
- Modern, responsive GUI built with tkinter

Requirements:
pip install bleak matplotlib pandas
"""

import asyncio
import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
from datetime import datetime
from collections import deque
import threading
import csv
from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError
import struct
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import matplotlib.animation as animation
import time
import traceback

# =============================================================================
# BLE CONFIGURATION
# =============================================================================

# SPL Meter (XIAO MG24) Configuration
SPL_DEVICE_NAME = "SPL_Meter"
SPL_SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214"
SPL_CHAR_UUID = "19b10001-e8f2-537e-4f6c-d104768a1214"  # lowercase

# AI Vision Node (ESP32-C3) Configuration
VISION_DEVICE_NAME = "AIVisionNode"
VISION_SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
VISION_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"

# Connection parameters
SCAN_TIMEOUT = 15.0
RECONNECT_DELAY = 5.0
CONNECTION_TIMEOUT = 20.0

# Data buffer size
MAX_DATA_POINTS = 100

# =============================================================================
# DATA LOGGING
# =============================================================================

class DataLogger:
    """Handles CSV logging of sensor data."""
    
    def __init__(self, filename_prefix="sensor_data"):
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.filename = f"{filename_prefix}_{timestamp}.csv"
        self.file = None
        self.writer = None
        self._init_file()
    
    def _init_file(self):
        """Initialize CSV file with headers."""
        self.file = open(self.filename, 'w', newline='')
        self.writer = csv.writer(self.file)
        self.writer.writerow(['Timestamp', 'SPL_dBA', 'People_Count'])
        self.file.flush()
    
    def log(self, spl_value, people_count):
        """Log a data point."""
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
        self.writer.writerow([timestamp, spl_value, people_count])
        self.file.flush()
    
    def close(self):
        """Close the log file."""
        if self.file:
            self.file.close()

# =============================================================================
# SIMPLIFIED BLE MANAGER
# =============================================================================

class SimpleBLEManager:
    """Simplified BLE manager with sequential connection handling."""
    
    def __init__(self, gui_callback, log_callback):
        self.gui_callback = gui_callback
        self.log_callback = log_callback
        self.running = False
        
        # Device addresses
        self.spl_address = None
        self.vision_address = None
        
        # Clients
        self.spl_client = None
        self.vision_client = None
        
        # Connection states
        self.spl_connected = False
        self.vision_connected = False
        
        # Latest values
        self.spl_value = 0.0
        self.people_count = 0
        
        # Last data timestamps
        self.spl_last_data = 0
        self.vision_last_data = 0
    
    def log(self, message):
        """Send log message to GUI."""
        self.log_callback(f"[{datetime.now().strftime('%H:%M:%S')}] {message}")
    
    async def scan_for_devices(self):
        """Scan for both devices."""
        self.log("Starting BLE scan...")
        
        try:
            devices = await BleakScanner.discover(timeout=SCAN_TIMEOUT)
            
            self.log(f"Found {len(devices)} BLE devices")
            
            for device in devices:
                if device.name:
                    self.log(f"  - {device.name} [{device.address}]")
                    
                    if device.name == SPL_DEVICE_NAME and not self.spl_address:
                        self.spl_address = device.address
                        self.log(f"✓ Found SPL Meter: {device.address}")
                    
                    elif device.name == VISION_DEVICE_NAME and not self.vision_address:
                        self.vision_address = device.address
                        self.log(f"✓ Found Vision Node: {device.address}")
            
            if not self.spl_address:
                self.log(f"⚠ SPL Meter '{SPL_DEVICE_NAME}' not found")
            if not self.vision_address:
                self.log(f"⚠ Vision Node '{VISION_DEVICE_NAME}' not found")
                
            return self.spl_address or self.vision_address
            
        except Exception as e:
            self.log(f"❌ Scan error: {e}")
            return False
    
    def spl_notification_handler(self, sender, data):
        """Handle SPL meter notifications."""
        try:
            self.spl_value = struct.unpack('<f', data)[0]
            self.spl_last_data = time.time()
            self.gui_callback('spl', self.spl_value)
        except Exception as e:
            self.log(f"Error parsing SPL data: {e}")
    
    def vision_notification_handler(self, sender, data):
        """Handle vision node notifications."""
        try:
            self.people_count = int(data[0])
            self.vision_last_data = time.time()
            self.gui_callback('vision', self.people_count)
        except Exception as e:
            self.log(f"Error parsing Vision data: {e}")
    
    async def connect_spl(self):
        """Connect to SPL Meter."""
        if not self.spl_address:
            return False
        
        try:
            self.log(f"Connecting to SPL Meter ({self.spl_address})...")
            
            # Disconnect if already connected
            if self.spl_client:
                try:
                    if self.spl_client.is_connected:
                        await self.spl_client.disconnect()
                        await asyncio.sleep(1)
                except:
                    pass
                self.spl_client = None
            
            # Create new client
            self.spl_client = BleakClient(
                self.spl_address,
                timeout=CONNECTION_TIMEOUT
            )
            
            # Connect
            await self.spl_client.connect()
            await asyncio.sleep(1)  # Wait for services to be ready
            
            if not self.spl_client.is_connected:
                self.log("❌ SPL connection failed")
                return False
            
            self.log("✓ SPL Meter connected")
            
            # Start notifications
            await self.spl_client.start_notify(
                SPL_CHAR_UUID,
                self.spl_notification_handler
            )
            
            self.spl_connected = True
            self.spl_last_data = time.time()
            self.log("✓ SPL notifications started")
            
            return True
            
        except Exception as e:
            self.log(f"❌ SPL connection error: {e}")
            self.spl_connected = False
            return False
    
    async def connect_vision(self):
        """Connect to Vision Node."""
        if not self.vision_address:
            return False
        
        try:
            self.log(f"Connecting to Vision Node ({self.vision_address})...")
            
            # Disconnect if already connected
            if self.vision_client:
                try:
                    if self.vision_client.is_connected:
                        await self.vision_client.disconnect()
                        await asyncio.sleep(1)
                except:
                    pass
                self.vision_client = None
            
            # Create new client
            self.vision_client = BleakClient(
                self.vision_address,
                timeout=CONNECTION_TIMEOUT
            )
            
            # Connect
            await self.vision_client.connect()
            await asyncio.sleep(1)  # Wait for services to be ready
            
            if not self.vision_client.is_connected:
                self.log("❌ Vision connection failed")
                return False
            
            self.log("✓ Vision Node connected")
            
            # Start notifications
            await self.vision_client.start_notify(
                VISION_CHAR_UUID,
                self.vision_notification_handler
            )
            
            self.vision_connected = True
            self.vision_last_data = time.time()
            self.log("✓ Vision notifications started")
            
            return True
            
        except Exception as e:
            self.log(f"❌ Vision connection error: {e}")
            self.vision_connected = False
            return False
    
    async def check_connections(self):
        """Check if connections are still alive."""
        current_time = time.time()
        
        # Check SPL connection
        if self.spl_connected:
            if self.spl_client and not self.spl_client.is_connected:
                self.log("⚠ SPL Meter disconnected")
                self.spl_connected = False
            elif current_time - self.spl_last_data > 15:
                self.log("⚠ SPL Meter no data (timeout)")
                self.spl_connected = False
        
        # Check Vision connection
        if self.vision_connected:
            if self.vision_client and not self.vision_client.is_connected:
                self.log("⚠ Vision Node disconnected")
                self.vision_connected = False
            elif current_time - self.vision_last_data > 15:
                self.log("⚠ Vision Node no data (timeout)")
                self.vision_connected = False
    
    async def connection_loop(self):
        """Main connection management loop."""
        while self.running:
            try:
                # Check current connections
                await self.check_connections()
                
                # Try to connect SPL if not connected
                if not self.spl_connected and self.spl_address:
                    await self.connect_spl()
                    await asyncio.sleep(2)
                
                # Try to connect Vision if not connected
                if not self.vision_connected and self.vision_address:
                    await self.connect_vision()
                    await asyncio.sleep(2)
                
                # If both failed and we don't have addresses, rescan
                if not self.spl_connected and not self.vision_connected:
                    if not self.spl_address or not self.vision_address:
                        self.log("Rescanning for devices...")
                        await self.scan_for_devices()
                
                # Wait before next check
                await asyncio.sleep(10)
                
            except Exception as e:
                self.log(f"❌ Error in connection loop: {e}")
                await asyncio.sleep(5)
    
    async def disconnect_all(self):
        """Disconnect all devices."""
        self.log("Disconnecting all devices...")
        
        if self.spl_client:
            try:
                if self.spl_client.is_connected:
                    await self.spl_client.disconnect()
            except:
                pass
        
        if self.vision_client:
            try:
                if self.vision_client.is_connected:
                    await self.vision_client.disconnect()
            except:
                pass
    
    async def run(self):
        """Main run loop."""
        self.running = True
        
        try:
            # Initial scan
            await self.scan_for_devices()
            
            # Initial connections (sequential)
            if self.spl_address:
                await self.connect_spl()
                await asyncio.sleep(2)
            
            if self.vision_address:
                await self.connect_vision()
                await asyncio.sleep(2)
            
            # Start connection monitoring loop
            await self.connection_loop()
            
        except Exception as e:
            self.log(f"❌ Fatal error: {e}")
            traceback.print_exc()
        finally:
            await self.disconnect_all()

# =============================================================================
# GUI APPLICATION
# =============================================================================

class EnvironmentalDashboard:
    """Main GUI application."""
    
    def __init__(self, root):
        self.root = root
        self.root.title("Environmental Monitoring Dashboard")
        self.root.geometry("1400x900")
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        
        # Data buffers
        self.spl_data = deque(maxlen=MAX_DATA_POINTS)
        self.people_data = deque(maxlen=MAX_DATA_POINTS)
        
        # Statistics
        self.spl_min = float('inf')
        self.spl_max = float('-inf')
        self.spl_avg_sum = 0
        self.spl_count = 0
        
        # Data logger
        self.logger = DataLogger()
        
        # BLE Manager
        self.ble_manager = SimpleBLEManager(self.on_data_received, self.on_log_message)
        self.ble_thread = None
        
        # Setup GUI
        self.setup_gui()
        
        # Start BLE
        self.start_ble()
    
    def setup_gui(self):
        """Create the GUI layout."""
        
        # Top panel
        top_frame = ttk.Frame(self.root, padding="10")
        top_frame.pack(fill=tk.X)
        
        ttk.Label(top_frame, text="Environmental Monitoring Dashboard", 
                 font=('Arial', 16, 'bold')).pack(side=tk.LEFT)
        
        self.status_label = ttk.Label(top_frame, text="Status: Initializing...", 
                                     foreground="orange")
        self.status_label.pack(side=tk.RIGHT, padx=10)
        
        # Main content
        content_frame = ttk.Frame(self.root)
        content_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        # Left panel
        left_panel = ttk.Frame(content_frame, width=300)
        left_panel.pack(side=tk.LEFT, fill=tk.BOTH, padx=5)
        left_panel.pack_propagate(False)
        
        # SPL Display
        spl_frame = ttk.LabelFrame(left_panel, text="SPL Meter", padding="10")
        spl_frame.pack(fill=tk.X, pady=5)
        
        self.spl_value_label = ttk.Label(spl_frame, text="-- dBA", 
                                        font=('Arial', 28, 'bold'))
        self.spl_value_label.pack()
        
        self.spl_status = ttk.Label(spl_frame, text="Disconnected", foreground="red")
        self.spl_status.pack()
        
        ttk.Separator(spl_frame, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=5)
        
        self.spl_min_label = ttk.Label(spl_frame, text="Min: --")
        self.spl_min_label.pack(anchor=tk.W)
        self.spl_max_label = ttk.Label(spl_frame, text="Max: --")
        self.spl_max_label.pack(anchor=tk.W)
        self.spl_avg_label = ttk.Label(spl_frame, text="Avg: --")
        self.spl_avg_label.pack(anchor=tk.W)
        
        # Vision Display
        vision_frame = ttk.LabelFrame(left_panel, text="Vision Node", padding="10")
        vision_frame.pack(fill=tk.X, pady=5)
        
        self.people_value_label = ttk.Label(vision_frame, text="0 people", 
                                           font=('Arial', 28, 'bold'))
        self.people_value_label.pack()
        
        self.vision_status = ttk.Label(vision_frame, text="Disconnected", foreground="red")
        self.vision_status.pack()
        
        # Log display
        log_frame = ttk.LabelFrame(left_panel, text="Connection Log", padding="5")
        log_frame.pack(fill=tk.BOTH, expand=True, pady=5)
        
        self.log_text = scrolledtext.ScrolledText(log_frame, height=15, width=35,
                                                   font=('Courier', 8))
        self.log_text.pack(fill=tk.BOTH, expand=True)
        
        # Right panel - Graphs
        right_panel = ttk.Frame(content_frame)
        right_panel.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=5)
        
        # Matplotlib
        self.fig = Figure(figsize=(10, 8), dpi=100)
        
        self.ax1 = self.fig.add_subplot(211)
        self.ax1.set_title('Sound Pressure Level (dBA)')
        self.ax1.set_xlabel('Samples')
        self.ax1.set_ylabel('dBA')
        self.ax1.grid(True, alpha=0.3)
        self.line1, = self.ax1.plot([], [], 'b-', linewidth=2)
        
        self.ax2 = self.fig.add_subplot(212)
        self.ax2.set_title('Occupancy (People Count)')
        self.ax2.set_xlabel('Samples')
        self.ax2.set_ylabel('Count')
        self.ax2.grid(True, alpha=0.3)
        self.line2, = self.ax2.plot([], [], 'g-', linewidth=2, marker='o')
        
        self.fig.tight_layout()
        
        self.canvas = FigureCanvasTkAgg(self.fig, master=right_panel)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        # Animation
        self.ani = animation.FuncAnimation(
            self.fig, self.update_plots, interval=1000, blit=False
        )
        
        # Status check timer
        self.root.after(1000, self.update_status)
    
    def start_ble(self):
        """Start BLE in background."""
        def run():
            loop = asyncio.new_event_loop()
            asyncio.set_event_loop(loop)
            loop.run_until_complete(self.ble_manager.run())
        
        self.ble_thread = threading.Thread(target=run, daemon=True)
        self.ble_thread.start()
    
    def on_log_message(self, message):
        """Handle log message from BLE manager."""
        def update():
            self.log_text.insert(tk.END, message + '\n')
            self.log_text.see(tk.END)
        self.root.after(0, update)
    
    def update_status(self):
        """Update status indicators."""
        # SPL status
        if self.ble_manager.spl_connected:
            self.spl_status.config(text="Connected ✓", foreground="green")
        else:
            self.spl_status.config(text="Disconnected ✗", foreground="red")
        
        # Vision status
        if self.ble_manager.vision_connected:
            self.vision_status.config(text="Connected ✓", foreground="green")
        else:
            self.vision_status.config(text="Disconnected ✗", foreground="red")
        
        # Overall status
        if self.ble_manager.spl_connected or self.ble_manager.vision_connected:
            count = sum([self.ble_manager.spl_connected, self.ble_manager.vision_connected])
            self.status_label.config(text=f"Status: {count}/2 devices connected", 
                                    foreground="green")
        else:
            self.status_label.config(text="Status: No devices connected", 
                                    foreground="red")
        
        self.root.after(1000, self.update_status)
    
    def on_data_received(self, sensor_type, value):
        """Handle incoming sensor data."""
        if sensor_type == 'spl':
            self.spl_data.append(value)
            
            if value < self.spl_min:
                self.spl_min = value
            if value > self.spl_max:
                self.spl_max = value
            self.spl_avg_sum += value
            self.spl_count += 1
            
            self.root.after(0, self.update_spl_display, value)
            
        elif sensor_type == 'vision':
            self.people_data.append(value)
            self.root.after(0, self.update_vision_display, value)
        
        # Log
        self.logger.log(self.ble_manager.spl_value, self.ble_manager.people_count)
    
    def update_spl_display(self, value):
        """Update SPL display."""
        self.spl_value_label.config(text=f"{value:.1f} dBA")
        self.spl_min_label.config(text=f"Min: {self.spl_min:.1f} dBA")
        self.spl_max_label.config(text=f"Max: {self.spl_max:.1f} dBA")
        
        if self.spl_count > 0:
            avg = self.spl_avg_sum / self.spl_count
            self.spl_avg_label.config(text=f"Avg: {avg:.1f} dBA")
    
    def update_vision_display(self, value):
        """Update vision display."""
        plural = "person" if value == 1 else "people"
        self.people_value_label.config(text=f"{value} {plural}")
    
    def update_plots(self, frame):
        """Update plots."""
        if len(self.spl_data) > 0:
            x = list(range(len(self.spl_data)))
            self.line1.set_data(x, list(self.spl_data))
            self.ax1.relim()
            self.ax1.autoscale_view()
        
        if len(self.people_data) > 0:
            x = list(range(len(self.people_data)))
            self.line2.set_data(x, list(self.people_data))
            self.ax2.relim()
            self.ax2.autoscale_view()
            if len(self.people_data) > 0:
                self.ax2.set_ylim(-0.5, max(self.people_data) + 1)
        
        return self.line1, self.line2
    
    def on_closing(self):
        """Handle window close."""
        if messagebox.askokcancel("Quit", "Do you want to quit?"):
            self.ble_manager.running = False
            self.logger.close()
            self.root.destroy()

# =============================================================================
# MAIN
# =============================================================================

def main():
    root = tk.Tk()
    app = EnvironmentalDashboard(root)
    root.mainloop()

if __name__ == "__main__":
    main()
