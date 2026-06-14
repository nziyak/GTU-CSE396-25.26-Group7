import html
import re

with open("../demo-main/MOD-01_Embedded/hardware_schemes/Mega_Arduino_Kodu.md", "r") as f:
    lines = f.readlines()

# Extract code between "#include <Wire.h>" and the end of the loop, roughly line 18 to 251.
# Or just find "#include <Wire.h>" and the end of the file or up to "Yukleme Adimlari"
start_idx = 0
end_idx = len(lines)
for i, line in enumerate(lines):
    if "#include <Wire.h>" in line:
        start_idx = i
        break

for i in range(start_idx, len(lines)):
    if "Yukleme Adimlari" in line:
        end_idx = i
        break
    if "--- Page" in lines[i]:
        lines[i] = "" # remove page markers

arduino_code = "".join(lines[start_idx:]).split("Yukleme Adimlari")[0]
arduino_code = re.sub(r'--- Page \d+ ---', '', arduino_code)

def highlight_cpp(code):
    code = html.escape(code)
    strings = []
    def repl_str(m):
        strings.append(m.group(0))
        return f"__STR_{len(strings)-1}__"
        
    code = re.sub(r'".*?"|\'.*?\'', repl_str, code)
    code = re.sub(r'//(.*?)\n', r'<span class="cm">//\1</span>\n', code)
    code = re.sub(r'\b(void|int|float|bool|char|String|if|else|for|while|return|true|false)\b', r'<span class="kw">\1</span>', code)
    code = re.sub(r'#(include|define)', r'<span class="kw">#\1</span>', code)
    code = re.sub(r'\b(\d+\.?\d*)\b', r'<span class="num">\1</span>', code)
    
    for i, s in enumerate(strings):
        code = code.replace(f"__STR_{i}__", f'<span class="str">{s}</span>')
    return code

arduino_html = highlight_cpp(arduino_code).replace('`', '\\`').replace('$', '\\$').strip()

with open("app.js", "r") as f:
    app_js = f.read()

# Replace the arduino code block in app.js
import ast
# we can just use regex to replace the content of arduino: { ... code: `...` }
app_js = re.sub(r'(arduino:\s*{\s*name:\s*\'mega_firmware.ino\',\s*lang:\s*\'Arduino C\+\+\',\s*code:\s*`)[^`]*(`)', r'\g<1>' + arduino_html + r'\2', app_js)

with open("app.js", "w") as f:
    f.write(app_js)

print("Arduino code updated.")
