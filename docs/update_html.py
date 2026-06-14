import glob
import re

old_font = '<link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700;800&family=JetBrains+Mono:wght@400;500;600;700&display=swap" rel="stylesheet">'
new_font = '<link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;500;600;700;800&family=Fira+Code:wght@400;500;600;700&display=swap" rel="stylesheet">'

new_nav = """<div class="layout">
<!-- Top Navbar -->
<header class="top-navbar">
  <div class="navbar-container">
    <div class="navbar-logo">
      <span class="brand">FireBot</span>
      <span class="badge">G7</span>
    </div>
    <button class="hamburger" aria-label="Menu"><svg fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2"><path stroke-linecap="round" stroke-linejoin="round" d="M4 6h16M4 12h16M4 18h16"/></svg></button>
    <nav class="navbar-nav">
      <a href="index.html" class="nav-link" data-nav="home">Home</a>
      <a href="technical.html" class="nav-link" data-nav="technical">Technical</a>
      <a href="modules.html" class="nav-link" data-nav="modules">Modules</a>
      <a href="team.html" class="nav-link" data-nav="team">Team</a>
      <a href="manual.html" class="nav-link" data-nav="manual">User Manual</a>
    </nav>
  </div>
</header>

<!-- Main Content -->
<main class="main">"""

for file_path in glob.glob('*.html'):
    with open(file_path, 'r') as f:
        content = f.read()
    
    # Replace Font
    content = content.replace(old_font, new_font)
    
    # Replace Sidebar
    # Find from <button class="hamburger" to <main class="main">
    pattern = re.compile(r'<button class="hamburger".*?<main class="main">', re.DOTALL)
    content = pattern.sub(new_nav, content)
    
    with open(file_path, 'w') as f:
        f.write(content)

print("HTML files updated.")
