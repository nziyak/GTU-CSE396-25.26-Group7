import base64
from typing import Dict, Any
# pyrefly: ignore [missing-import]
from flask import Flask, request
from flask_socketio import SocketIO, emit
from comms_dashboard_interface import IWebDashboard, AugmentedStatusReport

class WebDashboard(IWebDashboard):
    """
    @brief Flask ve Flask-SocketIO kullanarak Unity ve Operatör kontrol arayüzü ile 
           haberleşmeyi sağlayan web dashboard sınıfı.
    """
    def __init__(self):
        # Flask uygulamasını başlatıyoruz
        self.app = Flask(__name__)
        # WebSocket sunucusunu başlatıyoruz. Tüm origin'lere izin veriyoruz (Unity için gerekli)
        self.socketio = SocketIO(self.app, cors_allowed_origins="*")
        
        self._setup_routes()

    def _setup_routes(self):
        """Flask endpoint'lerini ve WebSocket event handler'larını tanımlar."""
        @self.app.route('/')
        def index():
            return "MOD-04 Communications Dashboard is running. Waiting for Unity/Operator connection..."

        @self.socketio.on('connect')
        def handle_connect():
            print(f"[WebDashboard] İstemci bağlandı: {request.sid}")

        @self.socketio.on('disconnect')
        def handle_disconnect():
            print(f"[WebDashboard] İstemci bağlantıyı kesti: {request.sid}")
            
        @self.socketio.on('operator_command')
        def handle_operator_command(data):
            """Unity'den veya Web arayüzünden gelen manuel override komutlarını yakalar."""
            print(f"[WebDashboard] 'operator_command' eventi alındı: {data}")
            self.on_operator_command_received(data)

    def start_server(self, host: str = "0.0.0.0", port: int = 5000) -> None:
        """
        @brief Flask uygulamasını ve SocketIO sunucusunu başlatır. (Bloklayıcı çağrı)
        """
        print(f"[WebDashboard] Sunucu {host}:{port} adresinde başlatılıyor...")
        # allow_unsafe_werkzeug dev ortamı içindir. Production'da gunicorn/eventlet kullanılmalıdır.
        self.socketio.run(self.app, host=host, port=port, allow_unsafe_werkzeug=True)

    def broadcast_telemetry(self, report: AugmentedStatusReport) -> None:
        """
        @brief Sensör ve yapay zeka verilerini Unity'ye gönderir.
        """
        # Dataclass nesnesini JSON/dict formatına dönüştürüyoruz
        report_dict = {
            "posX": report.pos_x,
            "posY": report.pos_y,
            "temperature": report.temperature,
            "smokeDetected": report.smoke_detected,
            "victimStatus": report.victim_status,
            "isStuck": report.is_stuck,
            "priorityLevel": report.priority_level,
            "acousticHit": report.acoustic_hit,
            "acousticAngle": report.acoustic_angle
        }
        # 'telemetry_update' adında bir event fırlatıyoruz
        self.socketio.emit('telemetry_update', report_dict)

    def stream_video_frame(self, jpeg_bytes: bytes) -> None:
        """
        @brief Kamera karesini Unity veya Web Arayüzünde gösterilmek üzere yayınlar.
        """
        # Raw byte verisini base64'e çevirip Data URL olarak paketliyoruz
        b64_img = base64.b64encode(jpeg_bytes).decode('utf-8')
        data_url = f"data:image/jpeg;base64,{b64_img}"
        
        self.socketio.emit('video_frame', {'image': data_url})

    def on_operator_command_received(self, command_payload: Dict[str, Any]) -> None:
        """
        @brief Operatörden gelen komutu yakalayıp FSM'e (Mod 6) iletmek üzere hazırlanan callback.
               Ana sistem bu metodu override ederek/kanca (hook) atarak dinleyebilir.
        """
        print(f"[Dashboard Callback] Operatör Override Komutu Yakalandı: {command_payload}")
        # Bu kısımda gerçek sistem entegrasyonu (örneğin event_bus'a veya FSM kuyruğuna yazma) yapılır.
