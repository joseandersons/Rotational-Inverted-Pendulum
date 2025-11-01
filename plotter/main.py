import serial
import time
import collections
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from serial.tools import list_ports
import numpy as np
import argparse
import os
from dotenv import load_dotenv

# --- Carregar Configurações do .env ---
load_dotenv()

class SerialPlotterConfig:
    def __init__(self):
        # Configurações da porta serial
        self.serial_port = os.getenv('SERIAL_PORT', None)
        self.baud_rate = int(os.getenv('BAUD_RATE', '115200'))
        
        # Configurações do gráfico
        self.max_points = int(os.getenv('MAX_POINTS', '100'))
        self.update_interval = int(os.getenv('UPDATE_INTERVAL', '20'))
        
        # Quais gráficos mostrar
        self.show_angle = os.getenv('SHOW_ANGLE', 'true').lower() == 'true'
        self.show_error = os.getenv('SHOW_ERROR', 'true').lower() == 'true'
        self.show_pid = os.getenv('SHOW_PID', 'true').lower() == 'true'
        self.show_p_term = os.getenv('SHOW_P_TERM', 'true').lower() == 'true'
        self.show_i_term = os.getenv('SHOW_I_TERM', 'true').lower() == 'true'
        self.show_d_term = os.getenv('SHOW_D_TERM', 'true').lower() == 'true'

def parse_arguments():
    # Analisar argumentos da linha de comando
    parser = argparse.ArgumentParser(description='Serial Data Plotter for PID Control System')
    parser.add_argument('--port', type=str, help='Serial port (e.g., COM3, /dev/ttyUSB0)')
    parser.add_argument('--baud', type=int, help='Baud rate (e.g., 9600, 115200)')
    parser.add_argument('--list-ports', action='store_true', help='List available serial ports')
    return parser.parse_args()

def list_available_ports():
    # Listar todas as portas seriais disponíveis
    ports = list_ports.comports()
    print("Available serial ports:")
    for port in ports:
        print(f"  {port.device}: {port.description}")
    return [port.device for port in ports]

def auto_detect_port():
    # Tentar detectar automaticamente a porta serial
    ports = list_ports.comports()
    for port in ports:
        if any(keyword in port.device for keyword in ['USB', 'ACM', 'COM']):
            return port.device
    return None

def initialize_serial_connection(port, baud_rate):
    # Inicializar conexão serial com tratamento de erros
    if port is None:
        print("Error: No serial port specified.")
        print("Use --port argument or set SERIAL_PORT in .env file")
        return None
    
    try:
        ser = serial.Serial(port, baud_rate, timeout=0.1)
        print(f"Connected to {port} with {baud_rate} baud.")
        time.sleep(1)  # Aguarda a inicialização da conexão
        return ser
    except serial.SerialException as e:
        print(f"Error opening serial port {port}: {e}")
        return None

def setup_plots(config):
    # Determinar quantos subplots precisamos
    plot_count = sum([config.show_angle, config.show_error, config.show_pid])
    
    if plot_count == 0:
        print("Error: No plots enabled in configuration!")
        return None, None, [], []
    
    fig, axes = plt.subplots(nrows=plot_count, ncols=1, sharex=True, figsize=(10, 8))
    fig.suptitle('Análise do Sistema de Controle (PID)', fontsize=16)
    
    # Converter para lista se houver apenas um subplot
    if plot_count == 1:
        axes = [axes]
    
    lines = []
    current_axis = 0
    
    # --- Gráfico 1: Ângulo (th) ---
    if config.show_angle:
        line_th, = axes[current_axis].plot([], [], label='Ângulo (th)')
        axes[current_axis].set_title("1. Ângulo do Pêndulo (th)")
        axes[current_axis].set_ylabel("Ângulo (graus)")
        axes[current_axis].axhline(0, color='red', linestyle='--', lw=1)
        axes[current_axis].set_ylim(-190, 190)  
        axes[current_axis].grid(True)
        lines.append(('th', line_th))
        current_axis += 1
    
    # --- Gráfico 2: Erro (e) ---
    if config.show_error:
        line_e, = axes[current_axis].plot([], [], label='Erro (e)', color='green')
        axes[current_axis].set_title("2. Erro (e)")
        axes[current_axis].set_ylabel("Unidade do Erro")
        axes[current_axis].axhline(0, color='red', linestyle='--', lw=1)
        axes[current_axis].grid(True)
        lines.append(('e', line_e))
        current_axis += 1
    
    # --- Gráfico 3: Componentes PID ---
    if config.show_pid:
        pid_lines = []
        if config.show_p_term:
            line_p, = axes[current_axis].plot([], [], label='Termo P', color='b')
            pid_lines.append(('P', line_p))
        if config.show_i_term:
            line_i, = axes[current_axis].plot([], [], label='Termo I', color='r')
            pid_lines.append(('I', line_i))
        if config.show_d_term:
            line_d, = axes[current_axis].plot([], [], label='Termo D', color='purple')
            pid_lines.append(('D', line_d))
        
        axes[current_axis].set_title("3. Componentes PID")
        axes[current_axis].set_xlabel("Tempo (s)")
        axes[current_axis].set_ylabel("Saída de Controle")
        axes[current_axis].grid(True)
        axes[current_axis].legend(loc='upper left')
        lines.extend(pid_lines)
    
    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    return fig, axes, lines

