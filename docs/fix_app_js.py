import html
import re

def highlight_python(code):
    code = html.escape(code)
    # Highlight strings first using a placeholder, to avoid matching HTML attributes later
    strings = []
    def repl_str(m):
        strings.append(m.group(0))
        return f"__STR_{len(strings)-1}__"
    
    code = re.sub(r'"""[\s\S]*?"""', repl_str, code)
    code = re.sub(r'".*?"|\'.*?\'', repl_str, code)
    
    code = re.sub(r'\b(def|class|import|from|return|if|elif|else|for|while|try|except|with|as|pass|break|continue|and|or|not|in|is|None|True|False)\b', r'<span class="kw">\1</span>', code)
    code = re.sub(r'#(.*?)\n', r'<span class="cm">#\1</span>\n', code)
    code = re.sub(r'\b(\d+\.?\d*)\b', r'<span class="num">\1</span>', code)
    
    for i, s in enumerate(strings):
        code = code.replace(f"__STR_{i}__", f'<span class="str">{s}</span>')
    return code

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

with open("../demo-main/robot_project1/autonomy_controller.py", "r") as f:
    autonomy_code = highlight_python(f.read())
    
with open("../demo-main/robot_project1/uart_reader.py", "r") as f:
    uart_code = highlight_python(f.read())

with open("../demo-main/robot_project1/ai_vision.py", "r") as f:
    vision_code = highlight_python(f.read())

with open("../demo-main/MOD-01_Embedded/hardware_schemes/Mega_Arduino_Kodu.md", "r") as f:
    arduino_md = f.read()
    arduino_match = re.search(r'```cpp\n([\s\S]*?)\n```', arduino_md)
    if arduino_match:
        arduino_code = highlight_cpp(arduino_match.group(1))
    else:
        arduino_code = "Code not found"

with open("app.js", "r") as f:
    content = f.read()

# We need to replace the codeFiles object in app.js
import ast
# Instead of regex, let's just generate a clean app.js from the template we used before
js_template = """/* ====== Group 7 Documentation — Interactivity ====== */
(function(){
  'use strict';

  /* ---- Active nav highlight ---- */
  const page = location.pathname.split('/').pop() || 'index.html';
  document.querySelectorAll('.sidebar-link').forEach(link => {
    if(link.getAttribute('href') === page) link.classList.add('active');
    else link.classList.remove('active');
  });

  /* ---- Mobile hamburger ---- */
  const ham = document.querySelector('.hamburger');
  const sidebar = document.querySelector('.sidebar');
  if(ham && sidebar){
    ham.addEventListener('click', () => sidebar.classList.toggle('open'));
    document.querySelector('.main')?.addEventListener('click', () => sidebar.classList.remove('open'));
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
      code: `""" + uart_code.replace('`', '\\`').replace('$', '\\$') + """`
    },
    autonomy: {
      name: 'autonomy_controller.py',
      lang: 'Python',
      code: `""" + autonomy_code.replace('`', '\\`').replace('$', '\\$') + """`
    },
    ai_vision: {
      name: 'ai_vision.py',
      lang: 'Python',
      code: `""" + vision_code.replace('`', '\\`').replace('$', '\\$') + """`
    },
    arduino: {
      name: 'mega_firmware.ino',
      lang: 'Arduino C++',
      code: `""" + arduino_code.replace('`', '\\`').replace('$', '\\$') + """`
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
"""

with open("app.js", "w") as f:
    f.write(js_template)

print("Done")
