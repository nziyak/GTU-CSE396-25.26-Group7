/* ====== Group 7 Documentation — Interactivity ====== */
(function(){
  'use strict';

  /* ---- Active nav highlight ---- */
  const page = location.pathname.split('/').pop() || 'index.html';
  document.querySelectorAll('.nav-link').forEach(link => {
    if(link.getAttribute('href') === page) link.classList.add('active');
    else link.classList.remove('active');
  });

  /* ---- Mobile hamburger ---- */
  const ham = document.querySelector('.hamburger');
  const navbarNav = document.querySelector('.navbar-nav');
  if(ham && navbarNav){
    ham.addEventListener('click', () => {
      navbarNav.classList.toggle('open');
    });
  }

  /* ---- Scroll observer fade ---- */
  const obsFade = new IntersectionObserver((entries) => {
    entries.forEach(e => { if(e.isIntersecting) e.target.classList.add('visible'); });
  }, { threshold: 0.08 });
  document.querySelectorAll('.obs-fade').forEach(el => obsFade.observe(el));

  /* ---- Tabs ---- */
  document.querySelectorAll('[data-tab-group]').forEach(group => {
    const buttons = group.querySelectorAll('.tab-btn');
    const container = group.closest('section') || group.parentElement;
    buttons.forEach(btn => {
      btn.addEventListener('click', () => {
        const target = btn.dataset.tab;
        buttons.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        container.querySelectorAll('.tab-panel').forEach(p => {
          p.classList.toggle('active', p.id === target);
        });
      });
    });
  });

  /* ---- Accordions ---- */
  document.querySelectorAll('.accordion-header').forEach(header => {
    header.addEventListener('click', () => {
      const item = header.parentElement;
      const wasOpen = item.classList.contains('open');
      // close siblings
      item.parentElement.querySelectorAll('.accordion-item.open').forEach(i => i.classList.remove('open'));
      if(!wasOpen) item.classList.add('open');
    });
  });

  /* ---- Animated counter ---- */
  function animateCounter(el, target, duration){
    let start = 0;
    const step = (ts) => {
      if(!start) start = ts;
      const progress = Math.min((ts - start) / duration, 1);
      el.textContent = Math.floor(progress * target);
      if(progress < 1) requestAnimationFrame(step);
      else el.textContent = target;
    };
    requestAnimationFrame(step);
  }
  const counterObs = new IntersectionObserver((entries) => {
    entries.forEach(e => {
      if(e.isIntersecting && !e.target.dataset.counted){
        e.target.dataset.counted = '1';
        animateCounter(e.target, parseInt(e.target.dataset.target), 1200);
      }
    });
  }, { threshold: 0.5 });
  document.querySelectorAll('[data-counter]').forEach(el => counterObs.observe(el));

  /* ---- IDE Code Explorer (technical page) ---- */
  const codeFiles = {
    uart_reader: {
      name: 'uart_reader.py',
      lang: 'Python',
      code: `<span class="kw">from</span> __future__ <span class="kw">import</span> annotations

<span class="kw">import</span> copy
<span class="kw">import</span> json
<span class="kw">import</span> logging
<span class="kw">import</span> threading
<span class="kw">import</span> time
<span class="kw">from</span> dataclasses <span class="kw">import</span> dataclass
<span class="kw">from</span> typing <span class="kw">import</span> Optional

<span class="kw">import</span> serial


logger = logging.getLogger(&quot;uart_reader&quot;)


@dataclass
<span class="kw">class</span> UartTelemetry:
    dist_front: int = <span class="num">0</span>
    dist_back: int = <span class="num">0</span>
    mic1: int = <span class="num">0</span>
    mic2: int = <span class="num">0</span>
    smoke: int = <span class="num">0</span>
    yaw: float = <span class="num">0.0</span>
    pitch: float = <span class="num">0.0</span>
    roll: float = <span class="num">0.0</span>
    temp: float = <span class="num">0.0</span>
    hum: float = <span class="num">0.0</span>
    raw_line: str = &quot;&quot;
    connected: bool = <span class="kw">False</span>


<span class="kw">class</span> SerialTelemetryReader:
    <span class="kw">def</span> __init__(self, port: str = &quot;/dev/ttyUSB0&quot;, baudrate: int = <span class="num">115200</span>, timeout: float = <span class="num">1.0</span>) -&gt; <span class="kw">None</span>:
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self._serial: Optional[serial.Serial] = <span class="kw">None</span>
        self._latest = UartTelemetry()
        self._lock = threading.Lock()
        self._serial_lock = threading.Lock()
        self._stop_event = threading.Event()
        self._thread: Optional[threading.Thread] = <span class="kw">None</span>

    <span class="kw">def</span> start(self) -&gt; <span class="kw">None</span>:
        self._serial = serial.Serial(self.port, self.baudrate, timeout=self.timeout)
        self._stop_event.clear()
        self._thread = threading.Thread(target=self._read_loop, name=&quot;UartReader&quot;, daemon=<span class="kw">True</span>)
        self._thread.start()

    <span class="kw">def</span> stop(self) -&gt; <span class="kw">None</span>:
        self._stop_event.set()
        <span class="kw">if</span> self._thread <span class="kw">is</span> <span class="kw">not</span> <span class="kw">None</span> <span class="kw">and</span> self._thread.is_alive():
            self._thread.join(timeout=<span class="num">2.0</span>)

        <span class="kw">if</span> self._serial <span class="kw">is</span> <span class="kw">not</span> <span class="kw">None</span>:
            <span class="kw">try</span>:
                self._serial.close()
            <span class="kw">except</span> Exception:
                <span class="kw">pass</span>

    <span class="kw">def</span> get_latest(self) -&gt; UartTelemetry:
        <span class="kw">with</span> self._lock:
            <span class="kw">return</span> copy.deepcopy(self._latest)

    <span class="kw">def</span> send_command(self, command: str) -&gt; bool:
        <span class="kw">if</span> self._serial <span class="kw">is</span> <span class="kw">None</span>:
            logger.warning(&quot;UART send failed: serial <span class="kw">is</span> <span class="kw">not</span> open&quot;)
            <span class="kw">return</span> <span class="kw">False</span>

        normalized = command.strip().upper()
        <span class="kw">if</span> <span class="kw">not</span> normalized:
            logger.warning(&quot;UART send failed: empty command&quot;)
            <span class="kw">return</span> <span class="kw">False</span>

        <span class="kw">try</span>:
            packet = f&quot;{normalized}\n&quot;.encode(&quot;utf-<span class="num">8</span>&quot;)
            <span class="kw">with</span> self._serial_lock:
                self._serial.write(packet)
                self._serial.flush()
            logger.info(&quot;UART command sent to Mega: %s&quot;, normalized)
            <span class="kw">return</span> <span class="kw">True</span>
        <span class="kw">except</span> Exception <span class="kw">as</span> exc:
            logger.exception(&quot;UART send failed: %s&quot;, exc)
            <span class="kw">return</span> <span class="kw">False</span>

    <span class="kw">def</span> _read_loop(self) -&gt; <span class="kw">None</span>:
        <span class="kw">while</span> <span class="kw">not</span> self._stop_event.is_set():
            <span class="kw">try</span>:
                <span class="kw">with</span> self._serial_lock:
                    line = self._serial.readline().decode(&quot;utf-<span class="num">8</span>&quot;, errors=&quot;ignore&quot;).strip()

                <span class="kw">if</span> <span class="kw">not</span> line:
                    <span class="kw">continue</span>

                <span class="kw">try</span>:
                    data = json.loads(line)
                <span class="kw">except</span> json.JSONDecodeError:
                    logger.warning(&quot;Bad UART JSON: %s&quot;, line)
                    <span class="kw">continue</span>

                telemetry = UartTelemetry(
                    dist_front=int(data.get(&quot;dist_front&quot;, <span class="num">0</span>)),
                    dist_back=int(data.get(&quot;dist_back&quot;, <span class="num">0</span>)),
                    mic1=int(data.get(&quot;mic1&quot;, <span class="num">0</span>)),
                    mic2=int(data.get(&quot;mic2&quot;, <span class="num">0</span>)),
                    smoke=int(data.get(&quot;smoke&quot;, <span class="num">0</span>)),
                    yaw=float(data.get(&quot;yaw&quot;, <span class="num">0.0</span>)),
                    pitch=float(data.get(&quot;pitch&quot;, <span class="num">0.0</span>)),
                    roll=float(data.get(&quot;roll&quot;, <span class="num">0.0</span>)),
                    temp=float(data.get(&quot;temp&quot;, <span class="num">0.0</span>)),
                    hum=float(data.get(&quot;hum&quot;, <span class="num">0.0</span>)),
                    raw_line=line,
                    connected=<span class="kw">True</span>,
                )

                <span class="kw">with</span> self._lock:
                    self._latest = telemetry

            <span class="kw">except</span> Exception <span class="kw">as</span> exc:
                logger.exception(&quot;UART read failed: %s&quot;, exc)
                time.sleep(<span class="num">0.5</span>)`
    },
    autonomy: {
      name: 'autonomy_controller.py',
      lang: 'Python',
      code: `<span class="kw">from</span> __future__ <span class="kw">import</span> annotations

<span class="kw">from</span> dataclasses <span class="kw">import</span> dataclass
<span class="kw">from</span> typing <span class="kw">import</span> Optional

<span class="kw">from</span> ai_vision_interface <span class="kw">import</span> TargetData
<span class="kw">from</span> uart_reader <span class="kw">import</span> UartTelemetry


@dataclass
<span class="kw">class</span> AutonomyDecision:
    mode: str
    command: str
    obstacle_detected: bool
    target_locked: bool
    target_severity: str
    reason: str


<span class="kw">class</span> AutonomyController:
    <span class="kw">def</span> __init__(
        self,
        *,
        frame_center_x: int = <span class="num">320</span>,
        center_tolerance_px: int = <span class="num">70</span>,
        obstacle_stop_cm: int = <span class="num">18</span>,
        target_stop_cm: int = <span class="num">60</span>,
    ) -&gt; <span class="kw">None</span>:
        self.frame_center_x = frame_center_x
        self.center_tolerance_px = center_tolerance_px
        self.obstacle_stop_cm = obstacle_stop_cm
        self.target_stop_cm = target_stop_cm
        self._next_turn = &quot;LEFT&quot;

    <span class="kw">def</span> decide(self, telemetry: UartTelemetry, target: Optional[TargetData]) -&gt; AutonomyDecision:
        front_distance = telemetry.dist_front
        obstacle_detected = front_distance &gt; <span class="num">0</span> <span class="kw">and</span> front_distance &lt;= self.obstacle_stop_cm

        <span class="kw">if</span> obstacle_detected:
            turn_command = self._consume_turn_direction()
            <span class="kw">return</span> AutonomyDecision(
                mode=&quot;AUTO_AVOID_OBSTACLE&quot;,
                command=turn_command,
                obstacle_detected=<span class="kw">True</span>,
                target_locked=<span class="kw">False</span>,
                target_severity=&quot;NONE&quot;,
                reason=f&quot;front_obstacle_{front_distance}cm&quot;,
            )

        <span class="kw">if</span> target <span class="kw">is</span> <span class="kw">not</span> <span class="kw">None</span> <span class="kw">and</span> str(target.severity).upper() <span class="kw">in</span> {&quot;TRAPPED&quot;, &quot;LYING&quot;}:
            severity = str(target.severity).upper()

            <span class="kw">if</span> target.distance_cm &lt;= self.target_stop_cm:
                <span class="kw">return</span> AutonomyDecision(
                    mode=&quot;AUTO_APPROACH_TARGET&quot;,
                    command=&quot;STOP&quot;,
                    obstacle_detected=<span class="kw">False</span>,
                    target_locked=<span class="kw">True</span>,
                    target_severity=severity,
                    reason=f&quot;target_close_{target.distance_cm:.1f}cm&quot;,
                )

            <span class="kw">if</span> target.pos_x &lt; self.frame_center_x - self.center_tolerance_px:
                <span class="kw">return</span> AutonomyDecision(
                    mode=&quot;AUTO_APPROACH_TARGET&quot;,
                    command=&quot;LEFT&quot;,
                    obstacle_detected=<span class="kw">False</span>,
                    target_locked=<span class="kw">True</span>,
                    target_severity=severity,
                    reason=&quot;target_left_of_center&quot;,
                )

            <span class="kw">if</span> target.pos_x &gt; self.frame_center_x + self.center_tolerance_px:
                <span class="kw">return</span> AutonomyDecision(
                    mode=&quot;AUTO_APPROACH_TARGET&quot;,
                    command=&quot;RIGHT&quot;,
                    obstacle_detected=<span class="kw">False</span>,
                    target_locked=<span class="kw">True</span>,
                    target_severity=severity,
                    reason=&quot;target_right_of_center&quot;,
                )

            <span class="kw">return</span> AutonomyDecision(
                mode=&quot;AUTO_APPROACH_TARGET&quot;,
                command=&quot;FORWARD&quot;,
                obstacle_detected=<span class="kw">False</span>,
                target_locked=<span class="kw">True</span>,
                target_severity=severity,
                reason=&quot;target_centered&quot;,
            )

        <span class="kw">return</span> AutonomyDecision(
            mode=&quot;AUTO_PATROL&quot;,
            command=&quot;FORWARD&quot;,
            obstacle_detected=<span class="kw">False</span>,
            target_locked=<span class="kw">False</span>,
            target_severity=str(target.severity).upper() <span class="kw">if</span> target <span class="kw">else</span> &quot;NONE&quot;,
            reason=&quot;clear_path_patrol&quot;,
        )

    <span class="kw">def</span> _consume_turn_direction(self) -&gt; str:
        current = self._next_turn
        self._next_turn = &quot;RIGHT&quot; <span class="kw">if</span> current == &quot;LEFT&quot; <span class="kw">else</span> &quot;LEFT&quot;
        <span class="kw">return</span> current
`
    },
    ai_vision: {
      name: 'ai_vision.py',
      lang: 'Python',
      code: `<span class="kw">from</span> __future__ <span class="kw">import</span> annotations

<span class="kw">import</span> copy
<span class="kw">import</span> json
<span class="kw">import</span> logging
<span class="kw">import</span> threading
<span class="kw">import</span> time
<span class="kw">from</span> collections <span class="kw">import</span> deque
<span class="kw">from</span> dataclasses <span class="kw">import</span> dataclass
<span class="kw">from</span> typing <span class="kw">import</span> Optional

<span class="kw">import</span> cv2
<span class="kw">import</span> numpy <span class="kw">as</span> np
<span class="kw">import</span> onnxruntime <span class="kw">as</span> ort
<span class="kw">from</span> picamera2 <span class="kw">import</span> Picamera2
<span class="kw">from</span> ultralytics <span class="kw">import</span> YOLO

<span class="kw">from</span> ai_vision_interface <span class="kw">import</span> IVisionPipeline, TargetData


logger = logging.getLogger(&quot;ai_vision&quot;)


@dataclass
<span class="kw">class</span> VisionPipelineConfig:
    exploration_fps: int = <span class="num">5</span>
    detection_threshold: float = <span class="num">0.35</span>
    frame_width: int = <span class="num">640</span>
    frame_height: int = <span class="num">480</span>
    detector_model_path: str = &quot;yolov8n.pt&quot;
    severity_model_path: str = &quot;severity_model.onnx&quot;
    class_names_path: str = &quot;class_names.json&quot;


<span class="kw">class</span> VisionPipeline(IVisionPipeline):
    DISTANCE_K = <span class="num">30000.0</span>
    CROP_EXPAND_RATIO = <span class="num">0.35</span>
    IMG_SIZE = (<span class="num">160</span>, <span class="num">160</span>)
    DECISION_WINDOW_SECONDS = <span class="num">2.0</span>
    TRAPPED_MIN_PROB = <span class="num">0.35</span>
    TRAPPED_MAX_GAP_FROM_BEST = <span class="num">0.20</span>
    TRAPPED_PROTECTION_PROB = <span class="num">0.30</span>

    <span class="kw">def</span> __init__(self, config: Optional[VisionPipelineConfig] = <span class="kw">None</span>) -&gt; <span class="kw">None</span>:
        self.config = config <span class="kw">or</span> VisionPipelineConfig()

        self._latest_target: Optional[TargetData] = <span class="kw">None</span>
        self._latest_frame_jpeg: Optional[bytes] = <span class="kw">None</span>
        self._latest_lock = threading.Lock()

        self._pause_event = threading.Event()
        self._stop_event = threading.Event()
        self._worker_thread: Optional[threading.Thread] = <span class="kw">None</span>
        self._initialized = <span class="kw">False</span>

        self._prediction_history = deque()
        self._picam2 = <span class="kw">None</span>
        self._yolo_model = <span class="kw">None</span>
        self._session = <span class="kw">None</span>
        self._input_name = <span class="kw">None</span>
        self._class_names = []

    <span class="kw">def</span> initialize_camera(self) -&gt; bool:
        <span class="kw">if</span> self._initialized:
            <span class="kw">return</span> <span class="kw">True</span>

        <span class="kw">try</span>:
            self._yolo_model = YOLO(self.config.detector_model_path)
            self._session = ort.InferenceSession(
                self.config.severity_model_path,
                providers=[&quot;CPUExecutionProvider&quot;],
            )
            self._input_name = self._session.get_inputs()[<span class="num">0</span>].name

            <span class="kw">with</span> open(self.config.class_names_path, &quot;r&quot;, encoding=&quot;utf-<span class="num">8</span>&quot;) <span class="kw">as</span> handle:
                self._class_names = json.load(handle)

            self._picam2 = Picamera2()
            self._picam2.configure(
                self._picam2.create_preview_configuration(
                    main={
                        &quot;format&quot;: &quot;RGB888&quot;,
                        &quot;size&quot;: (self.config.frame_width, self.config.frame_height),
                    }
                )
            )
            self._picam2.start()
            time.sleep(<span class="num">2</span>)

            self._stop_event.clear()
            self._pause_event.clear()
            self._worker_thread = threading.Thread(target=self._run_loop, name=&quot;VisionLoop&quot;, daemon=<span class="kw">True</span>)
            self._worker_thread.start()
            self._initialized = <span class="kw">True</span>
            <span class="kw">return</span> <span class="kw">True</span>
        <span class="kw">except</span> Exception <span class="kw">as</span> exc:
            logger.exception(&quot;Vision init failed: %s&quot;, exc)
            <span class="kw">return</span> <span class="kw">False</span>

    <span class="kw">def</span> get_latest_target(self) -&gt; Optional[TargetData]:
        <span class="kw">with</span> self._latest_lock:
            <span class="kw">return</span> copy.deepcopy(self._latest_target)

    <span class="kw">def</span> pause_vision_pipeline(self) -&gt; <span class="kw">None</span>:
        self._pause_event.set()

    <span class="kw">def</span> resume_vision_pipeline(self) -&gt; <span class="kw">None</span>:
        self._pause_event.clear()

    <span class="kw">def</span> shutdown(self) -&gt; <span class="kw">None</span>:
        self._stop_event.set()
        <span class="kw">if</span> self._worker_thread <span class="kw">and</span> self._worker_thread.is_alive():
            self._worker_thread.join(timeout=<span class="num">2.0</span>)

        <span class="kw">if</span> self._picam2 <span class="kw">is</span> <span class="kw">not</span> <span class="kw">None</span>:
            <span class="kw">try</span>:
                self._picam2.stop()
            <span class="kw">except</span> Exception:
                <span class="kw">pass</span>

    <span class="kw">def</span> get_latest_frame_jpeg(self) -&gt; Optional[bytes]:
        <span class="kw">with</span> self._latest_lock:
            <span class="kw">if</span> self._latest_frame_jpeg <span class="kw">is</span> <span class="kw">None</span>:
                <span class="kw">return</span> <span class="kw">None</span>
            <span class="kw">return</span> bytes(self._latest_frame_jpeg)

    <span class="kw">def</span> build_augmented_status_report(
        self,
        *,
        pos_x: float,
        pos_y: float,
        temperature: float,
        smoke_detected: bool,
        is_stuck: bool,
        acoustic_hit: bool,
        acoustic_angle: float,
    ):
        <span class="kw">from</span> comms_dashboard_interface <span class="kw">import</span> AugmentedStatusReport

        target = self.get_latest_target()
        <span class="kw">return</span> AugmentedStatusReport(
            pos_x=pos_x,
            pos_y=pos_y,
            temperature=temperature,
            smoke_detected=smoke_detected,
            victim_status=self.target_to_victim_status(target),
            is_stuck=is_stuck,
            priority_level=self.target_to_priority_level(target),
            acoustic_hit=acoustic_hit,
            acoustic_angle=acoustic_angle,
        )

    @staticmethod
    <span class="kw">def</span> target_to_victim_status(target: Optional[TargetData]) -&gt; str:
        <span class="kw">if</span> target <span class="kw">is</span> <span class="kw">None</span>:
            <span class="kw">return</span> &quot;NONE&quot;
        <span class="kw">return</span> str(target.severity).upper()

    @staticmethod
    <span class="kw">def</span> target_to_priority_level(target: Optional[TargetData]) -&gt; int:
        <span class="kw">if</span> target <span class="kw">is</span> <span class="kw">None</span>:
            <span class="kw">return</span> <span class="num">0</span>
        severity = str(target.severity).upper()
        <span class="kw">if</span> severity == &quot;TRAPPED&quot;:
            <span class="kw">return</span> <span class="num">1</span>
        <span class="kw">if</span> severity == &quot;LYING&quot;:
            <span class="kw">return</span> <span class="num">2</span>
        <span class="kw">if</span> severity == &quot;STANDING&quot;:
            <span class="kw">return</span> <span class="num">3</span>
        <span class="kw">return</span> <span class="num">0</span>

    <span class="kw">def</span> _run_loop(self) -&gt; <span class="kw">None</span>:
        <span class="kw">while</span> <span class="kw">not</span> self._stop_event.is_set():
            <span class="kw">if</span> self._pause_event.is_set():
                time.sleep(<span class="num">0.05</span>)
                <span class="kw">continue</span>

            <span class="kw">try</span>:
                frame_rgb = self._picam2.capture_array()
                frame = cv2.cvtColor(frame_rgb, cv2.COLOR_RGB2BGR)
                img_h, img_w = frame.shape[:<span class="num">2</span>]

                results = self._yolo_model(frame, verbose=<span class="kw">False</span>)
                best_person = self._select_best_person(results)

                <span class="kw">if</span> best_person <span class="kw">is</span> <span class="kw">not</span> <span class="kw">None</span>:
                    x1, y1, x2, y2, yolo_conf = best_person
                    ex1, ey1, ex2, ey2 = self._expand_bbox(x1, y1, x2, y2, img_w, img_h)
                    crop = frame[ey1:ey2, ex1:ex2]

                    <span class="kw">if</span> crop.size &gt; <span class="num">0</span>:
                        preds = self._predict_severity_probs(crop)
                        self._update_prediction_history(preds)
                        severity, cnn_conf, stable_probs = self._get_stable_prediction()
                        severity, cnn_conf, bbox_ratio, _ = self._apply_bbox_posture_correction(
                            severity,
                            cnn_conf,
                            stable_probs,
                            x1,
                            y1,
                            x2,
                            y2,
                        )

                        distance_cm = self._estimate_distance_cm(y1, y2)
                        final_conf = (float(yolo_conf) + float(cnn_conf)) / <span class="num">2.0</span>

                        target = TargetData(
                            pos_x=int((x1 + x2) / <span class="num">2</span>),
                            pos_y=int((y1 + y2) / <span class="num">2</span>),
                            distance_cm=round(float(distance_cm), <span class="num">2</span>),
                            severity=severity.upper(),
                            confidence=round(float(final_conf), <span class="num">3</span>),
                        )

                        color = self._severity_to_color(severity)
                        cv2.rectangle(frame, (x1, y1), (x2, y2), color, <span class="num">2</span>)
                        cv2.putText(
                            frame,
                            f&quot;{severity.upper()} {final_conf:.2f}&quot;,
                            (x1, max(<span class="num">30</span>, y1 - <span class="num">10</span>)),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            <span class="num">0.65</span>,
                            color,
                            <span class="num">2</span>,
                        )
                        cv2.putText(
                            frame,
                            f&quot;D:{distance_cm:.1f}cm ratio:{bbox_ratio:.2f}&quot;,
                            (x1, min(img_h - <span class="num">20</span>, y2 + <span class="num">25</span>)),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            <span class="num">0.55</span>,
                            color,
                            <span class="num">2</span>,
                        )

                        ok, buffer = cv2.imencode(&quot;.jpg&quot;, frame)
                        <span class="kw">if</span> ok:
                            <span class="kw">with</span> self._latest_lock:
                                self._latest_target = target
                                self._latest_frame_jpeg = buffer.tobytes()
                <span class="kw">else</span>:
                    self._prediction_history.clear()
                    cv2.putText(
                        frame,
                        &quot;NO PERSON DETECTED&quot;,
                        (<span class="num">20</span>, <span class="num">40</span>),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        <span class="num">0.8</span>,
                        (<span class="num">255</span>, <span class="num">255</span>, <span class="num">255</span>),
                        <span class="num">2</span>,
                    )
                    ok, buffer = cv2.imencode(&quot;.jpg&quot;, frame)
                    <span class="kw">if</span> ok:
                        <span class="kw">with</span> self._latest_lock:
                            self._latest_target = <span class="kw">None</span>
                            self._latest_frame_jpeg = buffer.tobytes()

            <span class="kw">except</span> Exception <span class="kw">as</span> exc:
                logger.exception(&quot;Vision loop failed: %s&quot;, exc)
                time.sleep(<span class="num">0.1</span>)

            time.sleep(<span class="num">1.0</span> / max(self.config.exploration_fps, <span class="num">1</span>))

    <span class="kw">def</span> _expand_bbox(self, x1, y1, x2, y2, img_w, img_h):
        width = x2 - x1
        height = y2 - y1
        pad_x = int(width * self.CROP_EXPAND_RATIO)
        pad_y = int(height * self.CROP_EXPAND_RATIO)
        nx1 = max(<span class="num">0</span>, x1 - pad_x)
        ny1 = max(<span class="num">0</span>, y1 - pad_y)
        nx2 = min(img_w, x2 + pad_x)
        ny2 = min(img_h, y2 + pad_y)
        <span class="kw">return</span> nx1, ny1, nx2, ny2

    <span class="kw">def</span> _estimate_distance_cm(self, y1, y2):
        bbox_height = max(<span class="num">1</span>, y2 - y1)
        <span class="kw">return</span> self.DISTANCE_K / bbox_height

    <span class="kw">def</span> _predict_severity_probs(self, crop_bgr):
        resized = cv2.resize(crop_bgr, self.IMG_SIZE)
        rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
        arr = np.expand_dims(rgb.astype(np.float32), axis=<span class="num">0</span>)
        preds = self._session.run(<span class="kw">None</span>, {self._input_name: arr})[<span class="num">0</span>][<span class="num">0</span>]
        <span class="kw">return</span> preds

    <span class="kw">def</span> _update_prediction_history(self, preds):
        now = time.time()
        self._prediction_history.append((now, preds))
        <span class="kw">while</span> self._prediction_history <span class="kw">and</span> now - self._prediction_history[<span class="num">0</span>][<span class="num">0</span>] &gt; self.DECISION_WINDOW_SECONDS:
            self._prediction_history.popleft()

    <span class="kw">def</span> _get_stable_prediction(self):
        <span class="kw">if</span> <span class="kw">not</span> self._prediction_history:
            <span class="kw">return</span> &quot;none&quot;, <span class="num">0.0</span>, {}

        all_preds = np.array([item[<span class="num">1</span>] <span class="kw">for</span> item <span class="kw">in</span> self._prediction_history])
        avg_preds = np.mean(all_preds, axis=<span class="num">0</span>)

        probs = {self._class_names[i]: round(float(avg_preds[i]), <span class="num">3</span>) <span class="kw">for</span> i <span class="kw">in</span> range(len(self._class_names))}
        class_id = int(np.argmax(avg_preds))
        severity = self._class_names[class_id]
        confidence = float(avg_preds[class_id])

        <span class="kw">if</span> &quot;trapped&quot; <span class="kw">in</span> self._class_names:
            trapped_idx = self._class_names.index(&quot;trapped&quot;)
            trapped_prob = float(avg_preds[trapped_idx])
            best_prob = float(np.max(avg_preds))
            <span class="kw">if</span> trapped_prob &gt;= self.TRAPPED_MIN_PROB <span class="kw">and</span> (best_prob - trapped_prob) &lt;= self.TRAPPED_MAX_GAP_FROM_BEST:
                severity = &quot;trapped&quot;
                confidence = trapped_prob

        <span class="kw">return</span> severity, confidence, probs

    <span class="kw">def</span> _select_best_person(self, results):
        best_box = <span class="kw">None</span>
        best_score = <span class="num">0.0</span>
        <span class="kw">for</span> result <span class="kw">in</span> results:
            <span class="kw">for</span> box <span class="kw">in</span> result.boxes:
                cls_id = int(box.cls[<span class="num">0</span>])
                conf = float(box.conf[<span class="num">0</span>])
                <span class="kw">if</span> cls_id != <span class="num">0</span> <span class="kw">or</span> conf &lt; self.config.detection_threshold:
                    <span class="kw">continue</span>

                x1, y1, x2, y2 = map(int, box.xyxy[<span class="num">0</span>])
                area = max(<span class="num">1</span>, (x2 - x1) * (y2 - y1))
                score = conf * area
                <span class="kw">if</span> score &gt; best_score:
                    best_score = score
                    best_box = (x1, y1, x2, y2, conf)
        <span class="kw">return</span> best_box

    <span class="kw">def</span> _apply_bbox_posture_correction(self, severity, cnn_conf, stable_probs, x1, y1, x2, y2):
        width = max(<span class="num">1</span>, x2 - x1)
        height = max(<span class="num">1</span>, y2 - y1)
        ratio = width / height

        trapped_prob = stable_probs.get(&quot;trapped&quot;, <span class="num">0.0</span>)
        standing_prob = stable_probs.get(&quot;standing&quot;, <span class="num">0.0</span>)

        <span class="kw">if</span> ratio &lt; <span class="num">0.85</span> <span class="kw">and</span> standing_prob &gt;= <span class="num">0.20</span>:
            <span class="kw">return</span> &quot;standing&quot;, max(standing_prob, <span class="num">0.82</span>), ratio, &quot;bbox_strong_corrected_to_standing&quot;

        <span class="kw">if</span> severity == &quot;trapped&quot; <span class="kw">or</span> trapped_prob &gt;= self.TRAPPED_PROTECTION_PROB:
            <span class="kw">if</span> ratio &lt; <span class="num">0.75</span> <span class="kw">and</span> standing_prob &gt;= <span class="num">0.25</span> <span class="kw">and</span> trapped_prob &lt; <span class="num">0.55</span>:
                <span class="kw">return</span> &quot;standing&quot;, max(standing_prob, <span class="num">0.80</span>), ratio, &quot;standing_overrides_weak_trapped&quot;
            <span class="kw">return</span> severity, cnn_conf, ratio, &quot;trapped_protected&quot;

        <span class="kw">if</span> ratio &gt; <span class="num">1.15</span> <span class="kw">and</span> cnn_conf &lt; <span class="num">0.80</span>:
            <span class="kw">return</span> &quot;lying&quot;, max(cnn_conf, <span class="num">0.70</span>), ratio, &quot;bbox_corrected_to_lying&quot;

        <span class="kw">return</span> severity, cnn_conf, ratio, &quot;cnn_decision&quot;

    <span class="kw">def</span> _severity_to_color(self, severity):
        sev = severity.lower()
        <span class="kw">if</span> sev == &quot;trapped&quot;:
            <span class="kw">return</span> (<span class="num">0</span>, <span class="num">0</span>, <span class="num">255</span>)
        <span class="kw">if</span> sev == &quot;lying&quot;:
            <span class="kw">return</span> (<span class="num">0</span>, <span class="num">255</span>, <span class="num">255</span>)
        <span class="kw">if</span> sev == &quot;standing&quot;:
            <span class="kw">return</span> (<span class="num">0</span>, <span class="num">255</span>, <span class="num">0</span>)
        <span class="kw">return</span> (<span class="num">255</span>, <span class="num">255</span>, <span class="num">255</span>)
`
    },
    arduino: {
      name: 'mega_firmware.ino',
      lang: 'Arduino C++',
      code: `<span class="kw">#include</span> &lt;Wire.h&gt;
    <span class="kw">#include</span> &lt;MPU6050_light.h&gt;
    <span class="kw">#include</span> &lt;DHT.h&gt;

    <span class="cm">// Sensor pinleri</span>
    <span class="kw">#define</span> TRIG_FRONT <span class="num">9</span>
    <span class="kw">#define</span> ECHO_FRONT <span class="num">10</span>
    <span class="kw">#define</span> TRIG_BACK  <span class="num">7</span>
    <span class="kw">#define</span> ECHO_BACK  <span class="num">8</span>
    <span class="kw">#define</span> MIC1_PIN A0
    <span class="kw">#define</span> MIC2_PIN A1
    <span class="kw">#define</span> MQ2_PIN A2
    <span class="kw">#define</span> DHT_PIN  <span class="num">4</span>
    <span class="kw">#define</span> DHT_TYPE DHT11

    <span class="cm">// Motor pinleri</span>
    <span class="kw">#define</span> ENA <span class="num">5</span>
    <span class="kw">#define</span> IN1 <span class="num">22</span>
    <span class="kw">#define</span> IN2 <span class="num">23</span>
    <span class="kw">#define</span> IN3 <span class="num">24</span>
    <span class="kw">#define</span> IN4 <span class="num">25</span>
    <span class="kw">#define</span> ENB <span class="num">6</span>

    const <span class="kw">int</span> SPEED = <span class="num">200</span>;
    const unsigned long KOMUT_SURE_MS = <span class="num">2000</span>;

    MPU6050 mpu(Wire);
    DHT dht(DHT_PIN, DHT_TYPE);
    <span class="kw">bool</span> mpu_ok = <span class="kw">false</span>;

    unsigned long komutBaslangic = <span class="num">0</span>;
    <span class="kw">bool</span> komutAktif = <span class="kw">false</span>;

    <span class="cm">// ==================== MOTOR ====================</span>
    <span class="kw">void</span> setMotors(<span class="kw">int</span> leftDir, <span class="kw">int</span> rightDir) {
    <span class="kw">if</span> (leftDir &gt; <span class="num">0</span>) {
digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    } <span class="kw">else</span> <span class="kw">if</span> (leftDir &lt; <span class="num">0</span>) {
digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    } <span class="kw">else</span> {
 digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
    }

    <span class="kw">if</span> (rightDir &gt; <span class="num">0</span>) {


  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
 } <span class="kw">else</span> <span class="kw">if</span> (rightDir &lt; <span class="num">0</span>) {
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
 } <span class="kw">else</span> {
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
 }

 analogWrite(ENA, leftDir == <span class="num">0</span> ? <span class="num">0</span> : SPEED);
 analogWrite(ENB, rightDir == <span class="num">0</span> ? <span class="num">0</span> : SPEED);
}

<span class="kw">void</span> ileri()   { setMotors(<span class="num">1</span>, <span class="num">1</span>); }
<span class="kw">void</span> geri()    { setMotors(-<span class="num">1</span>, -<span class="num">1</span>); }
<span class="kw">void</span> dur()     { setMotors(<span class="num">0</span>, <span class="num">0</span>); }
<span class="kw">void</span> sagaDon() { setMotors(<span class="num">1</span>, -<span class="num">1</span>); }
<span class="kw">void</span> solaDon() { setMotors(-<span class="num">1</span>, <span class="num">1</span>); }

<span class="cm">// ==================== SENSOR ====================</span>
<span class="kw">int</span> readDistance(<span class="kw">int</span> trigPin, <span class="kw">int</span> echoPin) {
 digitalWrite(trigPin, LOW);
 delayMicroseconds(<span class="num">2</span>);
 digitalWrite(trigPin, HIGH);
 delayMicroseconds(<span class="num">10</span>);
 digitalWrite(trigPin, LOW);

 unsigned long timeout = micros() + <span class="num">30000</span>;
 <span class="kw">while</span> (digitalRead(echoPin) == LOW) {
  <span class="kw">if</span> (micros() &gt; timeout) <span class="kw">return</span> <span class="num">0</span>;
 }
 unsigned long start = micros();
 <span class="kw">while</span> (digitalRead(echoPin) == HIGH) {
  <span class="kw">if</span> (micros() &gt; timeout) <span class="kw">return</span> <span class="num">0</span>;
 }
 <span class="kw">return</span> (micros() - start) / <span class="num">58</span>;
}

<span class="kw">void</span> sendSensorData() {
 <span class="kw">float</span> yaw = <span class="num">0.0</span>, pitch = <span class="num">0.0</span>, roll = <span class="num">0.0</span>;
 <span class="kw">if</span> (mpu_ok) {
  mpu.update();
  yaw = mpu.getAngleZ();
  pitch = mpu.getAngleY();
  roll = mpu.getAngleX();
  <span class="kw">if</span> (isnan(yaw)) yaw = <span class="num">0.0</span>;
  <span class="kw">if</span> (isnan(pitch)) pitch = <span class="num">0.0</span>;
  <span class="kw">if</span> (isnan(roll)) roll = <span class="num">0.0</span>;
 }

 <span class="kw">int</span> front = readDistance(TRIG_FRONT, ECHO_FRONT);
 delay(<span class="num">10</span>);
 <span class="kw">int</span> back = readDistance(TRIG_BACK, ECHO_BACK);

 <span class="kw">int</span> mic1 = analogRead(MIC1_PIN);
 <span class="kw">int</span> mic2 = analogRead(MIC2_PIN);
 <span class="kw">int</span> smoke = analogRead(MQ2_PIN);

 <span class="kw">float</span> temp = dht.readTemperature();
 <span class="kw">float</span> hum = dht.readHumidity();
 <span class="kw">if</span> (isnan(temp)) temp = <span class="num">0.0</span>;
 <span class="kw">if</span> (isnan(hum)) hum = <span class="num">0.0</span>;

 Serial.print(&quot;{\&quot;dist_front\&quot;:&quot;);
 Serial.print(front);
 Serial.print(&quot;,\&quot;dist_back\&quot;:&quot;);
 Serial.print(back);
 Serial.print(&quot;,\&quot;mic1\&quot;:&quot;);
 Serial.print(mic1);
 Serial.print(&quot;,\&quot;mic2\&quot;:&quot;);
 Serial.print(mic2);
 Serial.print(&quot;,\&quot;smoke\&quot;:&quot;);
 Serial.print(smoke);


 Serial.print(&quot;,\&quot;yaw\&quot;:&quot;);
 Serial.print(yaw, <span class="num">1</span>);
 Serial.print(&quot;,\&quot;pitch\&quot;:&quot;);
 Serial.print(pitch, <span class="num">1</span>);
 Serial.print(&quot;,\&quot;roll\&quot;:&quot;);
 Serial.print(roll, <span class="num">1</span>);
 Serial.print(&quot;,\&quot;temp\&quot;:&quot;);
 Serial.print(temp, <span class="num">1</span>);
 Serial.print(&quot;,\&quot;hum\&quot;:&quot;);
 Serial.print(hum, <span class="num">1</span>);
 Serial.print(&quot;,\&quot;connected\&quot;:<span class="kw">true</span>}&quot;);
 Serial.println();
}

<span class="cm">// ==================== KOMUT ====================</span>
<span class="kw">void</span> komutuUygula(<span class="kw">String</span> cmd) {
 cmd.trim();
 cmd.toUpperCase();

 <span class="kw">if</span> (cmd == &quot;FORWARD&quot;) {
  ileri();
  komutBaslangic = millis();
  komutAktif = <span class="kw">true</span>;
 } <span class="kw">else</span> <span class="kw">if</span> (cmd == &quot;BACKWARD&quot;) {
  geri();
  komutBaslangic = millis();
  komutAktif = <span class="kw">true</span>;
 } <span class="kw">else</span> <span class="kw">if</span> (cmd == &quot;LEFT&quot;) {
  solaDon();
  komutBaslangic = millis();
  komutAktif = <span class="kw">true</span>;
 } <span class="kw">else</span> <span class="kw">if</span> (cmd == &quot;RIGHT&quot;) {
  sagaDon();
  komutBaslangic = millis();
  komutAktif = <span class="kw">true</span>;
 } <span class="kw">else</span> <span class="kw">if</span> (cmd == &quot;STOP&quot;) {
  dur();
  komutAktif = <span class="kw">false</span>;
 }
}

<span class="cm">// ==================== SETUP ====================</span>
<span class="kw">void</span> setup() {
 Serial.begin(<span class="num">115200</span>);

 pinMode(TRIG_FRONT, OUTPUT);
 pinMode(ECHO_FRONT, INPUT);
 pinMode(TRIG_BACK, OUTPUT);
 pinMode(ECHO_BACK, INPUT);
 digitalWrite(TRIG_FRONT, LOW);
 digitalWrite(TRIG_BACK, LOW);

 pinMode(ENA, OUTPUT);
 pinMode(ENB, OUTPUT);
 pinMode(IN1, OUTPUT);
 pinMode(IN2, OUTPUT);
 pinMode(IN3, OUTPUT);
 pinMode(IN4, OUTPUT);

 Wire.begin();
 byte status = mpu.begin();
 <span class="kw">if</span> (status == <span class="num">0</span>) {
  delay(<span class="num">1000</span>);
  mpu.calcOffsets();
  mpu_ok = <span class="kw">true</span>;
 }

 dht.begin();
 delay(<span class="num">2000</span>);

 dur();


}

<span class="cm">// ==================== LOOP ====================</span>
unsigned long sonSensorGonderim = <span class="num">0</span>;

<span class="kw">void</span> loop() {
 <span class="cm">// Pi&#x27;den komut kontrol</span>
 <span class="kw">if</span> (Serial.available() &gt; <span class="num">0</span>) {
  <span class="kw">String</span> cmd = Serial.readStringUntil(&#x27;
&#x27;);
  komutuUygula(cmd);
 }

 <span class="cm">// Komut suresi doldu mu</span>
 <span class="kw">if</span> (komutAktif &amp;&amp; (millis() - komutBaslangic &gt;= KOMUT_SURE_MS)) {
  dur();
  komutAktif = <span class="kw">false</span>;
 }

 <span class="cm">// Sensor verisi gonder (300ms)</span>
 <span class="kw">if</span> (millis() - sonSensorGonderim &gt;= <span class="num">300</span>) {
  sendSensorData();
  sonSensorGonderim = millis();
 }
}`
    }
  };

  const codeBlock = document.getElementById('ide-code-block');
  const tabTitle = document.getElementById('ide-active-tab-title');
  const langLabel = document.getElementById('ide-lang-label');
  if(codeBlock){
    document.querySelectorAll('.ide-file-btn').forEach(btn => {
      btn.addEventListener('click', () => {
        const key = btn.dataset.file;
        const file = codeFiles[key];
        if(!file) return;
        document.querySelectorAll('.ide-file-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        codeBlock.innerHTML = file.code;
        if(tabTitle) tabTitle.querySelector('span:last-child').textContent = file.name;
        if(langLabel) langLabel.textContent = 'Language: ' + file.lang;
      });
    });
    // Load first file
    const first = document.querySelector('.ide-file-btn.active');
    if(first) first.click();
  }

})();
