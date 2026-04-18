import socket
import struct
import random
import time

CMD_SEND_CONFIG = 1
CMD_START_PROCESSING = 2
CMD_CHECK_STATUS = 3
CMD_GET_RESULT = 4
CMD_END_SESSION = 255

STATUS_PROCESSING = 10
STATUS_DONE = 11
STATUS_ERROR = 12

class ParallelClient:
    def __init__(self, host='127.0.0.1', port=666):
        self.host = host
        self.port = port
        self.sock = None

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((self.host, self.port))
        print(f"[Client] Successfully connected to {self.host}:{self.port}")

    def disconnect(self):
        if self.sock:
            try:
                self._send_header(CMD_END_SESSION, 0)
            except Exception:
                pass
            self.sock.close()
            self.sock = None
            print("[Client] Disconnected.")

    def _recv_exact(self, length):
        data = bytearray()
        while len(data) < length:
            packet = self.sock.recv(length - len(data))
            if not packet:
                raise ConnectionError("Connection closed by server unexpectedly.")
            data.extend(packet)
        return bytes(data)

    def _send_header(self, command_id, payload_length):
        header = struct.pack('!B I', command_id, payload_length)
        self.sock.sendall(header)

    def send_config(self, threads_num, matrix_size, matrix):
        config_data = struct.pack('!I I', threads_num, matrix_size)
        
        matrix_flat = []
        for row in matrix:
            matrix_flat.extend(row)
            
        matrix_data = struct.pack(f'!{len(matrix_flat)}i', *matrix_flat)
        
        payload = config_data + matrix_data
        
        self._send_header(CMD_SEND_CONFIG, len(payload))
        self.sock.sendall(payload)
        
        print(f"[Client] Sent config: threads={threads_num}, matrix_size={matrix_size}")

    def send_start(self):
        self._send_header(CMD_START_PROCESSING, 0)
        print("[Client] Sent command to start parallel processing.")

    def check_status(self):
        self._send_header(CMD_CHECK_STATUS, 0)
        
        header_data = self._recv_exact(5)
        cmd_id, msg_len = struct.unpack('!B I', header_data)
        
        if cmd_id != CMD_CHECK_STATUS or msg_len != 1:
            raise ValueError("Server returned an invalid status header.")
            
        status_data = self._recv_exact(1)
        status = struct.unpack('!B', status_data)[0]
        return status

    def get_result(self, matrix_size):
        self._send_header(CMD_GET_RESULT, 0)
        
        header_data = self._recv_exact(5)
        cmd_id, msg_len = struct.unpack('!B I', header_data)
        
        if cmd_id != CMD_GET_RESULT:
            raise ValueError(f"Expected CMD_GET_RESULT ({CMD_GET_RESULT}), got {cmd_id}")
            
        if msg_len == 0:
            return None
            
        expected_len = matrix_size * matrix_size * 4
        if msg_len != expected_len:
            raise ValueError(f"Payload size mismatch. Expected {expected_len}, got {msg_len}")
            
        matrix_data = self._recv_exact(msg_len)
        flat_matrix = struct.unpack(f'!{matrix_size * matrix_size}i', matrix_data)
        
        result = []
        for i in range(matrix_size):
            row = list(flat_matrix[i * matrix_size : (i + 1) * matrix_size])
            result.append(row)
            
        return result

def main():
    print("=== Python Parallel Client ===")
    
    HOST = '127.0.0.1'
    PORT = 666
    THREADS = 4
    MATRIX_SIZE = 12
    
    client = ParallelClient(host=HOST, port=PORT)
    
    try:
        client.connect()
        
        print(f"[Client] Generating random {MATRIX_SIZE}x{MATRIX_SIZE} matrix...")
        matrix = [[random.randint(-100, 100) for _ in range(MATRIX_SIZE)] for _ in range(MATRIX_SIZE)]
        client.send_config(THREADS, MATRIX_SIZE, matrix)
        
        client.send_start()
        
        print("[Client] Waiting for server processing to complete...")
        while True:
            status = client.check_status()
            if status == STATUS_DONE:
                print(f"[Client] Server returned STATUS_DONE ({status})")
                break
            elif status == STATUS_ERROR:
                print(f"[ERROR] Server returned STATUS_ERROR ({status}). Execution failed.")
                return
            elif status == STATUS_PROCESSING:
                pass
                
            time.sleep(0.05)
            
        print("[Client] Fetching result array...")
        result = client.get_result(MATRIX_SIZE)
        
        if result:
            print(f"\n[SUCCESS] Result matrix {len(result)}x{len(result)} fetched!")
            limit = min(len(result), 5)
            print(f"Top-left {limit}x{limit} slice preview:")
            for i in range(limit):
                print("\t".join(str(x) for x in result[i][:limit]))
        else:
            print("[Client] Received empty result. Matrix unavailable.")

    except ConnectionRefusedError:
        print(f"[CRITICAL] Could not connect to the server at {HOST}:{PORT}. Please ensure it is running.")
    except Exception as e:
        print(f"[CRITICAL] Error occurred during runtime: {e}")
    finally:
        client.disconnect()
        print("=== Execution Finished ===")

if __name__ == '__main__':
    main()