def main():
    # Analisar argumentos da linha de comando
    args = parse_arguments()
    
    if args.list_ports:
        list_available_ports()
        return
    
    # Carregar configuração
    config = SerialPlotterConfig()
    
    # Sobrepor com argumentos da linha de comando
    if args.port:
        config.serial_port = args.port
    if args.baud:
        config.baud_rate = args.baud
    
    # Detectar porta automaticamente se não especificada
    if config.serial_port is None:
        config.serial_port = auto_detect_port()
        if config.serial_port:
            print(f"Auto-detected serial port: {config.serial_port}")
        else:
            print("Could not auto-detect serial port.")
            list_available_ports()
            print("Please specify a port using --port argument or SERIAL_PORT in .env")
            return
    
    print(f"Configuration:")
    print(f"  Port: {config.serial_port}")
    print(f"  Baud rate: {config.baud_rate}")
    print(f"  Max points: {config.max_points}")
    print(f"  Enabled plots: Angle={config.show_angle}, Error={config.show_error}, PID={config.show_pid}")
    
    # Inicializar conexão serial
    ser = initialize_serial_connection(config.serial_port, config.baud_rate)
    if ser is None:
        return
    
    # --- Buffers de dados
    tempos = collections.deque(maxlen=config.max_points)
    dados = {
        'th': collections.deque(maxlen=config.max_points), # Ângulo
        'e': collections.deque(maxlen=config.max_points),  # Erro
        'P': collections.deque(maxlen=config.max_points),  # Termo P
        'I': collections.deque(maxlen=config.max_points),  # Termo I
        'D': collections.deque(maxlen=config.max_points)   # Termo D
    }
    # Lista de chaves na ordem que o Arduino envia (th, e, P, I, D)
    chaves_dados = list(dados.keys())
    
    # Configurar plots
    fig, axes, lines = setup_plots(config)
    if fig is None:
        ser.close()
        return
    
    # Tempo inicial
    start_time = time.time()

    def init_plot():
        for _, line in lines:
            line.set_data([], [])
        return [line for _, line in lines]

    # Função de atualização 
    def update_plot(frame):
        try:
            while ser.in_waiting > 0:
                serial_line = ser.readline()
                
                try:
                    data_str = serial_line.decode('utf-8').strip()
                    if not data_str:
                        continue
                    
                    # Parsear o CSV
                    parts = data_str.split(',')
                    # Verificar se temos 5 valores
                    if len(parts) != 5:
                        print(f"Ignorando linha mal formatada: {data_str}")
                        continue

                    # Converter para float
                    valores = [float(p) for p in parts]
                    
                    tempo_atual = time.time() - start_time
                    tempos.append(tempo_atual)
                    
                    # Adicionar aos deques corretos
                    for i, key in enumerate(chaves_dados):
                        dados[key].append(valores[i])

                except (ValueError, UnicodeDecodeError) as e:
                    print(f"Erro ao processar: {serial_line} | {e}")
                    continue
            
            # Atualizar o gráfico 
            if tempos:
                # Atualiza o X-lim (tempo) para todos
                x_min = tempos[0]
                x_max = tempos[-1]
                for ax in axes:
                    ax.set_xlim(x_min, x_max)

                # Atualiza dados Y de cada linha
                for key, line in lines:
                    if key in dados and dados[key]:
                        line.set_data(tempos, dados[key])
                
                # Auto-escala para ax2 (Erro)
                if config.show_error and dados['e']:
                    min_e = min(dados['e'])
                    max_e = max(dados['e'])
                    axes[1 if config.show_angle else 0].set_ylim(
                        min_e - (max_e-min_e)*0.1 - 0.1, 
                        max_e + (max_e-min_e)*0.1 + 0.1
                    )
                
                # Auto-escala para ax3 (PID)
                if config.show_pid:
                    pid_axis_index = sum([config.show_angle, config.show_error])
                    pid_data = []
                    if config.show_p_term and dados['P']:
                        pid_data.extend(dados['P'])
                    if config.show_i_term and dados['I']:
                        pid_data.extend(dados['I'])
                    if config.show_d_term and dados['D']:
                        pid_data.extend(dados['D'])
                    
                    if pid_data:
                        min_pid = min(pid_data)
                        max_pid = max(pid_data)
                        axes[pid_axis_index].set_ylim(
                            min_pid - (max_pid-min_pid)*0.1 - 0.1, 
                            max_pid + (max_pid-min_pid)*0.1 + 0.1
                        )

        except Exception as e:
            print(f"Erro na função de atualização: {e}")
        
        return [line for _, line in lines]

    # --- Iniciar Animação
    ani = animation.FuncAnimation(
        fig, 
        update_plot, 
        init_func=init_plot, 
        blit=True,
        interval=config.update_interval, # 50Hz, deve bater com o delay do Arduino
        cache_frame_data=False
    )

    plt.tight_layout(rect=[0, 0.03, 1, 0.95]) # Ajusta layout
    plt.show()

    ser.close()
    print("Porta serial fechada. Programa finalizado.")

if __name__ == "__main__":
    main()