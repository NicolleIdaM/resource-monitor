import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import psutil
import time
import threading
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt
from datetime import datetime
import os

class ResourceMonitorDashboard:
    def __init__(self, root):
        self.root = root
        self.root.title("Resource Monitor - Dashboard")
        self.root.geometry("1200x800")
        self.root.configure(bg='#2c3e50')
        
        # Configurações de exportação
        self.export_directory = os.path.expanduser("../resource-monitor/scripts")  # Diretório padrão
        self.export_filename = "resource_monitor"  # Nome base do arquivo
        
        # Variáveis para armazenar dados
        self.cpu_data = []
        self.memory_data = []
        self.io_read_data = []
        self.io_write_data = []
        self.timestamps = []
        
        # Configurar estilo
        self.setup_styles()
        
        # Criar interface
        self.create_header()
        self.create_main_metrics()
        self.create_charts()
        self.create_bottom_panels()
        
        # Iniciar monitoramento
        self.monitoring = True
        self.start_monitoring()
    
    def setup_styles(self):
        self.style = ttk.Style()
        self.style.configure('Title.TLabel', 
                           background='#2c3e50', 
                           foreground='white', 
                           font=('Arial', 16, 'bold'))
        
        self.style.configure('Metric.TLabel', 
                           background='#34495e', 
                           foreground='white', 
                           font=('Arial', 12))
        
        self.style.configure('Value.TLabel', 
                           background='#34495e', 
                           foreground='#3498db', 
                           font=('Arial', 20, 'bold'))
        
        self.style.configure('Panel.TFrame', 
                           background='#34495e', 
                           relief='raised', 
                           borderwidth=1)
    
    def create_header(self):
        header_frame = ttk.Frame(self.root, style='Panel.TFrame')
        header_frame.pack(fill='x', padx=10, pady=10)
        
        title_label = ttk.Label(header_frame, 
                              text="RESOURCE MONITOR - TORRE DE CONTROLE", 
                              style='Title.TLabel')
        title_label.pack(pady=10)
    
    def create_main_metrics(self):
        metrics_frame = ttk.Frame(self.root, style='Panel.TFrame')
        metrics_frame.pack(fill='x', padx=10, pady=5)
        
        # OEE - Overall Equipment Effectiveness
        oee_frame = ttk.Frame(metrics_frame, style='Panel.TFrame')
        oee_frame.pack(side='left', fill='both', expand=True, padx=5, pady=5)
        
        ttk.Label(oee_frame, text="OEE", style='Title.TLabel').pack(pady=5)
        self.oee_value = ttk.Label(oee_frame, text="0.00%", style='Value.TLabel')
        self.oee_value.pack(pady=10)
        
        # Barra de progresso OEE
        self.oee_progress = ttk.Progressbar(oee_frame, orient='horizontal', length=200, mode='determinate')
        self.oee_progress.pack(pady=5)
        
        # Sub-métricas OEE
        sub_metrics_frame = ttk.Frame(oee_frame, style='Panel.TFrame')
        sub_metrics_frame.pack(fill='x', padx=10, pady=10)
        
        # Disponibilidade
        disponibilidade_frame = ttk.Frame(sub_metrics_frame, style='Panel.TFrame')
        disponibilidade_frame.pack(side='left', fill='both', expand=True, padx=5)
        
        ttk.Label(disponibilidade_frame, text="Disponibilidade", style='Metric.TLabel').pack()
        self.disponibilidade_value = ttk.Label(disponibilidade_frame, text="0.00%", style='Value.TLabel')
        self.disponibilidade_value.pack()
        
        # Performance
        performance_frame = ttk.Frame(sub_metrics_frame, style='Panel.TFrame')
        performance_frame.pack(side='left', fill='both', expand=True, padx=5)
        
        ttk.Label(performance_frame, text="Performance", style='Metric.TLabel').pack()
        self.performance_value = ttk.Label(performance_frame, text="0.00%", style='Value.TLabel')
        self.performance_value.pack()
        
        # Qualidade
        qualidade_frame = ttk.Frame(sub_metrics_frame, style='Panel.TFrame')
        qualidade_frame.pack(side='left', fill='both', expand=True, padx=5)
        
        ttk.Label(qualidade_frame, text="Qualidade", style='Metric.TLabel').pack()
        self.qualidade_value = ttk.Label(qualidade_frame, text="0.00%", style='Value.TLabel')
        self.qualidade_value.pack()
    
    def create_charts(self):
        charts_frame = ttk.Frame(self.root, style='Panel.TFrame')
        charts_frame.pack(fill='both', expand=True, padx=10, pady=5)
        
        # Gráfico de CPU e Memória
        fig_cpu_mem = Figure(figsize=(8, 4), dpi=100, facecolor='#34495e')
        self.ax_cpu_mem = fig_cpu_mem.add_subplot(111)
        self.ax_cpu_mem.set_facecolor('#2c3e50')
        self.ax_cpu_mem.tick_params(colors='white')
        self.ax_cpu_mem.set_ylabel('Uso (%)', color='white')
        self.ax_cpu_mem.set_xlabel('Tempo', color='white')
        self.ax_cpu_mem.set_title('CPU e Memória', color='white', pad=20)
        
        self.canvas_cpu_mem = FigureCanvasTkAgg(fig_cpu_mem, charts_frame)
        self.canvas_cpu_mem.get_tk_widget().pack(side='left', fill='both', expand=True, padx=5, pady=5)
        
        # Gráfico de I/O
        fig_io = Figure(figsize=(8, 4), dpi=100, facecolor='#34495e')
        self.ax_io = fig_io.add_subplot(111)
        self.ax_io.set_facecolor('#2c3e50')
        self.ax_io.tick_params(colors='white')
        self.ax_io.set_ylabel('Bytes/s', color='white')
        self.ax_io.set_xlabel('Tempo', color='white')
        self.ax_io.set_title('I/O - Leitura e Escrita', color='white', pad=20)
        
        self.canvas_io = FigureCanvasTkAgg(fig_io, charts_frame)
        self.canvas_io.get_tk_widget().pack(side='left', fill='both', expand=True, padx=5, pady=5)
    
    def create_bottom_panels(self):
        bottom_frame = ttk.Frame(self.root, style='Panel.TFrame')
        bottom_frame.pack(fill='both', expand=True, padx=10, pady=5)
        
        # Ranking de Processos (CPU)
        processos_frame = ttk.Frame(bottom_frame, style='Panel.TFrame')
        processos_frame.pack(side='left', fill='both', expand=True, padx=5, pady=5)
        
        ttk.Label(processos_frame, text="Ranking Processos (CPU)", style='Title.TLabel').pack(pady=5)
        
        # Treeview para processos
        columns = ('PID', 'Nome', 'CPU%', 'Memória')
        self.process_tree = ttk.Treeview(processos_frame, columns=columns, show='headings', height=8)
        
        for col in columns:
            self.process_tree.heading(col, text=col)
            self.process_tree.column(col, width=100)
        
        self.process_tree.pack(fill='both', expand=True, padx=10, pady=10)
        
        # Métricas do Sistema
        system_frame = ttk.Frame(bottom_frame, style='Panel.TFrame')
        system_frame.pack(side='left', fill='both', expand=True, padx=5, pady=5)
        
        ttk.Label(system_frame, text="Métricas do Sistema", style='Title.TLabel').pack(pady=5)
        
        # Informações do sistema
        info_text = tk.Text(system_frame, height=8, width=40, bg='#34495e', fg='white', 
                           font=('Arial', 10), relief='flat')
        info_text.pack(fill='both', expand=True, padx=10, pady=10)
        self.info_text = info_text
        
        # Botões de controle
        control_frame = ttk.Frame(bottom_frame, style='Panel.TFrame')
        control_frame.pack(side='left', fill='both', expand=True, padx=5, pady=5)
        
        ttk.Label(control_frame, text="Controles", style='Title.TLabel').pack(pady=5)
        
        ttk.Button(control_frame, text="Iniciar Monitoramento", 
                  command=self.start_monitoring).pack(pady=5, fill='x', padx=20)
        ttk.Button(control_frame, text="Parar Monitoramento", 
                  command=self.stop_monitoring).pack(pady=5, fill='x', padx=20)
        ttk.Button(control_frame, text="Exportar Dados", 
                  command=self.export_data_dialog).pack(pady=5, fill='x', padx=20)
        ttk.Button(control_frame, text="Configurar Exportação", 
                  command=self.configure_export).pack(pady=5, fill='x', padx=20)
        ttk.Button(control_frame, text="Sair", 
                  command=self.root.quit).pack(pady=5, fill='x', padx=20)
    
    def configure_export(self):
        """Abre diálogo para configurar exportação"""
        config_window = tk.Toplevel(self.root)
        config_window.title("Configurar Exportação")
        config_window.geometry("400x200")
        config_window.configure(bg='#2c3e50')
        
        tk.Label(config_window, text="Configurações de Exportação", 
                bg='#2c3e50', fg='white', font=('Arial', 14, 'bold')).pack(pady=10)
        
        # Nome do arquivo
        filename_frame = tk.Frame(config_window, bg='#2c3e50')
        filename_frame.pack(fill='x', padx=20, pady=5)
        
        tk.Label(filename_frame, text="Nome do arquivo:", 
                bg='#2c3e50', fg='white').pack(side='left')
        
        filename_var = tk.StringVar(value=self.export_filename)
        filename_entry = tk.Entry(filename_frame, textvariable=filename_var, width=30)
        filename_entry.pack(side='left', padx=10)
        
        # Diretório
        directory_frame = tk.Frame(config_window, bg='#2c3e50')
        directory_frame.pack(fill='x', padx=20, pady=5)
        
        tk.Label(directory_frame, text="Diretório:", 
                bg='#2c3e50', fg='white').pack(side='left')
        
        directory_var = tk.StringVar(value=self.export_directory)
        directory_entry = tk.Entry(directory_frame, textvariable=directory_var, width=30)
        directory_entry.pack(side='left', padx=10)
        
        def browse_directory():
            directory = filedialog.askdirectory(initialdir=self.export_directory)
            if directory:
                directory_var.set(directory)
        
        ttk.Button(directory_frame, text="Procurar", 
                  command=browse_directory).pack(side='left', padx=5)
        
        def save_config():
            self.export_filename = filename_var.get()
            self.export_directory = directory_var.get()
            
            # Criar diretório se não existir
            os.makedirs(self.export_directory, exist_ok=True)
            
            messagebox.showinfo("Sucesso", 
                              f"Configurações salvas!\n"
                              f"Arquivo: {self.export_filename}\n"
                              f"Diretório: {self.export_directory}")
            config_window.destroy()
        
        def cancel_config():
            config_window.destroy()
        
        # Botões
        button_frame = tk.Frame(config_window, bg='#2c3e50')
        button_frame.pack(pady=20)
        
        ttk.Button(button_frame, text="Salvar", 
                  command=save_config).pack(side='left', padx=10)
        ttk.Button(button_frame, text="Cancelar", 
                  command=cancel_config).pack(side='left', padx=10)
    
    def export_data_dialog(self):
        """Diálogo para exportar dados"""
        if not self.timestamps:
            messagebox.showwarning("Aviso", "Nenhum dado coletado para exportar!")
            return
        
        # Perguntar formato
        format_window = tk.Toplevel(self.root)
        format_window.title("Exportar Dados")
        format_window.geometry("300x150")
        format_window.configure(bg='#2c3e50')
        
        tk.Label(format_window, text="Escolha o formato de exportação:", 
                bg='#2c3e50', fg='white', font=('Arial', 12)).pack(pady=20)
        
        def export_csv():
            format_window.destroy()
            self.export_data_csv()
        
        def export_txt():
            format_window.destroy()
            self.export_data_txt()
        
        def export_both():
            format_window.destroy()
            self.export_data_csv()
            self.export_data_txt()
        
        button_frame = tk.Frame(format_window, bg='#2c3e50')
        button_frame.pack(pady=10)
        
        ttk.Button(button_frame, text="CSV", 
                  command=export_csv).pack(side='left', padx=5)
        ttk.Button(button_frame, text="TXT", 
                  command=export_txt).pack(side='left', padx=5)
        ttk.Button(button_frame, text="Ambos", 
                  command=export_both).pack(side='left', padx=5)
        
        ttk.Button(format_window, text="Cancelar", 
                  command=format_window.destroy).pack(pady=10)
    
    def export_data_csv(self):
        """Exportar dados para CSV"""
        try:
            # Criar nome do arquivo com timestamp
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            filename = f"{self.export_filename}_{timestamp}.csv"
            filepath = os.path.join(self.export_directory, filename)
            
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write("Timestamp,CPU%,Memory%,Read_MB,Write_MB\n")
                for i, timestamp in enumerate(self.timestamps):
                    cpu = self.cpu_data[i] if i < len(self.cpu_data) else 0
                    memory = self.memory_data[i] if i < len(self.memory_data) else 0
                    read_mb = self.io_read_data[i] if i < len(self.io_read_data) else 0
                    write_mb = self.io_write_data[i] if i < len(self.io_write_data) else 0
                    
                    f.write(f"{timestamp},{cpu:.2f},{memory:.2f},{read_mb:.2f},{write_mb:.2f}\n")
            
            messagebox.showinfo("Sucesso", f"Dados exportados para:\n{filepath}")
            print(f"Dados exportados para {filepath}")
            
        except Exception as e:
            error_msg = f"Erro ao exportar dados: {e}"
            messagebox.showerror("Erro", error_msg)
            print(error_msg)
    
    def export_data_txt(self):
        """Exportar dados para TXT (formato legível)"""
        try:
            # Criar nome do arquivo com timestamp
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            filename = f"{self.export_filename}_{timestamp}.txt"
            filepath = os.path.join(self.export_directory, filename)
            
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write("=== RELATÓRIO DE MONITORAMENTO DE SISTEMA ===\n")
                f.write(f"Gerado em: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
                f.write("=" * 50 + "\n\n")
                
                f.write("DADOS COLETADOS:\n")
                f.write("-" * 30 + "\n")
                
                for i, timestamp in enumerate(self.timestamps):
                    cpu = self.cpu_data[i] if i < len(self.cpu_data) else 0
                    memory = self.memory_data[i] if i < len(self.memory_data) else 0
                    read_mb = self.io_read_data[i] if i < len(self.io_read_data) else 0
                    write_mb = self.io_write_data[i] if i < len(self.io_write_data) else 0
                    
                    f.write(f"Timestamp: {timestamp}\n")
                    f.write(f"  CPU: {cpu:.2f}%\n")
                    f.write(f"  Memória: {memory:.2f}%\n")
                    f.write(f"  Leitura: {read_mb:.2f} MB\n")
                    f.write(f"  Escrita: {write_mb:.2f} MB\n")
                    f.write("-" * 30 + "\n")
                
                # Estatísticas resumidas
                if self.cpu_data:
                    f.write("\nESTATÍSTICAS RESUMIDAS:\n")
                    f.write("-" * 30 + "\n")
                    f.write(f"CPU Médio: {sum(self.cpu_data)/len(self.cpu_data):.2f}%\n")
                    f.write(f"CPU Máximo: {max(self.cpu_data):.2f}%\n")
                    f.write(f"CPU Mínimo: {min(self.cpu_data):.2f}%\n")
                    f.write(f"Memória Média: {sum(self.memory_data)/len(self.memory_data):.2f}%\n")
            
            messagebox.showinfo("Sucesso", f"Dados exportados para:\n{filepath}")
            print(f"Dados exportados para {filepath}")
            
        except Exception as e:
            error_msg = f"Erro ao exportar dados: {e}"
            messagebox.showerror("Erro", error_msg)
            print(error_msg)
    
    def calculate_oee(self, cpu_usage, memory_usage, io_efficiency):
        """Calcula OEE baseado no uso de recursos"""
        disponibilidade = max(0, 100 - (cpu_usage * 0.3 + memory_usage * 0.2))
        performance = max(0, 100 - (cpu_usage * 0.4))
        qualidade = max(0, 100 - (io_efficiency * 0.3))
        
        oee = (disponibilidade * performance * qualidade) / 10000
        
        return oee, disponibilidade, performance, qualidade
    
    def update_metrics(self):
        while self.monitoring:
            try:
                # Coletar métricas do sistema
                cpu_percent = psutil.cpu_percent(interval=1)
                memory = psutil.virtual_memory()
                disk_io = psutil.disk_io_counters()
                
                # Coletar processos
                processes = []
                for proc in psutil.process_iter(['pid', 'name', 'cpu_percent', 'memory_percent']):
                    try:
                        processes.append((
                            proc.info['pid'],
                            proc.info['name'],
                            proc.info['cpu_percent'],
                            proc.info['memory_percent']
                        ))
                    except (psutil.NoSuchProcess, psutil.AccessDenied):
                        pass
                
                # Ordenar por uso de CPU
                processes.sort(key=lambda x: x[2], reverse=True)
                top_processes = processes[:10]
                
                # Calcular eficiência de I/O
                io_efficiency = 0
                if disk_io and hasattr(disk_io, 'read_bytes') and hasattr(disk_io, 'write_bytes'):
                    total_io = disk_io.read_bytes + disk_io.write_bytes
                    io_efficiency = min(100, total_io / (1024 * 1024))  # Normalizar
                
                # Calcular OEE
                oee, disponibilidade, performance, qualidade = self.calculate_oee(
                    cpu_percent, memory.percent, io_efficiency
                )
                
                # Atualizar interface na thread principal
                self.root.after(0, self.update_interface, 
                               cpu_percent, memory.percent, disk_io, 
                               oee, disponibilidade, performance, qualidade, 
                               top_processes)
                
                time.sleep(2)  # Atualizar a cada 2 segundos
                
            except Exception as e:
                print(f"Erro ao coletar métricas: {e}")
                time.sleep(5)
    
    def update_interface(self, cpu_usage, memory_usage, disk_io, 
                        oee, disponibilidade, performance, qualidade, top_processes):
        # Atualizar valores OEE
        self.oee_value.config(text=f"{oee:.2f}%")
        self.oee_progress['value'] = oee
        
        self.disponibilidade_value.config(text=f"{disponibilidade:.2f}%")
        self.performance_value.config(text=f"{performance:.2f}%")
        self.qualidade_value.config(text=f"{qualidade:.2f}%")
        
        # Atualizar gráficos
        self.update_charts(cpu_usage, memory_usage, disk_io)
        
        # Atualizar lista de processos
        self.update_process_list(top_processes)
        
        # Atualizar informações do sistema
        self.update_system_info(cpu_usage, memory_usage, disk_io)
    
    def update_charts(self, cpu_usage, memory_usage, disk_io):
        # Adicionar novos dados
        current_time = datetime.now()
        self.timestamps.append(current_time)
        self.cpu_data.append(cpu_usage)
        self.memory_data.append(memory_usage)
        
        if disk_io:
            self.io_read_data.append(disk_io.read_bytes / 1024 / 1024)  # MB
            self.io_write_data.append(disk_io.write_bytes / 1024 / 1024)  # MB
        
        # Manter apenas os últimos 50 pontos
        if len(self.timestamps) > 50:
            self.timestamps.pop(0)
            self.cpu_data.pop(0)
            self.memory_data.pop(0)
            if self.io_read_data:
                self.io_read_data.pop(0)
            if self.io_write_data:
                self.io_write_data.pop(0)
        
        # Atualizar gráfico CPU/Memória
        self.ax_cpu_mem.clear()
        if len(self.timestamps) > 1:
            self.ax_cpu_mem.plot(self.timestamps, self.cpu_data, label='CPU %', color='#e74c3c', linewidth=2)
            self.ax_cpu_mem.plot(self.timestamps, self.memory_data, label='Memória %', color='#3498db', linewidth=2)
        self.ax_cpu_mem.legend(facecolor='#34495e', labelcolor='white')
        self.ax_cpu_mem.set_facecolor('#2c3e50')
        self.ax_cpu_mem.tick_params(colors='white')
        self.ax_cpu_mem.set_ylabel('Uso (%)', color='white')
        self.ax_cpu_mem.set_xlabel('Tempo', color='white')
        self.ax_cpu_mem.set_title('CPU e Memória', color='white', pad=20)
        
        # Atualizar gráfico I/O
        self.ax_io.clear()
        if len(self.timestamps) > 1 and self.io_read_data and self.io_write_data:
            self.ax_io.plot(self.timestamps, self.io_read_data, label='Leitura', color='#2ecc71', linewidth=2)
            self.ax_io.plot(self.timestamps, self.io_write_data, label='Escrita', color='#f39c12', linewidth=2)
        self.ax_io.legend(facecolor='#34495e', labelcolor='white')
        self.ax_io.set_facecolor('#2c3e50')
        self.ax_io.tick_params(colors='white')
        self.ax_io.set_ylabel('MB/s', color='white')
        self.ax_io.set_xlabel('Tempo', color='white')
        self.ax_io.set_title('I/O - Leitura e Escrita', color='white', pad=20)
        
        # Atualizar canvas
        self.canvas_cpu_mem.draw()
        self.canvas_io.draw()
    
    def update_process_list(self, processes):
        # Limpar lista atual
        for item in self.process_tree.get_children():
            self.process_tree.delete(item)
        
        # Adicionar novos processos
        for pid, name, cpu, memory in processes:
            self.process_tree.insert('', 'end', values=(
                pid, 
                name[:20],  # Limitar tamanho do nome
                f"{cpu:.1f}%", 
                f"{memory:.1f}%"
            ))
    
    def update_system_info(self, cpu_usage, memory_usage, disk_io):
        info_text = f"""=== INFORMAÇÕES DO SISTEMA ===

CPU: {cpu_usage:.1f}%
Memória: {memory_usage:.1f}%

--- Disco I/O ---
"""
        if disk_io:
            info_text += f"""Leitura: {disk_io.read_bytes / 1024 / 1024:.1f} MB
Escrita: {disk_io.write_bytes / 1024 / 1024:.1f} MB
"""

        info_text += f"""
--- Rede ---
Conexões ativas: {len(psutil.net_connections())}

--- Tempo ---
{datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
"""
        
        self.info_text.delete(1.0, tk.END)
        self.info_text.insert(1.0, info_text)
    
    def start_monitoring(self):
        self.monitoring = True
        monitor_thread = threading.Thread(target=self.update_metrics, daemon=True)
        monitor_thread.start()
    
    def stop_monitoring(self):
        self.monitoring = False

def main():
    root = tk.Tk()
    app = ResourceMonitorDashboard(root)
    root.mainloop()

if __name__ == "__main__":
    main()