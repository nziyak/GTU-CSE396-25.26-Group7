import re

with open("team.html", "r") as f:
    content = f.read()

replacements = [
    (r'<div class="role-badges"><span class="role-badge primary">MOD-01</span><span\s*class="role-badge">MOD-03</span><span class="role-badge">INTEGRATION</span></div>\s*<p style="font-size:\.72rem;color:#64748b;margin-top:\.5rem">Embedded systems, Arduino Mega firmware, acoustic\s*filtering, system integration lead</p>',
     '<div class="role-badges"><span class="role-badge primary">MOD-01 (Primary)</span><span class="role-badge">MOD-05 (Secondary)</span></div>\n            <p style="font-size:.72rem;color:#64748b;margin-top:.5rem">Embedded systems, Arduino Mega firmware, system integration, Unity secondary support</p>'),
     
    (r'<div class="role-badges"><span class="role-badge primary">MOD-02</span><span class="role-badge">AI/ML</span>\s*</div>\s*<p style="font-size:\.72rem;color:#64748b;margin-top:\.5rem">Vision pipeline, YOLOv8 integration, CNN severity\s*classifier training</p>',
     '<div class="role-badges"><span class="role-badge primary">MOD-04 (Primary)</span><span class="role-badge">MOD-01 (Secondary)</span></div>\n            <p style="font-size:.72rem;color:#64748b;margin-top:.5rem">Web Dashboard, Flask backend, Arduino hardware integration support</p>'),
     
    (r'<div class="role-badges"><span class="role-badge primary">MOD-02</span><span\s*class="role-badge">CAMERA</span></div>\s*<p style="font-size:\.72rem;color:#64748b;margin-top:\.5rem">Pi Camera V3 integration, frame preprocessing,\s*ONNX model optimization</p>',
     '<div class="role-badges"><span class="role-badge primary">MOD-04 (Primary)</span><span class="role-badge">MOD-02 (Secondary)</span><span class="role-badge">MOD-03 (Secondary)</span></div>\n            <p style="font-size:.72rem;color:#64748b;margin-top:.5rem">Web integration, STT, AI Vision support, Audio navigation support</p>'),
     
    (r'<div class="role-badges"><span class="role-badge primary">MOD-03</span><span class="role-badge">DSP</span>\s*</div>\s*<p style="font-size:\.72rem;color:#64748b;margin-top:\.5rem">Acoustic homing, IIR digital filter design,\s*bearing computation algorithms</p>',
     '<div class="role-badges"><span class="role-badge primary">MOD-02 (Primary)</span><span class="role-badge">MOD-03 (Secondary)</span><span class="role-badge">MOD-05 (Secondary)</span></div>\n            <p style="font-size:.72rem;color:#64748b;margin-top:.5rem">AI Vision, YOLO & CNN modeling, audio processing support, Unity dashboard support</p>'),
     
    (r'<div class="role-badges"><span class="role-badge primary">MOD-04</span><span\s*class="role-badge">BACKEND</span></div>\s*<p style="font-size:\.72rem;color:#64748b;margin-top:\.5rem">Flask/WebSocket backend, telemetry broadcasting,\s*UART bridge architecture</p>',
     '<div class="role-badges"><span class="role-badge primary">MOD-03 (Primary)</span><span class="role-badge">MOD-02 (Secondary)</span><span class="role-badge">MOD-04 (Secondary)</span></div>\n            <p style="font-size:.72rem;color:#64748b;margin-top:.5rem">Acoustic homing, IIR filtering, AI Vision integration, Web backend support</p>'),
     
    (r'<div class="role-badges"><span class="role-badge primary">MOD-04</span><span class="role-badge">STT</span>\s*</div>\s*<p style="font-size:\.72rem;color:#64748b;margin-top:\.5rem">Vosk STT integration, offline speech-to-command\s*pipeline, Push-to-Talk system</p>',
     '<div class="role-badges"><span class="role-badge primary">MOD-05 (Primary)</span><span class="role-badge">MOD-02 (Secondary)</span><span class="role-badge">MOD-03 (Secondary)</span></div>\n            <p style="font-size:.72rem;color:#64748b;margin-top:.5rem">Unity Digital Twin lead, 3D HUD design, AI vision integration, Audio navigation support</p>'),
     
    (r'<div class="role-badges"><span class="role-badge primary">MOD-05</span><span class="role-badge">UNITY</span>\s*</div>\s*<p style="font-size:\.72rem;color:#64748b;margin-top:\.5rem">Unity Digital Twin, 3D HUD design, map manager,\s*victim pin visualization</p>',
     '<div class="role-badges"><span class="role-badge primary">MOD-02 (Primary)</span><span class="role-badge">MOD-04 (Secondary)</span></div>\n            <p style="font-size:.72rem;color:#64748b;margin-top:.5rem">Vision pipeline, YOLOv8 integration, CNN severity classifier, Web & STT support</p>'),
     
    (r'<div class="role-badges"><span class="role-badge primary">MOD-05</span><span class="role-badge">UI/UX</span>\s*</div>\s*<p style="font-size:\.72rem;color:#64748b;margin-top:\.5rem">Unity UI panels, operator controls, camera\s*controller, telemetry dashboard</p>',
     '<div class="role-badges"><span class="role-badge primary">MOD-02 (Primary)</span><span class="role-badge">MOD-01 (Secondary)</span></div>\n            <p style="font-size:.72rem;color:#64748b;margin-top:.5rem">Vision pipeline, AI optimization, Embedded & HW support</p>'),
]

for old, new in replacements:
    content = re.sub(old, new, content)

with open("team.html", "w") as f:
    f.write(content)

print("Done")
